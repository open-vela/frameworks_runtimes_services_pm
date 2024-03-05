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

#include "PackageInstaller.h"

#include <utils/Errors.h>
#include <utils/Log.h>
#include <uv_ext.h>

#include <filesystem>

#include "PackageUtils.h"

namespace os {
namespace pm {

using std::filesystem::copy;
using std::filesystem::exists;
using std::filesystem::temp_directory_path;

PackageInstaller::PackageInstaller() {
    mPackgeListPath = PackageConfig::getInstance().getPackageListPath();
}

int32_t PackageInstaller::createUserId() {
    // TODO
    return 0;
}

int PackageInstaller::installApp(const InstallParam &param) {
    size_t pos = param.path.find_last_of('.');
    std::string suffix;
    if (pos != std::string::npos) {
        suffix = param.path.substr(pos + 1);
        if (suffix == "rpk" || suffix == "apk") {
            return installQuickApp(param);
        }
    }
    return installNativeApp(param);
}

int PackageInstaller::installNativeApp(const InstallParam &param) {
    // TODO
    return android::INVALID_OPERATION;
}

int PackageInstaller::installQuickApp(const InstallParam &param) {
    size_t pos = param.path.find_last_of('/');
    std::string rpkFullName = param.path;
    if (pos != std::string::npos) {
        rpkFullName = param.path.substr(pos + 1);
    }
    pos = rpkFullName.rfind('.');
    std::string rpkName = rpkFullName.substr(0, pos);

    if (!exists(param.path.c_str())) {
        ALOGE("%s is not exist", param.path.c_str());
        return android::NAME_NOT_FOUND;
    }

    std::string tmp = joinPath(PackageConfig::getInstance().getAppDataPath(), "tmp");
    tmp = joinPath(tmp, rpkName);
    if (exists(tmp.c_str())) {
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

    int ret = app_verify_unzip(token);
    if (ret) {
        app_verify_close(token);
        removeDirectory(tmp.c_str());
        return android::NO_INIT;
    }

    app_verify_close(token);
    return 0;
}

bool PackageInstaller::loadPackageList(std::map<std::string, PackageInfo> *pkgInfos) {
    if (pkgInfos == nullptr) {
        return false;
    }

    rapidjson::Document document;
    int ret = getDocument(mPackgeListPath.c_str(), document);
    if (ret) return false;

    bool isNormal = true;
    const rapidjson::Value baseArray = rapidjson::Value(rapidjson::kArrayType);
    const rapidjson::Value &packagesArray =
            getValue<const rapidjson::Value &>(document, "packages", baseArray);
    for (unsigned int i = 0; i < packagesArray.Size(); i++) {
        PackageInfo info;
        info.packageName = getValue<std::string>(packagesArray[i], "package", "");
        if (info.packageName.empty()) {
            ALOGE("packages.list has package field is empty");
            isNormal = false;
            continue;
        }
        info.appType = getValue<std::string>(packagesArray[i], "appType", "");
        info.version = getValue<std::string>(packagesArray[i], "version", "");
        info.installedPath = getValue<std::string>(packagesArray[i], "installedPath", "");
        info.manifest = joinPath(info.installedPath, MANIFEST);
        info.installTime = getValue<std::string>(packagesArray[i], "installedTime", "");
        info.shasum = getValue<std::string>(packagesArray[i], "shasum", "");
        info.userId = getValue<int>(packagesArray[i], "uid", 0);
        info.size = getValue<int64_t>(packagesArray[i], "size", 0);
        info.bAllValid = false;
        pkgInfos->insert(std::make_pair(info.packageName, info));
    }
    return isNormal;
}

int PackageInstaller::createPackageList() {
    // create file and write
    rapidjson::Document doc;
    rapidjson::Document::AllocatorType &allocator = doc.GetAllocator();
    doc.SetObject();
    doc.AddMember("version", 1, allocator);
    rapidjson::Value packages(rapidjson::kArrayType);
    doc.AddMember("packages", packages, allocator);
    return writeFile(mPackgeListPath.c_str(), toPrettyString(doc));
}

int PackageInstaller::addInfoToPackageList(const PackageInfo &installInfo) {
    const std::vector<PackageInfo> vecExtraInfo = {installInfo};
    return addInfoToPackageList(vecExtraInfo);
}

int PackageInstaller::addInfoToPackageList(const std::vector<PackageInfo> &vecPackageInfo) {
    rapidjson::Document document;
    int ret = getDocument(mPackgeListPath.c_str(), document);
    if (ret) return ret;
    const rapidjson::Value baseArray = rapidjson::Value(rapidjson::kArrayType);
    const rapidjson::Value &cpackagesArray =
            getValue<const rapidjson::Value &>(document, "packages", baseArray);
    rapidjson::Value &packagesArray = const_cast<rapidjson::Value &>(cpackagesArray);
    rapidjson::Document::AllocatorType &allocator = document.GetAllocator();
    for (auto &packageInfo : vecPackageInfo) {
        rapidjson::Value info(rapidjson::kObjectType);
        rapidjson::Value strval(rapidjson::kStringType);
        info.AddMember("package",
                       strval.SetString(packageInfo.packageName.c_str(),
                                        packageInfo.packageName.length(), allocator),
                       allocator);
        info.AddMember("appType",
                       strval.SetString(packageInfo.appType.c_str(), packageInfo.appType.length(),
                                        allocator),
                       allocator);
        info.AddMember("uid", static_cast<int>(packageInfo.userId), allocator);
        info.AddMember("installedTime",
                       strval.SetString(packageInfo.installTime.c_str(),
                                        packageInfo.installTime.length(), allocator),
                       allocator);
        info.AddMember("installedPath",
                       strval.SetString(packageInfo.installedPath.c_str(),
                                        packageInfo.installedPath.length(), allocator),
                       allocator);
        info.AddMember("size", packageInfo.size, allocator);
        info.AddMember("shasum",
                       strval.SetString(packageInfo.shasum.c_str(), packageInfo.shasum.length(),
                                        allocator),
                       allocator);
        info.AddMember("version",
                       strval.SetString(packageInfo.version.c_str(), packageInfo.version.length(),
                                        allocator),
                       allocator);
        packagesArray.PushBack(info, allocator);
    }
    return writeFile(mPackgeListPath.c_str(), toPrettyString(document));
}

int PackageInstaller::deleteInfoFromPackageList(const std::string &packageName) {
    rapidjson::Document document;
    int ret = getDocument(mPackgeListPath.c_str(), document);
    if (ret) return ret;
    const rapidjson::Value baseArray = rapidjson::Value(rapidjson::kArrayType);
    const rapidjson::Value &cpackagesArray =
            getValue<const rapidjson::Value &>(document, "packages", baseArray);
    rapidjson::Value &packagesArray = const_cast<rapidjson::Value &>(cpackagesArray);
    for (auto it = packagesArray.Begin(); it != packagesArray.End(); ++it) {
        std::string pkgName = getValue<std::string>(*it, "package", "");
        if (packageName == pkgName) {
            it = packagesArray.Erase(it);
            break;
        }
    }
    return writeFile(mPackgeListPath.c_str(), toPrettyString(document));
}

} // namespace pm
} // namespace os