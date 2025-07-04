# Overlay ports

This directory contains an overlay for vcpkg ports, for cases where we need local modifications to a port.
In all cases, most of the recipe is taken from the [official vcpkg registry](https://github.com/Microsoft/vcpkg), and we only make small changes.

The whole directory can be re-created with `.\CreatePortOverlay.ps1`

## cpprestsdk

We add support for certificate pinning.
Note that we use v2.10.18, which is not the latest.

Changes:
* Add patch file: `add-server-certificate-validation.patch`
  Patch source: https://github.com/microsoft/winget-cli/commit/888b4ed8f4f7d25cb05a47210e083fe29348163b

## libyaml

We use an unreleased version that fixes a vulnerability.

Changes:
* New source commit: https://github.com/yaml/libyaml/commit/840b65c40675e2d06bf40405ad3f12dec7f35923
* Increase the port version so that Component Governance doesn't see it as the vulnerable version anymore