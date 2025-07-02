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
    m_OutputDataType = CyC_VECTOR_INT;
}

CLomacorMapsFilter::CLomacorMapsFilter(const ConfigFilterParameters& params) : CCycFilterBase(params)
{
    // Assign the filter type, input type and output type
    setFilterType("CyC_LOMACOR_MAPS_FILTER_TYPE");
    m_OutputDataType = CyC_VECTOR_INT;
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
            if (src.pCycFilter->getFilterType() == CStringUtils::CyC_HashFunc("CyC_LOMACOR_SERVER_FILTER_TYPE"))
                m_pInputFilterMapsServer = src.pCycFilter;
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
    std::vector<CyC_INT> maps_metadata;

    if (m_pInputFilterMapsServer != nullptr)
    {
        CyC_TIME_UNIT readInputTsServer = m_pInputFilterMapsServer->getTimestampStop();
        if (readInputTsServer > m_lastTsServer)
        {
            if (m_pInputFilterMapsServer->getData(maps_metadata))
            {
                m_lastTsServer = readInputTsServer;
                std::vector<std::pair<CyC_INT, std::string>> maps = decode(maps_metadata);
                bReturn = true;
            }
        }
    }

    if (bReturn)
    {
        maps_metadata[0] = 2;
        updateData(maps_metadata);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    
    return bReturn;
}

void CLomacorMapsFilter::loadFromDatastream(const std::string& datastream_entry, const std::string& db_root_path)
{}

std::vector<std::pair<CyC_INT, std::string>> CLomacorMapsFilter::decode(const std::vector<CyC_INT>& _maps_metadata)
{
    std::vector<std::pair<CyC_INT, std::string>> maps;

    CyC_INT cr = static_cast<CyC_INT>('\n');
    bool bIsId = false;
    CyC_INT decoded_cmd;
    CyC_INT decoded_id;
    std::vector<CyC_INT> decoded_link;

    // Deserialize: Extract id and link from maps_metadata
    for (int i = 1; i < _maps_metadata.size(); ++i)
    {
        const CyC_INT q = _maps_metadata[i];
        if (q == cr)
        {
            if (!decoded_link.empty())
            {
                std::string sLink;
                for (CyC_INT val : decoded_link)
                    sLink += static_cast<char>(val);
                maps.emplace_back(std::make_pair(decoded_id, sLink));
            }

            bIsId = true;
            continue;
        }

        if (bIsId)
        {
            decoded_id = q;
            decoded_link.clear();
            bIsId = false;
            continue;
        }

        decoded_link.emplace_back(q);
    }

    if (!decoded_link.empty())
    {
        std::string sLink;
        for (CyC_INT val : decoded_link)
            sLink += static_cast<char>(val);
        maps.emplace_back(std::make_pair(decoded_id, sLink));
    }

    return maps;
}
