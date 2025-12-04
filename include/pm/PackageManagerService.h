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

#include <utils/String16.h>

#include "app/UvLoop.h"
#include "os/pm/BnPackageManager.h"
#include "os/pm/IPackageManager.h"
#include "os/pm/InstallParam.h"
#include "os/pm/PackageStats.h"
#include "os/pm/UninstallParam.h"

namespace os {
namespace pm {

using android::binder::Status;

class PackageInstaller;
class PackageParser;

class SlicedPackageInfo;

/**
 * @class PackageManagerService
 * @brief This class provides the implementation for managing packages in the system.
 */
class PackageManagerService : public BnPackageManager {
public:
    /**
     * @brief Constructor for the `PackageManagerService` class.
     */
    PackageManagerService(uv_loop_t *looper);
    /**
     * @brief Destructor for the `PackageManagerService` class.
     */
    ~PackageManagerService();
    /**
     * @brief Retrieves information about all installed packages.
     *
     * @param[out] pkgInfos A vector to store the information of all installed packages.
     * @return Returns the status of the operation. If successful, it will return `Status::ok()`.
     */
    Status getAllPackageInfo(std::vector<PackageInfo> *pkgInfos);
    /**
     * @brief Retrieves information about all installed packages using a slicing mechanism.
     *
     * @param[in] sliceSize The size of each slice to retrieve.
     * @param[out] slicedInfo A pointer to a SlicedPackageInfo object to store the first slice and
     * provider.
     * @return Returns the status of the operation. If successful, it will return `Status::ok()`.
     */
    Status getAllPackageInfoEx(int32_t sliceSize, SlicedPackageInfo *slicedInfo);
    /**
     * @brief Retrieves information about packages that are currently in an ongoing operation
     *        (e.g., installing, or uninstalling).
     *
     * @param[out] operationStatus A vector of packages currently being operated on
     * @return Returns the status of the operation. If successful, it will return `Status::ok()`.
     */
    Status getPackagesInOperation(std::vector<PackageInOperation> *operationStatus);
    /**
     * @brief Retrieves information about a specific package.
     *
     * @param[in] packageName The name of the package to retrieve information for.
     * @param[out] pkgInfo A pointer to a `PackageInfo` object where the package information will be
     * stored.
     * @return Returns the status of the operation. If successful, it will return `Status::ok()`.
     */
    Status getPackageInfo(const std::string &packageName, PackageInfo *pkgInfo);
    /**
     * @brief Clears the cache of a specific application.
     *
     * @param[in] packageName The name of the package whose cache should be cleared.
     * @param[out] ret A pointer to an integer where the result of the cache clearing operation will
     * be stored.
     * @return Returns the status of the operation. If successful, it will return `Status::ok()`.
     */
    Status clearAppCache(const std::string &packageName, int32_t *ret);
    /**
     * @brief Installs a package.
     *
     * @param[in] param The parameters required for installing the package.
     * @param[in] observer An observer that will receive installation progress and completion
     * updates.
     * @return Returns the status of the operation. If successful, it will return `Status::ok()`.
     */
    Status installPackage(const InstallParam &param, const android::sp<IInstallObserver> &observer);

    /**
     * @brief Uninstalls a package.
     *
     * @param[in] param The parameters required for uninstalling the package.
     * @param[in] observer An observer that will receive uninstallation progress and completion
     * updates.
     * @return Returns the status of the operation. If successful, it will return `Status::ok()`.
     */
    Status uninstallPackage(const UninstallParam &param,
                            const android::sp<IUninstallObserver> &observer);

    /**
     * @brief Retrieves the size information of a specific package.
     *
     * @param[in] packageName The name of the package to retrieve size information for.
     * @param[out] pkgStats A pointer to a `PackageStats` object where the size information will be
     * stored.
     * @return Returns the status of the operation. If successful, it will return `Status::ok()`.
     */
    Status getPackageSizeInfo(const std::string &packageName, PackageStats *pkgStats);
    /**
     * @brief Determines if the system is being started for the first time after installation.
     *
     * @param[out] firstBoot A pointer to a boolean flag that will be set to true if it is the first
     * boot, or false otherwise.
     * @return Returns the status of the operation. If successful, it will return `Status::ok()`.
     */
    Status isFirstBoot(bool *firstBoot);
    /**
     * @brief Retrieves the names of all installed packages.
     *
     * @param[out] pkgNames A vector to store the names of all installed packages.
     * @return Returns the status of the operation. If successful, it will return `Status::ok()`.
     */
    Status getAllPackageName(std::vector<std::string> *pkgNames);
    /**
     * @brief Returns the name of the service.
     *
     * @return A `String16` representing the service name.
     */
    static android::String16 name() {
        return android::String16("package");
    }

    void handleAppInstallResult(const std::string &packageName,
                                const android::sp<IInstallObserver> &observer);

    void handleAppUninstallResult(const std::string &packageName,
                                  const android::sp<IUninstallObserver> &observer);

private:
    /**
     * @brief Initializes the internal state of the `PackageManagerService`.
     */
    void init();
    bool mFirstBoot; /**< Flag indicating whether it is the first boot of the system. */
    std::map<std::string, PackageInfo>
            mPackageInfo;         /**< Map of installed packages and their information. */
    PackageInstaller *mInstaller; /**< The package installer used for installing packages. */
    PackageParser *mParser;       /**< The package parser used for parsing package data. */
    os::app::UvLoop mLooper;      /**< The event loop used for running the service. */
};

} // namespace pm
} // namespace os
