#pragma once

#include <sys/types.h>
#include <uv.h>

#include <map>
#include <string>

#include "app/UvLoop.h"
#include "os/pm/IInstallObserver.h"
#include "os/pm/InstallParam.h"
#include "pm/PackageInfo.h"

namespace os {
namespace pm {

class InstallTask {
public:
    using TaskCompletedCallback = std::function<void(const std::string&)>;
    using InstallResultHandler =
            std::function<void(const std::string&, const android::sp<IInstallObserver>&)>;

    InstallTask(uv_loop_t* looper, const InstallParam& param,
                TaskCompletedCallback completedCallback, InstallResultHandler resultHandler,
                const std::string& packageName, const android::sp<IInstallObserver>& observer)
          : mLooper(looper),
            mParam(param),
            mPoll(nullptr),
            mCompletedCallback(completedCallback),
            mResultHandler(resultHandler),
            mPackageName(packageName),
            mObserver(observer),
            mPipeFdRead(-1),
            mPipeFdWrite(-1) {}
    InstallTask(const InstallTask&) = delete;
    InstallTask& operator=(const InstallTask&) = delete;
    InstallTask(InstallTask&& other)
          : mLooper(other.mLooper),
            mParam(std::move(other.mParam)),
            mPoll(std::move(other.mPoll)),
            mCompletedCallback(std::move(other.mCompletedCallback)),
            mResultHandler(std::move(other.mResultHandler)),
            mTmp(std::move(other.mTmp)),
            mPackageName(std::move(other.mPackageName)),
            mObserver(std::move(other.mObserver)),
            mPipeFdRead(other.mPipeFdRead),
            mPipeFdWrite(other.mPipeFdWrite) {
        other.mLooper = nullptr;
        other.mPipeFdRead = -1;
        other.mPipeFdWrite = -1;
    }
    InstallTask& operator=(InstallTask&& other) {
        if (this != &other) {
            mLooper = other.mLooper;
            mParam = std::move(other.mParam);
            mPoll = std::move(other.mPoll);
            mCompletedCallback = std::move(other.mCompletedCallback);
            mResultHandler = std::move(other.mResultHandler);
            mTmp = std::move(other.mTmp);
            mPackageName = std::move(other.mPackageName);
            mObserver = std::move(other.mObserver);
            mPipeFdRead = other.mPipeFdRead;
            mPipeFdWrite = other.mPipeFdWrite;

            other.mLooper = nullptr;
            other.mPipeFdRead = -1;
            other.mPipeFdWrite = -1;
        }
        return *this;
    }
    ~InstallTask() = default;

    int start();
    void handleProgressUpdate(int fd, int events);
    void cleanup();

private:
    uv_loop_t* mLooper;
    InstallParam mParam;
    std::unique_ptr<os::app::UvPoll> mPoll;
    TaskCompletedCallback mCompletedCallback;
    InstallResultHandler mResultHandler;
    std::string mTmp;
    std::string mPackageName;
    android::sp<IInstallObserver> mObserver;
    int mPipeFdRead;
    int mPipeFdWrite;
};

} // namespace pm
} // namespace os