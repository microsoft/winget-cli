## Bug Fixes

### Minor Bug Fixes

* Limit data copied through YAML aliases to 10 MiB, so manifests and configuration files that exceed it fail to load instead of using excessive memory.
