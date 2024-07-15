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

#include "PmCommand.h"

#include <binder/ProcessState.h>
#include <inttypes.h>

#include <cstdio>
#include <future>

namespace os {
namespace pm {

using android::binder::Status;

class InstallListener : public BnInstallObserver, public std::promise<int32_t> {
public:
    Status onInstallProcess(const std::string &packageName, int32_t process) override {
        printf("onInstallProcess %" PRIi32 "\n", process);
        return Status::ok();
    }

    Status onInstallResult(const std::string &packageName, int32_t code,
                           const std::string &msg) override {
        printf("onInstallResult: %s(%s %" PRIi32 ")\n", packageName.c_str(), msg.c_str(), code);
        this->set_value(code);
        return Status::ok();
    }
};

class UninstallListener : public BnUninstallObserver, public std::promise<int32_t> {
public:
    Status onUninstallResult(const std::string &packageName, int32_t code,
                             const std::string &msg) override {
        printf("onUninstallResult: %s(%s %" PRIi32 ")\n", packageName.c_str(), msg.c_str(), code);
        this->set_value(code);
        return Status::ok();
    }
};

PmCommand::PmCommand() : mNextArg(0) {}

PmCommand::~PmCommand() {}

int PmCommand::runInstall() {
    std::string_view path = nextArg();
    if (path.empty()) {
        return showUsage();
    }
    InstallParam installparam;
    installparam.path = path;
    sp<InstallListener> listener = sp<InstallListener>::make();
    int status = pm.installPackage(installparam, listener);
    if (status) {
        return status;
    }
    std::future<int32_t> f = listener->get_future();
    int result = f.get();
    return result;
}

int PmCommand::runUninstall() {
    UninstallParam uninstallparam;
    std::string_view arg = nextArg();
    std::string_view packageName;
    if (arg == "-f") {
        uninstallparam.clearCache = true;
        packageName = nextArg();
    } else {
        packageName = arg;
    }

    if (packageName.empty()) {
        return showUsage();
    }
    uninstallparam.packageName = packageName;

    sp<UninstallListener> listener = sp<UninstallListener>::make();
    int status = pm.uninstallPackage(uninstallparam, listener);
    if (status) {
        return status;
    }
    std::future<int32_t> f = listener->get_future();
    int result = f.get();
    return result;
}

int PmCommand::runList() {
    std::string_view arg = nextArg();
    int status = 0;
    if (arg == "-l") {
        std::vector<std::string> pkgNames;
        status = pm.getAllPackageName(&pkgNames);
        for (size_t i = 0; i < pkgNames.size(); i++) {
            printf("%s\n", pkgNames[i].c_str());
        }
    } else {
        std::vector<PackageInfo> pkgInfos;
        status = pm.getAllPackageInfo(&pkgInfos);
        for (size_t i = 0; i < pkgInfos.size(); i++) {
            if (arg == "-a") {
                printf("%s\n", pkgInfos[i].toString().c_str());
            } else {
                printf("%s\n", pkgInfos[i].dumpSimplePackageInfo().c_str());
            }
        }
    }
    return status;
}

int PmCommand::runClear() {
    std::string_view packageName = nextArg();
    if (packageName.empty()) {
        return showUsage();
    }
    int status = pm.clearAppCache(std::string(packageName));
    printf("clearAppCache %.*s(%d)\n", static_cast<int>(packageName.length()), packageName.data(),
           status);
    return status;
}

int PmCommand::runGetPackage() {
    std::string_view packageName = nextArg();
    if (packageName.empty()) {
        return showUsage();
    }
    PackageInfo pkginfo;
    int status = pm.getPackageInfo(std::string(packageName), &pkginfo);
    if (!status) {
        printf("%s\n", pkginfo.dumpSimplePackageInfo().c_str());
    } else {
        printf("get %.*s failed\n", static_cast<int>(packageName.length()), packageName.data());
    }
    return status;
}

int PmCommand::runPackageStats() {
    std::string_view packageName = nextArg();
    if (packageName.empty()) {
        return showUsage();
    }
    PackageStats pkgStats;
    int status = pm.getPackageSizeInfo(std::string(packageName), &pkgStats);
    if (!status) {
        auto print = [](int64_t size, const char *name) {
            if (size < 1024) {
                printf("%s:%" PRId64 "B\n", name, size);
            } else if (size < 1024 * 1024) {
                printf("%s:%.1fKB\n", name, size / 1024.0);
            } else {
                printf("%s:%.1fMB\n", name, size / (1024.0 * 1024.0));
            }
        };
        print(pkgStats.codeSize + pkgStats.dataSize, "totalSize");
        print(pkgStats.codeSize, "codeSize");
        print(pkgStats.dataSize - pkgStats.cacheSize, "dataSize");
        print(pkgStats.cacheSize, "cacheSize");
    } else {
        printf("get %.*s failed\n", static_cast<int>(packageName.length()), packageName.data());
    }
    return status;
}

int PmCommand::runFirstBoot() {
    bool isFirstBoot;
    int status = pm.isFirstBoot(&isFirstBoot);
    if (!status) {
        printf("%s\n", isFirstBoot ? "true" : "false");
    } else {
        printf("get isFirstBoot failed\n");
    }

    return 0;
}

int PmCommand::showUsage() {
    printf("usage: pm [subcommand] [options]\n\n");
    printf("  pm install PATH\n");
    printf("  pm uninstall [-f] PACKAGE\n");
    printf("  pm clear PACKAGE\n");
    printf("  pm list\n");
    printf("  pm get PACKAGE\n");
    printf("  pm stats PACKAGE\n");
    printf("  pm firstboot\n");
    return 0;
}

int PmCommand::run(int argc, char *argv[]) {
    if (argc < 2) {
        return showUsage();
    }

    for (int i = 0; i < argc; i++) {
        mArgs.push_back(std::string(argv[i]));
    }
    char *op = argv[1];
    mNextArg = 2;

    android::ProcessState::self()->startThreadPool();
    if (strcmp("list", op) == 0) {
        return runList();
    }
    if (strcmp("install", op) == 0) {
        return runInstall();
    }
    if (strcmp("uninstall", op) == 0) {
        return runUninstall();
    }
    if (strcmp("get", op) == 0) {
        return runGetPackage();
    }
    if (strcmp("clear", op) == 0) {
        return runClear();
    }
    if (strcmp("stats", op) == 0) {
        return runPackageStats();
    }
    if (strcmp("firstboot", op) == 0) {
        return runFirstBoot();
    }
    return showUsage();
}

std::string_view PmCommand::nextArg() {
    if (mNextArg >= mArgs.size()) {
        return std::string("");
    }
    std::string_view argView(mArgs[mNextArg]);
    mNextArg++;
    return argView;
}

extern "C" int main(int argc, char *argv[]) {
    PmCommand cmd;
    return cmd.run(argc, argv);
}

} // namespace pm
} // namespace os