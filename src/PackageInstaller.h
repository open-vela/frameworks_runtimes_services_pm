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

#include <vector>

#include "os/pm/IInstallObserver.h"
#include "os/pm/InstallParam.h"
#include "pm/PackageInfo.h"

namespace os {
namespace pm {

class PackageInstaller {
public:
    PackageInstaller();
    int installApp(const InstallParam& param);
    int32_t createUserId();
    int createPackageList();
    bool loadPackageList(std::map<std::string, PackageInfo>* pkgInfos);
    int addInfoToPackageList(const PackageInfo& installInfo);
    int addInfoToPackageList(const std::vector<PackageInfo>& vecExtraInfo);
    int deleteInfoFromPackageList(const std::string& packageName);

private:
    int installNativeApp(const InstallParam& param);
    int installQuickApp(const InstallParam& param);
    std::string mPackgeListPath;
};
} // namespace pm
} // namespace os
