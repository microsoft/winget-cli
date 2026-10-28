## Bug Fixes

### Consistent elevation checks for user-scope package operations

Added elevated-process protections for user-scope MSI uninstall and repair, and portable upgrades. Repair and uninstall-previous operations reject unsupported elevation before preparing the action. These operations report a consistent error directing callers to retry from a non-administrator context.

Fresh portable installations remain supported from an elevated process and no longer reuse pre-existing installation metadata or cleanup records.

The checks apply to the elevated half of a split UAC token. Existing behavior with UAC disabled is unchanged.
