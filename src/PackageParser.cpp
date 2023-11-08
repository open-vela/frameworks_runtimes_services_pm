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

#include "PackageParser.h"

#include "PackageUtils.h"
#include "os/pm/PageInfo.h"
#include "os/pm/QuickAppInfo.h"
#include "os/pm/Router.h"

namespace os {
namespace pm {
int PackageParser::parseManifest(PackageInfo *info) {
    if (info == nullptr) {
        return android::NO_INIT;
    }

    rapidjson::Document document;
    int ret = getDocument(info->manifest.c_str(), document);
    if (ret) return ret;

    if (info->packageName.empty()) {
        info->packageName = getValue<std::string>(document, "package", "");
        if (info->packageName.empty()) {
            ALOGE("Failed parse manifest:%s package field", info->manifest.c_str());
            return android::BAD_VALUE;
        }
        info->appType = getValue<std::string>(document, "appType", "QUICKAPP");
        info->version = getValue<std::string>(document, "versionName", "");
        std::string sPath = info->manifest;
        info->installedPath = sPath.replace(sPath.end() - strlen(MANIFEST), sPath.end(), "");
        info->installTime = getCurrentTime();
        info->size = getDirectorySize(info->installedPath.c_str());
        info->shasum = calculateShasum(info->installedPath.c_str());
    }
    info->name = getValue<std::string>(document, "name", "");
    info->icon = getValue<std::string>(document, "icon", "");

    switch (getApplicationType(info->appType)) {
        case ApplicationType::NATIVE:
            ret = parseNativeManifest(document, info);
            break;
        case ApplicationType::QUICKAPP:
            ret = parseQuickAppManifest(document, info);
            break;
        default:
            ret = android::BAD_TYPE;
    }

    if (!ret) {
        info->bAllValid = true;
    }
    return ret;
}

int PackageParser::parseNativeManifest(const rapidjson::Document &document, PackageInfo *info) {
    if (info == nullptr) return android::NO_INIT;
    info->entry = getValue<std::string>(document, "entry", "");
    if (info->entry.empty()) {
        ALOGE("Failed parse manifest:%s entry field", info->manifest.c_str());
        return android::BAD_VALUE;
    }
    info->execfile = getValue<std::string>(document, "execfile", "");
    if (info->execfile.empty()) {
        ALOGE("Failed parse manifest:%s execfile field", info->manifest.c_str());
        return android::BAD_VALUE;
    }
    const rapidjson::Value baseArray = rapidjson::Value(rapidjson::kArrayType);
    const rapidjson::Value baseObject = rapidjson::Value(rapidjson::kObjectType);
    const rapidjson::Value &activityArray =
            getValue<const rapidjson::Value &>(document, "activities", baseArray);
    for (size_t i = 0; i < activityArray.Size(); i++) {
        ActivityInfo activiyInfo;
        activiyInfo.name = getValue<std::string>(activityArray[i], "name", "");
        if (activiyInfo.name.empty()) {
            ALOGE("Failed parse manifest:%s activities.name field", info->manifest.c_str());
            return android::BAD_VALUE;
        }
        activiyInfo.launchMode = getValue<std::string>(activityArray[i], "launchMode", "standard");
        activiyInfo.taskAffinity =
                getValue<std::string>(activityArray[i], "taskAffinity", info->packageName);
        const rapidjson::Value &intent =
                getValue<const rapidjson::Value &>(activityArray[i], "intent-filter", baseObject);
        const rapidjson::Value &actionArray =
                getValue<const rapidjson::Value &>(intent, "actions", baseArray);
        for (size_t j = 0; j < actionArray.Size(); j++) {
            activiyInfo.actions.push_back(actionArray[j].GetString());
        }
        info->activitiesInfo.push_back(activiyInfo);
    }

    const rapidjson::Value &servicesArray =
            getValue<const rapidjson::Value &>(document, "services", baseArray);
    for (size_t i = 0; i < servicesArray.Size(); i++) {
        ServiceInfo serviceInfo;
        serviceInfo.name = getValue<std::string>(servicesArray[i], "name", "");
        if (serviceInfo.name.empty()) {
            ALOGE("Failed parse manifest:%s services.name field", info->manifest.c_str());
            return android::BAD_VALUE;
        }
        serviceInfo.exported = getValue<bool>(servicesArray[i], "exported", false);
        const rapidjson::Value &intent =
                getValue<const rapidjson::Value &>(servicesArray[i], "intent-filter", baseObject);
        const rapidjson::Value &actionArray =
                getValue<const rapidjson::Value &>(intent, "actions", baseArray);
        for (size_t j = 0; j < actionArray.Size(); j++) {
            serviceInfo.actions.push_back(actionArray[j].GetString());
        }
        info->servicesInfo.push_back(serviceInfo);
    }
    return 0;
}

int PackageParser::parseQuickAppManifest(const rapidjson::Document &document, PackageInfo *info) {
    if (info == nullptr) return android::NO_INIT;
    /* quickapp has not activities field,this place provides a default for ams to use. */
    /* QuickActivity object belong to application registered. */
    /* quickapp not has exec,vappxms is quickapp default exec,ams according to exec how to start. */
    info->execfile = getValue<std::string>(document, "execfile", "vappxms");
    info->entry = getValue<std::string>(document, "entry", "QuickActivity");
    ActivityInfo defaultActivity;
    defaultActivity.name = "QuickActivity";
    defaultActivity.launchMode = "singleTask";
    defaultActivity.taskAffinity = info->packageName;
    info->activitiesInfo.push_back(defaultActivity);

    QuickAppInfo quickappInfo;
    quickappInfo.versionCode = getValue<int>(document, "versionCode", 0);
    const rapidjson::Value baseArray = rapidjson::Value(rapidjson::kArrayType);
    const rapidjson::Value baseObject = rapidjson::Value(rapidjson::kObjectType);
    const rapidjson::Value &featuresArray =
            getValue<const rapidjson::Value &>(document, "features", baseArray);
    for (size_t i = 0; i < featuresArray.Size(); i++) {
        std::string name = getValue<std::string>(featuresArray[i], "name", "");
        if (!name.empty()) {
            quickappInfo.features.push_back(name);
        }
    }
    Router router;
    const rapidjson::Value &routerValue =
            getValue<const rapidjson::Value &>(document, "router", baseObject);
    router.entry = getValue<std::string>(routerValue, "entry", "");
    const rapidjson::Value &pagesValue =
            getValue<const rapidjson::Value &>(routerValue, "pages", baseObject);
    for (rapidjson::Value::ConstMemberIterator itr = pagesValue.MemberBegin();
         itr != pagesValue.MemberEnd(); itr++) {
        if (itr->name.IsString()) {
            PageInfo page;
            page.pageName = itr->name.GetString();
            router.pages.push_back(page);
        }
    }
    quickappInfo.router = router;

    const rapidjson::Value &serviceArray =
            getValue<const rapidjson::Value &>(document, "services", baseArray);
    for (size_t i = 0; i < serviceArray.Size(); i++) {
        ServiceInfo serviceInfo;
        serviceInfo.name = getValue<std::string>(serviceArray[i], "name", "");
        serviceInfo.path = getValue<std::string>(serviceArray[i], "path", "");
        info->servicesInfo.push_back(serviceInfo);
    }
    info->extra = quickappInfo;
    return 0;
}

ApplicationType PackageParser::getApplicationType(const std::string &str) {
    size_t pos = str.find_first_of('/');
    std::string type = str.substr(0, pos);
    if (type == "NATIVE") {
        return ApplicationType::NATIVE;
    } else if (type == "QUICKAPP") {
        return ApplicationType::QUICKAPP;
    }
    return ApplicationType::UNKNOWN;
}

} // namespace pm
} // namespace os