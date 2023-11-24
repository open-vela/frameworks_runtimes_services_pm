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

#include "pm/PackageInfo.h"

#include "ParcelUtils.h"

namespace os {
namespace pm {

using namespace android;

ApplicationType getApplicationType(const std::string &appType) {
    size_t pos = appType.find_first_of('/');
    std::string type = appType.substr(0, pos);
    if (type == "NATIVE") {
        return ApplicationType::NATIVE;
    } else if (type == "QUICKAPP") {
        return ApplicationType::QUICKAPP;
    }
    return ApplicationType::UNKNOWN;
}

ServicePriority getServicePriority(const std::string &str) {
    if (str == "low") {
        return ServicePriority::LOW;
    } else if (str == "middle") {
        return ServicePriority::MIDDLE;
    } else if (str == "high") {
        return ServicePriority::HIGH;
    } else {
        return ServicePriority::PERSISTENT;
    }
}

android::status_t PackageInfo::readFromParcel(const android::Parcel *parcel) {
    SAFE_PARCEL(parcel->readUtf8FromUtf16, &packageName);
    SAFE_PARCEL(parcel->readUtf8FromUtf16, &name);
    SAFE_PARCEL(parcel->readUtf8FromUtf16, &icon);
    SAFE_PARCEL(parcel->readUtf8FromUtf16, &execfile);
    SAFE_PARCEL(parcel->readUtf8FromUtf16, &entry);
    SAFE_PARCEL(parcel->readUtf8FromUtf16, &installedPath);
    SAFE_PARCEL(parcel->readUtf8FromUtf16, &manifest);
    SAFE_PARCEL(parcel->readUtf8FromUtf16, &appType);
    SAFE_PARCEL(parcel->readUtf8FromUtf16, &version);
    SAFE_PARCEL(parcel->readUtf8FromUtf16, &shasum);
    SAFE_PARCEL(parcel->readUtf8FromUtf16, &installTime);
    SAFE_PARCEL(parcel->readParcelableVector, &activitiesInfo);
    SAFE_PARCEL(parcel->readParcelableVector, &servicesInfo);
    SAFE_PARCEL(parcel->readParcelable, &extra);
    SAFE_PARCEL(parcel->readInt32, &userId);
    SAFE_PARCEL(parcel->readInt64, &size);
    SAFE_PARCEL(parcel->readBool, &bAllValid);
    return android::OK;
}

android::status_t PackageInfo::writeToParcel(android::Parcel *parcel) const {
    SAFE_PARCEL(parcel->writeUtf8AsUtf16, packageName);
    SAFE_PARCEL(parcel->writeUtf8AsUtf16, name);
    SAFE_PARCEL(parcel->writeUtf8AsUtf16, icon);
    SAFE_PARCEL(parcel->writeUtf8AsUtf16, execfile);
    SAFE_PARCEL(parcel->writeUtf8AsUtf16, entry);
    SAFE_PARCEL(parcel->writeUtf8AsUtf16, installedPath);
    SAFE_PARCEL(parcel->writeUtf8AsUtf16, manifest);
    SAFE_PARCEL(parcel->writeUtf8AsUtf16, appType);
    SAFE_PARCEL(parcel->writeUtf8AsUtf16, version);
    SAFE_PARCEL(parcel->writeUtf8AsUtf16, shasum);
    SAFE_PARCEL(parcel->writeUtf8AsUtf16, installTime);
    SAFE_PARCEL(parcel->writeParcelableVector, activitiesInfo);
    SAFE_PARCEL(parcel->writeParcelableVector, servicesInfo);
    SAFE_PARCEL(parcel->writeNullableParcelable, extra);
    SAFE_PARCEL(parcel->writeInt32, userId);
    SAFE_PARCEL(parcel->writeInt64, size);
    SAFE_PARCEL(parcel->writeBool, bAllValid);
    return android::OK;
}

std::string PackageInfo::toString() const {
    std::ostringstream os;
    os << "PackageInfo{";
    os << "packageName: " << ::android::internal::ToString(packageName);
    os << ", name: " << ::android::internal::ToString(name);
    os << ", icon: " << ::android::internal::ToString(icon);
    os << ", execfile: " << ::android::internal::ToString(execfile);
    os << ", entry: " << ::android::internal::ToString(entry);
    os << ", installedPath: " << ::android::internal::ToString(installedPath);
    os << ", manifest: " << ::android::internal::ToString(manifest);
    os << ", appType: " << ::android::internal::ToString(appType);
    os << ", version: " << ::android::internal::ToString(version);
    os << ", shasum: " << ::android::internal::ToString(shasum);
    os << ", installTime: " << ::android::internal::ToString(installTime);
    os << ", activitiesInfo: " << ::android::internal::ToString(activitiesInfo);
    os << ", servicesInfo: " << ::android::internal::ToString(servicesInfo);
    os << ", extra: " << ::android::internal::ToString(extra);
    os << ", userId: " << ::android::internal::ToString(userId);
    os << ", size: " << ::android::internal::ToString(size);
    os << ", bAllValid: " << ::android::internal::ToString(bAllValid);
    os << "}";
    return os.str();
}

std::string PackageInfo::dumpSimplePackageInfo() {
    std::ostringstream oss;
    oss << "{" << std::endl;
    oss << "\033[33m"
        << "  package: " << packageName << "\033[0m" << std::endl;
    oss << "  name: " << name << std::endl;
    oss << "  appType: " << appType << std::endl;
    oss << "  bAllValid: " << (bAllValid ? "true" : "false") << std::endl;
    oss << "  installPath: " << installedPath << std::endl;
    oss << "  manifest: " << manifest << std::endl;
    oss << "  versionName: " << version << std::endl;
    oss << "  icon: " << icon << std::endl;
    oss << "  execfile: " << execfile << std::endl;
    oss << "  entry: " << entry << std::endl;
    oss << "  installTime: " << installTime << std::endl;
    oss << "  shasum: " << shasum << std::endl;
    oss << "  userId: " << userId << std::endl;
    oss << "  size: " << size << std::endl;
    oss << "  activities:[" << std::endl;
    for (auto &activity : activitiesInfo) {
        oss << "     {" << std::endl;
        oss << "\tname: " << activity.name << std::endl;
        oss << "\tlaunchMode: " << activity.launchMode << std::endl;
        oss << "\ttaskAffinity: " << activity.taskAffinity << std::endl;
        oss << "\tactions:[" << std::endl;
        for (auto &action : activity.actions) {
            oss << "\t  " << action << std::endl;
        }
        oss << "\t]" << std::endl;
        oss << "     }" << std::endl;
    }
    oss << "  ]" << std::endl;
    oss << "  services:[" << std::endl;
    for (auto &service : servicesInfo) {
        oss << "     {" << std::endl;
        oss << "\tname: " << service.name << std::endl;
        oss << "\tpath: " << service.path << std::endl;
        oss << "\texported:" << service.exported << std::endl;
        oss << "\tpriority:" << service.priority << std::endl;
        oss << "\tactions:[" << std::endl;
        for (auto &action : service.actions) {
            oss << "\t  " << action << std::endl;
        }
        oss << "\t]" << std::endl;
        oss << "     }" << std::endl;
    }
    oss << "  ]" << std::endl;
    oss << "}";
    return oss.str();
}
} // namespace pm
} // namespace os
