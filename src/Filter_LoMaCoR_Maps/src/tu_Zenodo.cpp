// Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CyC_TYPES.h"
#include "CZenodo.h"
#include <cpr/cpr.h>
#include <json.hpp>
#include "os/CyC_FILESYSTEM.h"

void showUsage()
{
    printf("\nUsage:\n"
        "tu_Zenodo [options] credentials.conf \n\n"
        "Options:\n"
        "  --r  # Region name\n"
        "  --l  # List available maps of the region\n"
        "  --u  # Upload map\n"
        "  --d  # Download map\n"
        "  --o  # Path for downloading a map (if empty, same as current folder and map name on Zenodo)\n"
        "\n"
        "eg: tu_Zenodo ../etc/credentials.conf\n");
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
    std::string sCredentials = argv[argc - 1];
    std::string sRegionName;
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
        if (strcmp(argv[i], "--r") == 0)
        {
            sRegionName = argv[i + 1];
            ++i;
            continue;
        }
        else if (strcmp(argv[i], "--l") == 0)
        {
            bListMaps = true;
            continue;
        }
        else if (strcmp(argv[i], "--u") == 0)
        {
            bUploadMap = true;
            sUploadMapPath = argv[i + 1];
            ++i;
            continue;
        }
        else if (strcmp(argv[i], "--d") == 0)
        {
            bDownloadMap = true;
            sDownloadMapName = argv[i + 1];
            ++i;
            continue;
        }
        else if (strcmp(argv[i], "--o") == 0)
        {
            sDownloadMapOutput = argv[i + 1];
            ++i;
            continue;
        }

        printf("Unrecognized option : %s\n", argv[i]);
        showUsage();
    }

    // Zenodo object (retrieves the list of deposits)
    CZenodo m_Zenodo(sCredentials);
    if (m_Zenodo.is_active())
        std::cout << "\nConnected succesfully to Zenodo" << std::endl;

    // Check if the requested map exists
    int nDepositionID = m_Zenodo.find_deposit(sRegionName);
    if (nDepositionID > 0)
        std::cout << "Region '" << sRegionName << "' found on Zenodo with ID '" << nDepositionID << "'." << std::endl;
    else
        std::cout << "Region '" << sRegionName << "' could not be found." << std::endl;

    // List maps
    if (bListMaps)
    {
        std::cout << "\nAvailable maps for '" << sRegionName << "':" << std::endl;
        std::vector<CZenodo::File> maps = m_Zenodo.get_files(nDepositionID);
        for (const auto& map : maps)
            std::cout << "\t" << map.name << "\t" << map.id << std::endl;
    }

    // Upload file (map)
    if (bUploadMap && m_Zenodo.upload_file(nDepositionID, sUploadMapPath))
        std::cout << "\nMap '" << sUploadMapPath << "' uploaded succesfully." << std::endl;

    // Download file (map)
    if (bDownloadMap)
    {
        if (std::filesystem::is_directory(sDownloadMapOutput))
            sDownloadMapOutput = fs::path(sDownloadMapOutput) / fs::path(sDownloadMapName);

        if (m_Zenodo.download_file(nDepositionID, sDownloadMapName, sDownloadMapOutput))
        {
            std::string o = sDownloadMapOutput;
            if (sDownloadMapOutput.empty())
                o = std::filesystem::current_path().string();
            std::cout << "\nMap file '" << sDownloadMapName << "' succesfully downloaded to '" << o << "'" << std::endl;
        }
        else
        {
            std::cout << "\nCould not download map file '" << sDownloadMapName << "'" << std::endl;
        }
    }

    std::cout << std::endl;
    std::cout << "EXIT_SUCCESS" << std::endl;

    return EXIT_SUCCESS;
}
