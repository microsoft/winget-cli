## New Features

### Install from a remote manifest

Pass an HTTPS URL to `--manifest` instead of downloading manifests manually:

```powershell
winget install --manifest https://example.com/package.yaml
winget install --manifest https://example.com/manifests.zip
winget install --manifest .\manifests.zip
```

Remote manifests are disabled by default. From an administrator terminal, enable `winget settings --enable RemoteManifestFiles`. The `LocalManifestFiles` administrator setting is only required for local paths. The existing local-manifest group policy still applies to both local and remote manifests; enabling remote manifests does not bypass it.

URLs must end in `.yaml`, `.yml`, or `.zip`. A YAML URL must contain a complete singleton manifest. A ZIP must contain exactly one singleton manifest or one package version's multi-file manifests, optionally inside directories. Archive filenames do not select packages. Credentials, query strings, fragments, and nonstandard ports are unsupported.

Local ZIP paths are also supported under the existing `LocalManifestFiles` administrator setting. Local and remote ZIPs follow the same scanning, extraction, and single-manifest selection rules. Extracted files and remote downloads are temporary; the original local ZIP is preserved. Normal manifest parsing and installer hash verification still apply. Existing local YAML files and manifest directories remain supported. No source is registered.
