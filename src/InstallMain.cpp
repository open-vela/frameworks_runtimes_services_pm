#include <unistd.h>
#include <utils/Errors.h>
#include <uv_ext.h>

#include <array>
#include <string>

#include "PackageUtils.h"
#include "app/Logger.h"

#ifdef CONFIG_HAP_APP_PATH
#define ABS_PATH_PREFIX CONFIG_HAP_APP_PATH
#else
#define ABS_PATH_PREFIX "/data/quickapp"
#endif

#define PMS_RPK_UNZIP_ERROR 0xFF
#define PMS_RPK_INIT_ERROR 0xFE
#define PMS_CREATE_DIRECT_ERROR 0xFD
#define PMS_PARAM_ERROR 0xFC
#define PMS_RENAME_ERROR 0xFB

#define PMS_REMOVE_TMP_DATA_ERROR 0xFA
#define PMS_REMOVE_APP_DIRECT_ERROR 0xF9

namespace {

void writeError(int fd, uint8_t errorCode) {
    write(fd, &errorCode, sizeof(errorCode));
}
void writeProgress(int fd, uint8_t progress) {
    write(fd, &progress, sizeof(progress));
}

} // namespace

extern "C" int main(int argc, char **argv) {
    using std::filesystem::current_path;
    using std::filesystem::exists;
    using std::filesystem::rename;

    if (argc < 5) {
        return -1;
    }

    std::string command = argv[1];
    std::string path = argv[2];
    std::string packageName = argv[3];
    int progressFd = atoi(argv[4]);

    if (command != "install" && command != "uninstall") {
        ALOGE("Unknown command: %s", command.c_str());
        writeError(progressFd, PMS_PARAM_ERROR);
        return -1;
    }

    if (command == "install") {
        if (argc < 6) {
            return -1;
        }

        const char *tmp = argv[5];
        if (exists(tmp)) {
            os::pm::removeDirectory(tmp);
        }
        if (!os::pm::createDirectory(tmp)) {
            ALOGE("createDirectory failed");
            writeError(progressFd, PMS_CREATE_DIRECT_ERROR);
            return android::PERMISSION_DENIED;
        }
        auto *token = app_verify_init(path.c_str(), tmp);
        if (!token) {
            ALOGE("app_verify_init failed");
            os::pm::removeDirectory(tmp);
            writeError(progressFd, PMS_RPK_INIT_ERROR);
            return android::NO_INIT;
        }
        int ret = app_verify_unzip(token);
        if (ret) {
            ALOGE("app_verify_unzip failed");
            app_verify_close(token);
            os::pm::removeDirectory(tmp);
            writeError(progressFd, PMS_RPK_UNZIP_ERROR);
            return android::NO_INIT;
        }
        app_verify_close(token);

        std::string dstPath =
                os::pm::joinPath(os::pm::PackageConfig::getInstance().getAppInstalledPath(),
                                 packageName);
        if (exists(dstPath.c_str())) {
            os::pm::removeDirectory(dstPath.c_str());
        }

        std::error_code ec;
        rename(tmp, dstPath.c_str(), ec);
        if (ec) {
            ALOGE("Copy from %s to %s Failed:%s", tmp, dstPath.c_str(), ec.message().c_str());
            writeError(progressFd, PMS_RENAME_ERROR);
            return android::NO_INIT;
        }

        writeProgress(progressFd, 100);
        return 0;
    }

    // uninstall
    if (!os::pm::removeDirectory(path.c_str())) {
        ALOGE("removeDirectory failed: %s", path.c_str());
        writeError(progressFd, PMS_REMOVE_APP_DIRECT_ERROR);
        return android::PERMISSION_DENIED;
    }

    constexpr std::array<const char *, 3> kTypeList = {"cache", "files", "mass"};

    for (const auto &type : kTypeList) {
        std::string absolutePath;
#ifndef __NuttX__
        absolutePath = current_path();
#endif
        absolutePath += std::string(ABS_PATH_PREFIX) + "/" + type + "/" + packageName;

        if (exists(absolutePath)) {
            if (!os::pm::removeDirectory(absolutePath.c_str())) {
                ALOGE("removeDirectory failed: %s", absolutePath.c_str());
                writeError(progressFd, PMS_REMOVE_TMP_DATA_ERROR);
                return android::PERMISSION_DENIED;
            }
        }
    }

    writeProgress(progressFd, 100);
    return 0;
}