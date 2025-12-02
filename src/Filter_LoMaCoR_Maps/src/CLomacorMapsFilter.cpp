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

    // Maps folder
    m_MapsFolder = fs::path(params.sGlobalBasePath) / fs::path(m_CustomParameters.at("maps_folder"));
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

        // Zenodo url
        std::string sZenodoUrl;
        if (m_CustomParameters.find("zenodo_url") != m_CustomParameters.end())
        {
            sZenodoUrl = m_CustomParameters.at("zenodo_url");
        }
        else
        {
            log_error("Zenodo URL required");
            return false;
        }

        // Zenodo access token
        std::string sAccessToken;
        if (m_CustomParameters.find("access_token") != m_CustomParameters.end())
        {
            sAccessToken = m_CustomParameters.at("access_token");
        }
        else
        {
            log_error("Zenodo access token required.");
            return false;
        }

        // City
        if (m_CustomParameters.find("city") != m_CustomParameters.end())
        {
            m_sCity = m_CustomParameters.at("city");
        }
        else
        {
            log_error("City required.");
            return false;
        }

        // Map
        if (m_CustomParameters.find("map") != m_CustomParameters.end())
        {
            m_nMapID = std::stoi(m_CustomParameters.at("map"));
        }
        else
        {
            log_error("City required.");
            return false;
        }

        // Check if the maps folder exists
        if (!fs::exists(m_MapsFolder))
        {
            log_error("Maps folder does not exists.");
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
                // Query Zenodo for a map
                int nDepositionID = m_Zenodo.find_deposit(m_sCity);
                if (nDepositionID >= 0)
                {
                    log_info("City '{}' found on Zenodo with ID '{}'", m_sCity, nDepositionID);

                    std::string map_filename = std::to_string(m_nMapID) + ".zip";
                    fs::path map_download_path = m_MapsFolder / map_filename;
                    if (m_Zenodo.download_file(nDepositionID, map_filename, map_download_path.string()))
                    {
                        log_info("Downloaded '{}' map '{}' to '{}'", m_sCity, m_nMapID, map_download_path.string());
                    }
                    else
                    {
                        log_info("'{}' map '{}' not found on Zenodo", m_sCity, m_nMapID);
                    }
                }
                else
                {
                    log_error("City '{}' not found on Zenodo", m_sCity);
                }

                // If the map is found, download the map

                // Send the map to the slam system


                maps_metadata.clear();

                // Encode
                maps_metadata.emplace_back(2); // cmd
                maps_metadata.emplace_back(static_cast<CyC_INT>('\n')); // separator
                maps_metadata.emplace_back(1); // Map ID
                std::string path = "C:/dev/src/CyberCortex.AI/CyC_LoMaCoR/etc/env/maps/1.map";
                for (char c : path)
                    maps_metadata.emplace_back(static_cast<CyC_INT>(c));

                // Decode
                int _out_cmd, _out_map_id; 
                std::string _out_filepath;
                //decode(maps_metadata, _out_cmd, _out_map_id, _out_filepath);

                //spdlog::info("\tcmd {}: map {} -- {}", _out_cmd, _out_map_id, _out_filepath);

                bReturn = true;
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
