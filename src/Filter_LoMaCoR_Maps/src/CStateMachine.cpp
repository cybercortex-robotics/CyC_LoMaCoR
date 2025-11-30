// Copyright (c) 2025 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CStateMachine.h"

CStateMachine::CStateMachine(const std::string& _maps_folder, const std::string& _zenodo_url, const std::string& _access_token) :
    m_MapsFolder(_maps_folder)
{
    m_Zenodo.set_auth_headers(_zenodo_url, _access_token);
}

bool CStateMachine::query_map(const std::string& _region, const int& _map_id)
{
    int nDepositionID = m_Zenodo.find_deposit(_region);

    if (nDepositionID < 0)
        return false;

    fs::path region_folder_path = m_MapsFolder / fs::path(_region);
    
    // Check if the region folder exists
    if (!fs::exists(region_folder_path))
        fs::create_directories(region_folder_path);
    
    std::string map_filename = std::to_string(_map_id) + ".zip";
    fs::path map_download_path = region_folder_path / map_filename;
    if (m_Zenodo.download_file(nDepositionID, map_filename, map_download_path.string()))
    {
        spdlog::info("Downloaded '{}' map '{}' to '{}'", _region, _map_id, map_download_path.string());
    }
    else
    {
        spdlog::error("'{}' map '{}' not found on Zenodo", _region, _map_id);
        return false;
    }


}
