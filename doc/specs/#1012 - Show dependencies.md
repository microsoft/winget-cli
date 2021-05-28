---
author: Florencia Zanollo fzanollo/t-fzanollo@microsoft.com
created on: 2021-05-28
last updated: 2021-05-28
issue id: 163
---

# Show dependencies

For [#1](https://github.com/microsoft/winget-cli/issues/1012)

## Abstract
Several packages require other packages as dependencies. The Windows Package Manager should be able to support declared dependencies and inform the user about any required dependencies.

## Solution Design
The Windows Package Manager should be able to show package dependency information for each of the four different types of dependencies declared in the [v1.0 manifest schemas](https://github.com/microsoft/winget-cli/blob/master/schemas/JSON/manifests/v1.0.0/).

* Windows Features
* Windows Libraries
* Package Dependencies (same source)
* External Dependencies

This information will be shown in the commands: show, install and import. Adding to the information already showed. 

## Capabilities

It's only an informational feature, will not check if the dependency is a valid one, nor if the source is available.

## Future considerations

It may be able to enable/disable this feature using extra options for the command.