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

#include <map>
#include <vector>

#include "InstallTask.h"
#include "UninstallTask.h"
#include "app/Logger.h"
#include "os/pm/IInstallObserver.h"
#include "os/pm/InstallParam.h"
#include "pm/PackageInfo.h"

namespace os {
namespace pm {

class PackageInstaller {
public:
    PackageInstaller();
    int installApp(uv_loop_t* looper, const InstallParam& param,
                   const android::sp<IInstallObserver>& observer,
                   InstallTask::InstallResultHandler resultHandler, const std::string& packageName);
    int uninstallApp(uv_loop_t* looper, const std::string& path, const std::string& packageName,
                     const android::sp<IUninstallObserver>& observer,
                     UninstallTask::UninstallResultHandler resultHandler);
    int32_t createUserId();
    int createPackageList();
    bool loadPackageList(std::map<std::string, PackageInfo>* pkgInfos);
    int addInfoToPackageList(const PackageInfo& installInfo);
    int addInfoToPackageList(const std::vector<PackageInfo>& vecExtraInfo);
    int deleteInfoFromPackageList(const std::string& packageName);
    void onInstallTaskCompleted(const std::string packageName);
    void onUninstallTaskCompleted(const std::string packageName);
    std::vector<std::string> findInstallTask();
    std::vector<std::string> findUninstallTask();

private:
    int installNativeApp(const InstallParam& param);
    int installQuickApp(uv_loop_t* looper, const InstallParam& param,
                        const android::sp<IInstallObserver>& observer,
                        InstallTask::InstallResultHandler resultHandler,
                        const std::string& packageName);
    std::string mPackgeListPath;
    std::map<std::string, std::unique_ptr<InstallTask>> mInstallTasks;
    std::map<std::string, std::unique_ptr<UninstallTask>> mUninstallTasks;
};
} // namespace pm
} // namespace os
