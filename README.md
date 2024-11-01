Sure! Here’s the translation:

# Project Name

Package Management Module for Vela's XMS Module

## Table of Contents

- [Features](#features)
- [Installation](#installation)
- [Examples](#examples)
- [Contributions](#contributions)
- [License](#license)
- [Contact](#contact)

## Features

- Feature 1: Provides package installation functionality
- Feature 2: Provides package information querying capability
- Feature 3: Provides package uninstallation capability

## Installation

## Examples

- Package management can be done through the command line

    Install a package:

    ```
    pm install [packagename]
    ```

    Query which packages are installed:

    ```
    pm list
    ```

- Use the package management tool through source code

Install a package using the following format:

```
#include "pm/PackageManager.h"

PackageManager pm;
InstallParam parms;
pm.installPackage(parms);
```

Retrieve all package-related information with:

```
#include "pm/PackageManager.h"

PackageManager pm;
std::vector<PackageInfo> pgInfos;
pm.getAllPackageInfo(&pgInfos);
```

Uninstall a package using:

```
#include "pm/PackageManager.h"

PackageManager pm;
UninstallParam parms;
pm.uninstallPackage(parms);
```

## Contributions

## License

## Contact