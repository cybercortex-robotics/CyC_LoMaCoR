// Copyright (c) 2025 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CyC_TYPES.h"
#include "CZenodo.h"
#include <cpr/cpr.h>
#include <json.hpp>

const std::string ACCESS_TOKEN = "LP7zVxmRWR4WTRVWNn8pMEnyBNxyTpAEui3E2pKPMZ00tpGdtzDrLZB2dgFN";
const std::string ZENODO_URL = "https://sandbox.zenodo.org/api/deposit/depositions";

void showUsage()
{
    printf("\nUsage:\n"
        "tu_Zenodo [options] city_name \n\n"
        "Options:\n"
        "  --l  # List available maps of the city\n"
        "  --u  # Upload map\n"
        "  --d  # Download map\n"
        "  --o  # Output map (same as on Zenodo if empty)\n"
        "\n"
        "eg: tu_Zenodo Brasov\n");
    exit(1);
}

int main(int argc, char** argv)
{
    // Do not use scientific notation
    std::cout << std::fixed;
    std::cout << std::setprecision(3);

    bool bListMaps = false;
    bool bUploadMap = false;
    bool bDownloadMap = false;
    std::string sCityName = argv[argc - 1];
    std::string sUploadMapPath;
    std::string sDownloadMapName, sDownloadMapOutput;

    if (argc < 2)
    {
        showUsage();
        return EXIT_FAILURE;
    }

    // Parse arguments
    for (int i = 1; i < argc - 1; i++)
    {
        if (strcmp(argv[i], "--l") == 0)
        {
            bListMaps = true;
            continue;
        }
        if (strcmp(argv[i], "--u") == 0)
        {
            bUploadMap = true;
            sUploadMapPath = argv[i + 1];
            ++i;
            continue;
        }
        if (strcmp(argv[i], "--d") == 0)
        {
            bDownloadMap = true;
            sDownloadMapName = argv[i + 1];
            ++i;
            continue;
        }
        if (strcmp(argv[i], "--o") == 0)
        {
            sDownloadMapOutput = argv[i + 1];
            ++i;
            continue;
        }

        printf("Unrecognized option : %s\n", argv[i]);
        showUsage();
    }

    // Zenodo object (retrieves the list of deposits)
    CZenodo m_Zenodo(ZENODO_URL, ACCESS_TOKEN);
    if (m_Zenodo.is_active())
        std::cout << "\nConnected succesfully to Zenodo" << std::endl;

    // Check if the requested map exists
    int nDepositionID = m_Zenodo.find_deposit(sCityName);
    std::cout << "City '" << sCityName << "' found on Zenodo with ID '" << nDepositionID << "'" << std::endl;

    // List maps
    if (bListMaps)
    {
        std::cout << "\nAvailable maps for '" << sCityName << "':" << std::endl;
        std::vector<CZenodo::File> maps = m_Zenodo.get_files(nDepositionID);
        for (const auto& map : maps)
            std::cout << "\t" << map.name << "\t" << map.id << std::endl;
    }

    // Upload file (map)
    if (bUploadMap && m_Zenodo.upload_file(nDepositionID, sUploadMapPath))
        std::cout << "\nMap '" << sUploadMapPath << "' uploaded succesfully." << std::endl;

    // Download file (map)
    if (bDownloadMap && m_Zenodo.download_file(nDepositionID, sDownloadMapName, sDownloadMapOutput))
    {
        std::string o = sDownloadMapOutput;
        if (sDownloadMapOutput.empty())
            o = std::filesystem::current_path().string();
        std::cout << "\nMap file '" << sDownloadMapName << "' succesfully downloaded to '" << o << "'" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "EXIT_SUCCESS" << std::endl;

    return EXIT_SUCCESS;
}
