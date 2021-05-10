---
author: Demitrius Nelon denelon/denelon@microsoft.com
created on: 2021-05-06
last updated: 2021-05-06
issue id: 163
---

# Dependencies

"For [#163](https://github.com/microsoft/winget-cli/issues/163)"

## Abstract

Several packages require other packages as dependencies. The Windows Package Manager should be able to support declared dependencies. In the best case scenario the Windows Package Manager should be able to install any necessary dependencies a package needs. In the worst case scenario the Windows Package manger should be able to inform a user about any required dependencies not able to be installed automatically.

## Inspiration

Many other package mangers have this capability. It's frustrating to be required to figure out what other dependencies are needed for a package, and have to deal with their dependencies as well.

## Solution Design

The Windows Package Manager manifest v1.0 schema has provided keys for four different types of dependencies.

@Conan-Kudo [suggested](https://github.com/microsoft/winget-cli/issues/163#issuecomment-631091560) evaluating [openSUSE/libsolv](https://github.com/openSUSE/libsolv).

Four different types of dependencies have been declared in the [v1.0 manifest schemas](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/manifests/v1.0.0/).

* Windows Features
* Windows Libraries
* Package Dependencies (same source)
* External Dependencies

Each of these dependencies must be declared in the manifest. In the case of MSIX packages, the dependencies are not required to be declared in the manifest.

The implementation for dependency support should initially be enabled as an experimental feature.

Each of the dependency types should be handled the same way for the minimum viable approach. A warning should be presented to the user with the list of dependencies specified in the manifest.
```
warning: This package requires the following dependencies:
<dependency type>: <dependency>
Do you have these dependencies installed [y/n]?
```
If the user chooses "yes", the installation will proceed.

Note: This is essentially the complete implementation for External Dependencies. It is not required at this stage to deal with nested dependencies. 

In addition, the consumes and provides concept is not in scope with this implementation. 

### Windows Features
These include items like .NET Frameworks, Internet Information Services, and Windows Subsystem for Linux. In some cases, turning these features on may require a reboot.

### Windows Libraries
These include items like Microsoft.WinJS or Visual C++ Redistributable libraries.

### Package Dependencies
These include other packages. The restriction on these dependencies is that they come from the same source for the package to be installed. These include programming languages, runtime environments, or drivers.

### External Dependencies
These include dependencies from outside of the source the original package is distributed. In some cases suitable items may exist in the same source, but for licensing or personal preference, no explicit package should be required. One example is Java. Many vendors offer JREs and JDKs so it may be more reasonable to have a user informed, and allow the user to confirm the presence of a dependency.



## UI/UX Design

@dustojnikhummer [suggested](https://github.com/microsoft/winget-cli/issues/163#issuecomment-633901489) a syntax like:

`winget install package -y` to install all dependencies and then the package and `winget install package` to prompt the user for each required dependency before installing it.

## Capabilities

Several different package installers exist and treat dependencies differently.

MSIX installers have an internal mechanism to identify dependencies.

MSI and .exe installers may include dependencies. 

### Accessibility

The Windows Package Manager has been built in such a way that screen readers will still provide audible output as the command is executed keeping the user informed of progress, warnings, and errors. This should have no direct impact on accessibility.

### Security

There should be no security impact directly, although we must remember that different sources may not guarantee the safety of packages. The Windows Package Manager community repository performs static and dynamic analysis, and in some cases additional manual validation before accepting a package.

### Reliability

The Windows Package Manager community repository does not directly host packages that may be called as dependencies. These packages may be removed or become unavailable if the site they are hosted on encounters a failure (or no network access is available).

### Compatibility

No current implementation for dependencies exists, so no compatibility issues are expected to occur as a result of dependency support.

### Performance, Power, and Efficiency

The time needed to download and install a package and it's dependencies is directly related to network speed and the size of any packages.

## Potential Issues

Not all dependencies are identified using [semantic versioning](https://semver.org/). This may cause complications for packages depending on a range of versions. It is possible a breaking change is introduced in a newer version of a dependency the Windows Package Manager cannot heuristically reason about.

Some systems have limited storage, and may not have suitable space to install a package and all of it's dependencies.

## Future considerations

The import command may be able to take advantage of determining all dependencies for all packages, and avoid repeatedly installing the same dependencies multiple times.

## Resources

[RPM Dependencies](https://jfearn.fedorapeople.org/en-US/RPM/4/html/RPM_Guide/ch-advanced-packaging.html)

[Node.js package.json configuration](https://docs.microsoft.com/en-us/visualstudio/javascript/configure-packages-with-package-json?view=vs-2019#:~:text=package.json%20configuration%201%20In%20a%20major%20version%20update%2C,fixes%20are%20included.%20Bug%20fixes%20are%20always%20backwards-compatible.)

[R Package metadata](https://r-pkgs.org/description.html)

[Debian package relationships](https://www.debian.org/doc/debian-policy/ch-relationships.html)

[Brew Untangling Dependencies](https://blog.jpalardy.com/posts/untangling-your-homebrew-dependencies/)

[Scoop Dependencies](https://scoop.netlify.app/concepts/#dependencies)

[Chocolatey Create Packages](https://docs.chocolatey.org/en-us/create/create-packages)