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

#include "os/pm/BnInstallObserver.h"
#include "os/pm/BnUninstallObserver.h"
#include "os/pm/IPackageManager.h"

namespace os {
namespace pm {

using android::sp;

class PackageManager {
public:
    PackageManager();
    int32_t getAllPackageInfo(std::vector<PackageInfo> *pkgsInfo);
    int32_t getPackageInfo(const std::string &packageName, PackageInfo *info);
    int32_t clearAppCache(const std::string &packageName);
    int32_t installPackage(const InstallParam &param, sp<BnInstallObserver> listener = nullptr);
    int32_t uninstallPackage(const UninstallParam &param,
                             sp<BnUninstallObserver> listener = nullptr);

private:
    sp<IPackageManager> mService;
}; // class PackageManager

} // namespace pm
} // namespace os
