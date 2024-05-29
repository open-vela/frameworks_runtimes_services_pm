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
#include <memory>

#include "../src/PackageUtils.h"
#include "pm/PackageManager.h"

namespace os {
namespace pm {

using android::binder::Status;
using std::filesystem::directory_iterator;
using std::filesystem::exists;

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
        mExistPackage = "com.vela.demo";
        mNotExistPackage = "com.vela.demo1";
        mExistRpkPath = "/data/package/com.application.demo.debug.1.0.0.rpk";
        mNotExistRpkPath = "/data/package/com.vela.demo.rpk";
        mInstallPackageName = "com.application.demo";
    }
    virtual void TearDown() override {}

public:
    std::string mExistPackage;
    std::string mNotExistPackage;
    std::string mExistRpkPath;
    std::string mNotExistRpkPath;
    std::string mInstallPackageName;
    PackageManager pm;
};

class InstallListenerTest : public BnInstallObserver, public std::promise<int32_t> {
public:
    Status onInstallProcess(const std::string &packageName, int32_t process) override {
        return Status::ok();
    }

    Status onInstallResult(const std::string &packageName, int32_t code,
                           const std::string &msg) override {
        printf("onUninstallResult: %s(%s %" PRIi32 ")\n", packageName.c_str(), msg.c_str(), code);
        this->set_value(code);
        return Status::ok();
    }
};

class UninstallListenerTest : public BnUninstallObserver, public std::promise<int32_t> {
public:
    Status onUninstallResult(const std::string &packageName, int32_t code,
                             const std::string &msg) override {
        printf("onUninstallResult: %s(%s %" PRIi32 ")\n", packageName.c_str(), msg.c_str(), code);
        this->set_value(code);
        return Status::ok();
    }
};

TEST_F(PmTest, InitStart) {
    rapidjson::Document doc;
    getDocument(PACKAGE_LIST, doc);
    const rapidjson::Value baseArray = rapidjson::Value(rapidjson::kArrayType);
    const rapidjson::Value &packagesArray =
            getValue<const rapidjson::Value &>(doc, "packages", baseArray);
    std::vector<PackageInfo> pkgInfos;
    pm.getAllPackageInfo(&pkgInfos);
    unsigned int manifestCount = 0;
    manifestCount += getDirectoryCount("/system/app");
    manifestCount += getDirectoryCount("/data/app");
    EXPECT_EQ(exists(PACKAGE_LIST), true);
    EXPECT_EQ(packagesArray.Size(), pkgInfos.size());
    EXPECT_EQ(manifestCount, pkgInfos.size());
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
        EXPECT_STREQ(getValue<std::string>(doc, "execfile", "").c_str(), info.execfile.c_str());
        EXPECT_STREQ(getValue<std::string>(doc, "name", "").c_str(), info.name.c_str());
    }
}

TEST_F(PmTest, GetExistPackage) {
    PackageInfo info;
    EXPECT_EQ(pm.getPackageInfo(mExistPackage, &info), 0);
    EXPECT_STREQ(mExistPackage.c_str(), info.packageName.c_str());
}

TEST_F(PmTest, GetNotExistPackage) {
    PackageInfo info;
    EXPECT_NE(pm.getPackageInfo(mNotExistPackage, &info), 0);
}

TEST_F(PmTest, InstallExistPackage) {
    InstallParam param;
    param.path = mExistRpkPath;
    sp<InstallListenerTest> listener = new InstallListenerTest();
    int ret = pm.installPackage(param, listener);
    EXPECT_EQ(ret, 0);
    std::future<int32_t> f = listener->get_future();
    int result = f.get();
    EXPECT_EQ(result, 0);
    EXPECT_EQ(exists("/data/app/com.application.demo.debug.1.0.0"), true);
    PackageInfo info;
    EXPECT_EQ(pm.getPackageInfo(mInstallPackageName, &info), 0);
}

TEST_F(PmTest, InstallRepeatPackage) {
    InstallParam param;
    param.path = mExistRpkPath;
    sp<InstallListenerTest> listener = new InstallListenerTest();
    int ret = pm.installPackage(param, listener);
    EXPECT_EQ(ret, 0);
    std::future<int32_t> f = listener->get_future();
    int result = f.get();
    EXPECT_EQ(result, 0);

    EXPECT_EQ(exists("/data/app/com.application.demo.debug.1.0.0"), true);
    PackageInfo info;
    EXPECT_EQ(pm.getPackageInfo(mInstallPackageName, &info), 0);
}

TEST_F(PmTest, UninstallPackage) {
    UninstallParam uninstallparam;
    uninstallparam.packageName = mInstallPackageName;
    sp<UninstallListenerTest> listener = new UninstallListenerTest();
    int ret = pm.uninstallPackage(uninstallparam, listener);
    EXPECT_EQ(ret, 0);
    std::future<int32_t> f = listener->get_future();
    int result = f.get();
    EXPECT_EQ(result, 0);
    PackageInfo pkginfo;
    EXPECT_EQ(pm.getPackageInfo(mInstallPackageName, &pkginfo), Status::EX_SERVICE_SPECIFIC);
}

TEST_F(PmTest, UninstallNotExistPackage) {
    UninstallParam uninstallparam;
    uninstallparam.packageName = mNotExistPackage;
    sp<UninstallListenerTest> listener = new UninstallListenerTest();
    int ret = pm.uninstallPackage(uninstallparam, listener);
    EXPECT_EQ(ret, 0);
    std::future<int32_t> f = listener->get_future();
    int result = f.get();
    EXPECT_EQ(result, android::NAME_NOT_FOUND);
    PackageInfo pkginfo;
    EXPECT_EQ(pm.getPackageInfo(mNotExistPackage, &pkginfo), Status::EX_SERVICE_SPECIFIC);
}

TEST_F(PmTest, InstallNotExistPackage) {
    InstallParam param;
    param.path = mNotExistPackage;
    sp<InstallListenerTest> listener = new InstallListenerTest();
    int ret = pm.installPackage(param, listener);
    EXPECT_EQ(ret, 0);
    std::future<int32_t> f = listener->get_future();
    int result = f.get();
    EXPECT_EQ(result, android::NAME_NOT_FOUND);
    EXPECT_EQ(exists("/data/app/com.vela.demo"), false);
}

extern "C" int main(int argc, char **argv) {
    android::ProcessState::self()->startThreadPool();
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace pm
} // namespace os
