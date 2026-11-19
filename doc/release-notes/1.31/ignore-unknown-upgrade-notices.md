## New Features

### Quietly skip packages with unknown versions

Use `winget upgrade --all --ignore-unknown` to keep skipping packages whose installed versions cannot be determined without displaying the unknown-version summary. The option also works with `winget upgrade` when listing upgrades and with `winget list --upgrade-available`.

Default behavior and other diagnostics are unchanged. `--ignore-unknown` cannot be combined with `--include-unknown`, which instead opts into packages with unknown versions, or used for an explicit package or manifest upgrade.
