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

#include <binder/ProcessState.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <future>
#include <map>
#include <memory>

#include "PackageUtils.h"
#include "pm/PackageManager.h"

namespace os {
namespace pm {

using android::binder::Status;
using std::filesystem::directory_iterator;
using std::filesystem::exists;

struct TestParams {
    std::string packageName;
    std::string rpkPath;
    std::string notExistPackageName;
    std::string notExistRpkPath;
    const std::string dump() const {
        return std::string("packageName: ")
                .append(packageName)
                .append("\n")
                .append("rpkPath: ")
                .append(rpkPath)
                .append("\n")
                .append("notExistPackageName: ")
                .append(notExistPackageName)
                .append("\n")
                .append("notExistRpkPath: ")
                .append(notExistRpkPath);
    }
};

static TestParams g_testParams = {"com.application.demo",
                                  "/resource/package/com.application.demo.debug.1.0.0.rpk",
                                  "com.vela.demo1", "/data/package/com.vela.demo.rpk"};

class PmTest : public testing::Test {
public:
    unsigned int getDirectoryCount(const std::string &path) {
        unsigned int count = 0;
        for (const auto &entry : directory_iterator(path.c_str())) {
            if (entry.is_directory()) {
                count++;
            }
        }
        return count;
    }

protected:
    virtual void SetUp() override {
        android::ProcessState::self()->startThreadPool();
    }
    virtual void TearDown() override {}

public:
    PackageManager pm;
};

class InstallListenerTest : public BnInstallObserver, public std::promise<int32_t> {
public:
    Status onInstallProcess(const std::string &packageName, int32_t process) override {
        return Status::ok();
    }

    Status onInstallResult(const std::string &packageName, int32_t code,
                           const std::string &msg) override {
        printf("onInstallResult: %s(%s %" PRIi32 ")\n", packageName.c_str(), msg.c_str(), code);
        promise_.set_value(code);
        return Status::ok();
    }

    std::future<int32_t> get_future() {
        return promise_.get_future();
    }

private:
    std::promise<int32_t> promise_;
};

class UninstallListenerTest : public BnUninstallObserver, public std::promise<int32_t> {
public:
    Status onUninstallResult(const std::string &packageName, int32_t code,
                             const std::string &msg) override {
        printf("onUninstallResult: %s(%s %" PRIi32 ")\n", packageName.c_str(), msg.c_str(), code);
        promise_.set_value(code);
        return Status::ok();
    }

    std::future<int32_t> get_future() {
        return promise_.get_future();
    }

private:
    std::promise<int32_t> promise_;
};

TEST_F(PmTest, InitStart) {
    rapidjson::Document doc;
    getDocument(PACKAGE_CFG, doc);
    std::string appPresetPath = getValue<std::string>(doc, "appPresetPath", "/system/app");
    std::string appInstalledPath = getValue<std::string>(doc, "appInstalledPath", "/data/app");
    std::string packageListPath = joinPath(appInstalledPath, PACKAGE_LIST);

    rapidjson::Document docForLoad;
    getDocument(packageListPath.c_str(), docForLoad);
    const auto baseArray = rapidjson::Value(rapidjson::kArrayType);
    const auto &packagesArray =
            getValue<const rapidjson::Value &>(docForLoad, "packages", baseArray);
    std::map<std::string, PackageInfo> pkgInfosForLoad;
    for (unsigned int i = 0; i < packagesArray.Size(); i++) {
        PackageInfo info;
        info.packageName = getValue<std::string>(packagesArray[i], "package", "");
        if (!(info.packageName.empty())) {
            pkgInfosForLoad.insert(std::make_pair(info.packageName, info));
        }
    }
    std::vector<PackageInfo> pkgInfos;
    pm.getAllPackageInfo(&pkgInfos);
    unsigned int manifestCount = 0;
    manifestCount += getDirectoryCount(appPresetPath);
    manifestCount += getDirectoryCount(appInstalledPath);
    EXPECT_EQ(exists(packageListPath), true);
    EXPECT_EQ(pkgInfosForLoad.size(), pkgInfos.size());
    EXPECT_EQ(manifestCount, packagesArray.Size());
}

TEST_F(PmTest, CheckKeyField) {
    std::vector<PackageInfo> pkgInfos;
    pm.getAllPackageInfo(&pkgInfos);
    for (const auto &info : pkgInfos) {
        rapidjson::Document doc;
        getDocument(info.manifest.c_str(), doc);
        EXPECT_STREQ(getValue<std::string>(doc, "package", "").c_str(), info.packageName.c_str());
        EXPECT_STREQ(getValue<std::string>(doc, "appType", "QUICKAPP").c_str(),
                     info.appType.c_str());
        EXPECT_STREQ(getValue<std::string>(doc, "versionName", "").c_str(), info.version.c_str());
        EXPECT_STREQ(getValue<std::string>(doc, "name", "").c_str(), info.name.c_str());
        EXPECT_STREQ(getValue<std::string>(doc, "icon", "").c_str(), info.icon.c_str());
        EXPECT_EQ(getProcessPriority(getValue<std::string>(doc, "priority", "middle")),
                  info.priority);
        EXPECT_EQ(getValue<bool>(doc, "isSystemUI", false), info.isSystemUI);
    }
}

TEST_F(PmTest, GetNotExistPackage) {
    PackageInfo info;
    EXPECT_NE(pm.getPackageInfo(g_testParams.notExistPackageName, &info), 0);
}

TEST_F(PmTest, InstallExistPackage) {
    InstallParam param;
    param.path = g_testParams.rpkPath;
    sp<InstallListenerTest> listener = new InstallListenerTest();
    int ret = pm.installPackage(param, listener);
    EXPECT_EQ(ret, 0);
    auto f = listener->get_future();
    int result = f.get();
    EXPECT_EQ(result, 0);
    EXPECT_EQ(exists(g_testParams.rpkPath.c_str()), true);
    PackageInfo info;
    EXPECT_EQ(pm.getPackageInfo(g_testParams.packageName, &info), 0);
}

TEST_F(PmTest, GetExistPackage) {
    PackageInfo info;
    EXPECT_EQ(pm.getPackageInfo(g_testParams.packageName, &info), 0);
    auto str1 = g_testParams.packageName.c_str();
    auto str2 = info.packageName.c_str();
    // Note: EXPECT_STREQ crashed in sim when assert failed
    // use EXPECT_EQ instead.
    EXPECT_EQ(strcmp(str1, str2), 0);
}

TEST_F(PmTest, InstallRepeatPackage) {
    InstallParam param;
    param.path = g_testParams.rpkPath;
    sp<InstallListenerTest> listener = new InstallListenerTest();
    int ret = pm.installPackage(param, listener);
    EXPECT_EQ(ret, 0);
    auto f = listener->get_future();
    int result = f.get();
    EXPECT_EQ(result, 0);

    PackageInfo info;
    EXPECT_EQ(pm.getPackageInfo(g_testParams.packageName, &info), 0);
    EXPECT_EQ(exists(info.installedPath.c_str()), true);
}

TEST_F(PmTest, UninstallPackage) {
    UninstallParam uninstallparam;
    uninstallparam.packageName = g_testParams.packageName;
    sp<UninstallListenerTest> listener = new UninstallListenerTest();
    int ret = pm.uninstallPackage(uninstallparam, listener);
    EXPECT_EQ(ret, 0);
    auto f = listener->get_future();
    int result = f.get();
    EXPECT_EQ(result, 0);
    PackageInfo pkginfo;
    EXPECT_EQ(pm.getPackageInfo(g_testParams.packageName, &pkginfo), Status::EX_SERVICE_SPECIFIC);
}

TEST_F(PmTest, UninstallNotExistPackage) {
    UninstallParam uninstallparam;
    uninstallparam.packageName = g_testParams.notExistPackageName;
    sp<UninstallListenerTest> listener = new UninstallListenerTest();
    int ret = pm.uninstallPackage(uninstallparam, listener);
    EXPECT_EQ(ret, 0);
    auto f = listener->get_future();
    int result = f.get();
    EXPECT_EQ(result, android::NAME_NOT_FOUND);
    PackageInfo pkginfo;
    EXPECT_EQ(pm.getPackageInfo(g_testParams.notExistPackageName, &pkginfo),
              Status::EX_SERVICE_SPECIFIC);
}

TEST_F(PmTest, InstallNotExistPackage) {
    // if package not exist, try install not exist package
    // otherwise, it's already exist, do nothing, this case skipped.
    PackageInfo info;
    if (pm.getPackageInfo(g_testParams.notExistPackageName, &info) != 0) {
        InstallParam param;
        param.path = g_testParams.notExistRpkPath;
        sp<InstallListenerTest> listener = new InstallListenerTest();
        int ret = pm.installPackage(param, listener);
        EXPECT_EQ(ret, 0);
        auto f = listener->get_future();
        int result = f.get();
        EXPECT_EQ(result, android::NAME_NOT_FOUND);
        EXPECT_NE(pm.getPackageInfo(g_testParams.notExistPackageName, &info), 0);
    }
}

extern "C" int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    // parse cli parameters
    std::vector<std::string> args(argv + 1, argv + argc);
    for (auto it = args.begin(); it != args.end();) {
        if (it->find("--packageName=") == 0) {
            g_testParams.packageName = it->substr(15);
            it = args.erase(it);
        } else if (it->find("--rpkPath=") == 0) {
            g_testParams.rpkPath = it->substr(10);
            it = args.erase(it);
        } else if (*it == "--packageName" && next(it) != args.end()) {
            g_testParams.packageName = *(++it);
            it = args.erase(prev(it), it + 1);
        } else if (*it == "--rpkPath" && next(it) != args.end()) {
            g_testParams.rpkPath = *(++it);
            it = args.erase(prev(it), it + 1);
        } else {
            ++it;
        }
    }
    ::testing::GTEST_FLAG(filter) = "PmTest.*";
    return RUN_ALL_TESTS();
}

} // namespace pm
} // namespace os
