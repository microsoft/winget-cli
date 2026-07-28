# Overlay ports

This directory contains an overlay for vcpkg ports, for cases where we need local modifications to a port.
In all cases, most of the recipe is taken from the [official vcpkg registry](https://github.com/Microsoft/vcpkg), and we only make small changes.

The whole directory can be re-created with `.\CreatePortOverlay.ps1`

## cpprestsdk

We add support for certificate pinning.
Note that we use v2.10.19

Changes:
* Add patch file: `add-server-certificate-validation.patch`
* Patch source: https://github.com/microsoft/winget-cli/commit/888b4ed8f4f7d25cb05a47210e083fe29348163b

## detours

We use the version used by UndockedRegFreeWinRT (https://github.com/microsoft/winget-cli/tree/release-v1.10/src/Xlang/UndockedRegFreeWinRT/src/UndockedRegFreeWinRT/detours).
The only official release of detours (4.0.1) does not include complete support for ARM64.
While the exact version that we pulled from UndockedRegFreeWinRT is unclear (https://github.com/microsoft/xlang/pull/644), through manually comparing versions it is equivalent to
https://github.com/microsoft/Detours/commit/404c153ff390cb14f1787c7feeb4908c6d79b0ab (only some whitespace changes are present).

Changes:
* New source commit: https://github.com/microsoft/Detours/commit/404c153ff390cb14f1787c7feeb4908c6d79b0ab
* Remove the patch on the official port as it is already present in the newer commit

## sfs-client

sfs-client is not in the official vcpkg registry. The port is based on the [template included in the sfs-client repository](https://github.com/microsoft/sfs-client/tree/main/sfs-client-vcpkg-port/sfs-client).

We use version 1.1.0 at commit https://github.com/microsoft/sfs-client/commit/0e27525d597c730e71646fd0b15bdc8c8503f24d.
Tests and samples are disabled (`-DSFS_BUILD_TESTS=OFF -DSFS_BUILD_SAMPLES=OFF`).

## libyaml

We use an unreleased version that fixes a vulnerability.

Changes:
* New source commit: https://github.com/yaml/libyaml/commit/840b65c40675e2d06bf40405ad3f12dec7f35923
* Increase the port version so that Component Governance doesn't see it as the vulnerable version anymore
