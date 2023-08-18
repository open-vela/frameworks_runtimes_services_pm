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
using android::String8;
using android::binder::Status;

#define ASSERT_SERVICE(cond)                                   \
    if (cond) {                                                \
        ALOGE("ServiceManager can't find the service:%s",      \
              String8(PackageManagerService::name()).c_str()); \
        return DEAD_OBJECT;                                    \
    }

PackageManager::PackageManager() {
    android::getService<IPackageManager>(PackageManagerService::name(), &mService);
}

int32_t PackageManager::getAllPackageInfo(std::vector<PackageInfo> *pkgsInfo) {
    ASSERT_SERVICE(mService == nullptr);
    Status status = mService->getAllPackageInfo(pkgsInfo);
    if (!status.isOk()) {
        ALOGE("getAllPackageInfo failed:%s", status.toString8().c_str());
    }
    return status.exceptionCode();
}

int32_t PackageManager::getPackageInfo(const std::string &packageName, PackageInfo *info) {
    ASSERT_SERVICE(mService == nullptr);
    Status status = mService->getPackageInfo(packageName, info);
    if (!status.isOk()) {
        ALOGE("getPackageInfo failed:%s", status.toString8().c_str());
    }
    return status.exceptionCode();
}

int32_t PackageManager::clearAppCache(const std::string &packageName) {
    ASSERT_SERVICE(mService == nullptr);
    int32_t ret;
    Status status = mService->clearAppCache(packageName, &ret);
    if (!status.isOk()) {
        ALOGE("clearAppCache failed:%s", status.toString8().c_str());
        return status.exceptionCode();
    }
    return ret;
}

int32_t PackageManager::installPackage(const InstallParam &param, sp<BnInstallObserver> listener) {
    ASSERT_SERVICE(mService == nullptr);
    Status status = mService->installPackage(param, listener);
    if (!status.isOk()) {
        ALOGE("installPackage failed:%s", status.toString8().c_str());
    }
    return status.exceptionCode();
}

int32_t PackageManager::uninstallPackage(const UninstallParam &param,
                                         sp<BnUninstallObserver> listener) {
    ASSERT_SERVICE(mService == nullptr);
    Status status = mService->uninstallPackage(param, listener);
    if (!status.isOk()) {
        ALOGE("uninstallPackage failed:%s", status.toString8().c_str());
    }
    return status.exceptionCode();
}

} // namespace pm
} // namespace os