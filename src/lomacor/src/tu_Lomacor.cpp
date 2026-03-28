// Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CStateMachine.h"
#include "CZip.h"
#include <iostream>
#include <filesystem>
namespace fs {
    using namespace std::filesystem;
    using ifstream = std::ifstream;
    using ofstream = std::ofstream;
    using fstream = std::fstream;
}

const std::string NAME = "CyberCortex Robotics";
const std::string ZENODO_URL = "https://sandbox.zenodo.org/api/deposit/depositions";
const std::string ACCESS_TOKEN = "LP7zVxmRWR4WTRVWNn8pMEnyBNxyTpAEui3E2pKPMZ00tpGdtzDrLZB2dgFN";

void showUsage()
{
    printf("\nUsage:\n"
        "tu_Lomacor [options] credentials.conf \n\n"
        "Options:\n"
        "  --r  # Region name\n"
        "  --l  # List available maps of the region\n"
        "  --u  # Upload map\n"
        "  --d  # Download map\n"
        "  --o  # Path for downloading a map (if empty, same as current folder and map name on Zenodo)\n"
        "\n"
        "eg: tu_Lomacor ../etc/credentials.conf\n");
    exit(1);
}

int main(int argc, char** argv)
{
    // Do not use scientific notation
    std::cout << std::fixed;
    std::cout << std::setprecision(3);

    std::string m_sRegion = "Brasov";
    fs::path m_MapsFolder = "../etc/env/maps";
    std::string maps_file_ext = ".map";
    int m_nMapID = 2;
    int m_nUploadTh = 30;
    CStateMachine m_StateMachine(ZENODO_URL, ACCESS_TOKEN, m_MapsFolder.string(), maps_file_ext, m_nUploadTh, false, NAME);

    // Query Zenodo for a map
    m_StateMachine.step(m_sRegion, m_nMapID);
    std::cout << m_StateMachine.print_state() << std::endl << std::endl;

    m_StateMachine.step(m_sRegion, m_nMapID);
    std::cout << m_StateMachine.print_state() << std::endl << std::endl;
    
    std::cout << std::endl;
    std::cout << "EXIT_SUCCESS" << std::endl;

    return EXIT_SUCCESS;
}
