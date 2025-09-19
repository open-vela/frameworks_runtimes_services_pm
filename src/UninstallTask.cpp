#include "UninstallTask.h"

#include <spawn.h>
#include <sys/types.h>
#include <unistd.h>
#include <utils/Errors.h>
#include <utils/Log.h>

#include <filesystem>

#include "PackageUtils.h"
#include "app/Logger.h"

namespace os {
namespace pm {

int UninstallTask::start() {
    int pipefd[2]; // 0: read, 1: write
    if (pipe(pipefd) != 0) {
        ALOGE("create pipe failed: %s", strerror(errno));
        cleanup();
        return -1;
    }
    mPipeFdRead = pipefd[0];
    mPipeFdWrite = pipefd[1];

    mPoll = std::make_unique<os::app::UvPoll>(mLooper, mPipeFdRead);
    int startResult = mPoll->start(
            UV_READABLE | UV_PRIORITIZED,
            [](int fd, int status, int events, void *data) {
                UninstallTask *task = static_cast<UninstallTask *>(data);
                task->handleProgressUpdate(fd, events);
            },
            this);

    if (startResult != 0) {
        ALOGE("uv_poll_start failed: %s", uv_strerror(startResult));
        cleanup();
        return -1;
    }

    pid_t pid = -1;
    char *argv[6];
    argv[0] = const_cast<char *>("pmsInstaller");       // program name
    argv[1] = const_cast<char *>("uninstall");          // Indicates install
    argv[2] = const_cast<char *>(mPath.c_str());        // package path
    argv[3] = const_cast<char *>(mPackageName.c_str()); // package name
    std::string str = std::to_string(mPipeFdWrite);
    argv[4] = const_cast<char *>(str.c_str()); // write fd
    argv[5] = nullptr;

    posix_spawnattr_t attr;
    posix_spawnattr_init(&attr);
    posix_spawnattr_setstacksize(&attr, CONFIG_SYSTEM_SERVER_STACKSIZE);

    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addclose(&actions, mPipeFdRead); // 子进程关闭读端

    const int ret = posix_spawn(&pid, argv[0], &actions, &attr, argv, NULL);
    posix_spawnattr_destroy(&attr);
    posix_spawn_file_actions_destroy(&actions);
    close(mPipeFdWrite); // 父进程关闭写端
    mPipeFdWrite = -1;
    if (ret != 0) {
        ALOGE("posix_spawn %s failed error:%d", argv[0], ret);
        cleanup();
        return ret > 0 ? -ret : ret;
    }

    return pid;
}

void UninstallTask::handleProgressUpdate(int fd, int events) {
    if (!(events & UV_READABLE)) {
        return;
    }

    uint8_t progress;
    ssize_t nread = read(fd, &progress, sizeof(progress));
    if (nread == sizeof(progress)) {
        ALOGI("Uninstallation progress: %d%% on fd:%d", progress, fd);
        if (progress > 100) {
            ALOGE("remove %s failed: 0x%X", mPath.c_str(), progress);
            mObserver->onUninstallResult(mPackageName, -1, "Failed to remove package");
            cleanup(); // 关闭父进程读端->关闭管道
            return;
        }

        if (progress == 100) {
            ALOGI("Uninstallation completed via progress pipe fd:%d", fd);
            if (mResultHandler) {
                mResultHandler(mPackageName, mObserver);
            }
            cleanup(); // 关闭父进程读端->关闭管道
        }
    } else if (nread == 0) {
        // 管道关闭（子进程关闭了写端）
        ALOGI("Pipe fd:%d closed by child process", fd);
        cleanup(); // 关闭父进程读端->关闭管道
    } else if (nread < 0) {
        // 读取错误
        ALOGE("Read error on pipe fd:%d: %s", fd, strerror(errno));
        cleanup(); // 关闭父进程读端->关闭管道
    }
}

void UninstallTask::cleanup() {
    if (mPipeFdRead >= 0) {
        close(mPipeFdRead);
        mPipeFdRead = -1;
    }

    if (mPipeFdWrite >= 0) {
        close(mPipeFdWrite);
        mPipeFdWrite = -1;
    }

    if (mCompletedCallback) {
        mCompletedCallback(mPackageName);
    }
}

} // namespace pm
} // namespace os