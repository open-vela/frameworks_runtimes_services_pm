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

#include <condition_variable>
#include <memory>
#include <mutex>
#include <string_view>

#include "pm/PackageManager.h"

namespace os {
namespace pm {
class PmCommand {
public:
    PmCommand();
    ~PmCommand();
    void wake();
    void wait();
    int runInstall();
    int runUninstall();
    int runList();
    int runClear();
    int runGetPackage();
    int showUsage();
    int run(int argc, char *argv[]);

private:
    std::string_view nextArg();
    std::vector<std::string> mArgs;
    size_t mNextArg;
    int mWaitNum;
    PackageManager pm;
    std::condition_variable mCond;
    std::mutex mMutex;
};
} // namespace pm
} // namespace os