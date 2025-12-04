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

#include "os/pm/BnInstallObserver.h"
#include "os/pm/BnUninstallObserver.h"
#include "os/pm/IPackageInfoProvider.h"
#include "os/pm/IPackageManager.h"
#include "os/pm/PackageStats.h"

namespace os {
namespace pm {

using android::sp;

/**
 * @class PackageManager
 * @brief Provides functionality for managing packages in the system.
 */
class PackageManager {
public:
    /**
     * @brief Constructor for the PackageManager class.
     */
    PackageManager();
    /**
     * @brief Retrieves information about all installed packages.
     *
     * @param[out] pkgsInfo A pointer to a vector where the package information will be stored.
     * @return Returns 0 on success or a non-zero error code on failure.
     */
    int32_t getAllPackageInfo(std::vector<PackageInfo> *pkgsInfo);
    /**
     * @brief Retrieves information about all installed packages.
     *
     * @param[out] pkgsInfo A pointer to a vector where the package information will be stored.
     * @param[in] sliceSize The size of each slice to retrieve.
     * @return Returns 0 on success or a non-zero error code on failure.
     */
    int32_t getAllPackageInfoEx(std::vector<PackageInfo> *pkgsInfo, int32_t sliceSize);
    /**
     * @brief Retrieves information about packages that are currently undergoing operations
     *        such as installation, or uninstallation.
     *
     * @param[out] operationStatus A pointer to a vector where the operation status information will
     * be stored.
     * @return Returns 0 on success or a non-zero error code on failure.
     */
    int32_t getPackagesInOperation(std::vector<PackageInOperation> *operationStatus);
    /**
     * @brief Retrieves information about a specific package.
     *
     * @param[in] packageName The name of the package to fetch information for.
     * @param[out] info A pointer to a `PackageInfo` object where the information will be stored.
     * @return Returns 0 on success or a non-zero error code on failure.
     */
    int32_t getPackageInfo(const std::string &packageName, PackageInfo *info);
    /**
     * @brief Clears the cache of a specific application.
     *
     * @param[in] packageName The name of the package whose cache needs to be cleared.
     * @return Returns 0 on success or a non-zero error code on failure.
     */
    int32_t clearAppCache(const std::string &packageName);
    /**
     * @brief Installs a package.
     *
     * @param[in] param The parameters required for installing the package.
     * @param[in] listener An optional listener to monitor the installation progress (default is
     * nullptr).
     * @return Returns 0 on success or a non-zero error code on failure.
     */
    int32_t installPackage(const InstallParam &param, sp<BnInstallObserver> listener = nullptr);

    /**
     * @brief Uninstalls a package.
     *
     * @param[in] param The parameters required for uninstalling the package.
     * @param[in] listener An optional listener to monitor the uninstallation progress (default is
     * nullptr).
     * @return Returns 0 on success or a non-zero error code on failure.
     */
    int32_t uninstallPackage(const UninstallParam &param,
                             sp<BnUninstallObserver> listener = nullptr);
    /**
     * @brief Retrieves the size information for a specific package.
     *
     * @param[in] packageName The name of the package whose size information is to be retrieved.
     * @param[out] stats A pointer to a `PackageStats` object where the size information will be
     * stored.
     * @return int32_t Returns 0 on success or a non-zero error code on failure.
     */
    int32_t getPackageSizeInfo(const std::string &packageName, PackageStats *stats);
    /**
     * @brief Checks if this is the first boot of the system.
     *
     * @param[out] firstBoot A pointer to a boolean that will hold the result (true if it's the
     * first boot, false otherwise).
     * @return Returns 0 on success or a non-zero error code on failure.
     */
    int32_t isFirstBoot(bool *firstBoot);
    /**
     * @brief Retrieves the names of all installed packages.
     *
     * @param[out] pkgNames A pointer to a vector where the package names will be stored.
     * @return Returns 0 on success or a non-zero error code on failure.
     */
    int32_t getAllPackageName(std::vector<std::string> *pkgNames);

private:
    sp<IPackageManager>
            mService; /**< The service interface to interact with the package manager. */
};

} // namespace pm
} // namespace os
