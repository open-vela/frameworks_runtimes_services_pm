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

#include <utils/String16.h>

#include "os/pm/BnPackageManager.h"
#include "os/pm/IPackageManager.h"
#include "os/pm/InstallParam.h"
#include "os/pm/UninstallParam.h"

namespace os {
namespace pm {

using android::binder::Status;

class PackageManagerService : public BnPackageManager {
public:
    Status getAllPackageInfo(std::vector<PackageInfo> *pkgInfos);
    Status getPackageInfo(const std::string &packageName, PackageInfo *pkgInfo);
    Status clearAppCache(const std::string &packageName, int32_t *ret);
    Status installPackage(const InstallParam &param, const android::sp<IInstallObserver> &observer);
    Status uninstallPackage(const UninstallParam &param,
                            const android::sp<IUninstallObserver> &observer);
    static android::String16 name() {
        return android::String16("package");
    }
}; // class PackageManagerService

} // namespace pm
} // namespace os