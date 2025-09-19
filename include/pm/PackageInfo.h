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

/**
 * @enum PackageOperationStatus
 * @brief Represents the status of package installation or uninstallation operations.
 */
enum class PackageOperationStatus {
    UNKNOWN = 0,  /**< Unknown operation status */
    INSTALLING,   /**< Package is installing */
    UNINSTALLING, /**< Package is uninstalling */
};

/**
 * @enum ProcessPriority
 * @brief Represents the priority levels for a process.
 */
enum ProcessPriority {
    LOW = 0,   /**< Low priority for a process */
    MIDDLE,    /**< Medium priority for a process */
    HIGH,      /**< High priority for a process */
    PERSISTENT /**< Persistent priority for a process, indicating high importance */
};

/**
 * @enum ApplicationType
 * @brief Represents the types of applications.
 */
enum ApplicationType {
    UNKNOWN = -1, /**< Unknown application type */
    NATIVE = 0,   /**< Native application type */
    QUICKAPP      /**< Quick application type */
};

/**
 * @brief Converts a string to a corresponding ProcessPriority value.
 *
 * @param[in] str The string representation of the process priority.
 * @return The corresponding enum value of the process priority. @see ProcessPriority
 */
ProcessPriority getProcessPriority(const std::string& str);
/**
 * @brief Converts a string to a corresponding ApplicationType value.
 *
 * @param[in] appType The string representation of the application type.
 * @return The corresponding enum value of the application type. @see ApplicationType
 */
ApplicationType getApplicationType(const std::string& appType);

/**
 * @class PackageInfo
 * @brief Represents the information related to a package installed on the system.
 */
class PackageInfo : public ::android::Parcelable {
public:
    std::string packageName; /**< The name of the package. */
    std::string name;        /**< The human-readable name of the package. */
    bool isSystemUI;         /**< Flag indicating whether the package is part of the system UI. */
    std::string
            icon; /**< The icon associated with the package, typically the application's icon. */
    std::string execfile;      /**< The path to the executable file for the package. */
    std::string entry;         /**< The entry point for the application. */
    std::string installedPath; /**< The installation path of the package. */
    std::string manifest;      /**< The manifest file associated with the package. */
    std::string appType;       /**< The type of the application */
    std::string version;       /**< The version of the package. */
    std::string shasum;        /**< The SHA checksum of the package to verify its integrity. */
    std::string installTime;   /**< The installation time of the package, typically in a timestamp
                                  format. */
    std::vector<ActivityInfo> activitiesInfo; /**< A list of `ActivityInfo` objects representing the
                                                 activities within the package. */
    std::vector<ServiceInfo> servicesInfo;    /**< A list of `ServiceInfo` objects representing the
                                                 services within the package. */
    std::optional<QuickAppInfo>
            extra;    /**< Optional additional information related to the quick app (if any). */
    int32_t priority; /**< he priority level of the package. */
    int32_t userId;   /**< The user ID associated with the package. */
    int64_t size;     /**< The size of the package in bytes. */
    bool bAllValid;   /**< Flag indicating whether all package data is valid. */
    std::string windowEnterAnim; /**< The animation used when a window enters the screen. */
    std::string windowExitAnim;  /**< The animation used when a window exits the screen. */
    /**
     * @brief Reads the package information from a parcel.
     *
     * This method is used to deserialize the `PackageInfo` object from the provided `Parcel`.
     *
     * @param[in] parcel The parcel from which the package information will be read.
     * @return The status of the read operation.
     */
    android::status_t readFromParcel(const android::Parcel* parcel) final;
    /**
     * @brief Writes the package information to a parcel.
     *
     * This method is used to serialize the `PackageInfo` object to the provided `Parcel`.
     *
     * @param[in] parcel The parcel to which the package information will be written.
     * @return The status of the write operation.
     */
    android::status_t writeToParcel(android::Parcel* parcel) const final;
    /**
     * @brief Converts the `PackageInfo` object to a string representation.
     *
     * @return A string representation of the `PackageInfo`.
     */
    std::string toString() const;
    /**
     * @brief Dumps a simplified package information string.
     *
     * @return A simplified string representation of the package info.
     */
    std::string dumpSimplePackageInfo();
}; // class PackageInfo
} // namespace pm
} // namespace os