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

#include "pm/SlicedPackageInfo.h"

namespace os {
namespace pm {

using android::Parcel;
using android::status_t;

status_t SlicedPackageInfo::readFromParcel(const Parcel* parcel) {
    status_t status;

    // 读取firstSlice
    status = parcel->readParcelableVector(&firstSlice);
    if (status != android::OK) return status;

    // 读取provider
    provider = android::interface_cast<IPackageInfoProvider>(parcel->readStrongBinder());

    // 读取totalSize
    status = parcel->readInt32(&totalSize);

    return status;
}

status_t SlicedPackageInfo::writeToParcel(Parcel* parcel) const {
    status_t status;

    // 写入firstSlice
    status = parcel->writeParcelableVector(firstSlice);
    if (status != android::OK) return status;

    // 写入provider
    status = parcel->writeStrongBinder(provider != nullptr ? android::IInterface::asBinder(provider)
                                                           : nullptr);
    if (status != android::OK) return status;

    // 写入totalSize
    status = parcel->writeInt32(totalSize);

    return status;
}

} // namespace pm
} // namespace os