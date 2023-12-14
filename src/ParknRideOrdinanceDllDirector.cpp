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
 
#include "version.h"
#include "Logger.h"
#include "ParknRideOrdinance.h"
#include "cIGZFrameWork.h"
#include "cIGZApp.h"
#include "cISC4App.h"
#include "cISC4City.h"
#include "cIGZLanguageManager.h"
#include "cGZPersistResourceKey.h"
#include "cIGZPersistResourceManager.h"
#include "cISCProperty.h"
#include "cISCPropertyHolder.h"
#include "cISC4Ordinance.h"
#include "cISC4OrdinanceSimulator.h"
#include "cISC4ResidentialSimulator.h"
#include "cIGZMessageServer2.h"
#include "cIGZMessageTarget.h"
#include "cIGZMessageTarget2.h"
#include "cIGZString.h"
#include "cRZMessage2COMDirector.h"
#include "cRZMessage2Standard.h"
#include "cRZBaseString.h"
#include "GZServPtrs.h"
#include <array>
#include <filesystem>
#include <fstream>
#include <memory>
#include <locale.h>
#include <string>
#include <vector>
#include <Windows.h>
#include "wil/resource.h"
#include "wil/win32_helpers.h"

static constexpr uint32_t kSC4MessagePostCityInit = 0x26D31EC1;
static constexpr uint32_t kSC4MessagePreCityShutdown = 0x26D31EC2;
static constexpr uint32_t kSC4MessagePostAppServicesInit = 0x2B96B3EA;

static constexpr uint32_t kParknRideOrdinancePluginDirectorID = 0x198d91a2;

static constexpr std::string_view PluginConfigFileName = "SC4ParknRideOrdinance.ini";
static constexpr std::string_view PluginLogFileName = "SC4ParknRideOrdinance.log";

class ParknRideOrdinanceDllDirector : public cRZMessage2COMDirector
{
public:

	ParknRideOrdinanceDllDirector()
		: parkAndRideOrdinance(),
		  configFilePath(),
		  localizedName(),
		  localizedDescription()
	{
		std::filesystem::path dllFolderPath = GetDllFolderPath();

		configFilePath = dllFolderPath;
		configFilePath /= PluginConfigFileName;

		std::filesystem::path logFilePath = dllFolderPath;
		logFilePath /= PluginLogFileName;

		Logger& logger = Logger::GetInstance();
#ifdef _DEBUG
		logger.Init(logFilePath, LogOptions::All);
#else
		logger.Init(logFilePath, LogOptions::InfoAndErrors);
#endif // _DEBUG


		logger.WriteLogFileHeader("SC4ParknRideOrdinance v" PLUGIN_VERSION_STR);
	}

	uint32_t GetDirectorID() const
	{
		return kParknRideOrdinancePluginDirectorID;
	}

	void EnumClassObjects(cIGZCOMDirector::ClassObjectEnumerationCallback pCallback, void* pContext)
	{
		// The classes you want to add must be initialized in the DLL constructor because
		// the framework calls this method before OnStart or any of the hook callbacks.
		// This method is called once when initializing a director, the list of class IDs
		// it returns is cached by the framework.
		pCallback(parkAndRideOrdinance.GetID(), 0, pContext);
	}

	bool GetClassObject(uint32_t rclsid, uint32_t riid, void** ppvObj)
	{
		// To retrieve an instance of a registered class the framework will call the
		// GetClassObject method whenever it needs the director to provide one.

		bool result = false;

		if (rclsid == parkAndRideOrdinance.GetID())
		{
			result = parkAndRideOrdinance.QueryInterface(riid, ppvObj);
		}

		return result;
	}

	bool TryGetResourceString(uint32_t groupID, uint32_t instanceID, cIGZString** outString)
	{
		bool result = false;

		if (outString)
		{
			if (*outString)
			{
				(*outString)->Release();
				*outString = nullptr;
			}

			constexpr uint32_t LTEXTTypeID = 0x2026960B;

			cGZPersistResourceKey key(LTEXTTypeID, groupID, instanceID);

			cIGZPersistResourceManagerPtr resourceManager;
			if (resourceManager)
			{
				// GetPrivateResource skips adding the value to the game's resource cache.
				result = resourceManager->GetPrivateResource(
					key,
					GZIID_cIGZString,
					reinterpret_cast<void**>(outString),
					0,
					nullptr);
			}
		}

		return result;
	}

	bool GetLocalizedText(uint32_t defaultLanguageGroupID, uint32_t instanceID, cIGZString** outString)
	{
		bool result = false;

		cIGZLanguageManagerPtr languageManager;

		if (languageManager)
		{
			// The localized resources use a group ID that is offset from the default language
			// group ID. This system allows a single DAT file to contain string resources for all
			// of the languages that are supported by the game.

			const uint32_t currentLanguage = languageManager->GetCurrentLanguage();
			const uint32_t currentLanguageGroupID = defaultLanguageGroupID + currentLanguage;

			// We will search the loaded string resources for a matching value in
			// the game's currently configured language. If one is not found we will use
			// the default English string resources.
			// If both of those fail we will fall back to using the hard-coded ordinance
			// name and description.

			result = TryGetResourceString(currentLanguageGroupID, instanceID, outString);
			if (!result)
			{
				result = TryGetResourceString(defaultLanguageGroupID, instanceID, outString);
			}
		}

		return result;
	}

	void LoadLocalizedStringResources()
	{
		const uint32_t DefaultLanguageGroupID = 0xB5E861D2;
		constexpr uint32_t OrdinanceNameInstanceID = 0xB9E7C616;
		constexpr uint32_t OrdinanceDescriptionInstanceID = 0x0F85A3C7;

		cIGZString* name = nullptr;
		cIGZString* description = nullptr;

		if (GetLocalizedText(DefaultLanguageGroupID, OrdinanceNameInstanceID, &name))
		{
			if (GetLocalizedText(DefaultLanguageGroupID, OrdinanceDescriptionInstanceID, &description))
			{
				localizedName.Copy(*name);
				localizedDescription.Copy(*description);

				description->Release();
			}

			name->Release();
		}
	}

	void PostAppServicesInit()
	{
		LoadLocalizedStringResources();

		cIGZMessageServer2Ptr pMsgServ;

		if (pMsgServ)
		{
			pMsgServ->RemoveNotification(this, kSC4MessagePostAppServicesInit);
		}
	}

	void PostCityInit(cIGZMessage2Standard* pStandardMsg)
	{
		cISC4City* pCity = reinterpret_cast<cISC4City*>(pStandardMsg->GetIGZUnknown());

		if (pCity)
		{
			cISC4OrdinanceSimulator* pOrdinanceSimulator = pCity->GetOrdinanceSimulator();

			if (pOrdinanceSimulator)
			{
				cISC4Ordinance* pOrdinance = pOrdinanceSimulator->GetOrdinanceByID(parkAndRideOrdinance.GetID());
				bool ordinanceInitialized = false;

				if (!pOrdinance)
				{
					// Only add the ordinance if it is not already present. If it is part
					// of the city save file it will have already been loaded at this point.
					parkAndRideOrdinance.PostCityInit(pCity);
					ordinanceInitialized = true;

					pOrdinanceSimulator->AddOrdinance(parkAndRideOrdinance);
					pOrdinance = pOrdinanceSimulator->GetOrdinanceByID(parkAndRideOrdinance.GetID());
				}

				if (pOrdinance)
				{
					ParknRideOrdinance* pParkAndRideOrdinance = reinterpret_cast<ParknRideOrdinance*>(pOrdinance);

					if (!ordinanceInitialized)
					{
						pParkAndRideOrdinance->PostCityInit(pCity);
					}

					pParkAndRideOrdinance->SetName(localizedName);
					pParkAndRideOrdinance->SetDescription(localizedDescription);
					pParkAndRideOrdinance->UpdateCarCanReachDestination(/*calledFromPostCityInit*/true);
				}
				else
				{
					Logger::GetInstance().WriteLine(LogOptions::Errors, "Failed to add the ordinance.");
				}

				//DumpRegisteredOrdinances(pCity, pOrdinanceSimulator);
			}
		}
	}

	void PreCityShutdown(cIGZMessage2Standard* pStandardMsg)
	{
		cISC4City* pCity = reinterpret_cast<cISC4City*>(pStandardMsg->GetIGZUnknown());

		if (pCity)
		{
			cISC4OrdinanceSimulator* pOrdinanceSimulator = pCity->GetOrdinanceSimulator();

			if (pOrdinanceSimulator)
			{
				cISC4Ordinance* pOrdinance = pOrdinanceSimulator->GetOrdinanceByID(parkAndRideOrdinance.GetID());

				if (pOrdinance)
				{
					ParknRideOrdinance* pParkAndRideOrdinance = reinterpret_cast<ParknRideOrdinance*>(pOrdinance);
					pParkAndRideOrdinance->PreCityShutdown(pCity);
					pOrdinanceSimulator->RemoveOrdinance(*pOrdinance);
				}
			}
		}
	}

	bool DoMessage(cIGZMessage2* pMessage)
	{
		cIGZMessage2Standard* pStandardMsg = static_cast<cIGZMessage2Standard*>(pMessage);
		uint32_t dwType = pMessage->GetType();

		switch (dwType)
		{
		case kSC4MessagePostAppServicesInit:
			PostAppServicesInit();
			break;
		case kSC4MessagePostCityInit:
			PostCityInit(pStandardMsg);
			break;
		case kSC4MessagePreCityShutdown:
			PreCityShutdown(pStandardMsg);
			break;
		}

		return true;
	}

	bool PostAppInit()
	{
		Logger& logger = Logger::GetInstance();

		cIGZMessageServer2Ptr pMsgServ;
		if (pMsgServ)
		{
			std::vector<uint32_t> requiredNotifications;
			requiredNotifications.push_back(kSC4MessagePostCityInit);
			requiredNotifications.push_back(kSC4MessagePreCityShutdown);
			requiredNotifications.push_back(kSC4MessagePostAppServicesInit);

			for (uint32_t messageID : requiredNotifications)
			{
				if (!pMsgServ->AddNotification(this, messageID))
				{
					logger.WriteLine(LogOptions::Errors, "Failed to subscribe to the required notifications.");
					return true;
				}
			}
		}
		else
		{
			logger.WriteLine(LogOptions::Errors, "Failed to subscribe to the required notifications.");
			return true;
		}
		return true;
	}

	bool OnStart(cIGZCOM* pCOM)
	{
		cIGZFrameWork* const pFramework = RZGetFrameWork();

		if (pFramework->GetState() < cIGZFrameWork::kStatePreAppInit)
		{
			pFramework->AddHook(this);
		}
		else
		{
			PreAppInit();
		}
		return true;
	}

private:

	std::filesystem::path GetDllFolderPath()
	{
		wil::unique_cotaskmem_string modulePath = wil::GetModuleFileNameW(wil::GetModuleInstanceHandle());

		std::filesystem::path temp(modulePath.get());

		return temp.parent_path();
	}

	void DumpRegisteredOrdinances(cISC4City* pCity, cISC4OrdinanceSimulator* pOrdinanceSimulator)
	{
		Logger& logger = Logger::GetInstance();

		if (!logger.IsEnabled(LogOptions::DumpRegisteredOrdinances))
		{
			return;
		}

		uint32_t dwCountOut = 0;

		uint32_t registeredOrdinances = pOrdinanceSimulator->GetOrdinanceIDArray(nullptr, dwCountOut);

		logger.WriteLineFormatted(
			LogOptions::DumpRegisteredOrdinances,
			"The game has %d ordinances registered.",
			registeredOrdinances);

		if (registeredOrdinances > 0)
		{
			std::vector<uint32_t> registeredOrdinanceIDs(static_cast<size_t>(registeredOrdinances));
			uint32_t ordinancesRequested = registeredOrdinances;

			uint32_t ordinancesFetched = pOrdinanceSimulator->GetOrdinanceIDArray(registeredOrdinanceIDs.data(), ordinancesRequested);

			if (ordinancesFetched > 0)
			{
				int32_t cityPopulation = -1;
				cISC4ResidentialSimulator* pResidentialSimulator = pCity->GetResidentialSimulator();
				if (pResidentialSimulator)
				{
					cityPopulation = pResidentialSimulator->GetPopulation();
				}

				for (uint32_t i = 0; i < ordinancesFetched; i++)
				{
					uint32_t clsid = registeredOrdinanceIDs[i];

					cISC4Ordinance* pOrdinance = pOrdinanceSimulator->GetOrdinanceByID(clsid);

					if (pOrdinance)
					{
						uint32_t id = pOrdinance->GetID();
						cIGZString* name = pOrdinance->GetName();
						bool isIncome = pOrdinance->IsIncomeOrdinance();
						int64_t enactmentIncome = pOrdinance->GetEnactmentIncome();
						int64_t retracmentIncome = pOrdinance->GetRetracmentIncome();
						int64_t monthlyConstantIncome = pOrdinance->GetMonthlyConstantIncome();
						float monthlyIncomeFactor = pOrdinance->GetMonthlyIncomeFactor();
						int64_t currentMonthlyIncome = pOrdinance->GetCurrentMonthlyIncome();

						if (name)
						{
							logger.WriteLineFormatted(
								LogOptions::DumpRegisteredOrdinances,
								"0x%08x = %s, income=%s, enactment=%lld, retracment=%lld, monthly: constant=%lld, factor=%f, current=%lld, city population=%d",
								clsid,
								name->ToChar(),
								isIncome ? "true" : "false",
								enactmentIncome,
								retracmentIncome,
								monthlyConstantIncome,
								monthlyIncomeFactor,
								currentMonthlyIncome,
								cityPopulation);
						}
						else
						{
							logger.WriteLineFormatted(LogOptions::DumpRegisteredOrdinances, "0x%08x", clsid);
						}
					}
					else
					{
						logger.WriteLineFormatted(LogOptions::DumpRegisteredOrdinances, "0x%08x", clsid);
					}
				}
			}
		}
	}

	ParknRideOrdinance parkAndRideOrdinance;
	std::filesystem::path configFilePath;
	cRZBaseString localizedName;
	cRZBaseString localizedDescription;
};

cRZCOMDllDirector* RZGetCOMDllDirector() {
	static ParknRideOrdinanceDllDirector sDirector;
	return &sDirector;
}