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

#pragma once

#include <binder/Parcel.h>
#include <binder/Status.h>
#include <os/pm/ActivityInfo.h>
#include <os/pm/QuickAppInfo.h>
#include <os/pm/ServiceInfo.h>

namespace os {
namespace pm {

enum ProcessPriority { LOW = 0, MIDDLE, HIGH, PERSISTENT };

enum ApplicationType { UNKNOWN = -1, NATIVE = 0, QUICKAPP };

ProcessPriority getProcessPriority(const std::string& str);
ApplicationType getApplicationType(const std::string& appType);

class PackageInfo : public ::android::Parcelable {
public:
    std::string packageName;
    std::string name;
    bool isSystemUI;
    std::string icon;
    std::string execfile;
    std::string entry;
    std::string installedPath;
    std::string manifest;
    std::string appType;
    std::string version;
    std::string shasum;
    std::string installTime;
    std::vector<ActivityInfo> activitiesInfo;
    std::vector<ServiceInfo> servicesInfo;
    std::optional<QuickAppInfo> extra;
    int32_t priority;
    int32_t userId;
    int64_t size;
    bool bAllValid;

    android::status_t readFromParcel(const android::Parcel* parcel) final;
    android::status_t writeToParcel(android::Parcel* parcel) const final;
    std::string toString() const;
    std::string dumpSimplePackageInfo();
}; // class PackageInfo
} // namespace pm
} // namespace os