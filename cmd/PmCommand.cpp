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

namespace os {
namespace pm {

using android::binder::Status;

class InstallListener : public BnInstallObserver {
public:
    InstallListener(PmCommand &cmd) : mCmd(cmd) {}

    Status onInstallProcess(const std::string &packageName, int32_t process) override {
        printf("onInstallProcess %" PRIi32 "\n", process);
        return Status::ok();
    }

    Status onInstallResult(const std::string &packageName, int32_t code,
                           const std::string &msg) override {
        printf("onInstallResult: %s(%s %" PRIi32 ")\n", packageName.c_str(), msg.c_str(), code);
        mCmd.wake();
        return Status::ok();
    }

private:
    PmCommand &mCmd;
};

class UninstallListener : public BnUninstallObserver {
public:
    UninstallListener(PmCommand &cmd) : mCmd(cmd) {}

    Status onUninstallResult(const std::string &packageName, int32_t code,
                             const std::string &msg) override {
        printf("onUninstallResult: %s(%s %" PRIi32 ")\n", packageName.c_str(), msg.c_str(), code);
        mCmd.wake();
        return Status::ok();
    }

private:
    PmCommand &mCmd;
};

PmCommand::PmCommand() : mNextArg(0) {}

PmCommand::~PmCommand() {}

void PmCommand::wake() {
    std::unique_lock<std::mutex> lock(mMutex);
    return mCond.notify_one();
}

void PmCommand::wait() {
    std::unique_lock<std::mutex> lock(mMutex);
    mCond.wait(lock);
}

int PmCommand::runInstall() {
    std::string_view path = nextArg();
    if (path.empty()) {
        return showUsage();
    }
    InstallParam installparam;
    installparam.path = path;
    sp<InstallListener> listener = sp<InstallListener>::make(*this);
    int status = pm.installPackage(installparam, listener);
    if (!status) {
        wait();
    }
    return status;
}

static std::string dumpPackageInfo(const PackageInfo &info) {
    std::ostringstream oss;
    oss << "{" << std::endl;
    oss << "\033[33m"
        << "  package: " << info.packageName << "\033[0m" << std::endl;
    oss << "  name: " << info.name << std::endl;
    oss << "  appType: " << info.appType << std::endl;
    oss << "  bAllValid: " << (info.bAllValid ? "true" : "false") << std::endl;
    oss << "  installPath: " << info.installedPath << std::endl;
    oss << "  manifest: " << info.manifest << std::endl;
    oss << "  versionName: " << info.version << std::endl;
    oss << "  icon: " << info.icon << std::endl;
    oss << "  execfile: " << info.execfile << std::endl;
    oss << "  entry: " << info.entry << std::endl;
    oss << "  installTime: " << info.installTime << std::endl;
    oss << "  shasum: " << info.shasum << std::endl;
    oss << "  userId: " << info.userId << std::endl;
    oss << "  size: " << info.size << std::endl;
    oss << "  activities:[" << std::endl;
    for (auto &activity : info.activitiesInfo) {
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
    for (auto &service : info.servicesInfo) {
        oss << "     {" << std::endl;
        oss << "\tname: " << service.name << std::endl;
        oss << "\texported:" << service.exported << std::endl;
        oss << "\tactions:[" << std::endl;
        for (auto &action : service.actions) {
            oss << "\t  " << action << std::endl;
        }
        oss << "\t]" << std::endl;
        oss << "     }" << std::endl;
    }
    oss << "  ]" << std::endl;
    if (info.extra.has_value()) {
        QuickAppInfo quickappInfo = info.extra.value();
        oss << "  versionCode: " << quickappInfo.versionCode << std::endl;
        oss << "  features: [" << std::endl;
        for (auto &feature : quickappInfo.features) {
            oss << "    " << feature << std::endl;
        }
        oss << "  ]" << std::endl;
        oss << "  router:" << quickappInfo.router.toString() << std::endl;
    }
    oss << "}";
    return oss.str();
}

int PmCommand::runUninstall() {
    std::string_view packageName = nextArg();
    if (packageName.empty()) {
        return showUsage();
    }
    UninstallParam uninstallparam;
    uninstallparam.packageName = packageName;
    sp<UninstallListener> listener = sp<UninstallListener>::make(*this);
    int status = pm.uninstallPackage(uninstallparam, listener);
    if (!status) {
        wait();
    }
    return status;
}

int PmCommand::runList() {
    std::vector<PackageInfo> pkgInfos;
    int status = pm.getAllPackageInfo(&pkgInfos);
    for (size_t i = 0; i < pkgInfos.size(); i++) {
        printf("%s\n", dumpPackageInfo(pkgInfos[i]).c_str());
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
        printf("%s\n", dumpPackageInfo(pkginfo).c_str());
    } else {
        printf("get %.*s failed\n", static_cast<int>(packageName.length()), packageName.data());
    }
    return status;
}

int PmCommand::showUsage() {
    printf("usage: pm [subcommand] [options]\n\n");
    printf("  pm install PATH\n");
    printf("  pm uninstall PACKAGE\n");
    printf("  pm clear PACKAGE\n");
    printf("  pm list\n");
    printf("  pm get PACKAGE\n");
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