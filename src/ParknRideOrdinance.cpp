////////////////////////////////////////////////////////////////////////////
//
// This file is part of sc4-park-and-ride-ordinance, a DLL Plugin for
// SimCity 4 that adds a Park and Ride ordinance to the game.
//
// Copyright (c) 2023, 2024 Nicholas Hayes
//
// This file is licensed under terms of the MIT License.
// See LICENSE.txt for more information.
//
////////////////////////////////////////////////////////////////////////////

#include "ParknRideOrdinance.h"
#include "cSC4TrafficSimulator.h"
#include "Stopwatch.h"
#include "cISC4City.h"
#include "cISC4Simulator.h"
#include "cGZPersistResourceKey.h"
#include "cIGZMessageServer.h"
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

	void RunMessageServerPump(int maxIterations, int maxTimeInMilliseconds)
	{
		cIGZMessageServerPtr pMsgServ;

		if (pMsgServ)
		{
			Stopwatch timer;
			timer.Start();

			for (int i = 0; i < maxIterations; i++)
			{
				int queueSize = pMsgServ->GetMessageQueueSize();

				if (queueSize == 0)
				{
					break;
				}

				pMsgServ->OnTick(0);

				int64_t elapsedMilliseconds = timer.ElapsedMilliseconds();

				if (elapsedMilliseconds > maxTimeInMilliseconds)
				{
					break;
				}
			}
		}
	}

	void RunMessageServer2Pump(int maxIterations, int maxTimeInMilliseconds)
	{
		cIGZMessageServer2Ptr pMsgServ;

		if (pMsgServ)
		{
			Stopwatch timer;
			timer.Start();

			for (int i = 0; i < maxIterations; i++)
			{
				int queueSize = pMsgServ->GetMessageQueueSize();

				if (queueSize == 0)
				{
					break;
				}

				pMsgServ->OnTick(0);

				int64_t elapsedMilliseconds = timer.ElapsedMilliseconds();

				if (elapsedMilliseconds > maxTimeInMilliseconds)
				{
					break;
				}
			}
		}
	}
}

ParknRideOrdinance::ParknRideOrdinance()
	: OrdinanceBase(
		kParknRideOrdinanceCLSID,
		"Park n Ride",
		StringResourceKey(0xB5E861D2, 0xB9E7C616),
		"Program that promotes park and ride.",
		StringResourceKey(0xB5E861D2, 0x0F85A3C7),
		/* enactment income */		  0,
		/* retracment income */       0,
		/* monthly constant income */ 0,
		/* monthly income factor */   0.0f,
		/* income ordinance */		  false,
	    CreateOrdinanceEffects()),
	  pCity(nullptr)
{
}

void ParknRideOrdinance::UpdateCarCanReachDestination(bool calledFromPostCityInit) const
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

	constexpr int maxIterations = 500;
	constexpr int maxTimeInMilliseconds = 5000;

	// Process messages for a few seconds, this allows the pause
	// message subscribers time to process to the message.
	RunMessageServerPump(maxIterations, maxTimeInMilliseconds);
	RunMessageServer2Pump(maxIterations, maxTimeInMilliseconds);

	const bool carCanReachDestination = !on;

	if (pCity)
	{
		cSC4TrafficSimulator* pTrafficSim = reinterpret_cast<cSC4TrafficSimulator*>(pCity->GetTrafficSimulator());

		if (pTrafficSim)
		{
			if (pTrafficSim->rgTravelTypeCanReachDestination[1] != carCanReachDestination)
			{
				logger.WriteLineFormatted(
					LogOptions::Info,
					"Setting 'Travel type can reach destination' value for cars to %s.",
					carCanReachDestination ? "true" : "false");

				pTrafficSim->rgTravelTypeCanReachDestination[1] = carCanReachDestination;
			}
		}
		else
		{
			logger.WriteLine(
				LogOptions::Errors,
				"The traffic simulator pointer was null.");
		}
	}
	else
	{
		logger.WriteLine(
			LogOptions::Errors,
			"The city pointer was null.");
	}

	if (!pSimulator->HiddenResume())
	{
		logger.WriteLine(LogOptions::Errors, "Failed to resume the game.");
	}
}

int64_t ParknRideOrdinance::GetCurrentMonthlyIncome()
{
	return 0;
}

bool ParknRideOrdinance::SetOn(bool isOn)
{
	bool oldIsOn = isOn;

	OrdinanceBase::SetOn(isOn);
	if (oldIsOn != isOn)
	{
		UpdateCarCanReachDestination(/*calledFromPostCityInit*/false);
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
			this->pCity = pCity;
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
	pCity = nullptr;

	return result;
}
