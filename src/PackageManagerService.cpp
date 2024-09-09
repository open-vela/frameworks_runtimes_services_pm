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

namespace fs = std::filesystem;

PackageManagerService::PackageManagerService() : mFirstBoot(false) {
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
    auto scanAndGetPackages = [this](const std::vector<std::string> &scanPath) {
        std::vector<PackageInfo> vecPackageInfo;
        for (const auto &path : scanPath) {
            PackageInfo pkgInfo;
            pkgInfo.manifest = joinPath(path, MANIFEST);
            int ret = mParser->parseManifest(&pkgInfo);
            if (!ret) {
                pkgInfo.userId = mInstaller->createUserId();
                auto status = mPackageInfo.insert(std::make_pair(pkgInfo.packageName, pkgInfo));
                if (status.second) {
                    vecPackageInfo.push_back(pkgInfo);
                    std::string appDataPath =
                            joinPath(PackageConfig::getInstance().getAppDataPath(),
                                     pkgInfo.packageName);
                    if (!fs::exists(appDataPath.c_str())) {
                        createDirectory(appDataPath.c_str());
                    }
                }
            }
        }
        return vecPackageInfo;
    };

    // create and scan manifest
    std::string packageListPath = PackageConfig::getInstance().getPackageListPath();
    if (!fs::exists(packageListPath.c_str())) {
        mFirstBoot = true;
        mInstaller->createPackageList();
        std::vector<std::string> vecScanPath =
                getChildDirectories(PackageConfig::getInstance().getAppPresetPath().c_str());
#ifdef CONFIG_SYSTEM_PACKAGE_SERVICE_DEBUG
        std::vector<std::string> installPath =
                getChildDirectories(PackageConfig::getInstance().getAppInstalledPath().c_str());
        vecScanPath.insert(vecScanPath.begin(), installPath.begin(), installPath.end());
#endif
        std::vector<PackageInfo> vecPackageInfo = scanAndGetPackages(vecScanPath);
        mInstaller->addInfoToPackageList(vecPackageInfo);
    } else {
#ifdef CONFIG_SYSTEM_PACKAGE_SERVICE_DEBUG
        unlink(packageListPath.c_str());
        mInstaller->createPackageList();
        std::vector<std::string> vecScanPath =
                getChildDirectories(PackageConfig::getInstance().getAppPresetPath().c_str());
        std::vector<std::string> installPath =
                getChildDirectories(PackageConfig::getInstance().getAppInstalledPath().c_str());
        vecScanPath.insert(vecScanPath.begin(), installPath.begin(), installPath.end());
        std::vector<PackageInfo> vecPackageInfo = scanAndGetPackages(vecScanPath);
        mInstaller->addInfoToPackageList(vecPackageInfo);
#else
        auto packagesIsEmpty = [&packageListPath]() {
            rapidjson::Document document;
            int ret = getDocument(packageListPath.data(), document);
            if (ret) {
                ALOGE("package list exist:%s, but parse document failed", packageListPath.data());
                assert(0);
            }

            const auto baseArray = rapidjson::Value(rapidjson::kArrayType);
            const auto &packagesArray =
                    getValue<const rapidjson::Value &>(document, "packages", baseArray);
            return packagesArray.Empty();
        };

        if (packagesIsEmpty()) {
            ALOGI("package list exist:%s, but parse packages empty", packageListPath.data());
            {
                fs::path tmppath{packageListPath};
                fs::remove(tmppath);
            }

            mInstaller->createPackageList();
            std::vector<std::string> vecScanPath =
                    getChildDirectories(PackageConfig::getInstance().getAppPresetPath().data());
            std::vector<PackageInfo> vecPackageInfo = scanAndGetPackages(vecScanPath);
            if (packagesIsEmpty()) {
                ALOGE("reparse packages : %s, but it is empty", packageListPath.data());
                assert(0);
            }
            mInstaller->addInfoToPackageList(vecPackageInfo);
        }

        mInstaller->loadPackageList(&mPackageInfo);
#endif
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
    std::string path = joinPath(PackageConfig::getInstance().getAppDataPath(), packageName);
    bool success = true;
    if (fs::exists(path)) {
        for (const auto &entry : fs::directory_iterator(path)) {
            if (entry.is_directory()) {
                if (!removeDirectory(entry.path().string().c_str())) {
                    success = false;
                }
            } else {
                if (unlink(entry.path().string().c_str()) != 0) {
                    ALOGE("unlink %s failed", entry.path().string().c_str());
                    success = false;
                }
            }
        }
    }
    if (success) {
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
    std::string tmp = joinPath(PackageConfig::getInstance().getAppDataPath(), "tmp");
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

    std::string dstPath =
            joinPath(PackageConfig::getInstance().getAppInstalledPath(), packageinfo.packageName);
    if (fs::exists(dstPath.c_str())) {
        removeDirectory(dstPath.c_str());
    }

    std::error_code ec;
    fs::rename(tmp.c_str(), dstPath.c_str(), ec);
    if (ec) {
        observer->onInstallResult(packageinfo.packageName, Status::EX_SECURITY,
                                  "Failed to copy file");
        ALOGE("Copy from %s to %s Failed:%s", tmp.c_str(), dstPath.c_str(), ec.message().c_str());
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_SECURITY);
    }
    std::string appDataPath =
            joinPath(PackageConfig::getInstance().getAppDataPath(), packageinfo.packageName);
    if (!fs::exists(appDataPath.c_str())) {
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

Status PackageManagerService::isFirstBoot(bool *firstBoot) {
    *firstBoot = mFirstBoot;
    return Status::ok();
}

Status PackageManagerService::getAllPackageName(std::vector<std::string> *pkgNames) {
    PM_PROFILER_BEGIN();
    for (auto it = mPackageInfo.begin(); it != mPackageInfo.end(); it++) {
        if (it->second.bAllValid) {
            ALOGD("getAllPackageName:%s", it->second.toString().c_str());
            pkgNames->push_back(it->second.packageName);
        } else {
            int ret = mParser->parseManifest(&it->second);
            if (!ret) {
                ALOGD("getAllPackageName:%s", it->second.toString().c_str());
                pkgNames->push_back(it->second.packageName);
            }
        }
    }
    PM_PROFILER_END();
    return Status::ok();
}

} // namespace pm
} // namespace os
