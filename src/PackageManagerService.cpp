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
#include "PackageUtils.h"

namespace os {
namespace pm {

using std::filesystem::directory_iterator;
using std::filesystem::exists;

class PackageConfig {
public:
    void loadConfig() {
        rapidjson::Document doc;
        getDocument(PACKAGE_CFG, doc);
        mPresetAppPath = getValue<std::string>(doc, "presetAppPath", "/system/app");
        mInstalledPath = getValue<std::string>(doc, "installedPath", "/data/app");
        mAppDataPath = getValue<std::string>(doc, "appDataPath", "/data/data");
    }

    std::string mPresetAppPath;
    std::string mInstalledPath;
    std::string mAppDataPath;
};

PackageManagerService::PackageManagerService() {
    mConfig = new PackageConfig();
    mInstaller = new PackageInstaller();
    mParser = new PackageParser();
    init();
}

PackageManagerService::~PackageManagerService() {
    if (mConfig) {
        delete mConfig;
        mConfig = nullptr;
    }
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
    mConfig->loadConfig();
    // create and scan manifest
    if (!exists(PACKAGE_LIST_PATH)) {
        mInstaller->createPackageList();
        std::vector<PackageInfo> vecPackageInfo;
        for (const auto &entry : directory_iterator(mConfig->mPresetAppPath.c_str())) {
            if (entry.is_directory()) {
                PackageInfo pkgInfo;
                pkgInfo.manifest = joinPath(entry.path().string(), MANIFEST);
                int ret = mParser->parseManifest(&pkgInfo);
                if (!ret) {
                    pkgInfo.userId = mInstaller->createUserId();
                    mPackageInfo.insert(std::make_pair(pkgInfo.packageName, pkgInfo));
                    vecPackageInfo.push_back(pkgInfo);
                }
            }
        }
        mInstaller->addInfoToPackageList(vecPackageInfo);
    } else {
        mInstaller->loadPackageList(&mPackageInfo);
    }
}

Status PackageManagerService::getAllPackageInfo(std::vector<PackageInfo> *pkgInfos) {
    for (auto it = mPackageInfo.begin(); it != mPackageInfo.end(); it++) {
        if (it->second.bAllValid) {
            pkgInfos->push_back(it->second);
        } else {
            int ret = mParser->parseManifest(&it->second);
            if (!ret) {
                pkgInfos->push_back(it->second);
            }
        }
    }
    return Status::ok();
}

Status PackageManagerService::getPackageInfo(const std::string &packageName, PackageInfo *pkgInfo) {
    if (mPackageInfo.find(packageName) == mPackageInfo.end()) {
        return Status::fromExceptionCode(Status::EX_SERVICE_SPECIFIC);
    }

    if (!mPackageInfo[packageName].bAllValid) {
        int ret = mParser->parseManifest(&mPackageInfo[packageName]);
        if (ret) {
            return Status::fromExceptionCode(Status::EX_ILLEGAL_ARGUMENT);
        }
    }

    *pkgInfo = mPackageInfo[packageName];
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
