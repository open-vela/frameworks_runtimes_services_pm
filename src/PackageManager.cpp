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

#include "PackageTrace.h"
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
    PM_PROFILER_BEGIN();
    Status status = mService->getAllPackageInfo(pkgsInfo);
    if (!status.isOk()) {
        ALOGE("getAllPackageInfo failed:%s", status.toString8().c_str());
    }
    PM_PROFILER_END();
    return status.exceptionCode();
}

int32_t PackageManager::getAllPackageInfoEx(std::vector<PackageInfo> *pkgsInfo, int32_t sliceSize) {
    ASSERT_SERVICE(mService == nullptr);
    PM_PROFILER_BEGIN();

    pkgsInfo->clear();
    ::os::pm::SlicedPackageInfo slicedInfo;
    Status status = mService->getAllPackageInfoEx(sliceSize, &slicedInfo);

    if (!status.isOk()) {
        ALOGE("getAllPackageInfoEx failed:%s", status.toString8().c_str());
        PM_PROFILER_END();
        return status.exceptionCode();
    }

    // Add first slice
    pkgsInfo->insert(pkgsInfo->end(), slicedInfo.firstSlice.begin(), slicedInfo.firstSlice.end());

    int receivedCount = slicedInfo.firstSlice.size();
    sp<IPackageInfoProvider> provider = slicedInfo.provider;

    if (provider != nullptr) {
        while (receivedCount < slicedInfo.totalSize) {
            std::vector<PackageInfo> nextSlice;
            status = provider->getNext(receivedCount, sliceSize, &nextSlice);
            if (!status.isOk() || nextSlice.empty()) {
                ALOGE("Failed to get next slice or slice empty. error: %s",
                      status.toString8().c_str());
                break;
            }
            pkgsInfo->insert(pkgsInfo->end(), nextSlice.begin(), nextSlice.end());
            receivedCount += nextSlice.size();
        }
    }

    if (receivedCount != slicedInfo.totalSize) {
        ALOGW("Mismatched package count: expected %" PRId32 ", got %" PRId32, slicedInfo.totalSize,
              static_cast<int32_t>(pkgsInfo->size()));
    }

    PM_PROFILER_END();
    return status.isOk() ? 0 : status.exceptionCode();
}

int32_t PackageManager::getPackagesInOperation(std::vector<PackageInOperation> *operationStatus) {
    ASSERT_SERVICE(mService == nullptr);
    PM_PROFILER_BEGIN();
    Status status = mService->getPackagesInOperation(operationStatus);
    if (!status.isOk()) {
        ALOGE("getPackagesInOperation failed:%s", status.toString8().c_str());
    }
    PM_PROFILER_END();
    return status.exceptionCode();
}

int32_t PackageManager::getPackageInfo(const std::string &packageName, PackageInfo *info) {
    ASSERT_SERVICE(mService == nullptr);
    PM_PROFILER_BEGIN();
    Status status = mService->getPackageInfo(packageName, info);
    if (!status.isOk()) {
        ALOGE("getPackageInfo failed:%s", status.toString8().c_str());
    }
    PM_PROFILER_END();
    return status.exceptionCode();
}

int32_t PackageManager::clearAppCache(const std::string &packageName) {
    ASSERT_SERVICE(mService == nullptr);
    PM_PROFILER_BEGIN();
    int32_t ret;
    Status status = mService->clearAppCache(packageName, &ret);
    if (!status.isOk()) {
        ALOGE("clearAppCache failed:%s", status.toString8().c_str());
        PM_PROFILER_END();
        return status.exceptionCode();
    }
    PM_PROFILER_END();
    return ret;
}

int32_t PackageManager::installPackage(const InstallParam &param, sp<BnInstallObserver> listener) {
    ASSERT_SERVICE(mService == nullptr);
    PM_PROFILER_BEGIN();
    Status status = mService->installPackage(param, listener);
    if (!status.isOk()) {
        ALOGE("installPackage failed:%s", status.toString8().c_str());
    }
    PM_PROFILER_END();
    return status.exceptionCode();
}

int32_t PackageManager::uninstallPackage(const UninstallParam &param,
                                         sp<BnUninstallObserver> listener) {
    ASSERT_SERVICE(mService == nullptr);
    PM_PROFILER_BEGIN();
    Status status = mService->uninstallPackage(param, listener);
    if (!status.isOk()) {
        ALOGE("uninstallPackage failed:%s", status.toString8().c_str());
    }
    PM_PROFILER_END();
    return status.exceptionCode();
}

int32_t PackageManager::getPackageSizeInfo(const std::string &packageName, PackageStats *stats) {
    ASSERT_SERVICE(mService == nullptr);
    PM_PROFILER_BEGIN();
    Status status = mService->getPackageSizeInfo(packageName, stats);
    if (!status.isOk()) {
        ALOGE("getPackageStats failed:%s", status.toString8().c_str());
    }
    PM_PROFILER_END();
    return status.exceptionCode();
}

int32_t PackageManager::isFirstBoot(bool *firstBoot) {
    ASSERT_SERVICE(mService == nullptr);
    PM_PROFILER_BEGIN();
    Status status = mService->isFirstBoot(firstBoot);
    if (!status.isOk()) {
        ALOGE("isFirstBoot failed:%s", status.toString8().c_str());
    }
    PM_PROFILER_END();
    return status.exceptionCode();
}

int32_t PackageManager::getAllPackageName(std::vector<std::string> *pkgNames) {
    ASSERT_SERVICE(mService == nullptr);
    PM_PROFILER_BEGIN();
    Status status = mService->getAllPackageName(pkgNames);
    if (!status.isOk()) {
        ALOGE("getAllPackageName failed:%s", status.toString8().c_str());
    }
    PM_PROFILER_END();
    return status.exceptionCode();
}

} // namespace pm
} // namespace os