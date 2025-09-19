#pragma once

#include <sys/types.h>
#include <uv.h>

#include <map>
#include <string>

#include "app/UvLoop.h"
#include "os/pm/IInstallObserver.h"
#include "os/pm/IUninstallObserver.h"
#include "pm/PackageInfo.h"

namespace os {
namespace pm {

class UninstallTask {
public:
    using TaskCompletedCallback = std::function<void(const std::string&)>;
    using UninstallResultHandler =
            std::function<void(const std::string&, const android::sp<IUninstallObserver>&)>;

    UninstallTask(uv_loop_t* looper, const std::string& path, const std::string& packageName,
                  const android::sp<IUninstallObserver>& observer,
                  TaskCompletedCallback completedCallback, UninstallResultHandler resultHandler)
          : mLooper(looper),
            mPath(path),
            mPackageName(packageName),
            mPoll(nullptr),
            mObserver(observer),
            mCompletedCallback(completedCallback),
            mResultHandler(resultHandler),
            mPipeFdRead(-1),
            mPipeFdWrite(-1) {}

    UninstallTask(const UninstallTask&) = delete;
    UninstallTask& operator=(const UninstallTask&) = delete;

    UninstallTask(UninstallTask&& other)
          : mLooper(other.mLooper),
            mPath(std::move(other.mPath)),
            mPackageName(std::move(other.mPackageName)),
            mPoll(std::move(other.mPoll)),
            mObserver(std::move(other.mObserver)),
            mCompletedCallback(std::move(other.mCompletedCallback)),
            mResultHandler(std::move(other.mResultHandler)),
            mPipeFdRead(other.mPipeFdRead),
            mPipeFdWrite(other.mPipeFdWrite) {
        other.mLooper = nullptr;
        other.mPipeFdRead = -1;
        other.mPipeFdWrite = -1;
    }

    UninstallTask& operator=(UninstallTask&& other) noexcept {
        if (this != &other) {
            mLooper = other.mLooper;
            mPath = std::move(other.mPath);
            mPackageName = std::move(other.mPackageName);
            mPoll = std::move(other.mPoll);
            mObserver = std::move(other.mObserver);
            mCompletedCallback = std::move(other.mCompletedCallback);
            mResultHandler = std::move(other.mResultHandler);
            mPipeFdRead = other.mPipeFdRead;
            mPipeFdWrite = other.mPipeFdWrite;

            other.mLooper = nullptr;
            other.mPipeFdRead = -1;
            other.mPipeFdWrite = -1;
        }
        return *this;
    }

    ~UninstallTask() = default;

    int start();
    void handleProgressUpdate(int fd, int events);
    void cleanup();

private:
    uv_loop_t* mLooper;
    std::string mPath;
    std::string mPackageName;
    std::unique_ptr<os::app::UvPoll> mPoll;
    android::sp<IUninstallObserver> mObserver;
    TaskCompletedCallback mCompletedCallback;
    UninstallResultHandler mResultHandler;
    int mPipeFdRead;
    int mPipeFdWrite;
};

} // namespace pm
} // namespace os