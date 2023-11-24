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

#include <optional>

#include "PackageUtils.h"
#include "pm/PackageInfo.h"

namespace os {
namespace pm {

class PackageParser {
public:
    int parseManifest(PackageInfo *info);

private:
    int parseNativeManifest(const rapidjson::Document &document, PackageInfo *info);
    int parseQuickAppManifest(const rapidjson::Document &document, PackageInfo *info);
}; // class PackageParser

} // namespace pm
} // namespace os
