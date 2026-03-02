// Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CZenodo.h"
#include "os/CyC_FILESYSTEM.h"
#pragma warning(disable : 4275)
#include <libconfig.h++>
#pragma warning(default : 4275)

CZenodo::CZenodo(const std::string& _credentials_file)
{
    std::string zenodo_url, access_token;

    if (!fs::exists(_credentials_file.c_str()))
    {
        spdlog::error("CZenodo: Credentials file '{}' not found.", _credentials_file);
        return;
    }

    libconfig::Config configFile;
    try
    {
        configFile.readFile(_credentials_file.c_str());
    }
    catch (libconfig::ParseException& ex)
    {
        spdlog::error("CZenodo: Failed to read configuration with error: {} at line {}", ex.getError(), ex.getLine());
        return;
    }

    if (!configFile.exists("ZENODO_URL"))
    {
        spdlog::error("CZenodo: 'ZENODO_URL' missing in the credentials file");
        return;
    }
    else
    {
        configFile.lookupValue("ZENODO_URL", zenodo_url);
    }

    if (!configFile.exists("ACCESS_TOKEN"))
    {
        spdlog::error("CZenodo: 'ACCESS_TOKEN' missing in the credentials file");
        return;
    }
    else
    {
        configFile.lookupValue("ACCESS_TOKEN", access_token);
    }

    set_auth_headers(zenodo_url, access_token);
}

CZenodo::CZenodo(const std::string& _zenodo_url, const std::string& _access_token)
{
    set_auth_headers(_zenodo_url, _access_token);
}

bool CZenodo::set_auth_headers(const std::string& _zenodo_url, const std::string& _access_token)
{
    m_Deposits.clear();

    cpr::Header headers = cpr::Header{
        {"Content-Type", "application/json"},
        {"Authorization", "Bearer " + _access_token}
    };

    cpr::Response r_deposits = cpr::Get(cpr::Url{ _zenodo_url }, headers);
    if (r_deposits.status_code != cpr::status::HTTP_OK)
    {
        spdlog::error("CZenodo::set_auth_headers(): zenodo url or access token incorrect.");
        m_bIsActive = false;
        return false;
    }

    try
    {
        nlohmann::json deposits_json = nlohmann::json::parse(r_deposits.text);
        if (deposits_json.is_array() && !deposits_json.empty())
        {
            for (const auto& deposition : deposits_json)
            {
                if (deposition.is_object())
                {
                    int id = deposition.value("id", 0);
                    std::string title = deposition.value("title", "No Title Found");
                    m_Deposits.emplace_back(id, title);
                }
            }
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        spdlog::error("CZenodo::set_auth_headers(): JSON Parsing Error (Depositions): {}", e.what());
        return false;
    }

    m_sZenodoUrl = _zenodo_url;
    m_sAccessToken = _access_token;
    m_bIsActive = true;

    return true;
}

cpr::Header CZenodo::get_auth_headers()
{
    return cpr::Header{
        {"Content-Type", "application/json"},
        {"Authorization", "Bearer " + m_sAccessToken}
    };
}

int CZenodo::find_deposit(const std::string& _deposit_name)
{
    if (!m_bIsActive)
    {
        spdlog::error("CZenodo::find_deposit(): No active connection.");
        return false;
    }

    int deposit_id = -1;

    for (const auto& deposit : m_Deposits)
    {
        if (deposit.title.compare(_deposit_name) == 0)
        {
            deposit_id = deposit.id;
            break;
        }
    }

    return deposit_id;
}

bool CZenodo::upload_file(const int& _deposition_id, const std::string& _filepath)
{
    if (!m_bIsActive)
    {
        spdlog::error("CZenodo::upload_file(): No active connection.");
        return false;
    }

    std::string upload_url = m_sZenodoUrl + "/" + std::to_string(_deposition_id) + "/files";
    std::string filename = std::filesystem::path(_filepath).filename().string();

    if (!std::filesystem::exists(_filepath))
    {
        spdlog::error("CZenodo::upload_file(): File '{}' not found.", _filepath);
        return false;
    }

    cpr::Multipart multipart_data = {
        {"name", filename},
        {"file", cpr::File(_filepath)}
    };

    cpr::Response r_add_file = cpr::Post(
        cpr::Url{ upload_url },
        multipart_data,
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} }
    );

    try
    {
        nlohmann::json response_json = nlohmann::json::parse(r_add_file.text);
        //std::cout << response_json.dump(2) << std::endl;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        spdlog::error("CZenodo::upload_file(): JSON Parse Error: '{}'", e.what());
        return false;
    }

    return true;
}

std::vector<CZenodo::File> CZenodo::get_files(const int& _deposition_id)
{
    std::vector<CZenodo::File> files;

    if (!m_bIsActive)
    {
        spdlog::error("CZenodo::get_files(): No active connection.");
        return files;
    }

    cpr::Header headers = get_auth_headers();

    std::string files_url = m_sZenodoUrl + "/" + std::to_string(_deposition_id) + "/files";
    cpr::Response r_files = cpr::Get(cpr::Url{ files_url }, headers);

    nlohmann::json files_json;
    try
    {
        files_json = nlohmann::json::parse(r_files.text);
        //std::cout << files_json.dump(2) << std::endl;

        if (files_json.is_array() && !files_json.empty())
        {
            for (const auto& file : files_json)
            {
                if (file.is_object())
                {
                    std::string id = file["id"].get<std::string>();
                    std::string name = file["filename"].get<std::string>();
                    std::string download_link = file["links"]["download"].get<std::string>();
                    files.emplace_back(id, name, download_link);
                }
            }
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        spdlog::error("CZenodo::upload_file(): JSON Parsing Error (Files): '{}'", e.what());
    }

    return files;
}

bool CZenodo::download_file(const long& _deposition_id, const std::string& _filename_on_zenodo, const std::string& _filename_local)
{
    // Check if Zenodo is active
    if (!m_bIsActive)
    {
        spdlog::error("CZenodo::download_file(): No active connection.");
        return false;
    }

    // Check local filepath
    if (!_filename_local.empty() && !std::filesystem::is_directory(std::filesystem::path(_filename_local).parent_path()))
    {
        spdlog::error("CZenodo::download_file(): Local folder '{}' does not exists.", _filename_local);
        return false;
    }

    std::vector<CZenodo::File> files = get_files(_deposition_id);

    bool file_found = false;
    for (const auto& file : files)
    {
        if (file.name.compare(_filename_on_zenodo) == 0)
        {
            file_found = download_file(file, _filename_local);
            break;
        }
    }

    return file_found;
}

bool CZenodo::download_file(const CZenodo::File& _file, const std::string& _filename_local)
{
    if (!m_bIsActive)
    {
        spdlog::error("CZenodo::download_file(): No active connection.");
        return false;
    }

    std::string filename = _file.name;
    if (!_filename_local.empty())
        filename = _filename_local;

    std::ofstream output_file(filename, std::ios::binary);
    if (!output_file.is_open())
    {
        spdlog::error("CZenodo::download_file(): Could not open file '{}", filename);
        return false;
    }

    auto write_callback = [&](std::string_view data, std::intptr_t user_data) -> bool {
        std::ofstream* file_stream = reinterpret_cast<std::ofstream*>(user_data);
        file_stream->write(data.data(), data.size());
        return true; // Return true to continue the transfer
        };

    // Perform the GET request with the WriteCallback
    cpr::Header headers = get_auth_headers();
    cpr::Response r_file = cpr::Get(
        cpr::Url{ _file.download_link },
        headers,
        cpr::WriteCallback{ write_callback, reinterpret_cast<std::intptr_t>(&output_file) }
    );

    output_file.close();
    
    // Check the status code
    if (r_file.status_code != cpr::status::HTTP_OK)
    {
        // If download failed, clean up the partially written file
        std::remove(filename.c_str());
        spdlog::error("CZenodo::download_file(): Failed to download file. Status code: {}", r_file.status_code);
        return false;
    }

    return true;
}
