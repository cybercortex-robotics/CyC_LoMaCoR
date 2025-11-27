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
            switch (src.pCycFilter->getOutputDataType())
            {
                case CyC_SLAM:
                    m_pInputFilterSlam = src.pCycFilter;
                    break;
            }

            if (src.pCycFilter->getFilterType() == CStringUtils::CyC_HashFunc("CyC_LOMACOR_SERVER_FILTER_TYPE"))
                m_pInputFilterMapsServer = src.pCycFilter;
        }

        // Config parameters
        std::string sZenodoUrl, sAccessToken;
        if (m_CustomParameters.find("zenodo_url") != m_CustomParameters.end())
        {
            sZenodoUrl = m_CustomParameters.at("zenodo_url");
        }
        else
        {
            log_error("Zenodo URL required");
            return false;
        }

        if (m_CustomParameters.find("access_token") != m_CustomParameters.end())
        {
            sAccessToken = m_CustomParameters.at("access_token");
        }
        else
        {
            log_error("Zenodo access token required.");
            return false;
        }

        if (m_CustomParameters.find("city") != m_CustomParameters.end())
        {
            m_sCity = m_CustomParameters.at("city");
        }
        else
        {
            log_error("City required.");
            return false;
        }

        if (!m_Zenodo.set_auth_headers(sZenodoUrl, sAccessToken))
            return false;
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

    // Data from maps server
    /*if (m_pInputFilterMapsServer != nullptr)
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
    }*/

    // Data from Slam algorithm
    if (m_pInputFilterSlam != nullptr)
    {
        CyC_TIME_UNIT readInputTsSlam = m_pInputFilterSlam->getTimestampStop();
        if (readInputTsSlam > m_lastTsSlam)
        {
            CycSlam slam_data;
            if (m_pInputFilterSlam->getData(slam_data))
            {
                /*spdlog::info("Slam mapping: {}", slam_data.is_mapping);

                maps_metadata.clear();
                maps_metadata.push_back(1);
                bReturn = true;*/
            }
        }
    }

    if (bReturn)
    {
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

    // Deserialize: Extract cmd, id and links from maps_metadata
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
