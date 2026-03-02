// Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#ifndef CZenodo_H_
#define CZenodo_H_

#include "CyC_TYPES.h"
#include <cpr/cpr.h>
#include <json.hpp>

class CZenodo
{
public:
    struct Deposit
    {
        Deposit() {};
        Deposit(const int _id, const std::string _title) :
            id(_id),
            title(_title)
        {};

        int         id;
        std::string title;
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
    CZenodo(const std::string& _zenodo_url, const std::string& _access_token);
    virtual ~CZenodo() {};

    bool is_active() { return m_bIsActive; };
    bool set_auth_headers(const std::string& _zenodo_url, const std::string& _access_token);

    std::vector<Deposit> deposits() { return m_Deposits; };
    int find_deposit(const std::string& _deposit_name); // Returns the deposit's ID
    int create_deposit(const std::string& _deposit_name, const std::string& _description, const std::string& _upload_type = "poster");

    std::vector<File> get_files(const int& _deposition_id);
    bool upload_file(const int& _deposition_id, const std::string& _filepath);
    bool download_file(const long& _deposition_id, const std::string& _filename_on_zenodo, const std::string& _filename_local = std::string());
    bool download_file(const File& _file, const std::string& _filename_local = std::string());

private:
    cpr::Header get_auth_headers();

private:
    bool        m_bIsActive = false;
    std::string m_sAccessToken;
    std::string m_sZenodoUrl;

    std::vector<Deposit> m_Deposits;
};

#endif /* CZenodo_H_ */
