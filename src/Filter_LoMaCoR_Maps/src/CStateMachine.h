// Copyright (c) 2025 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#ifndef CStateMachine_H_
#define CStateMachine_H_

#include "CyC_TYPES.h"
#include "os/CyC_FILESYSTEM.h"
#include "CZenodo.h"

class CStateMachine
{
public:
    enum state
    {
        UNDEF = -1,
    };

public:
    CStateMachine(const std::string& _maps_folder, const std::string& _zenodo_url, const std::string& _access_token);
    virtual ~CStateMachine() {};

    bool query_map(const std::string& _region, const int& _map_id);

private:
    fs::path m_MapsFolder;
    CZenodo m_Zenodo;
};

#endif /* CStateMachine_H_ */
