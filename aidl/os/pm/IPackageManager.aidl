/*
 * Copyright (C) 2023 The Android Open Source Project
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

package os.pm;

import os.pm.PackageInfo;
import os.pm.InstallParam;
import os.pm.IInstallObserver;
import os.pm.UninstallParam;
import os.pm.IUninstallObserver;
import os.pm.PackageStats;

interface IPackageManager {
    PackageInfo[] getAllPackageInfo();
    PackageInfo getPackageInfo(@utf8InCpp String packageName);
    int clearAppCache(@utf8InCpp String packageName);
    oneway void installPackage(in InstallParam param, IInstallObserver observer);
    oneway void uninstallPackage(in UninstallParam param, IUninstallObserver observer);
    PackageStats getPackageSizeInfo(@utf8InCpp String packageName);
    boolean isFirstBoot();
    @utf8InCpp String[] getAllPackageName();
}
