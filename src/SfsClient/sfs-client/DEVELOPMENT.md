# cgmanifest.json

Keep this file updated as you update `vcpkg.json`. It must indicate the dependencies we use for proper automatic scanning of dependencies done in the Azure DevOps backend.
The folder `build\vcpkg_installed\vcpkg\info` is a good source of truth of which vcpkg packages are installed. It does however accumulate temporary packages you may have added in the past. Before using it, try cleaning the build folder and re-building.

Once you find the GitHub repo of the dependency, use the version on the tags page to find the commit hash that corresponds to the installed version.
