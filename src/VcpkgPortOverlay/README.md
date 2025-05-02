# Overlay ports

This directory contains an overlay for vcpkg ports, for cases where we need to apply local patches.
In all cases, most of the recipe is taken from the [official vcpkg registry](https://github.com/Microsoft/vcpkg), and we only add a patch.

## cpprestsdk

We add support for certificate pinning.

Patch file: `add-server-certificate-validation.patch`
Source for the change: https://github.com/microsoft/winget-cli/commit/888b4ed8f4f7d25cb05a47210e083fe29348163b

## libyaml

We apply a patch for a vulnerability.

Patch file: `fix-parser-nesting.patch`
Source for the change: https://github.com/yaml/libyaml/commit/51843fe48257c6b7b6e70cdec1db634f64a40818