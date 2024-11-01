# 项目名称

Vela的XMS模块的包安装管理器模块

## 目录

- [特性](#特性)
- [安装](#安装)
- [示例](#示例)
- [贡献](#贡献)
- [许可证](#许可证)
- [联系方式](#联系方式)

## 特性

- 特性1：提供包安装功能
- 特性2：提供包信息查询能力
- 特性3：提供包卸载能力

## 安装

## 示例

- 可以通过命令行来进行包管理

    安装一个包

    ```
    pm install [packagename]
    ```

    查询哪些包已经安装

    ```
    pm list
    ```

- 通过源码形式来使用包管理工具

安装一个包，可通过如下形式

```
#include "pm/PackageManger.h"

PackageManager pm;
InstallParam parms;
pm.installPackage(parms)；

```

可以通过如下形式来获取所有包相关的信息

```
#include "pm/PackageManger.h"

PackageManager pm;
std::vector<PackageInfo> pginfos;
pm.getAllPackageInfo(&pginfo);
```

可以通过如下形式来卸载一个包

```
#include "pm/PackageManger.h"

PackageManager pm;
UninstallParam parms;
pm.uninstallPackage(parms);

```


## 贡献

## 许可证

## 联系方式