// Copyright (c) 2025 CyberCortex Robotics SRL. All rights reserved
// Author: Sorin Mihai Grigorescu

#ifndef CZip_H_
#define CZip_H_

#include "CyC_TYPES.h"

class CZip
{
public:
    // Create a new zip file and add files
    static void create_zip(const std::string& zipPath, const std::vector<std::string>& files);

    // Extract all files from a zip archive into a directory
    static void extract_zip(const std::string& zipPath, const std::string& destDir);

private:
    static std::string readFileToString(const std::string& path);
    static void writeFile(const std::string& path, const std::string& data);
};

#endif /* CZip_H_ */
