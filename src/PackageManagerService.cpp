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

#include "pm/PackageManagerService.h"

#include <utils/Log.h>

namespace os {
namespace pm {
Status PackageManagerService::getAllPackageInfo(std::vector<PackageInfo> *pkgInfos) {
    // TODO
    return Status::ok();
}

Status PackageManagerService::getPackageInfo(const std::string &packageName, PackageInfo *pkgInfo) {
    // TODO
    return Status::ok();
}

Status PackageManagerService::clearAppCache(const std::string &packageName, int32_t *ret) {
    // TODO
    return Status::ok();
}

Status PackageManagerService::installPackage(const InstallParam &param,
                                             const android::sp<IInstallObserver> &observer) {
    // TODO
    if (observer) {
        observer->onInstallProcess("", 100);
        observer->onInstallResult("", 0, "success");
    }
    return Status::ok();
}

Status PackageManagerService::uninstallPackage(const UninstallParam &param,
                                               const android::sp<IUninstallObserver> &observer) {
    // TODO
    if (observer) {
        observer->onUninstallResult(param.packageName, 0, "success");
    }
    return Status::ok();
}

} // namespace pm
} // namespace os
