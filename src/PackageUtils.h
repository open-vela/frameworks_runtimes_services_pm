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

} // namespace pm
} // namespace os
