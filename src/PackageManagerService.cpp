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

#include <filesystem>

#include "PackageInstaller.h"
#include "PackageParser.h"
#include "PackageTrace.h"
#include "PackageUtils.h"

namespace os {
namespace pm {

using std::filesystem::copy_options;
using std::filesystem::directory_iterator;
using std::filesystem::exists;
using std::filesystem::remove_all;
using std::filesystem::rename;
using std::filesystem::temp_directory_path;

PackageManagerService::PackageManagerService() {
    mInstaller = new PackageInstaller();
    mParser = new PackageParser();
    init();
}

PackageManagerService::~PackageManagerService() {
    if (mParser) {
        delete mParser;
        mParser = nullptr;
    }
    if (mInstaller) {
        delete mInstaller;
        mInstaller = nullptr;
    }
}

void PackageManagerService::init() {
    PM_PROFILER_BEGIN();
    // create and scan manifest
    if (!exists(PACKAGE_LIST_PATH)) {
        mInstaller->createPackageList();
        std::vector<PackageInfo> vecPackageInfo;
        for (const auto &entry :
             directory_iterator(PackageConfig::getInstance().getAppPresetPath().c_str())) {
            if (entry.is_directory()) {
                PackageInfo pkgInfo;
                pkgInfo.manifest = joinPath(entry.path().string(), MANIFEST);
                int ret = mParser->parseManifest(&pkgInfo);
                if (!ret) {
                    pkgInfo.userId = mInstaller->createUserId();
                    mPackageInfo.insert(std::make_pair(pkgInfo.packageName, pkgInfo));
                    vecPackageInfo.push_back(pkgInfo);
                    std::string appDataPath =
                            joinPath(PackageConfig::getInstance().getAppDataPath(),
                                     pkgInfo.packageName);
                    if (!exists(appDataPath.c_str())) {
                        createDirectory(appDataPath.c_str());
                    }
                }
            }
        }
        mInstaller->addInfoToPackageList(vecPackageInfo);
    } else {
        mInstaller->loadPackageList(&mPackageInfo);
    }
    PM_PROFILER_END();
}

Status PackageManagerService::getAllPackageInfo(std::vector<PackageInfo> *pkgInfos) {
    PM_PROFILER_BEGIN();
    for (auto it = mPackageInfo.begin(); it != mPackageInfo.end(); it++) {
        if (it->second.bAllValid) {
            ALOGD("getAllPackageInfo:%s", it->second.toString().c_str());
            pkgInfos->push_back(it->second);
        } else {
            int ret = mParser->parseManifest(&it->second);
            if (!ret) {
                ALOGD("getAllPackageInfo:%s", it->second.toString().c_str());
                pkgInfos->push_back(it->second);
            }
        }
    }
    PM_PROFILER_END();
    return Status::ok();
}

Status PackageManagerService::getPackageInfo(const std::string &packageName, PackageInfo *pkgInfo) {
    PM_PROFILER_BEGIN();
    ALOGD("getPackageInfo package:%s", packageName.c_str());
    if (mPackageInfo.find(packageName) == mPackageInfo.end()) {
        ALOGE("getPackageInfo package:%s can't find", packageName.c_str());
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_SERVICE_SPECIFIC);
    }

    if (!mPackageInfo[packageName].bAllValid) {
        int ret = mParser->parseManifest(&mPackageInfo[packageName]);
        if (ret) {
            PM_PROFILER_END();
            return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        }
    }
    *pkgInfo = mPackageInfo[packageName];
    ALOGD("packageInfo: %s", pkgInfo->toString().c_str());
    PM_PROFILER_END();
    return Status::ok();
}

Status PackageManagerService::clearAppCache(const std::string &packageName, int32_t *ret) {
    PM_PROFILER_BEGIN();
    ALOGD("clearAppCache package:%s", packageName.c_str());
    *ret = Status::EX_ILLEGAL_ARGUMENT;
    if (mPackageInfo.find(packageName) == mPackageInfo.end()) {
        ALOGE("clearAppCache package:%s can't find", packageName.c_str());
        PM_PROFILER_END();
        return Status::ok();
    }

    std::error_code ec;
    remove_all(joinPath(PackageConfig::getInstance().getAppDataPath(), packageName), ec);
    if (ec) {
        ALOGE("clearAppCache remove_all error:%s", ec.message().c_str());
    } else {
        *ret = 0;
    }
    PM_PROFILER_END();
    return Status::ok();
}

Status PackageManagerService::installPackage(const InstallParam &param,
                                             const android::sp<IInstallObserver> &observer) {
    PM_PROFILER_BEGIN();
    ALOGD("installPackage:%s", param.toString().c_str());
    size_t pos = param.path.find_last_of('/');
    std::string rpkFullName = param.path;
    if (pos != std::string::npos) {
        rpkFullName = param.path.substr(pos + 1);
    }
    pos = rpkFullName.rfind('.');
    std::string rpkName = rpkFullName.substr(0, pos);
    std::string dstPath = joinPath(PackageConfig::getInstance().getAppInstalledPath(), rpkName);
    std::string tmp = joinPath(PackageConfig::getInstance().getAppInstalledPath(), "tmp");
    tmp = joinPath(tmp, rpkName);

    int ret = mInstaller->installApp(param);
    if (ret) {
        observer->onInstallResult(param.path, ret, "Failed to deal with rpkpackage");
        ALOGE("decompress %s failed", param.path.c_str());
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_ILLEGAL_STATE);
    }

    PackageInfo packageinfo;
    packageinfo.manifest = joinPath(tmp, MANIFEST);
    ret = mParser->parseManifest(&packageinfo);
    if (ret) {
        removeDirectory(tmp.c_str());
        ALOGE("parse manifest:%s failed\n", packageinfo.manifest.c_str());
        observer->onInstallResult(packageinfo.packageName, ret, "Failed to parse manifest");
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
    }

    if (exists(dstPath.c_str())) {
        removeDirectory(dstPath.c_str());
    }

    std::error_code ec;
    rename(tmp.c_str(), dstPath.c_str(), ec);
    if (ec) {
        observer->onInstallResult(packageinfo.packageName, Status::EX_SECURITY,
                                  "Failed to copy file");
        ALOGE("Copy from %s to %s Failed:%s", tmp.c_str(), dstPath.c_str(), ec.message().c_str());
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_SECURITY);
    }
    std::string appDataPath =
            joinPath(PackageConfig::getInstance().getAppDataPath(), packageinfo.packageName);
    if (!exists(appDataPath.c_str())) {
        createDirectory(appDataPath.c_str());
    }

    packageinfo.installedPath = dstPath;
    packageinfo.manifest = joinPath(dstPath, MANIFEST);
    if (mPackageInfo.find(packageinfo.packageName) != mPackageInfo.end()) {
        PackageInfo oldPackageInfo = mPackageInfo[packageinfo.packageName];
        packageinfo.userId = oldPackageInfo.userId;
        mPackageInfo.erase(packageinfo.packageName);
        mInstaller->deleteInfoFromPackageList(packageinfo.packageName);
        if (oldPackageInfo.installedPath != packageinfo.installedPath) {
            removeDirectory(oldPackageInfo.installedPath.c_str());
        }
    }
    mPackageInfo.insert(std::make_pair(packageinfo.packageName, packageinfo));
    mInstaller->addInfoToPackageList(packageinfo);
    observer->onInstallResult(packageinfo.packageName, 0, "success");
    PM_PROFILER_END();
    return Status::ok();
}

Status PackageManagerService::uninstallPackage(const UninstallParam &param,
                                               const android::sp<IUninstallObserver> &observer) {
    PM_PROFILER_BEGIN();
    ALOGD("uninstallPackage:%s\n", param.toString().c_str());
    if (mPackageInfo.find(param.packageName) == mPackageInfo.end()) {
        if (observer) {
            observer->onUninstallResult(param.packageName, android::NAME_NOT_FOUND,
                                        "Not found package");
        }
        ALOGE("uninstallPackage package:%s can't find", param.packageName.c_str());
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
    }

    if (!removeDirectory(mPackageInfo[param.packageName].installedPath.c_str())) {
        if (observer) {
            observer->onUninstallResult(param.packageName, android::PERMISSION_DENIED,
                                        "Delete Directory Failed");
        }
        ALOGE("Delete Directory:%s Failed", mPackageInfo[param.packageName].installedPath.c_str());
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION);
    }

    mPackageInfo.erase(param.packageName);
    mInstaller->deleteInfoFromPackageList(param.packageName);
    if (param.clearCache) {
        removeDirectory(
                joinPath(PackageConfig::getInstance().getAppDataPath(), param.packageName).c_str());
    }
    if (observer) {
        observer->onUninstallResult(param.packageName, 0, "success");
    }
    PM_PROFILER_END();
    return Status::ok();
}

Status PackageManagerService::getPackageSizeInfo(const std::string &packageName,
                                                 PackageStats *pkgStats) {
    PM_PROFILER_BEGIN();
    if (mPackageInfo.find(packageName) == mPackageInfo.end()) {
        ALOGE("getPackageSizeInfo package:%s can't find", packageName.c_str());
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_SERVICE_SPECIFIC);
        ;
    }

    PackageInfo pkgInfo = mPackageInfo[packageName];
    std::string codePath = pkgInfo.installedPath;
    std::string dataPath = joinPath(PackageConfig::getInstance().getAppDataPath(), packageName);
    std::string cachePath = joinPath(dataPath, "cache");
    pkgStats->codeSize = getDirectorySize(codePath.c_str());
    pkgStats->dataSize = getDirectorySize(dataPath.c_str());
    pkgStats->cacheSize = getDirectorySize(cachePath.c_str());
    PM_PROFILER_END();
    return Status::ok();
}

} // namespace pm
} // namespace os
