/*
 * Copyright (C) 2025 Xiaomi Corporation
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

#include "os/pm/IPackageInfoProvider.h"
#include "pm/PackageInfo.h"

namespace os {
namespace pm {

/**
 * @class SlicedPackageInfo
 * @brief Represents a slice of package information, used for paginated transfer of large package
 * lists.
 *
 * This Parcelable class is designed to carry a partial list of PackageInfo objects along with a
 * Binder interface that allows the client to retrieve subsequent slices of data from the service.
 * This mechanism helps to overcome Binder transaction size limits when dealing with a large number
 * of packages.
 */
class SlicedPackageInfo : public android::Parcelable {
public:
    /**
     * @brief Constructor for the SlicedPackageInfo class.
     */
    SlicedPackageInfo() = default;
    /**
     * @brief Destructor for the SlicedPackageInfo class.
     */
    ~SlicedPackageInfo() override = default;
    /**
     * @brief Reads the package information from a parcel.
     *
     * This method is used to deserialize the `SlicedPackageInfo` object from the provided `Parcel`.
     *
     * @param[in] parcel The parcel from which the package information will be read.
     * @return The status of the read operation.
     */
    android::status_t readFromParcel(const android::Parcel* parcel) override;
    /**
     * @brief Writes the package information to a parcel.
     *
     * This method is used to serialize the `SlicedPackageInfo` object to the provided `Parcel`.
     *
     * @param[in] parcel The parcel to which the package information will be written.
     * @return The status of the write operation.
     */
    android::status_t writeToParcel(android::Parcel* parcel) const override;

    std::vector<PackageInfo> firstSlice; /**< The first slice of package information. */
    android::sp<IPackageInfoProvider>
            provider;     /**< The Binder interface for retrieving subsequent slices. */
    int32_t totalSize{0}; /**< The total number of PackageInfo objects available. */
};

} // namespace pm
} // namespace os
