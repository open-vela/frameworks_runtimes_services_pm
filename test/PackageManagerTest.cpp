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

#include <gtest/gtest.h>

#include "pm/PackageManager.h"

namespace os {
namespace pm {

class PmTest : public testing::Test {
public:
protected:
    virtual void SetUp() override {
        mExistPackage = "com.vela.demo";
        mNotExistPackage = "com.vela.demo1";
    }
    virtual void TearDown() override {}

public:
    PackageManager pm;
    std::string mExistPackage;
    std::string mNotExistPackage;
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

extern "C" int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

} // namespace pm
} // namespace os
