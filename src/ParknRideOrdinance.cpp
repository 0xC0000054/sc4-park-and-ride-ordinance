////////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-park-and-ride-ordinance, a DLL Plugin for
// SimCity 4 that adds a Park and Ride ordinance to the game.
//
// Copyright (c) 2023 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
////////////////////////////////////////////////////////////////////////////

#include "ParknRideOrdinance.h"
#include "cISC4Simulator.h"
#include "cGZPersistResourceKey.h"
#include "cIGZMessageServer2.h"
#include "cIGZPersistResourceManager.h"
#include "GZServPtrs.h"
#include "cRZMessage2Standard.h"

// The unique ID that identifies this ordinance.
// The value must never be reused, when creating a new ordinance generate a random 32-bit integer and use that.
static constexpr uint32_t kParknRideOrdinanceCLSID = 0x479bf2c7;

namespace
{
	OrdinancePropertyHolder CreateOrdinanceEffects()
	{
		OrdinancePropertyHolder properties;

		// Positive effects:

		// Commercial Demand Effect: +5%
		properties.AddProperty(0x2a633000, 1.05f);
		// Demand Effect:Cs$: +5%
		properties.AddProperty(0x2a653110, 1.05f);
		// Demand Effect:Cs$$: +5%
		properties.AddProperty(0x2a653120, 1.05f);
		// Demand Effect:Cs$$$: +5%
		properties.AddProperty(0x2a653130, 1.05f);
		// Demand Effect:Co$$: +5%
		properties.AddProperty(0x2a653320, 1.05f);
		// Demand Effect:Co$$$: +5%
		properties.AddProperty(0x2a653330, 1.05f);
		// Air Effect: -5% for all pollution
		properties.AddProperty(0x08f79b8e, 0.95f);
		// Health Quotient Boost Effect: +5%
		properties.AddProperty(0xe91b3aee, 105.0f);

		// Negative effects:

		// Demand Effect:IR: -2%
		properties.AddProperty(0x2a654100, 0.98f);
		// Demand Effect:ID: -2%
		properties.AddProperty(0x2a654200, 0.98f);
		// Demand Effect:IM: -2%
		properties.AddProperty(0x2a654300, 0.98f);

		return properties;
	}
}

ParknRideOrdinance::ParknRideOrdinance()
	: OrdinanceBase(
		kParknRideOrdinanceCLSID,
		"Park n Ride",
		"Program that promotes park and ride.",
		/* enactment income */		  0,
		/* retracment income */       0,
		/* monthly constant income */ 0,
		/* monthly income factor */   0.0f,
		/* income ordinance */		  false,
	    CreateOrdinanceEffects())
{
}

void ParknRideOrdinance::UpdateCarCanReachDestination() const
{
	// Don't bother updating the value if the plugin has not been initialized.
	// This would occur when the ordinance is removed from the ordinance simulator
	// as part of the process of exiting a city.
	if (!initialized)
	{
		return;
	}

	cISC4SimulatorPtr pSimulator;

	if (!pSimulator)
	{
		logger.WriteLine(LogOptions::Errors, "The cISC4Simulator pointer was null.");
		return;
	}

	// Pause the game before making any changes to the traffic simulator tuning exemplar.
	// This should prevent the issues caused by having the traffic simulator reload its
	// tuning exemplar while the simulation is running.
	if (!pSimulator->HiddenPause())
	{
		logger.WriteLine(LogOptions::Errors, "Failed to pause the game.");
		return;
	}

	const bool carCanReachDestination = !on;

	cIGZPersistResourceManagerPtr pResourceManager;
	if (pResourceManager)
	{
		constexpr uint32_t kTravelTypeCanReachDestination = 0xA92356B5;

		// The resource key is set to the TGI of the traffic simulator tuning exemplar.
		cGZPersistResourceKey key(0x6534284a, 0xe7e2c2db, 0xc9133286);
		cISCPropertyHolder* propertyHolder = nullptr;
		bool valueChanged = false;

		// Load the traffic simulator tuning exemplar.
		// The game will temporarily cache the loaded exemplar, which allows us
		// to modify the in-memory copy.

		bool result = pResourceManager->GetResource(
			key,
			GZIID_cISCPropertyHolder,
			reinterpret_cast<void**>(&propertyHolder),
			0,
			nullptr);

		if (result)
		{
			cISCProperty* property = propertyHolder->GetProperty(kTravelTypeCanReachDestination);

			if (property)
			{
				cIGZVariant* data = property->GetPropertyValue();

				if (data)
				{
					uint16_t type = data->GetType();
					uint32_t count = data->GetCount();

					if (type == cIGZVariant::Type::BoolArray && count == 9)
					{
						bool* values = data->RefBool();

						// Update the "Travel type can reach destination" value for cars.
						// The property order is walk, car, bus...
						if (values[1] != carCanReachDestination)
						{
							logger.WriteLineFormatted(
								LogOptions::Info,
								"Setting 'Travel type can reach destination' value for cars to %s.",
								carCanReachDestination ? "true" : "false");

							values[1] = carCanReachDestination;
							valueChanged = true;
						}
					}
					else
					{
						logger.WriteLineFormatted(
							LogOptions::Errors,
							"The 'Travel type can reach destination' property data has an unexpected type and/or count"
							", type=0x04x, count=%u. Expected type=0x8001 and count=9.",
							type,
							count);
					}
				}
				else
				{
					logger.WriteLine(LogOptions::Errors, "The 'Travel type can reach destination' property data was null.");
				}
			}
			else
			{
				logger.WriteLine(LogOptions::Errors, "The 'Travel type can reach destination' property does not exist.");
			}

			propertyHolder->Release();
			propertyHolder = nullptr;
		}
		else
		{
			logger.WriteLine(LogOptions::Errors, "Failed to load the traffic simulator tuning exemplar.");
		}

		if (valueChanged)
		{
			// If we modified the in-memory copy of the traffic simulator tuning exemplar
			// we first tell the traffic simulator to reload its tuning parameters so that
			// it picks up the new data.
			// After that message is sent we verify that the in-memory modifications are
			// still present.

			cIGZMessageServer2Ptr pMsgServ;

			if (pMsgServ)
			{
				// A number of the games's simulators support a message that forces
				// them to reload their tunable values.
				// The message takes 2 integer parameters that identify the intended
				// target. These values appear to be the group and instance IDs of
				// the simulator's tuning exemplar.
				// 
				// This feature was likely used during SC4's development to allow
				// the tuning values to be applied after they were modified in the
				// in-game editor.

				constexpr uint32_t kSC4MessageReloadTunableValues = 0xC53D10AA;
				constexpr uint32_t trafficSimData1 = 0xE7E2C2DB;
				constexpr uint32_t trafficSimData2 = 0xC9133286;

				cRZMessage2Standard message;
				message.SetType(kSC4MessageReloadTunableValues);
				message.SetData1(trafficSimData1);
				message.SetData2(trafficSimData2);

				logger.WriteLine(
					LogOptions::Info,
					"Sending the updated 'Travel type can reach destination' value to the traffic simulator.");

				result = pMsgServ->MessageSend(static_cast<cIGZMessage2*>(static_cast<cIGZMessage2Standard*>(&message)));
				if (!result)
				{
					logger.WriteLine(
						LogOptions::Errors,
						"Failed to send the updated 'Travel type can reach destination' value to the traffic simulator.");
				}
			}
			else
			{
				logger.WriteLine(
					LogOptions::Errors,
					"Failed to send the updated 'Travel type can reach destination' value to the traffic simulator.");
			}

			// Verify that are in-memory modifications to the exemplar are sill present.

			result = pResourceManager->GetResource(
				key,
				GZIID_cISCPropertyHolder,
				reinterpret_cast<void**>(&propertyHolder),
				0,
				nullptr);

			if (result)
			{
				cISCProperty* property = propertyHolder->GetProperty(kTravelTypeCanReachDestination);

				if (property)
				{
					cIGZVariant* data = property->GetPropertyValue();

					if (data)
					{
						uint16_t type = data->GetType();
						uint32_t count = data->GetCount();

						if (type == cIGZVariant::Type::BoolArray && count == 9)
						{
							bool* values = data->RefBool();

							if (values[1] != carCanReachDestination)
							{
								logger.WriteLine(
									LogOptions::Errors,
									"Someone else changed the 'Travel type can reach destination' value, cache refresh?.");
							}
						}
					}
				}

				propertyHolder->Release();
				propertyHolder = nullptr;
			}
		}
	}

	if (!pSimulator->HiddenResume())
	{
		logger.WriteLine(LogOptions::Errors, "Failed to resume the game.");
	}
}

void ParknRideOrdinance::SetName(const cIGZString& name)
{
	if (name.Strlen() > 0 && !this->name.IsEqual(name, false))
	{
		this->name.Copy(name);
	}
}

void ParknRideOrdinance::SetDescription(const cIGZString& description)
{
	if (name.Strlen() > 0 && !this->description.IsEqual(description, false))
	{
		this->description.Copy(description);
	}
}

int64_t ParknRideOrdinance::GetCurrentMonthlyIncome()
{
	return 0;
}

bool ParknRideOrdinance::SetOn(bool isOn)
{
	if (on != isOn)
	{
		on = isOn;
		UpdateCarCanReachDestination();
	}

	return true;
}

bool ParknRideOrdinance::PostCityInit(cISC4City* pCity)
{
	bool result = OrdinanceBase::PostCityInit(pCity);

	if (result)
	{
		if (pCity)
		{
			result = true;
		}
		else
		{
			result = false;
		}
	}

	return result;
}

bool ParknRideOrdinance::PreCityShutdown(cISC4City* pCity)
{
	bool result = OrdinanceBase::PreCityShutdown(pCity);

	return result;
}
