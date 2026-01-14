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

#include <sys/statvfs.h>
#include <utils/Log.h>
#include <uv_ext.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>
#include <unordered_set>

#include "PackageInstaller.h"
#include "PackageParser.h"
#include "PackageTrace.h"
#include "PackageUtils.h"
#include "app/Logger.h"
#include "os/pm/BnPackageInfoProvider.h"
#include "pm/SlicedPackageInfo.h"

#ifdef CONFIG_HAP_APP_PATH
#define ABS_PATH_PREFIX CONFIG_HAP_APP_PATH
#else
#define ABS_PATH_PREFIX "/data/quickapp"
#endif

namespace os {
namespace pm {

namespace fs = std::filesystem;

class PackageInfoProvider : public BnPackageInfoProvider {
public:
    explicit PackageInfoProvider(const std::vector<PackageInfo> &allInfos)
          : mAllPackageInfos(allInfos) {}
    virtual ~PackageInfoProvider() = default;

    Status getNext(int32_t from, int32_t count, std::vector<PackageInfo> *pkgInfos) override {
        auto size = static_cast<int32_t>(mAllPackageInfos.size());
        if (from < 0 || from >= size) {
            // Invalid start index, return empty list.
            return Status::ok();
        }

        int32_t end = std::min(from + count, size);
        for (int32_t i = from; i < end; ++i) {
            pkgInfos->push_back(mAllPackageInfos[i]);
        }
        return Status::ok();
    }

private:
    const std::vector<PackageInfo> mAllPackageInfos;
};

PackageManagerService::PackageManagerService(uv_loop_t *looper)
      : mFirstBoot(false), mLooper(looper) {
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
        std::unordered_set<std::string> filters;
        for (const auto &path : scanPath) {
            PackageInfo pkgInfo;
            pkgInfo.manifest = joinPath(path, MANIFEST);
            int ret = mParser->parseManifest(&pkgInfo);
            if (!ret) {
                pkgInfo.userId = mInstaller->createUserId();
                if (filters.count(pkgInfo.packageName)) continue;
                vecPackageInfo.push_back(pkgInfo);
                std::string appDataPath = joinPath(PackageConfig::getInstance().getAppDataPath(),
                                                   pkgInfo.packageName);
                if (!fs::exists(appDataPath.c_str())) {
                    createDirectory(appDataPath.c_str());
                }
            }
        }
        return vecPackageInfo;
    };

    auto appPresetPath = PackageConfig::getInstance().getAppPresetPath();
    auto getScanPackages = [scanAndGetPackages, &appPresetPath]() {
        std::vector<std::string> vecScanPath = getChildDirectories(appPresetPath.data());
#ifdef CONFIG_SYSTEM_PACKAGE_SERVICE_DEBUG
        std::vector<std::string> installPath =
                getChildDirectories(PackageConfig::getInstance().getAppInstalledPath().c_str());
        vecScanPath.insert(vecScanPath.begin(), installPath.begin(), installPath.end());
#endif
        std::vector<PackageInfo> vecPackageInfo = scanAndGetPackages(vecScanPath);
        return vecPackageInfo;
    };

    // create and scan manifest
    std::string packageListPath = PackageConfig::getInstance().getPackageListPath();
    if (!fs::exists(packageListPath.c_str())) {
        mFirstBoot = true;
        mInstaller->createPackageList();
        std::vector<PackageInfo> vecPackageInfo = getScanPackages();
        mInstaller->addInfoToPackageList(vecPackageInfo);
    } else {
        mInstaller->loadPackageList(&mPackageInfo);

        std::vector<PackageInfo> vecPackageInfo = getScanPackages();

        // 找到packages.list中有的，而vecPackage中没有的,删除
        for (const auto &[packagename, pkgInfo] : mPackageInfo) {
            if (std::search(pkgInfo.installedPath.begin(), pkgInfo.installedPath.end(),
                            appPresetPath.begin(),
                            appPresetPath.end()) != pkgInfo.installedPath.end() &&
                std::find_if(vecPackageInfo.begin(), vecPackageInfo.end(),
                             [&packagename](const auto &pkginfo) {
                                 return packagename == pkginfo.packageName;
                             }) == vecPackageInfo.end()) {
                mInstaller->deleteInfoFromPackageList(packagename);
            }
        }

        // 找到vecPackage中有的 而packages.list中没有的， 增加
        for (const auto &pkginfo : vecPackageInfo) {
            if (mPackageInfo.find(pkginfo.packageName) == mPackageInfo.end()) {
                mInstaller->addInfoToPackageList(pkginfo);
            }
        }
        mPackageInfo.clear();
    }
    mInstaller->loadPackageList(&mPackageInfo);

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

Status PackageManagerService::getAllPackageInfoEx(int32_t sliceSize,
                                                  SlicedPackageInfo *slicedInfo) {
    PM_PROFILER_BEGIN();
    std::vector<PackageInfo> allInfos;
    for (auto it = mPackageInfo.begin(); it != mPackageInfo.end(); it++) {
        if (it->second.bAllValid) {
            allInfos.push_back(it->second);
        } else {
            int ret = mParser->parseManifest(&it->second);
            if (!ret) {
                allInfos.push_back(it->second);
            }
        }
    }

    slicedInfo->totalSize = allInfos.size();

    // Create the provider
    android::sp<IPackageInfoProvider> provider = new PackageInfoProvider(allInfos);
    slicedInfo->provider = provider;

    // Prepare the first slice
    int32_t firstSliceSize = std::min(static_cast<int32_t>(allInfos.size()), sliceSize);
    for (int32_t i = 0; i < firstSliceSize; ++i) {
        slicedInfo->firstSlice.push_back(allInfos[i]);
    }

    ALOGD("getAllPackageInfoEx: total= %" PRId32 "firstSlice= %" PRId32, slicedInfo->totalSize,
          static_cast<int32_t>(slicedInfo->firstSlice.size()));

    PM_PROFILER_END();
    return Status::ok();
}

Status PackageManagerService::getPackagesInOperation(
        std::vector<PackageInOperation> *operationStatus) {
    PM_PROFILER_BEGIN();

    const auto &installingList = mInstaller->findInstallTask();
    for (const std::string &packageName : installingList) {
        PackageInOperation statusInfo;
        statusInfo.packageName = packageName;
        statusInfo.operationStatus = static_cast<int>(PackageOperationStatus::INSTALLING);
        operationStatus->push_back(statusInfo);
    }

    const auto &uninstallingList = mInstaller->findUninstallTask();
    for (const std::string &packageName : uninstallingList) {
        PackageInOperation statusInfo;
        statusInfo.packageName = packageName;
        statusInfo.operationStatus = static_cast<int>(PackageOperationStatus::UNINSTALLING);
        operationStatus->push_back(statusInfo);
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

    bool success = true;
    constexpr std::array<const char *, 3> kTypeList = {"cache", "files", "mass"};

    for (const auto &type : kTypeList) {
        std::string absolutePath;
#ifndef __NuttX__
        // 在非NuttX系统上，从当前工作目录开始构建路径
        // 路径格式: current_working_directory + ABS_PATH_PREFIX/type/packageName
        absolutePath = fs::current_path();
#endif
        // 在NuttX系统上，absolutePath初始为空，路径格式直接为: ABS_PATH_PREFIX/type/packageName
        absolutePath += std::string(ABS_PATH_PREFIX) + "/" + type + "/" + packageName;

        if (fs::exists(absolutePath)) {
            if (!removeDirectory(absolutePath.c_str())) {
                success = false;
                ALOGE("removeDirectory %s failed", absolutePath.c_str());
            }
        }
    }

    if (success) {
        *ret = 0;
    }
    PM_PROFILER_END();
    return Status::ok();
}

void PackageManagerService::handleAppInstallResult(const std::string &packageName,
                                                   const android::sp<IInstallObserver> &observer) {
    std::string dstPath = joinPath(PackageConfig::getInstance().getAppInstalledPath(), packageName);

    PackageInfo packageInfo;
    packageInfo.manifest = joinPath(dstPath, MANIFEST);
    int ret = mParser->parseManifest(&packageInfo);
    if (ret) {
        ALOGE("parse manifest:%s failed\n", packageInfo.manifest.c_str());
        observer->onInstallResult(packageInfo.packageName, ret, "Failed to parse manifest");
        return;
    }

    packageInfo.installedPath = dstPath;
    packageInfo.manifest = joinPath(dstPath, MANIFEST);
    if (mPackageInfo.find(packageInfo.packageName) != mPackageInfo.end()) {
        PackageInfo oldPackageInfo = mPackageInfo[packageInfo.packageName];
        packageInfo.userId = oldPackageInfo.userId;
        mPackageInfo.erase(packageInfo.packageName);
        mInstaller->deleteInfoFromPackageList(packageInfo.packageName);
        if (oldPackageInfo.installedPath != packageInfo.installedPath) {
            removeDirectory(oldPackageInfo.installedPath.c_str());
        }
    }
    mPackageInfo.insert(std::make_pair(packageInfo.packageName, packageInfo));
    mInstaller->addInfoToPackageList(packageInfo);
    observer->onInstallResult(packageInfo.packageName, 0, "success");
}

static std::string generateUniqueTmpName(const std::string &rpkName) {
    auto now = std::chrono::system_clock::now();
    auto timestamp =
            std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

    pthread_t thread_id = pthread_self();

    return rpkName + "_" + std::to_string(timestamp) + "_" +
            std::to_string(static_cast<uint64_t>(thread_id));
}

static int unzipAndParseManifest(const InstallParam &param, PackageInfo &info,
                                 PackageParser *parser) {
    if (!fs::exists(param.path.c_str())) {
        ALOGE("%s is not exist", param.path.c_str());
        return android::NAME_NOT_FOUND;
    }

    size_t pos = param.path.find_last_of('/');
    std::string rpkFullName = param.path;
    if (pos != std::string::npos) {
        rpkFullName = param.path.substr(pos + 1);
    }

    pos = rpkFullName.rfind('.');
    std::string rpkName = rpkFullName.substr(0, pos);
    std::string uniqueTmpName = generateUniqueTmpName(rpkName);
    std::string tmpBase = joinPath(PackageConfig::getInstance().getAppDataPath(), "tmp");
    std::string tmp = joinPath(tmpBase, uniqueTmpName);

    if (fs::exists(tmp.c_str())) {
        removeDirectory(tmp.c_str());
    }
    if (!createDirectory(tmp.c_str())) {
        return android::PERMISSION_DENIED;
    }

    auto *token = app_verify_init(param.path.c_str(), tmp.c_str());
    if (!token) {
        ALOGE("app_verify_init failed");
        removeDirectory(tmp.c_str());
        return android::NO_INIT;
    }

    ALOGI("app_pre_unzip manifest.json...");
    int error_code = app_pre_unzip(token, "manifest.json");
    if (error_code) {
        ALOGE("app_pre_unzip manifest.json failed");
        app_verify_close(token);
        removeDirectory(tmp.c_str());
        return android::NO_INIT;
    }

    info.manifest = joinPath(tmp, MANIFEST);
    int ret = parser->parseManifest(&info);
    if (ret) {
        removeDirectory(tmp.c_str());
        ALOGE("parse manifest:%s failed\n", info.manifest.c_str());
        app_verify_close(token);
        return ret;
    }

    app_verify_close(token);
    removeDirectory(tmp.c_str());
    return 0;
}

static bool hasEnoughDiskSpace(const char *diskPath, const char *packagePath) {
    struct statvfs diskStat;
    struct stat packageStat;
    if (statvfs(diskPath, &diskStat) != 0) {
        ALOGE("Failed to statvfs path: %s", diskPath);
        return false;
    }

    uint64_t available = diskStat.f_bsize * diskStat.f_bavail;
    if (stat(packagePath, &packageStat) != 0) {
        ALOGE("Failed to get file size for: %s", packagePath);
        return false;
    }

    uint64_t requiredSpace =
            static_cast<uint64_t>(packageStat.st_size) * 3; // 通常解压后的大小是压缩包的2-3倍

    return available >= requiredSpace;
}

Status PackageManagerService::installPackage(const InstallParam &param,
                                             const android::sp<IInstallObserver> &observer) {
    PM_PROFILER_BEGIN();
    ALOGD("installPackage:%s", param.toString().c_str());

    if (!fs::exists(param.path)) {
        observer->onInstallResult(param.path, android::NAME_NOT_FOUND,
                                  "Failed to install package, path does not exist");
        ALOGE("Install package path does not exist: %s", param.path.c_str());
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_ILLEGAL_STATE);
    }

    std::string diskPath = PackageConfig::getInstance().getAppInstalledPath();
    if (!hasEnoughDiskSpace(diskPath.c_str(), param.path.c_str())) { // 50KB

        observer->onInstallResult(param.path, android::NO_MEMORY,
                                  "Failed to install package, not enough disk space");
        ALOGE("Not enough disk space under %s", diskPath.c_str());
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_SECURITY);
    }

    PackageInfo info;
    int pre_unzip_result = unzipAndParseManifest(param, info, mParser);
    if (pre_unzip_result) {
        observer->onInstallResult(param.path, pre_unzip_result,
                                  "Failed to pre-unzip and parse manifest.json");
        ALOGE("pre-unzip and parse manifest.json failed");
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_ILLEGAL_STATE);
    }

    auto packageName = info.packageName;
    auto it = mPackageInfo.find(packageName);
    if (it != mPackageInfo.end()) {
        auto &oldQuickAppInfo = it->second.extra;
        auto &newQuickAppInfo = info.extra;
        if ((oldQuickAppInfo->versionCode > newQuickAppInfo->versionCode) &&
            (param.force == false)) {
            observer->onInstallResult(packageName, android::NOT_ENOUGH_DATA,
                                      "Failed to install package, version is too low");
            ALOGE("install package:%s failed, version is too low", packageName.c_str());
            PM_PROFILER_END();
            return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        }
    }

    auto resultHandler = [this](const std::string &tmpPath,
                                const android::sp<IInstallObserver> &installObserver) {
        this->handleAppInstallResult(tmpPath, installObserver);
    };

    int ret = mInstaller->installApp(mLooper.get(), param, observer, resultHandler, packageName);
    if (ret < 0) {
        std::string msg;
        if (ret == android::NAME_NOT_FOUND) {
            msg = "application package does not exist";
        } else if (ret == android::ALREADY_EXISTS) {
            msg = "installation task in progress, repeated submission error";
        } else {
            msg = "failed to deal with rpkpackage";
        }
        observer->onInstallResult(param.path, ret, msg);
        ALOGE("decompress %s failed", param.path.c_str());
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_ILLEGAL_STATE);
    }

    return Status::ok();
}

void PackageManagerService::handleAppUninstallResult(
        const std::string &packageName, const android::sp<IUninstallObserver> &observer) {
    mPackageInfo.erase(packageName);
    mInstaller->deleteInfoFromPackageList(packageName);

    if (observer) {
        observer->onUninstallResult(packageName, 0, "success");
    }
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

    auto resultHandler = [this](const std::string &packageName,
                                const android::sp<IUninstallObserver> &uninstallObserver) {
        this->handleAppUninstallResult(packageName, uninstallObserver);
    };
    int ret = mInstaller->uninstallApp(mLooper.get(), mPackageInfo[param.packageName].installedPath,
                                       param.packageName, observer, resultHandler);

    if (ret < 0) {
        std::string msg = "failed to remove package";
        if (ret == android::ALREADY_EXISTS) {
            msg = "uninstall task in progress, repeated submission error";
        }
        if (observer) {
            observer->onUninstallResult(param.packageName, ret, msg);
        }
        ALOGE("Delete Directory:%s Failed", mPackageInfo[param.packageName].installedPath.c_str());
        PM_PROFILER_END();
        return Status::fromExceptionCode(Status::EX_UNSUPPORTED_OPERATION);
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
