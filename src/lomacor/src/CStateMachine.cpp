// Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CStateMachine.h"
#include "CZip.h"
#include <chrono>
#include <spdlog/spdlog-inl.h>

CStateMachine::CStateMachine(const std::string& _zenodo_url, const std::string& _access_token, 
    const std::string& _maps_folder, const std::string& _map_filetype, 
    const int& _upload_th, const bool& _is_mapper, 
    const std::string& _family_name, const std::string& _given_name, const std::string& _affiliation) :
    m_MapsFolder(_maps_folder),
    m_MapsFileType(_map_filetype),
    m_UpdateTh(std::chrono::seconds(_upload_th)),
    m_bIsMapper(_is_mapper)
{
    m_Zenodo.set_auth_headers(_family_name, _given_name, _affiliation, _zenodo_url, _access_token);
}

std::vector<int> CStateMachine::encode(const int& _cmd, const int& _map_id, std::string& _local_path)
{
    std::vector<int> metadata;
    metadata.emplace_back(_cmd);
    metadata.emplace_back(static_cast<int>('\n')); // separator
    metadata.emplace_back(_map_id);
    for (char c : _local_path)
        metadata.emplace_back(static_cast<int>(c));
    return metadata;
}

std::vector<int> CStateMachine::encode()
{
    LomacorCmd cmd; int map_id; std::string local_path;
    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        cmd = m_Cmd;
        map_id = m_CurrentMap;
        local_path = m_CurrentMapFilePath.string();
    }
    return encode(cmd, map_id, local_path);
}

bool CStateMachine::decode(const std::vector<int>& _metadata, int& _out_cmd, int& _out_map_id, std::string& _out_filepath)
{
    _out_cmd = _metadata[0];
    _out_filepath.clear();

    if (_metadata.size() < 6)
        return false;

    std::vector<std::pair<int, std::string>> maps;

    int cr = static_cast<int>('\n');
    bool bIsId = false;
    std::vector<int> decoded_link;

    // Deserialize: Extract cmd, id and links from maps_metadata
    for (int i = 1; i < _metadata.size(); ++i)
    {
        const int q = _metadata[i];
        if (q == cr)
        {
            if (!decoded_link.empty())
            {
                std::string sLink;
                for (int val : decoded_link)
                    sLink += static_cast<char>(val);
                maps.emplace_back(std::make_pair(_out_map_id, sLink));
            }

            bIsId = true;
            continue;
        }

        if (bIsId)
        {
            _out_map_id = q;
            decoded_link.clear();
            bIsId = false;
            continue;
        }

        decoded_link.emplace_back(q);
    }

    if (!decoded_link.empty())
    {
        for (int val : decoded_link)
            _out_filepath += static_cast<char>(val);
        maps.emplace_back(std::make_pair(_out_map_id, _out_filepath));
    }

    return true;
}

bool CStateMachine::is_active()
{
    return m_Zenodo.is_active();
}

void CStateMachine::set_is_mapper(const bool& _is_mapper)
{
    m_bIsMapper = _is_mapper;
}

CStateMachine::LomacorCmd CStateMachine::get_cmd()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_Cmd;
}

std::string CStateMachine::get_map_file()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_CurrentMapFilePath.string();
}

std::string CStateMachine::get_region_name()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    return m_CurrentRegionName;
}

void CStateMachine::set_build_map()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Cmd = CMD_BUILD_MAP;
}

void CStateMachine::set_use_map()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Cmd = CMD_USE_MAP;
}

void CStateMachine::set_undef()
{
    std::lock_guard<std::mutex> lock(m_Mutex);
    m_Cmd = CMD_UNDEF;
}

void CStateMachine::step(const std::string& _region, const int& _map_id)
{
    // Get the ID of the region from Zenodo
    int nDepositionID = m_Zenodo.find_deposit(_region);

    if (nDepositionID < 0)
        return;

    std::filesystem::path region_folder_path = m_MapsFolder / std::filesystem::path(_region);

    // Check if the region folder exists
    if (!std::filesystem::exists(region_folder_path))
        std::filesystem::create_directories(region_folder_path);

    // Check if the region or the map ID have changed
    if (m_Cmd == CMD_USE_MAP && _map_id == m_CurrentMap && nDepositionID == m_CurrentRegion)
    {
        spdlog::info("CStateMachine::step(): Region and map unchanged.");
        return;
    }

    // Map and archive files paths
    std::string arch_filename = std::to_string(_map_id) + m_ArchFileType;
    std::string map_filename = std::to_string(_map_id) + m_MapsFileType;
    std::filesystem::path arch_path = region_folder_path / arch_filename;
    std::filesystem::path map_path = region_folder_path / map_filename;

    {
        std::lock_guard<std::mutex> lock(m_Mutex);
        m_CurrentRegion = nDepositionID;
        m_CurrentRegionName = _region;
        m_CurrentMap = _map_id;
        m_CurrentMapFilePath = map_path;
    }

    // Download the map file (if it exists in Zenodo)
    if (m_Zenodo.download_file(nDepositionID, arch_filename, arch_path.string()))
    {
        spdlog::info("Downloaded '{}' map '{}' to '{}'", _region, _map_id, arch_path.string());

        set_use_map();

        // Unzip
        CZip::extract_zip(arch_path.string(), arch_path.parent_path().string());

        // Remove zip file
        if (!std::filesystem::remove(arch_path))
            spdlog::error("Map file '{}' could not be removed.", arch_path.string());
    }
    else if (m_bIsMapper)
    {
        spdlog::info("'{}' map '{}' not found on Zenodo. Initiating BUILD_MAP command.", _region, _map_id);

        set_build_map();

        // Check if the file exists
        //if (is_map_building_finished(map_path))
        spdlog::info("qqq: {} -- {}", std::filesystem::exists(map_path.c_str()), map_path.string());
        if (std::filesystem::exists(map_path.c_str()))
        {
            // Zip map
            CZip::create_zip(arch_path.string(), { map_path.string()});

            // Upload to Zenodo
            if (m_Zenodo.upload_file(nDepositionID, arch_path.string()))
                spdlog::info("Uploaded '{}' map '{}' on Zenodo", _region, _map_id);
            else
                spdlog::error("Failed to upload '{}' map '{}' on Zenodo", _region, _map_id);

            // Delete archive file
            std::filesystem::remove(arch_path);

            set_use_map();
        }
    }
    else
    {
        set_undef();
    }
}

bool CStateMachine::is_map_building_finished(const std::string& _map_file)
{
    if (!std::filesystem::exists(_map_file))
        return false;

    std::filesystem::file_time_type last_map_write_time = std::filesystem::file_time_type(std::filesystem::file_time_type::duration::zero());
    try
    {
        last_map_write_time = std::filesystem::last_write_time(_map_file);
    }
    catch (const std::filesystem::filesystem_error& e)
    {
        spdlog::error("CStateMachine::step(): {}", e.what());
        return false;
    }

    std::chrono::system_clock::duration time_since_write = std::filesystem::file_time_type::clock::now() - last_map_write_time;
    //auto seconds_since_write = std::chrono::duration_cast<std::chrono::seconds>(time_since_write);
    //std::cout << "time_since_write = " << seconds_since_write.count() << " sec" << std::endl;

    if (time_since_write > m_UpdateTh)
        return true;
    else
        return false;
}

std::string CStateMachine::cmd2str(const int& _cmd)
{
    if (_cmd == CMD_BUILD_MAP)
        return std::string("BUILD_MAP");
    else if (_cmd == CMD_USE_MAP)
        return std::string("USE_MAP");
    else
        return std::string("UNDEF");
}

std::string CStateMachine::print_state()
{
    std::string sReturn;
    std::vector<int> state = encode();
    
    int cmd, map_id; std::string map_filepath;
    if (decode(state, cmd, map_id, map_filepath))
        sReturn = "(deposit " + m_CurrentRegionName + ") cmd '" + cmd2str(cmd) + "' using file ID " + std::to_string(map_id) + " '" + map_filepath + "'";
    else
        sReturn = "Could not decode Lomacor state.";

    return sReturn;
}
