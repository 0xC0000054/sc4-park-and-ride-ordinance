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

#pragma once
#include <cstdint>
#include <stdlib.h>

class cSC4TrafficSimulator
{
public:
	uint8_t unknown[0xc3d];
	bool rgTravelTypeCanReachDestination[9];
};

static_assert(offsetof(cSC4TrafficSimulator, rgTravelTypeCanReachDestination) == 0xc3d);