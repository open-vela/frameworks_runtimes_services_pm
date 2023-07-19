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

#pragma once

#include <rapidjson/document.h>
#include <rapidjson/rapidjson.h>

#include <filesystem>
#include <iostream>

namespace os {
namespace pm {

#define PM_VERSION "1.0.0"
#define MANIFEST "manifest.json"
#define PACKAGE_CFG "/etc/package.cfg"
#define PACKAGE_LIST_PATH "/data/app/packages.list"

std::string getCurrentTime();
bool createDirectory(const char *path);
bool removeDirectory(const char *path);
int64_t getDirectorySize(const char *path);
int readFile(const char *filename, std::string &content);
int writeFile(const char *filename, const std::string &data);
std::string joinPath(std::string basic, std::string suffix);
bool hasMember(const rapidjson::Value &parent, const std::string &name);
int getDocument(const char *path, rapidjson::Document &document);
std::string toPrettyString(const rapidjson::Document &doc);
std::string calculateShasum(const char *path);

template <typename T>
T getValue(const rapidjson::Value &parent, const std::string &key, const T &defaultValue) {
    if (parent.IsNull()) return defaultValue;
    if (!hasMember(parent, key)) return defaultValue;
    const rapidjson::Value &value = parent[key.c_str()];
    if constexpr (std::is_same<T, int>::value) {
        return value.IsInt() ? value.GetInt() : defaultValue;
    } else if constexpr (std::is_same<T, int64_t>::value) {
        return value.IsInt64() ? value.GetInt64() : defaultValue;
    } else if constexpr (std::is_same<T, std::string>::value) {
        return value.IsString() ? value.GetString() : defaultValue;
    } else if constexpr (std::is_same<T, bool>::value) {
        return value.IsBool() ? value.GetBool() : defaultValue;
    } else if constexpr (std::is_same<T, float>::value) {
        return value.IsFloat() ? value.GetFloat() : defaultValue;
    } else if constexpr (std::is_same<T, double>::value) {
        return value.IsDouble() ? value.GetDouble() : defaultValue;
    } else if constexpr (std::is_same<T, const rapidjson::Value &>::value) {
        if (defaultValue.IsObject() && value.IsObject()) return value;
        if (defaultValue.IsArray() && value.IsArray()) return value;
    }
    return defaultValue;
}

} // namespace pm
} // namespace os
