// Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#ifndef CStateMachine_H_
#define CStateMachine_H_

#include "CZenodo.h"
#include <filesystem>

class CStateMachine
{
public:
    enum LomacorCmd
    {
        CMD_UNDEF = 0,
        CMD_BUILD_MAP = 1,
        CMD_USE_MAP = 2
    };

public:
    CStateMachine(const std::string& _zenodo_url, const std::string& _access_token,
                  const std::string& _maps_folder, const std::string& _map_filetype,
                  const int& _upload_th, const bool& _is_mapper = true,
                  const std::string& _family_name = "", const std::string& _given_name = "", const std::string& _affiliation = "");
    virtual ~CStateMachine() {};

    bool        is_active();
    void        set_is_mapper(const bool& _is_mapper);
    LomacorCmd  get_cmd();
    std::string get_map_file();
    std::string get_region_name();

    void step(const std::string& _region, const int& _map_id);

    static bool decode(const std::vector<int>& _maps_metadata, int& _out_cmd, int& _out_map_id, std::string& _out_filepath);
    std::vector<int> encode();

    // Debug functions
    std::string cmd2str(const int& _cmd);
    std::string print_state();
    
private:
    bool is_map_building_finished(const std::string& _map_file);
    void set_build_map();
    void set_use_map();
    void set_undef();

    // Encodes to output of LoMaCoR, which will be sent to the Slam method
    // cmd, map_id, path_to_map_file
    // cmd to slam algorithm:
    //      0 = does nothing (slam in map building mode, without saving)
    //      1 = build map and save it the path given by the LoMaCoR filter
    //      2 = use map provided by the path in the output of the LoMaCoR filter
    std::vector<int> encode(const int& _cmd, const int& _map_id, std::string& _local_path);

private:
    std::filesystem::path   m_MapsFolder;
    std::string             m_MapsFileType;
    std::string             m_ArchFileType = ".zip";
    CZenodo                 m_Zenodo;

    int                     m_CurrentRegion;
    std::string             m_CurrentRegionName;
    int                     m_CurrentMap;
    std::filesystem::path   m_CurrentMapFilePath;

    LomacorCmd              m_Cmd = CMD_BUILD_MAP;
    std::mutex              m_Mutex;

    std::chrono::seconds    m_UpdateTh;
    bool                    m_bIsMapper;
};

#endif /* CStateMachine_H_ */
