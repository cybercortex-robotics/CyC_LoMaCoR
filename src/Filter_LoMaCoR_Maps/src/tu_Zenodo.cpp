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
        "  -r  # Region name\n"
        "  -n  # Create a new region\n"
        "  -l  # List available maps of the region\n"
        "  -u  # Upload map\n"
        "  -d  # Download map\n"
        "  -o  # Path for downloading a map (if empty, same as current folder and map name on Zenodo)\n"
        "\n"
        "eg: tu_Zenodo ../etc/credentials.conf\n");
    exit(1);
}

int main(int argc, char** argv)
{
    // Do not use scientific notation
    std::cout << std::fixed;
    std::cout << std::setprecision(3);

    bool bListDeposits = false;
    bool bListMaps = false;
    bool bUploadMap = false;
    bool bDownloadMap = false;
    bool bCreatesRegion = false;
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
        if (strcmp(argv[i], "-r") == 0)
        {
            sRegionName = argv[i + 1];
            ++i;
            continue;
        }
        else if (strcmp(argv[i], "-n") == 0)
        {
            bCreatesRegion = true;
            sRegionName = argv[i + 1];
            ++i;
            continue;
        }
        else if (strcmp(argv[i], "-l") == 0)
        {
            bListMaps = true;
            continue;
        }
        else if (strcmp(argv[i], "-u") == 0)
        {
            bUploadMap = true;
            sUploadMapPath = argv[i + 1];
            ++i;
            continue;
        }
        else if (strcmp(argv[i], "-d") == 0)
        {
            bDownloadMap = true;
            sDownloadMapName = argv[i + 1];
            ++i;
            continue;
        }
        else if (strcmp(argv[i], "-o") == 0)
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
    {
        std::cout << "\nConnected succesfully to Zenodo." << std::endl;
        std::cout << "User:\t\t" << m_Zenodo.m_sGivenName << " " << m_Zenodo.m_sFamilyName << std::endl;
        std::cout << "Affiliation:\t" << m_Zenodo.m_sAffiliation << std::endl << std::endl;
    }
    else
    {
        std::cout << "Could not connect to Zenodo. Exiting." << std::endl;
        return EXIT_FAILURE;
    }

    // List deposits if no other action is requested
    if (!bListMaps && !bUploadMap && !bDownloadMap && !bCreatesRegion)
    {
        std::vector<CZenodo::Deposit> deposits = m_Zenodo.get_deposits();

        if (!deposits.empty())
        {
            std::cout << "All deposits belonging to '" << m_Zenodo.m_sGivenName << " " << m_Zenodo.m_sFamilyName << "':" << std::endl;
            for (const auto& dep : deposits)
            {
                std::cout << "\t" << dep.id << ":\t" << dep.title << " (" << dep.status << ")" << std::endl;
            }
        }
        else
        {
            std::cout << "'" << m_Zenodo.m_sGivenName << " " << m_Zenodo.m_sFamilyName << "' has no deposits." << std::endl;
        }

        std::cout << std::endl;
        std::cout << "EXIT_SUCCESS" << std::endl;
        return EXIT_SUCCESS;
    }

    // Check if the requested map exists
    int nDepositionID = m_Zenodo.find_deposit(sRegionName);
    if (bCreatesRegion)
    {
        if (nDepositionID > 0)
        {
            std::cout << "Region '" << sRegionName << "' already exist in Zenodo with ID '" << nDepositionID << "'. Choose a diferent name." << std::endl;
            return EXIT_FAILURE;;
        }
        else
        {
            int new_deposit_id = m_Zenodo.create_deposit(sRegionName, "Region " + sRegionName);
            if (new_deposit_id > 0)
                std::cout << "Region '" << sRegionName << "' created successfully on Zenodo with ID '" << new_deposit_id << "'." << std::endl;
            else
                std::cout << "Could not create region '" << sRegionName << "' on Zenodo" << std::endl;
            return EXIT_SUCCESS;
        }
    }
    else
    {
        if (nDepositionID > 0)
        {
            std::cout << "\nRegion '" << sRegionName << "' found on Zenodo with latest version ID '" << nDepositionID << "'." << std::endl;
        }
        else
        {
            std::cout << "\nRegion '" << sRegionName << "' could not be found." << std::endl;
            return EXIT_FAILURE;
        }
    }

    // List maps
    if (bListMaps)
    {
        std::cout << "\nPublished maps for '" << sRegionName << "' (id " << nDepositionID << "):" << std::endl;
        std::vector<CZenodo::File> maps_published = m_Zenodo.get_published_files(nDepositionID);
        for (const auto& map : maps_published)
            std::cout << "\t" << map.name << std::endl;

        int draft_id = m_Zenodo.find_active_draft_id(nDepositionID);
        if (draft_id >= 0)
        {
            std::vector<CZenodo::File> maps_draft = m_Zenodo.get_draft_files(draft_id);
            std::cout << "\nDraft maps for '" << sRegionName << "' (id " << draft_id << "):" << std::endl;
            for (const auto& map : maps_draft)
                std::cout << "\t" << map.name << std::endl;
        }
        else
        {
            std::cout << "\n'" << sRegionName << "' has no working draft." << std::endl;
        }
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
