// Copyright (c) 2025 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CLomacorServerFilter.h"
#include <os/CCsvReader.h>

#define DYNALO_EXPORT_SYMBOLS
#include <dynalo/symbol_helper.hpp>

DYNALO_EXPORT CyC_FILTER_TYPE DYNALO_CALL getFilterType()
{
    CycDatablockKey key;
    return CLomacorServerFilter(key).getFilterType();
}

DYNALO_EXPORT CCycFilterBase* DYNALO_CALL createFilter(const ConfigFilterParameters _params)
{
    return new CLomacorServerFilter(_params);
}

CLomacorServerFilter::CLomacorServerFilter(CycDatablockKey key) : CCycFilterBase(key)
{
    // Assign the filter type, input type and output type
    setFilterType("CyC_LOMACOR_SERVER_FILTER_TYPE");
    m_OutputDataType = CyC_VECTOR_INT;
}

CLomacorServerFilter::CLomacorServerFilter(const ConfigFilterParameters& params) : CCycFilterBase(params)
{
    // Assign the filter type, input type and output type
    setFilterType("CyC_LOMACOR_SERVER_FILTER_TYPE");
    m_OutputDataType = CyC_VECTOR_INT;

    if (m_CustomParameters.find("maps") != m_CustomParameters.end())
        m_sMapsPath = fs::path(params.sGlobalBasePath) / fs::path(m_CustomParameters.at("maps"));
}

CLomacorServerFilter::~CLomacorServerFilter()
{
	if (m_bIsEnabled)
		disable();
}

bool CLomacorServerFilter::enable()
{
    // Read input sources
    if (!this->isReplayFilter() && !this->isNetworkFilter())
    {
        for (CycInputSource& src : getInputSources())
        {
            
        }
    }
    
    spdlog::info("Filter [{}-{}]: CLomacorServerFilter::enable() successful", getFilterKey().nCoreID, getFilterKey().nFilterID);

    m_bIsEnabled = true;
	return true;
}

bool CLomacorServerFilter::disable()
{	
	if (isRunning())
        stop();

	m_bIsEnabled = false;
	return true;
}

bool CLomacorServerFilter::process()
{
    bool bReturn(false);
    
    //if (!m_bMapsRead)
    {
        m_MapsMetadata.clear();

        csv::reader csv_reader;
        if (!csv_reader.open(m_sMapsPath))
        {
            spdlog::error("Filter [{}-{}]: {}: failed to open csv {}", getFilterKey().nCoreID, getFilterKey().nFilterID, typeid(*this).name(), m_sMapsPath);
            return false;
        }

        CyC_INT cmd, id; std::string path;

        // Serialize: Add entries to maps_metadata
        while (csv_reader.read_row(cmd, id, path))
        {
            m_MapsMetadata.emplace_back(cmd);
            m_MapsMetadata.emplace_back(static_cast<CyC_INT>('\n'));
            m_MapsMetadata.emplace_back(id);
            for (char c : path)
                m_MapsMetadata.emplace_back(static_cast<CyC_INT>(c));
        }
        
        bReturn = true;
        m_bMapsRead = true;
    }
    
    if (bReturn)
    {
        updateData(m_MapsMetadata);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    
    return bReturn;
}

void CLomacorServerFilter::loadFromDatastream(const std::string& datastream_entry, const std::string& db_root_path)
{}
