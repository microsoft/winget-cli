---
author: Florencia Zanollo fzanollo/t-fzanollo@microsoft.com
created on: 2021-07-15
last updated: 2021-07-15
issue id: 1287
---

# Management of package type dependencies

For [#1287](https://github.com/microsoft/winget-cli/issues/1287)

## Abstract
As a new step in the pursue of dependency management, the Windows Package Manager will take care of one of the four types of dependencies (Package) for most of the commands. The underlying logic of the code will prove useful when implementing the rest of the types and commands; since it will manage dependencies' graph building and validation, as well as the correct order of installation.

## Solution Design
The Windows Package Manager will build the dependencies' graph and corroborate there's not a cyclic dependency, among other validations (depending on the command). 

### Install:
Install command will build the dependency graph at runtime, from installer information. It will report on the other three types of dependencies and manage package type installation/validation (for new/installed dependencies respectively).
As a best effort, in case a cyclic dependency exists, Windows Package Manager will inform the user but try to install in some order; this is because most of the dependencies are at run time so there shouldn't present an issue on install.

While building the graph, install command will verify:
* Availability: the package declared as dependency will need to be an existing one.
* Installed version: it will check for existing versions of the dependency and update if the minimum required version is bigger than the installed. 
* Version: minimum required version will need to be less or equal to the latest available one.

### Import:
Import command will work as install command for each of the packages found, iteratively. Ex. if packages *foo* and *foo1* are part of the import, dependencies for *foo* will be fetch and installed before continuing with *foo1*.

### Update:
* Update one: will work as install command, we need to check again for dependencies as the new installer can have new ones or bigger minimum required versions.
* Update many: will work iteratively (as import).

## Capabilities
Will manage package type dependencies installation for install, import and update commands.
Will not manage or try to install any of the other types of dependencies (Windows Features, Windows Libraries, External).