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

#include <condition_variable>
#include <mutex>

#include "pm/PackageManager.h"

namespace os {
namespace pm {

using android::binder::Status;

class PmTest : public testing::Test {
public:
    void wake() {
        std::unique_lock<std::mutex> lock(mMutex);
        return mCond.notify_one();
    }

    void wait() {
        std::unique_lock<std::mutex> lock(mMutex);
        mCond.wait(lock);
    }

protected:
    virtual void SetUp() override {
        mExistPackage = "com.vela.demo";
        mNotExistPackage = "com.vela.demo1";
        mExistRpkPath = "/data/package/com.application.demo.debug.1.0.0.rpk";
        mNotExistRpkPath = "/data/package/com.vela.demo.rpk";
    }
    virtual void TearDown() override {}

public:
    PackageManager pm;
    std::condition_variable mCond;
    std::mutex mMutex;
    std::string mExistPackage;
    std::string mNotExistPackage;
    std::string mExistRpkPath;
    std::string mNotExistRpkPath;
};

TEST_F(PmTest, GetNotExistPackage) {
    PackageInfo info;
    EXPECT_NE(pm.getPackageInfo(mNotExistPackage, &info), 0);
}

TEST_F(PmTest, GetExistPackage) {
    PackageInfo info;
    EXPECT_EQ(pm.getPackageInfo(mExistPackage, &info), 0);
    EXPECT_STREQ(mExistPackage.c_str(), info.packageName.c_str());
}

class InstallListenerTest : public BnInstallObserver {
public:
    InstallListenerTest(PmTest &test, int32_t result) : mTest(test), mExpectResult(result) {}

    Status onInstallProcess(const std::string &packageName, int32_t process) override {
        return Status::ok();
    }

    Status onInstallResult(const std::string &packageName, int32_t code,
                           const std::string &msg) override {
        EXPECT_EQ(code, mExpectResult);
        mTest.wake();
        return Status::ok();
    }

private:
    PmTest &mTest;
    int32_t mExpectResult;
};

TEST_F(PmTest, InstallExistPackage) {
    InstallParam param;
    param.path = mExistRpkPath;
    sp<InstallListenerTest> listener = sp<InstallListenerTest>::make(*this, 0);
    int ret = pm.installPackage(param, listener);
    EXPECT_EQ(ret, 0);
    wait();
}

TEST_F(PmTest, InstallNotExistPackage) {
    InstallParam param;
    param.path = mNotExistRpkPath;
    sp<InstallListenerTest> listener =
            sp<InstallListenerTest>::make(*this, android::NAME_NOT_FOUND);
    int ret = pm.installPackage(param, listener);
    EXPECT_EQ(ret, 0);
    wait();
}

TEST_F(PmTest, InstallRepeatPackage) {
    InstallParam param;
    param.path = mExistRpkPath;
    sp<InstallListenerTest> listener = sp<InstallListenerTest>::make(*this, 0);
    int ret = pm.installPackage(param, listener);
    EXPECT_EQ(ret, 0);
    wait();
    ret = pm.installPackage(param, listener);
    EXPECT_EQ(ret, 0);
    wait();
}

extern "C" int main(int argc, char **argv) {
    android::ProcessState::self()->startThreadPool();
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace pm
} // namespace os
