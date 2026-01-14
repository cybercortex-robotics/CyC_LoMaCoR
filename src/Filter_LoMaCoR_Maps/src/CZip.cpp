// Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#include "CZip.h"
#include <zip.h>
#include "os/CyC_FILESYSTEM.h"

std::string CZip::readFileToString(const std::string& path)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Unable to open file: " + path);
    return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

void CZip::writeFile(const std::string& path, const std::string& data)
{
    fs::create_directories(fs::path(path).parent_path());
    std::ofstream file(path, std::ios::binary);
    if (!file)
        throw std::runtime_error("Unable to write file: " + path);
    file.write(data.data(), data.size());
}

void CZip::create_zip(const std::string& zipPath, const std::vector<std::string>& files)
{
    int errorp;
    zip_t* zip = zip_open(zipPath.c_str(),
        ZIP_CREATE | ZIP_TRUNCATE, &errorp);
    if (!zip)
        throw std::runtime_error("Failed to create zip: " + zipPath);

    for (const auto& filePath : files)
    {
        std::string content = readFileToString(filePath);

        // Allocate memory libzip will own
        char* heapData = static_cast<char*>(malloc(content.size()));
        if (!heapData)
        {
            zip_discard(zip);
            throw std::runtime_error("Memory allocation failed for: " + filePath);
        }

        memcpy(heapData, content.data(), content.size());

        zip_source_t* source = zip_source_buffer(zip,
            heapData,
            content.size(),
            1); // libzip will free heapData
        
        if (!source)
        {
            free(heapData);
            zip_discard(zip);
            throw std::runtime_error("Failed to create zip source for: " + filePath);
        }

        if (zip_file_add(zip, fs::path(filePath).filename().string().c_str(), source, ZIP_FL_OVERWRITE) < 0)
        {
            zip_source_free(source);
            zip_discard(zip);
            throw std::runtime_error("Failed to add file to zip: " + filePath);
        }
    }

    if (zip_close(zip) < 0)
        throw std::runtime_error("Failed to finalize zip file.");
}


void CZip::extract_zip(const std::string& zipPath, const std::string& destDir)
{
    int errorp;
    zip_t* zip = zip_open(zipPath.c_str(), 0, &errorp);
    if (!zip)
        throw std::runtime_error("Could not open zip: " + zipPath);

    zip_int64_t numEntries = zip_get_num_entries(zip, 0);

    for (zip_uint64_t i = 0; i < static_cast<zip_uint64_t>(numEntries); ++i)
    {
        struct zip_stat st;
        zip_stat_init(&st);

        if (zip_stat_index(zip, i, 0, &st) != 0)
        {
            zip_close(zip);
            throw std::runtime_error("Failed to stat entry index: " + std::to_string(i));
        }

        zip_file_t* zf = zip_fopen_index(zip, i, 0);
        if (!zf)
        {
            zip_close(zip);
            throw std::runtime_error("Failed to open file in zip: " + std::string(st.name));
        }

        std::string buffer(st.size, '\0');
        zip_int64_t bytesRead = zip_fread(zf, buffer.data(), st.size);
        zip_fclose(zf);

        if (bytesRead < 0)
        {
            zip_close(zip);
            throw std::runtime_error("Failed to read zipped file: " + std::string(st.name));
        }

        std::string outPath = (fs::path(destDir) / st.name).string();
        writeFile(outPath, buffer);
    }

    zip_close(zip);
}
