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

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <utils/Errors.h>
#include <utils/Log.h>

#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace os {
namespace pm {

using rapidjson::PrettyWriter;
using rapidjson::StringBuffer;
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

bool createDirectory(const char *path) {
    error_code ec;
    bool ret = create_directories(path, ec);
    if (ec) {
        ALOGE("Failed create directory %s:%s", path, ec.message().c_str());
        return ret;
    }
    return ret;
}

bool removeDirectory(const char *path) {
    error_code ec;
    int ret = remove_all(path, ec);
    if (ec) {
        ALOGE("Failed to delete %s:%s", path, ec.message().c_str());
        return ret;
    }
    return true;
}

int readFile(const char *filename, std::string &content) {
    ifstream file(filename);
    if (!file) {
        ALOGE("Failed to open file %s", filename);
        return android::NAME_NOT_FOUND;
    }
    file.seekg(0, std::ios::end);
    content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&content[0], content.size());
    file.close();
    return 0;
}

int writeFile(const char *filename, const std::string &data) {
    std::filesystem::path fullPath(filename);
    std::filesystem::path parent = fullPath.parent_path();
    if (!exists(parent)) {
        if (!createDirectory(parent.string().c_str())) {
            ALOGE("writeFile failed at create parent directory:%s", parent.string().c_str());
            return android::PERMISSION_DENIED;
        }
    }
    ofstream file(filename);
    if (!file) {
        ALOGE("Failed to open file %s", filename);
        return android::NAME_NOT_FOUND;
    }
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

bool hasMember(const rapidjson::Value &parent, const std::string &name) {
    if (parent.IsNull()) return false;
    return parent.HasMember(name.c_str());
}

int getDocument(const char *path, rapidjson::Document &document) {
    std::string content;
    int ret = readFile(path, content);
    if (ret < 0) {
        ALOGE("read file %s failed", path);
        return ret;
    }
    if (document.Parse(content.c_str()).HasParseError()) {
        ALOGE("parse file %s failed,is not json format", path);
        return android::BAD_VALUE;
    }
    return 0;
}

std::string toPrettyString(const rapidjson::Document &doc) {
    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    doc.Accept(writer);
    return buffer.GetString();
}

std::string calculateShasum(const char *path) {
    // TODO
    return "";
}

} // namespace pm
} // namespace os