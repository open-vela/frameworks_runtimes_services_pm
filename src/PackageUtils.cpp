/*
 * Copyright (C) 2023 Xiaomi Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PackageUtils.h"

#include <utils/Errors.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace os {
namespace pm {

using std::error_code;
using std::ifstream;
using std::ofstream;
using std::chrono::system_clock;
using std::filesystem::create_directories;
using std::filesystem::exists;
using std::filesystem::file_size;
using std::filesystem::is_regular_file;
using std::filesystem::perm_options;
using std::filesystem::permissions;
using std::filesystem::perms;
using std::filesystem::recursive_directory_iterator;
using std::filesystem::remove_all;

std::string getCurrentTime() {
    const auto finish = system_clock::to_time_t(system_clock::now());
    std::tm finish_tm;
    localtime_r(&finish, &finish_tm);
    std::stringstream oss;
    oss << std::put_time(&finish_tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

bool createDirectory(const char *path, perms p) {
    error_code ec;
    bool ret = create_directories(path, ec);
    IF_OK_RETURN_WITH_LOG(ec, ret, "Failed create directory %s:%s\n", path, ec.message().c_str());
    permissions(path, p, perm_options::add);
    return ret;
}

bool removeDirectory(const char *path) {
    error_code ec;
    int ret = remove_all(path, ec);
    IF_OK_RETURN_WITH_LOG(ec, ret, "Failed to delete %s:%s", path, ec.message().c_str());
    return true;
}

int readFile(const char *filename, std::string &content) {
    ifstream file(filename);
    IF_OK_RETURN_WITH_LOG(!file, android::NAME_NOT_FOUND, "Failed to open file %s", filename);
    file.seekg(0, std::ios::end);
    content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&content[0], content.size());
    file.close();
    return 0;
}

int writeFile(const char *filename, const std::string &data) {
    ofstream file(filename);
    IF_OK_RETURN_WITH_LOG(!file, android::NAME_NOT_FOUND, "Failed to open file %s", filename);
    file << data;
    file.close();
    return 0;
}

int64_t getDirectorySize(const char *path) {
    int64_t totalSize = 0;
    std::filesystem::path folderPath = path;
    for (const auto &entry : recursive_directory_iterator(folderPath)) {
        if (is_regular_file(entry)) {
            totalSize += file_size(entry);
        }
    }
    return totalSize;
}

std::string joinPath(std::string basic, std::string suffix) {
    std::filesystem::path basePath = basic;
    basePath.append(suffix);
    return basePath.string();
}

} // namespace pm
} // namespace os