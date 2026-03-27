// Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CZenodo.h"
#include "os/CyC_FILESYSTEM.h"
#pragma warning(disable : 4275)
#include <libconfig.h++>
#pragma warning(default : 4275)

CZenodo::CZenodo(const std::string& _credentials_file)
{
    std::string family_name, given_name, affiliation, zenodo_url, access_token;

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

    // Creator family name
    if (!configFile.exists("FAMILY_NAME"))
    {
        spdlog::error("CZenodo: 'FAMILY_NAME' missing in the credentials file");
        return;
    }
    else
    {
        configFile.lookupValue("FAMILY_NAME", family_name);
    }

    // Creator given name
    if (!configFile.exists("GIVEN_NAME"))
    {
        spdlog::error("CZenodo: 'GIVEN_NAME' missing in the credentials file");
        return;
    }
    else
    {
        configFile.lookupValue("GIVEN_NAME", given_name);
    }

    // Creator affiliation
    if (!configFile.exists("AFFILIATION"))
    {
        spdlog::error("CZenodo: 'AFFILIATION' missing in the credentials file");
        return;
    }
    else
    {
        configFile.lookupValue("AFFILIATION", affiliation);
    }

    // URL
    if (!configFile.exists("ZENODO_URL"))
    {
        spdlog::error("CZenodo: 'ZENODO_URL' missing in the credentials file");
        return;
    }
    else
    {
        configFile.lookupValue("ZENODO_URL", zenodo_url);
    }

    // Acces token
    if (!configFile.exists("ACCESS_TOKEN"))
    {
        spdlog::error("CZenodo: 'ACCESS_TOKEN' missing in the credentials file");
        return;
    }
    else
    {
        configFile.lookupValue("ACCESS_TOKEN", access_token);
    }

    set_auth_headers(family_name, given_name, affiliation, zenodo_url, access_token);
}

CZenodo::CZenodo(const std::string& _family_name, const std::string& _given_name, const std::string& _affiliation, const std::string& _zenodo_url, const std::string& _access_token)
{
    set_auth_headers(_family_name, _given_name, _affiliation, _zenodo_url, _access_token);
}

bool CZenodo::set_auth_headers(const std::string& _family_name, const std::string& _given_name, const std::string& _affiliation, const std::string& _zenodo_url, const std::string& _access_token)
{
    // 1. Store basic connection info
    m_sZenodoUrl = _zenodo_url;
    m_sAccessToken = _access_token;

    // Ensure we have a clean base API URL
    std::string api_url = m_sZenodoUrl;
    if (api_url.find("/api") == std::string::npos)
    {
        while (!api_url.empty() && api_url.back() == '/') api_url.pop_back();
        api_url += "/api";
    }

    // 2. Prepare Headers
    cpr::Header headers = cpr::Header{
        {"Content-Type", "application/json"},
        {"Authorization", "Bearer " + m_sAccessToken}
    };

    // 3. Request User Profile (/api/me)
    // This validates the token AND gives us the name/affiliation
    cpr::Response r = cpr::Get(cpr::Url{ api_url + "/me" }, headers);

    if (r.status_code != cpr::status::HTTP_OK)
    {
        spdlog::error("CZenodo: Authentication failed for {} {}. Status: {}", _given_name, _family_name, r.status_code);
        m_bIsActive = false;
        return false;
    }

    // 4. Parse Profile Data
    try {
        auto j = nlohmann::json::parse(r.text);

        if (j.contains("profile") && j["profile"].is_object())
        {
            // Priority 1: Use the official Zenodo Profile Name
            // Priority 2: Use the _name parameter passed to the function
            m_sFamilyName = j["profile"].value("family_name", _family_name);
            m_sGivenName = j["profile"].value("given_name", _family_name);
            m_sAffiliation = j["profile"].value("affiliation", _affiliation);

            spdlog::info("CZenodo: Successfully authenticated as {} {} ({})", m_sGivenName, m_sFamilyName, m_sAffiliation);
        }
        else
        {
            // Fallback if the profile object is empty
            m_sFamilyName = _family_name;
            m_sGivenName = _given_name;
            m_sAffiliation = _affiliation;
            spdlog::warn("CZenodo: Token valid, but profile details are not accessible from the Zenodo account. Using data from the credentials file.");
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("CZenodo: Error parsing user profile: {}", e.what());
        m_sFamilyName = _family_name;
        m_sGivenName = _given_name;
        m_sAffiliation = _affiliation;
    }

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
        return -1;
    }

    // 1. Prepare the Search Query
    // We wrap the title in quotes to ensure an exact match
    cpr::Parameters params = {
        {"q", "title:\"" + _deposit_name + "\""},
        {"size", "1"} // We only need the best match
    };

    // 2. Execute the Search
    cpr::Response r = cpr::Get(
        cpr::Url{ m_sZenodoUrl + "/api/records" },
        get_auth_headers(),
        params
    );

    if (r.status_code != cpr::status::HTTP_OK)
    {
        spdlog::error("CZenodo::find_deposit(): Search failed. Status: {}", r.status_code);
        return -1;
    }

    try
    {
        nlohmann::json data = nlohmann::json::parse(r.text);
        nlohmann::json items;

        //std::cout << data.dump(2) << std::endl;

        // 3. Handle different API structures
        if (data.is_object() && data.contains("hits"))
        {
            // Records API: { "hits": { "hits": [...] } }
            items = data["hits"]["hits"];
        }
        else if (data.is_array())
        {
            // Depositions API: [ {...}, {...} ]
            items = data;
        }

        // 4. Extract the ID
        if (!items.empty())
        {
            const auto& best_match = items[0];

            // Double-check the title to be 100% sure
            std::string found_title = "";
            if (best_match.contains("metadata"))
                found_title = best_match["metadata"].value("title", "");
            else
                found_title = best_match.value("title", "");

            if (found_title == _deposit_name)
            {
                int deposit_id = best_match.value("id", -1);
                //spdlog::info("CZenodo::find_deposit(): Found '{}' with ID: {}", _deposit_name, deposit_id);
                return deposit_id;
            }
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        spdlog::error("CZenodo::find_deposit(): JSON Parsing Error: {}", e.what());
    }

    //spdlog::warn("CZenodo::find_deposit(): No deposit found with title '{}'", _deposit_name);
    return -1;
}

bool CZenodo::upload_file(const int& _original_id, const std::string& _filepath, const bool& _publish)
{
    if (!m_bIsActive) return false;

    // 1. SETUP API & GET ORIGINAL TITLE
    std::string base_api = m_sZenodoUrl;
    if (base_api.find("/api") == std::string::npos) base_api += "/api";

    cpr::Response r_orig = cpr::Get(cpr::Url{ base_api + "/records/" + std::to_string(_original_id) },
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });

    std::string original_title = "GRVC Map";
    try {
        original_title = nlohmann::json::parse(r_orig.text)["metadata"].value("title", "GRVC Map");
    }
    catch (...) {}

    // 2. CREATE/FIND DRAFT
    std::string version_url = base_api + "/records/" + std::to_string(_original_id) + "/versions";
    cpr::Response r_ver = cpr::Post(cpr::Url{ version_url }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });

    std::string s_new_id;
    if (r_ver.status_code == 201) {
        s_new_id = std::to_string(nlohmann::json::parse(r_ver.text).value("id", -1));
    }
    else {
        cpr::Response r_list = cpr::Get(cpr::Url{ version_url }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });
        auto list_j = nlohmann::json::parse(r_list.text);
        if (list_j.contains("hits") && list_j["hits"].contains("hits")) {
            for (auto& hit : list_j["hits"]["hits"]) {
                if (hit.value("is_published", true) == false) {
                    s_new_id = std::to_string(hit["id"].get<int>());
                    break;
                }
            }
        }
    }

    if (s_new_id.empty()) return false;
    std::string draft_url = base_api + "/records/" + s_new_id + "/draft";

    // 3. ENABLE FILES & IMPORT
    nlohmann::json enable_j = { {"files", {{"enabled", true}}} };
    cpr::Put(cpr::Url{ draft_url }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken}, {"Content-Type", "application/json"} }, cpr::Body{ enable_j.dump() });
    cpr::Post(cpr::Url{ draft_url + "/actions/files-import" }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });

    // 4. FETCH DRAFT & PREPARE METADATA
    cpr::Response r_get = cpr::Get(cpr::Url{ draft_url }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });
    auto full_payload = nlohmann::json::parse(r_get.text);

    nlohmann::json update_payload;
    update_payload["metadata"] = full_payload["metadata"];

    // --- FIX 1: Define the stringstream for the date ---
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d");

    update_payload["metadata"]["title"] = original_title;
    update_payload["metadata"]["publication_date"] = ss.str();
    update_payload["metadata"]["resource_type"] = { {"id", "dataset"} };
    update_payload["metadata"]["publisher"] = m_sAffiliation;

    update_payload["metadata"]["creators"] = nlohmann::json::array({
        {
            {"person_or_org", {
                {"name", m_sFamilyName + ", " + m_sGivenName},
                {"type", "personal"},
                {"given_name", m_sGivenName},
                {"family_name", m_sFamilyName}
            }}
        }
        });

    if (full_payload.contains("access")) update_payload["access"] = full_payload["access"];

    // 5. PUT METADATA BACK
    // --- FIX 2: Use update_payload.dump() instead of payload.dump() ---
    cpr::Response r_put = cpr::Put(
        cpr::Url{ draft_url },
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken}, {"Content-Type", "application/json"} },
        cpr::Body{ update_payload.dump() }
    );

    if (r_put.status_code == 200 || r_put.status_code == 201) {
        spdlog::info("CZenodo: Metadata updated successfully. Title set to: {}", original_title);
    }
    else {
        spdlog::error("CZenodo: Metadata update FAILED (Status {}).", r_put.status_code);
        spdlog::error("CZenodo: Error Details: {}", r_put.text);
    }

    // 6. UPLOAD FILE
    std::string filename = std::filesystem::path(_filepath).filename().string();
    std::string files_url = draft_url + "/files";
    cpr::Delete(cpr::Url{ files_url + "/" + filename }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });

    nlohmann::json init_f = nlohmann::json::array({ {{"key", filename}} });
    cpr::Post(cpr::Url{ files_url }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken}, {"Content-Type", "application/json"} }, cpr::Body{ init_f.dump() });

    cpr::Put(cpr::Url{ files_url + "/" + filename + "/content" },
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken}, {"Content-Type", "application/octet-stream"} },
        cpr::Body{ cpr::File{_filepath} });

    cpr::Post(cpr::Url{ files_url + "/" + filename + "/commit" }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });

    // 7. PUBLISH
    if (_publish) {
        cpr::Response r_pub = cpr::Post(cpr::Url{ draft_url + "/actions/publish" }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });
        return (r_pub.status_code == 202 || r_pub.status_code == 200);
    }
    return true;
}

std::vector<CZenodo::File> CZenodo::get_published_files(const int& _deposition_id)
{
    std::vector<CZenodo::File> files;

    if (!m_bIsActive)
    {
        spdlog::error("CZenodo::get_published_files(): No active connection.");
        return files;
    }

    cpr::Header headers = get_auth_headers();

    std::string files_url = m_sZenodoUrl + "/api/records/" + std::to_string(_deposition_id) + "/files";
    cpr::Response r_files = cpr::Get(cpr::Url{ files_url }, headers);

    try
    {
        nlohmann::json files_json = nlohmann::json::parse(r_files.text);
        //std::cout << files_json.dump(2) << std::endl;

        // Determine where the file list is located
        nlohmann::json entries;
        if (files_json.is_array())
        {
            // Legacy/Deposition API style: [ {...}, {...} ]
            entries = files_json;
        }
        else if (files_json.is_object() && files_json.contains("entries"))
        {
            // Modern/Records API style: { "entries": [ {...} ] }
            entries = files_json["entries"];
        }

        if (!entries.empty())
        {
            for (const auto& file : entries)
            {
                if (file.is_object())
                {
                    // 1. Get the ID (Modern uses 'file_id' or 'id')
                    std::string id = file.value("file_id", file.value("id", "unknown_id"));

                    // 2. Get the Name (Modern uses 'key', Legacy uses 'filename')
                    std::string name = file.value("key", file.value("filename", "unknown_file"));

                    // 3. Get the Download Link (Modern uses 'content', Legacy uses 'download')
                    std::string download_link = "";
                    if (file.contains("links"))
                    {
                        auto links = file["links"];
                        if (links.contains("content"))
                            download_link = links["content"].get<std::string>();
                        else if (links.contains("download"))
                            download_link = links["download"].get<std::string>();
                    }

                    if (!download_link.empty()) {
                        files.emplace_back(id, name, download_link);
                    }
                }
            }
        }
    }
    catch (const nlohmann::json::exception& e)
    {
        spdlog::error("CZenodo::get_files(): JSON Parsing Error (Files): '{}'", e.what());
    }

    return files;
}


void CZenodo::verify_specific_draft(const int& _id)
{
    if (!m_bIsActive) return;

    // Use the direct draft endpoint
    std::string url = m_sZenodoUrl + "/api/records/" + std::to_string(_id) + "/draft";

    spdlog::info("CZenodo: Direct check for ID {} at {}...", _id, url);

    cpr::Response r = cpr::Get(
        cpr::Url{ url },
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} }
    );

    if (r.status_code == 200) {
        auto j = nlohmann::json::parse(r.text);
        std::string title = j["metadata"].value("title", "No Title");
        bool is_published = j.value("is_published", false);

        spdlog::info("--- Success! Draft Found ---");
        spdlog::info("ID: {} | Title: {}", _id, title);
        spdlog::info("Published: {}", is_published ? "YES" : "NO");

        // Check if files are enabled
        bool files_enabled = false;

        if (j.contains("files") && j["files"].is_object())
        {
            // Only call .value() if we are sure it's an object
            files_enabled = j["files"].value("enabled", false);
        }
        else if (j.contains("files") && j["files"].is_boolean())
        {
            // Some API versions/states might return 'files' as a direct boolean
            files_enabled = j["files"].get<bool>();
        }

        if (files_enabled) {
            spdlog::info("Files: ENABLED");
        }
        else {
            spdlog::warn("Files: DISABLED (Metadata-only mode)");
        }
    }
    else {
        spdlog::error("CZenodo: Direct check failed! Status: {}", r.status_code);
        spdlog::error("Response: {}", r.text);

        if (r.status_code == 404) {
            spdlog::error("HINT: The API cannot see this ID. Check if your Token matches your Browser account.");
        }
    }
}


int CZenodo::find_active_draft_id(const int& _published_id)
{
    if (!m_bIsActive) return -1;

    // 1. Get the Parent/Concept ID for the record we are looking for
    std::string rec_url = m_sZenodoUrl + "/api/records/" + std::to_string(_published_id);
    cpr::Response r_rec = cpr::Get(cpr::Url{ rec_url }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });

    std::string target_concept;
    try {
        auto j_rec = nlohmann::json::parse(r_rec.text);
        // Look for legacy 'conceptrecid' or modern 'parent.id'
        if (j_rec.contains("conceptrecid"))
            target_concept = j_rec["conceptrecid"].get<std::string>();
        else if (j_rec.contains("parent") && j_rec["parent"].contains("id"))
            target_concept = j_rec["parent"]["id"].get<std::string>();
    }
    catch (...) { return -1; }

    // 2. Scan ALL user records for a draft matching that family
    std::string user_url = m_sZenodoUrl + "/api/user/records";
    cpr::Response r_user = cpr::Get(cpr::Url{ user_url }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });

    auto j_user = nlohmann::json::parse(r_user.text);
    if (j_user.contains("hits") && j_user["hits"].contains("hits"))
    {
        for (const auto& hit : j_user["hits"]["hits"])
        {
            std::string status = hit.value("status", "");

            // Check if it's a draft
            if (status == "draft")
            {
                // Check if it belongs to our map family
                std::string hit_concept;
                if (hit.contains("conceptrecid")) hit_concept = hit["conceptrecid"].get<std::string>();

                if (hit_concept == target_concept)
                {
                    int found_id = hit["id"].get<int>();
                    //spdlog::info("CZenodo: Located active draft {} for family {}.", found_id, target_concept);
                    return found_id;
                }
            }
        }
    }

    return -1; // No active draft found for this map
}


int CZenodo::find_draft_by_title(const std::string& _title)
{
    if (!m_bIsActive) return -1;

    // 1. Setup User Records API 
    // This endpoint lists ONLY your drafts/records
    std::string url = m_sZenodoUrl + "/api/user/records?q=status:draft";

    spdlog::info("CZenodo: Searching all user drafts for title: '{}'...", _title);

    cpr::Response r = cpr::Get(
        cpr::Url{ url },
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} }
    );

    if (r.status_code != 200) {
        spdlog::error("CZenodo: Failed to fetch user drafts. Status: {}", r.status_code);
        return -1;
    }

    // 2. Parse hits and match the title
    try {
        auto json_data = nlohmann::json::parse(r.text);
        if (json_data.contains("hits") && json_data["hits"].contains("hits")) {
            for (const auto& hit : json_data["hits"]["hits"]) {
                std::string draft_title = hit["metadata"].value("title", "");

                // Case-insensitive or partial match
                if (draft_title.find(_title) != std::string::npos) {
                    int draft_id = hit["id"].get<int>();
                    spdlog::info("CZenodo: Found matching draft! ID: {} | Title: {}", draft_id, draft_title);
                    return draft_id;
                }
            }
        }
    }
    catch (const std::exception& e) {
        spdlog::error("CZenodo: Parsing error: {}", e.what());
    }

    spdlog::warn("CZenodo: No draft with title '{}' found in your account.", _title);
    return -1;
}

std::vector<CZenodo::File> CZenodo::get_draft_files(const int& _id_draft)
{
    std::vector<CZenodo::File> file_list;
    if (!m_bIsActive) return file_list;

    // 1. SETUP BASE API
    std::string base_api = m_sZenodoUrl;
    if (base_api.find("/api") == std::string::npos) base_api += "/api";

    // 2. TARGET THE DRAFT FILES ENDPOINT
    // URL: /api/records/{id}/draft/files
    std::string files_url = base_api + "/records/" + std::to_string(_id_draft) + "/draft/files";

    //spdlog::info("CZenodo: Listing files specifically for DRAFT ID {}...", _id_draft);

    // 3. PERFORM GET REQUEST
    cpr::Response r = cpr::Get(
        cpr::Url{ files_url },
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} }
    );

    // 4. HANDLE RESPONSE
    if (r.status_code == 404)
    {
        spdlog::error("CZenodo: Draft files not found. ID {} is likely PUBLISHED (use your other function) or doesn't exist.", _id_draft);
        return file_list;
    }

    if (r.status_code != 200)
    {
        spdlog::error("CZenodo: Failed to list draft files ({}): {}", r.status_code, r.text);
        return file_list;
    }

    // 5. PARSE ENTRIES
    try
    {
        auto json_data = nlohmann::json::parse(r.text);

        if (json_data.contains("entries") && json_data["entries"].is_array())
        {
            for (const auto& entry : json_data["entries"]) {
                std::string f_id = entry.value("id", "");
                std::string f_name = entry.value("key", "");
                std::string f_link = "";

                // Get the content/download link for the draft file
                if (entry.contains("links") && entry["links"].contains("content"))
                    f_link = entry["links"]["content"].get<std::string>();

                file_list.emplace_back(f_id, f_name, f_link);
                //spdlog::info(" -> Draft File Found: {}", f_name);
            }
        }
    }
    catch (const std::exception& e)
    {
        spdlog::error("CZenodo: JSON parsing error in get_draft_files: {}", e.what());
    }

    return file_list;
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

    std::vector<CZenodo::File> files = get_published_files(_deposition_id);

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

int CZenodo::create_deposit(const std::string& _title, const std::string& _description, const std::string& _upload_type)
{
    if (!m_bIsActive)
    {
        spdlog::error("CZenodo: No active connection.");
        return -1;
    }

    // Check title
    if (_title.size() < 3)
    {
        spdlog::error("The name of a deposit has to be minimum 3 characters long.");
        return -1;
    }

    // 1. SETUP URL (Modern Endpoint)
    std::string base_api = m_sZenodoUrl;
    if (base_api.find("/api") == std::string::npos) base_api += "/api";
    std::string records_url = base_api + "/records";

    // 2. DEFINE MODERN METADATA (DOI Compliant)
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d");

    nlohmann::json payload = {
        {"metadata", {
            {"title", _title},
            {"description", _description},
            {"publication_date", ss.str()},
            {"publisher", "CyberCortex Robotics"},
            {"resource_type", {{"id", "dataset"}}}, // Modern ID-based type
            {"creators", nlohmann::json::array({{
                {"person_or_org", {
                    {"family_name", m_sFamilyName},   // <--- Required field
                    {"given_name", m_sGivenName},     // <--- Recommended field
                    {"name", m_sFamilyName + ", " + m_sGivenName}, // Assuming format "Family, Given"
                    {"type", "personal"}
                }},
                {"affiliations", nlohmann::json::array({{ {"name", "CyberCortex Robotics"} }})}
            }})}
        }},
        {"access", {
            {"record", "public"},
            {"files", "public"}
        }},
        {"files", {{"enabled", true}}}
    };

    // 3. CREATE THE DRAFT
    cpr::Response r_create = cpr::Post(
        cpr::Url{ records_url },
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken}, {"Content-Type", "application/json"} },
        cpr::Body{ payload.dump() }
    );

    if (r_create.status_code != 201) {
        spdlog::error("CZenodo: Record creation failed: {}", r_create.text);
        return -1;
    }

    auto resp_json = nlohmann::json::parse(r_create.text);
    std::string s_id = std::to_string(resp_json.value("id", -1));
    int new_id = resp_json.value("id", -1);
    std::string draft_url = records_url + "/" + s_id + "/draft";

    // 4. UPLOAD DUMMY FILE (cyckeep)
    std::string dummy_name = "cyckeep";
    std::string files_url = draft_url + "/files";

    // 4a. Initialize
    nlohmann::json init_file = nlohmann::json::array({ { {"key", dummy_name} } });
    cpr::Post(cpr::Url{ files_url },
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken}, {"Content-Type", "application/json"} },
        cpr::Body{ init_file.dump() });

    // 4b. Upload (Create dummy on disk first)
    {
        std::ofstream f(dummy_name);
        f << "Initial placeholder for " << _title;
    }

    cpr::Put(cpr::Url{ files_url + "/" + dummy_name + "/content" },
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken}, {"Content-Type", "application/octet-stream"} },
        cpr::Body{ cpr::File{dummy_name} });

    // 4c. Commit
    cpr::Post(cpr::Url{ files_url + "/" + dummy_name + "/commit" },
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });

    // 5. PUBLISH
    cpr::Response r_pub = cpr::Post(
        cpr::Url{ draft_url + "/actions/publish" },
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} }
    );

    if (r_pub.status_code == 202 || r_pub.status_code == 200)
    {
        spdlog::info("CZenodo: Modern Record {} published successfully.", new_id);
        //m_Deposits.emplace_back(new_id, _title);
        std::filesystem::remove(dummy_name);
        return new_id;
    }

    spdlog::error("CZenodo: Modern Publish failed: {}", r_pub.text);
    return new_id; // Returns ID anyway so you can fix the draft manually if needed
}

//int CZenodo::create_deposit(const std::string& _title, const std::string& _description, const std::string& _upload_type)
//{
//    if (!m_bIsActive)
//    {
//        spdlog::error("CZenodo::create_deposit(): No active connection.");
//        return -1;
//    }
//
//    // 1. Define the metadata
//    nlohmann::json upload_metadata = {
//        {"metadata", {
//            {"title", _title},
//            {"upload_type", _upload_type},
//            {"access_right", "open"},
//            {"description", _description},
//            {"creators", nlohmann::json::array({
//                {{"name", m_sName}}
//            })}
//        }}
//    };
//
//    std::string url = m_sZenodoUrl + "/api/deposit/depositions";
//
//    // 2. Perform the POST request to create the draft
//    cpr::Response r = cpr::Post(
//        cpr::Url{ url },
//        get_auth_headers(),
//        cpr::Body{ upload_metadata.dump() },
//        cpr::Header{ {"Content-Type", "application/json"} } // Ensure JSON content type
//    );
//
//    if (r.status_code != cpr::status::HTTP_CREATED && r.status_code != cpr::status::HTTP_OK)
//    {
//        spdlog::error("CZenodo::create_deposit(): Creation failed. Status: {}, Response: {}", r.status_code, r.text);
//        return -1;
//    }
//
//    int new_id = -1;
//    try
//    {
//        nlohmann::json response_json = nlohmann::json::parse(r.text);
//        new_id = response_json.value("id", -1);
//    }
//    catch (const nlohmann::json::exception& e)
//    {
//        spdlog::error("CZenodo::create_deposit(): JSON Parsing Error: {}", e.what());
//        return -1;
//    }
//
//    // 3. Optional: Add files here if needed before publishing
//    // Note: Zenodo requires at least 1 file to be present to publish.
//    std::string dummy_path = "cyckeep";
//    if (new_id != -1)
//    {
//        std::ofstream dummy_file(dummy_path);
//        if (dummy_file.is_open()) {
//            dummy_file.close();
//        }
//
//        // UPLOAD THE DUMMY FILE
//        // ?????
//    }
//
//    // 4. Perform the Publish Action
//    if (new_id != -1)
//    {
//        std::string publish_url = url + "/" + std::to_string(new_id) + "/actions/publish";
//
//        cpr::Response r_pub = cpr::Post(
//            cpr::Url{ publish_url },
//            get_auth_headers()
//        );
//
//        if (r_pub.status_code == cpr::status::HTTP_ACCEPTED || r_pub.status_code == cpr::status::HTTP_OK)
//        {
//            spdlog::info("CZenodo::create_deposit(): Successfully created and PUBLISHED '{}' with ID: {}", _title, new_id);
//
//            // Update local cache
//            m_Deposits.emplace_back(new_id, _title);
//
//            //if (std::filesystem::exists(dummy_path))
//            //    std::filesystem::remove(dummy_path);
//
//            return new_id;
//        }
//        else
//        {
//            // If this fails, the record still exists as a DRAFT.
//            spdlog::warn("CZenodo::create_deposit(): Created draft {}, but PUBLISH failed (Status: {}). Check if files are attached.", new_id, r_pub.status_code);
//            spdlog::debug("Zenodo Response: {}", r_pub.text);
//
//            //if (std::filesystem::exists(dummy_path))
//            //    std::filesystem::remove(dummy_path);
//
//            return new_id; // Still return the ID so the user can manage the draft
//        }
//    }
//
//    return -1;
//}



void CZenodo::debug_version_history(const int& _published_id)
{
    std::string url = m_sZenodoUrl + "/api/records/" + std::to_string(_published_id) + "/versions";
    cpr::Response r = cpr::Get(cpr::Url{ url }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });

    auto j = nlohmann::json::parse(r.text);
    spdlog::info("--- Version History for {} ---", _published_id);

    if (j.contains("hits") && j["hits"].contains("hits")) {
        for (const auto& hit : j["hits"]["hits"]) {
            int id = hit["id"].get<int>();
            bool pub = hit.value("is_published", false);
            std::string status = hit.value("status", "unknown");
            spdlog::info(" > ID: {} | Published: {} | Status: {}", id, pub ? "YES" : "NO", status);
        }
    }
    else {
        spdlog::warn("No version hits found for this ID.");
    }
}

void CZenodo::verify_draft_parentage(const int& _draft_id)
{
    std::string url = m_sZenodoUrl + "/api/records/" + std::to_string(_draft_id) + "/draft";
    cpr::Response r = cpr::Get(cpr::Url{ url }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });

    if (r.status_code != 200) {
        spdlog::error("CZenodo: Draft {} unreachable. Status: {}. Body: {}", _draft_id, r.status_code, r.text);
        return;
    }

    try {
        auto j = nlohmann::json::parse(r.text);
        spdlog::info("--- Draft {} Relationship Analysis ---", _draft_id);

        // SAFE CHECK: Ensure 'j' is an object and 'parent' is an object
        if (j.is_object() && j.contains("parent") && j["parent"].is_object()) {
            if (j["parent"].contains("id") && j["parent"]["id"].is_string()) {
                std::string parent_id = j["parent"]["id"].get<std::string>();
                spdlog::info("Draft {} reports Parent ID: {}", _draft_id, parent_id);
            }
            else {
                spdlog::warn("Draft {} has a parent object, but no string ID found.", _draft_id);
            }
        }
        else {
            spdlog::warn("Draft {} does not have a valid 'parent' object in metadata.", _draft_id);
            // Let's see what is actually there
            spdlog::debug("Raw Metadata: {}", r.text.substr(0, 500));
        }
    }
    catch (const nlohmann::json::exception& e) {
        spdlog::error("CZenodo: JSON Crash Prevented! Error: {}", e.what());
    }
}

void CZenodo::debug_family_identifiers(const int& _id, bool _is_draft)
{
    std::string url = m_sZenodoUrl + "/api/records/" + std::to_string(_id);
    if (_is_draft) url += "/draft";

    cpr::Response r = cpr::Get(cpr::Url{ url }, cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} });
    auto j = nlohmann::json::parse(r.text);

    spdlog::info("--- Family ID Search for {} ---", _id);

    // 1. Check Modern RDM Parent
    if (j.contains("parent")) {
        spdlog::info("Found 'parent' key. Value: {}", j["parent"].dump());
    }

    // 2. Check Legacy Concept ID (The most likely culprit)
    if (j.contains("conceptrecid")) {
        spdlog::info("Found 'conceptrecid': {}", j["conceptrecid"].dump());
    }

    // 3. Check Metadata Concept ID
    if (j.contains("metadata") && j["metadata"].contains("conceptrecid")) {
        spdlog::info("Found 'metadata.conceptrecid': {}", j["metadata"]["conceptrecid"].dump());
    }

    // 4. Check Root Links
    if (j.contains("links") && j["links"].contains("conceptbadge")) {
        spdlog::info("Concept Link exists: {}", j["links"]["conceptbadge"].get<std::string>());
    }
}

std::vector<CZenodo::Deposit> CZenodo::get_deposits()
{
    std::vector<CZenodo::Deposit> deposits;
    std::string url = m_sZenodoUrl + "/api/user/records";

    cpr::Response r = cpr::Get(
        cpr::Url{ url },
        cpr::Header{ {"Authorization", "Bearer " + m_sAccessToken} }
    );

    if (r.status_code != 200)
    {
        spdlog::error("CZenodo: Failed to fetch user records. Status: {}", r.status_code);
        return deposits;
    }

    auto j = nlohmann::json::parse(r.text);
    //spdlog::info("--- All Records in My Account ---");

    if (j.contains("hits") && j["hits"].contains("hits"))
    {
        for (const auto& hit : j["hits"]["hits"])
        {
            int id = hit["id"].get<int>();
            std::string title = hit["metadata"].value("title", "No Title");
            std::string status = hit.value("status", "unknown");
            bool is_published = hit.value("is_published", false);
            
            deposits.emplace_back(id, title, status);

            //spdlog::info(" > ID: {} | Status: {} | Pub: {} | Title: {}", id, status, is_published ? "YES" : "NO", title);
        }
    }
    else
    {
        //spdlog::warn("Account is empty.");
    }
 
    return deposits;
}
