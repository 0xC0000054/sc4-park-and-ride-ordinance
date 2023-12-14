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

#pragma once
#include "OrdinanceBase.h"

class ParknRideOrdinance final : public OrdinanceBase
{
public:

	ParknRideOrdinance();

	void UpdateCarCanReachDestination(bool calledFromPostCityInit) const;

	// Sets the name of the ordinance.
	// This is used to allow the value to be translated.
	void SetName(const cIGZString& name);

	// Sets the description of the ordinance.
	// This is used to allow the value to be translated.
	void SetDescription(const cIGZString& description);

	// Gets the monthly income or expense when the ordinance is enabled.
	int64_t GetCurrentMonthlyIncome() override;

	bool SetOn(bool isOn) override;

	// Initializes the ordinance when entering a city.
	bool PostCityInit(cISC4City* pCity) override;

	// Shuts down the ordinance when exiting a city.
	bool PreCityShutdown(cISC4City* pCity) override;

private:

	cISC4City* pCity;
};

