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
        
        // Region
        if (m_CustomParameters.find("region") != m_CustomParameters.end())
        {
            m_sRegion = m_CustomParameters.at("region");
        }
        else
        {
            log_error("Region required.");
            return false;
        }

        // Map
        if (m_CustomParameters.find("map") != m_CustomParameters.end())
        {
            m_nMapID = std::stoi(m_CustomParameters.at("map"));
        }
        else
        {
            log_error("Map required.");
            return false;
        }

        // Map filetype
        std::string map_filetype;
        if (m_CustomParameters.find("map_filetype") != m_CustomParameters.end())
        {
            map_filetype = m_CustomParameters.at("map_filetype");
        }
        else
        {
            log_error("Map filetype required.");
            return false;
        }

        // Upload threshold
        int uploat_th;
        if (m_CustomParameters.find("map_upload_th") != m_CustomParameters.end())
        {
            uploat_th = std::stoi(m_CustomParameters.at("map_upload_th"));
        }
        else
        {
            log_error("Map upload threshold required.");
            return false;
        }

        // Are we allowed to map?
        bool mapper = true;
        if (m_CustomParameters.find("mapper") != m_CustomParameters.end())
        {
            CStringUtils::stringToBool(m_CustomParameters.at("mapper"), mapper);
        }
        else
        {
            log_error("Mapper setting required.");
            return false;
        }

        // Check if the maps folder exists
        if (!fs::exists(m_MapsFolder))
        {
            log_error("Maps folder does not exists.");
            return false;
        }

        m_pStateMachine = std::make_unique<CStateMachine>(sZenodoUrl, sAccessToken, m_MapsFolder, map_filetype, uploat_th, mapper);

        if (!m_pStateMachine->is_active())
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

    m_pStateMachine->step(m_sRegion, m_nMapID);
    std::vector<CyC_INT> metadata = m_pStateMachine->encode();
    spdlog::info("{}", m_pStateMachine->print_state());
    bReturn = true;

    if (bReturn)
    {
        updateData(metadata);
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
    
    return bReturn;
}

void CLomacorMapsFilter::loadFromDatastream(const std::string& datastream_entry, const std::string& db_root_path)
{}
