#pragma once
#include "cIGZUnknown.h"
#include "cIGZMessageTarget2.h"
#include "ilist.h"
#include "SC4Rect.h"

class cISCPropertyHolder;
class cISC4Occupant;
class cISC4PathFinder;
class cISC4Lot;
class FerryRouteInfo;
class SC4Percentage;
class TransitSwitchQueryData;

// This version of the cISC4TrafficSimulator header implements cIGZMessageTarget2.
// This is a hack that allows messages to be sent directly to the traffic simulator
// without using the game's messaging system.

class cISC4TrafficSimulator : public cIGZUnknown, public cIGZMessageTarget2
{
public:

	virtual bool Init() = 0;
	virtual bool Shutdown() = 0;

	virtual uint32_t GetSimulatorType() = 0;

	virtual bool CreatePathFinder(cISC4PathFinder** pathFinder) = 0;
	virtual bool SetupPathFinderForLot(cISC4PathFinder* pathFinder, cISC4Lot* lot) = 0;

	virtual intptr_t GetAirPollutingTrafficMap() const = 0; // Returns a cISC4SimGrid<uint8_t>*
	virtual intptr_t GetCommercialTrafficMap() const = 0; // Returns a cISC4SimGrid<uint8_t>*
	virtual intptr_t GetCongestionMap() const = 0; // Returns a cISC4SimGrid<uint8_t>*
	virtual intptr_t GetBackgroundTraffic(int unknown1, int unknown2) = 0;
	virtual int64_t GetTrafficEdgeDensity(uint8_t unknown1, uint32_t travelType, bool unknown3) = 0;
	virtual intptr_t GetTripLengthMap() const = 0; // Returns a cISC4SimGrid<uint8_t>*

	virtual float GetTripScale() const = 0;
	virtual float GetTripScaleForDisplay() const = 0;

	virtual bool IsRoadDamaged(int unknown1, int unknown2) = 0;
	virtual bool CheckRailAccident(int unknown1, int unknown2) = 0;

	virtual float GetLastMonthlyIncome() = 0; // Not sure about the return type
	virtual bool SetMaxTripCapacity(cISCPropertyHolder* unknown1, uint32_t unknown2, uint32_t unknown3) = 0;
	virtual uint32_t GetMaxTripCapacity(cISCPropertyHolder* unknown1, uint32_t unknown2) = 0;
	virtual uint32_t GetTrafficArrived(cISCPropertyHolder* unknown1, uint32_t unknown2) = 0;
	virtual uint32_t GetDesiredLotInsertionPoint() = 0;
	virtual uint32_t GetCapacity(uint32_t networkType, int unknown2, int unknown3) = 0;

	virtual float GetTravelTimeRatio(long unknown1, long unknown2, uint32_t travelType) = 0;

	virtual uint32_t GetConnectionCount(uint32_t networkType, int unknown2, int unknown3) = 0;
	virtual bool GetTravelStrategyPercentages(uint32_t wealthType, std::vector<SC4Percentage> unknown2) = 0;
	virtual float GetFreightScalingFactor() = 0;
	virtual bool GetTransitSwitches(ilist<cISC4Occupant*>& unknown1) = 0;

	virtual int32_t GetFerryRouteBetweenTiles(long unknown1, long unknown2, long unknown3, long unknown4) = 0;
	virtual bool GetAllFerryRoutes(std::list<std::vector<uint8_t>>& unknown1) = 0;
	virtual bool GetFerryRoutesInUse(std::list<FerryRouteInfo>& unknown1) = 0;
	virtual uint32_t GetFerryTerminalCount(uint32_t ferryType) = 0;
	virtual bool GetWaterRoute(long unknown1, long unknown2, long unknown3, long unknown4, std::vector<uint8_t>& unknown5) = 0;

	virtual intptr_t GetTrafficStats() = 0; // No idea what the return type is, structure pointer, float array?
	virtual bool GetTransitSwitchQueryData(uint32_t unknown1, uint32_t unknown2, TransitSwitchQueryData& data) = 0;

	virtual bool AreLotsConnected(cISC4Lot* unknown1, cISC4Lot* unknown2) const = 0;
	virtual uint32_t GetConnectedOccupantCount(cISC4Lot* unknown1, uint32_t unknown2) const = 0;
	virtual uint32_t GetConnectedDestinationCount(cISC4Lot* unknown1, int unknown2) const = 0;
	virtual bool GetSubnetworksForLot(cISC4Lot* unknown1, std::vector<uint32_t>& unknown2) = 0;
	virtual bool GetSubnetworksInRectangle(SC4Rect<int> const& rect, std::vector<uint32_t>& unknown2) = 0;
	virtual bool GetSubnetworksInRegion(intptr_t cellRegion, std::vector<uint32_t>& unknown2) = 0; // cellRegion is SC4CellRegion<long> const&
	virtual bool GetOccupantCountForAllSubnetworks(uint32_t unknown1, std::vector<uint32_t>& unknown2) = 0;
};
