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

#include "pm/PackageManager.h"

#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>

#include "pm/PackageManagerService.h"

namespace os {
namespace pm {

using namespace android;
using android::binder::Status;

PackageManager::PackageManager() {
    android::getService<IPackageManager>(PackageManagerService::name(), &mService);
}

int PackageManager::getAllPackageInfo(std::vector<PackageInfo> *pkgsInfo) {
    if (mService == nullptr) {
        return DEAD_OBJECT;
    }
    Status status = mService->getAllPackageInfo(pkgsInfo);
    if (!status.isOk()) {
        ALOGE("getAllPackageInfo failed:%s", status.toString8().c_str());
    }
    return status.exceptionCode();
}

int PackageManager::getPackageInfo(const std::string &packageName, PackageInfo *info) {
    if (mService == nullptr) {
        return DEAD_OBJECT;
    }
    Status status = mService->getPackageInfo(packageName, info);
    if (!status.isOk()) {
        ALOGE("getPackageInfo failed:%s", status.toString8().c_str());
    }
    return status.exceptionCode();
}

int PackageManager::clearAppCache(const std::string &packageName) {
    if (mService == nullptr) {
        return DEAD_OBJECT;
    }
    int32_t ret;
    Status status = mService->clearAppCache(packageName, &ret);
    if (!status.isOk()) {
        ALOGE("clearAppCache failed:%s", status.toString8().c_str());
        return status.exceptionCode();
    }
    return ret;
}

int PackageManager::installPackage(const InstallParam &param, sp<BnInstallObserver> listener) {
    if (mService == nullptr) {
        return DEAD_OBJECT;
    }
    Status status = mService->installPackage(param, listener);
    if (!status.isOk()) {
        ALOGE("installPackage failed:%s", status.toString8().c_str());
    }
    return status.exceptionCode();
}

int PackageManager::uninstallPackage(const UninstallParam &param,
                                     sp<BnUninstallObserver> listener) {
    if (mService == nullptr) {
        return DEAD_OBJECT;
    }
    Status status = mService->uninstallPackage(param, listener);
    if (!status.isOk()) {
        ALOGE("uninstallPackage failed:%s", status.toString8().c_str());
    }
    return status.exceptionCode();
}

} // namespace pm
} // namespace os