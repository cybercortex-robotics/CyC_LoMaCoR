// Copyright (c) 2025 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CLomacorMapsFilter.h"

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>

DYNALO_EXPORT CyC_FILTER_TYPE DYNALO_CALL getFilterType()
{
    CycDatablockKey key;
    return CLomacorMapsFilter(key).getFilterType();
}

DYNALO_EXPORT CCycFilterBase* DYNALO_CALL createFilter(const ConfigFilterParameters _params)
{
    return new CLomacorMapsFilter(_params);
}

CLomacorMapsFilter::CLomacorMapsFilter(CycDatablockKey key) : CCycFilterBase(key)
{
    // Assign the filter type, input type and output type
    setFilterType("CyC_LOMACOR_MAPS_FILTER_TYPE");
    m_OutputDataType = CyC_OCTREE;
}

CLomacorMapsFilter::CLomacorMapsFilter(const ConfigFilterParameters& params) : CCycFilterBase(params)
{
    // Assign the filter type, input type and output type
    setFilterType("CyC_LOMACOR_MAPS_FILTER_TYPE");
    m_OutputDataType = CyC_OCTREE;
}

CLomacorMapsFilter::~CLomacorMapsFilter()
{
	if (m_bIsEnabled)
		disable();
}

bool CLomacorMapsFilter::enable()
{
    // Read input sources
    if (!this->isReplayFilter() && !this->isNetworkFilter())
    {
        for (CycInputSource& src : getInputSources())
        {
            
        }
    }
    
    spdlog::info("Filter [{}-{}]: CLomacorMapsFilter::enable() successful", getFilterKey().nCoreID, getFilterKey().nFilterID);

    m_bIsEnabled = true;
	return true;
}

bool CLomacorMapsFilter::disable()
{	
	if (isRunning())
        stop();

	m_bIsEnabled = false;
	return true;
}

bool CLomacorMapsFilter::process()
{
    bool bReturn(false);

    if (bReturn)
    {
        //updateData(???);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    
    return bReturn;
}

void CLomacorMapsFilter::loadFromDatastream(const std::string& datastream_entry, const std::string& db_root_path)
{}
