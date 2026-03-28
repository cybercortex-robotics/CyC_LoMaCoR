// Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#ifndef CZenodo_H_
#define CZenodo_H_

#include <cpr/cpr.h>
#include <json.hpp>

class CZenodo
{
public:
    struct Deposit
    {
        Deposit() {};
        Deposit(const int _id, const std::string _title, const std::string _status) :
            id(_id),
            title(_title),
            status(_status)
        {};

        int         id;
        std::string title;
        std::string status;
    };

    struct File
    {
        File() {};
        File(const std::string _id, const std::string _name, const std::string _download_link) :
            id(_id),
            name(_name),
            download_link(_download_link)
        {};

        std::string id;
        std::string name;
        std::string download_link;
    };

public:
    CZenodo() {};
    CZenodo(const std::string& _credentials_file);
    CZenodo(const std::string& _family_name, const std::string& _given_name, const std::string& _affiliation, const std::string& _zenodo_url, const std::string& _access_token);
    virtual ~CZenodo() {};

    bool is_active() { return m_bIsActive; };
    bool set_auth_headers(const std::string& _family_name, const std::string& _given_name, const std::string& _affiliation, const std::string& _zenodo_url, const std::string& _access_token);

    std::vector<Deposit> get_deposits();
    int find_deposit(const std::string& _deposit_name); // Returns the deposit's ID
    int create_deposit(const std::string& _deposit_name, const std::string& _description, const std::string& _upload_type = "dataset");

    void debug_version_history(const int& _id);
    void verify_draft_parentage(const int& _id);
    void debug_family_identifiers(const int& _published_id, bool _is_draft);
    
    void verify_specific_draft(const int& _id);
    int find_active_draft_id(const int& _published_id);
    int find_draft_by_title(const std::string& _title);

    std::vector<CZenodo::File> get_draft_files(const int& _id_draft);
    std::vector<File> get_published_files(const int& _deposition_id);

    bool upload_file(const int& _deposition_id, const std::string& _filepath, const bool& _publish = false);
    bool download_file(const long& _deposition_id, const std::string& _filename_on_zenodo, const std::string& _filename_local = std::string());
    bool download_file(const File& _file, const std::string& _filename_local = std::string());

private:
    cpr::Header get_auth_headers();

public:
    bool        m_bIsActive = false;
    std::string m_sFamilyName;
    std::string m_sGivenName;
    std::string m_sAffiliation;
    std::string m_sAccessToken;
    std::string m_sZenodoUrl;
};

#endif /* CZenodo_H_ */
