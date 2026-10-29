## Bug Fixes

### Consistent elevation checks for user-scope package operations

Added elevated-process protections for user-scope MSI uninstall and repair, portable upgrades, and font installation and removal. Repair and uninstall-previous operations reject unsupported elevation before preparing the action. These operations report a consistent error directing callers to retry from a non-administrator context.

Font protection also covers initial and forced installations, which can inspect or replace existing font registrations.

Fresh portable installations remain supported from an elevated process and no longer reuse preexisting installation metadata or cleanup records.

The checks apply to the elevated half of a split UAC token. Existing behavior with UAC disabled is unchanged.

### Minor Bug Fixes

* Fixed installing new dependencies during an upgrade and applying upgrade protections when an installed dependency needs an update.
