// Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CyC_TYPES.h"
#include "os/CyC_FILESYSTEM.h"
#include "CStateMachine.h"
#include "CZip.h"

const std::string ZENODO_URL = "https://sandbox.zenodo.org/api/deposit/depositions";
const std::string ACCESS_TOKEN = "LP7zVxmRWR4WTRVWNn8pMEnyBNxyTpAEui3E2pKPMZ00tpGdtzDrLZB2dgFN";

void showUsage()
{
    printf("\nUsage:\n"
        "tu_Lomacor [options] region_name \n\n"
        "Options:\n"
        "  --l  # List available maps of the region\n"
        "  --u  # Upload map\n"
        "  --d  # Download map\n"
        "  --o  # Output map (same as on Zenodo if empty)\n"
        "\n"
        "eg: tu_Lomacor Brasov\n");
    exit(1);
}

int main(int argc, char** argv)
{
    // Do not use scientific notation
    std::cout << std::fixed;
    std::cout << std::setprecision(3);

    std::string m_sRegion = "Brasov";
    fs::path m_MapsFolder = "../etc/env/maps";
    std::string maps_filetype = ".map";
    int m_nMapID = 2;
    int m_nUploadTh = 30;
    CStateMachine m_StateMachine(ZENODO_URL, ACCESS_TOKEN, m_MapsFolder, maps_filetype, m_nUploadTh, false);

    // Query Zenodo for a map
    m_StateMachine.step(m_sRegion, m_nMapID);
    std::cout << m_StateMachine.print_state() << std::endl << std::endl;

    m_StateMachine.step(m_sRegion, m_nMapID);
    std::cout << m_StateMachine.print_state() << std::endl << std::endl;
    
    std::cout << std::endl;
    std::cout << "EXIT_SUCCESS" << std::endl;

    return EXIT_SUCCESS;
}
