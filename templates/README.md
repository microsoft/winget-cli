# Pipeline templates

These templates hold the Azure Pipelines jobs that build and test winget-cli. `azure-pipelines.yml`
at the repository root consumes them, and so can a pipeline in another repository that vendors
winget-cli as a subtree.

## Layout

| File | Kind | Purpose |
| --- | --- | --- |
| `jobs-build.yml` | jobs | Builds the solution and publishes the `Build.*` artifacts |
| `jobs-test.yml` | jobs | Unit, E2E, and configuration tests against those artifacts |
| `jobs-powershell-module.yml` | jobs | Builds and Pester-tests the PowerShell modules |
| `e2e-setup.yml` | steps | Test certificates, localhost web server, local PS repository |
| `e2e-test.template.yml` | steps | One E2E test pass, with optional COM tracing |

The two step templates are used by the job templates; a consumer does not reference them directly.

## `sourceRoot`

The job templates and `e2e-test.template.yml` take a `sourceRoot` parameter that points at the root
of the winget-cli sources. It defaults to `$(Build.SourcesDirectory)`, so a pipeline at the
repository root needs to pass nothing. (`e2e-setup.yml` predates this and spells the same idea
`sourceDir`, which is required rather than defaulted.)

```yaml
- template: templates/jobs-build.yml
  parameters:
    sourceRoot: $(Build.SourcesDirectory)\subtree
```

Everything that lives in the winget-cli tree is rooted at this parameter: the solution, the wapproj
files, the PowerShell scripts invoked by `filePath`, the `workingDirectory` of those tasks, the
`.csproj` restore glob, and the `tools\` tracing profiles.

Paths that belong to the agent or to the build output are deliberately **not** rooted at it, because
they do not move with the subtree:

- `$(Build.ArtifactStagingDirectory)`, `$(Pipeline.Workspace)`
- `$(Agent.TempDirectory)`, `$(VCPKG_INSTALLATION_ROOT)`
- `$(buildOutDir)`, `$(buildOutDirAnyCpu)`, `$(artifactsDir)`, `$(packageLayoutDir)`

Nested template references need no help. Azure Pipelines resolves a `- template:` reference inside a
template file relative to *that file*, so the job templates find their siblings at any depth. Only
the entry point has to know where the subtree lives.

## What a consuming pipeline must provide

**Pipeline variables.** None. `jobs-build.yml` sets what it needs at job scope — `solution`, so it
can be rooted, and `EnableDetectorVcpkg`, so Component Governance detects the vcpkg dependencies.
Both were previously pipeline-level variables in `azure-pipelines.yml`; a consumer that omitted
`EnableDetectorVcpkg` would have gotten a passing scan that silently skipped the native
dependencies, so the template owns it now.

**A Windows agent** with Visual Studio 2022 and vcpkg, exposing `VCPKG_INSTALLATION_ROOT`. The
Microsoft-hosted `windows-2025` image qualifies.

**Outbound network access.** The test jobs reach several public endpoints, which is worth checking
before wiring this up on a restricted network:

- PSGallery, for `Microsoft.WinGet.Client` and `platyPS`
- nuget.org, for the `VisualStudioTestPlatformInstaller` feed selector
- the `winget` source, for `Microsoft.Sysinternals.PsTools`

## `preSteps`

Each job template takes a `preSteps` step list, injected ahead of every winget-cli step. It exists
because a job template cannot know what a consuming repository needs done to the agent first.

The case that forced it: MSBuild resolves `global.json` by walking up from the *project* directory,
so it walks straight out of the subtree and into the consuming repository's root. If that repository
pins a .NET SDK version, every restore in these jobs inherits the pin — and fails if the agent does
not have that exact SDK. winget-cli has no `global.json` of its own, so this never happens here and
this repository's pipeline cannot catch it.

```yaml
- template: subtree/templates/jobs-build.yml
  parameters:
    preSteps:
    - task: UseDotNet@2
      inputs:
        useGlobalJson: true
```

The same hook suits feed authentication (`NuGetAuthenticate@1`) and any other agent preparation.
Keep it to environment setup: steps that build or test winget-cli belong in the templates, where
both consumers get them.

## `additionalMSBuildArgs`

`jobs-build.yml` appends this string to both of its build tasks. It exists for the other half of the
walk-up problem: a consuming repository's `Directory.Build.props` can not only *break* our build but
silently *reconfigure* it, and unlike a path there is nothing for `sourceRoot` to fix.

`src\Directory.Build.props` imports any `Directory.Build.props` found above winget-cli, by design,
so a consumer's settings reach every project under `src\`. If those settings turn on the release
switches this repository defines — `WingetDisableTestHooks`, `UseProdCLSIDs`, `UseProdWingetServer`,
`WingetEnableReleaseBuild` — the result is a *product* build, not the build this pipeline's tests
expect. `WingetDisableTestHooks` compiles out members that the test projects use, so the solution
does not even build.

```yaml
- template: subtree/templates/jobs-build.yml
  parameters:
    additionalMSBuildArgs: >-
      /p:WingetDisableTestHooks=false
      /p:UseProdCLSIDs=false
```

These must be command line properties. A consumer that assigns the property in a `.props` file
assigns it unconditionally, and only a global property — which is what `/p:` creates — takes
precedence over that.

## `releaseTagJob`

`jobs-build.yml` stamps a build version obtained from a separate job. That job is named by the
`releaseTagJob` parameter, which defaults to `GetReleaseTag`:

```yaml
- job: 'GetReleaseTag'
  steps:
  - task: PowerShell@2
    name: 'GetTag'          # the job must expose a 'tag' output under this step name
    ...
```

The name is load-bearing in two places — `dependsOn`, and the `BuildVer` counter that reads
`dependencies.<job>.outputs['GetTag.tag']`.

Pass an empty string when the consuming pipeline has no such job:

```yaml
- template: external/pkg/templates/jobs-build.yml
  parameters:
    releaseTagJob: ''
```

That drops the `dependsOn`, the `BuildVer` variable, and the *Update Binary Version* step together,
leaving the binaries with their checked-in version. For a validation-only pipeline that is usually
what you want, since nothing is being released.

## Job graph

`Build` publishes one artifact per matrix leg, named `Build.x86release` and `Build.x64release`.
`Test` and `BuildPowerShellModule` both depend on `Build` and consume those artifacts by name, so the
three jobs travel together.

```
[releaseTagJob] ──> Build ──┬──> Test
                            └──> BuildPowerShellModule
```

## Worked example

A validation pipeline in a repository that carries winget-cli at `subtree`:

```yaml
pool:
  vmImage: 'windows-2025'

jobs:
- template: subtree/templates/jobs-build.yml
  parameters:
    sourceRoot: $(Build.SourcesDirectory)\subtree
    releaseTagJob: ''

- template: subtree/templates/jobs-test.yml
  parameters:
    sourceRoot: $(Build.SourcesDirectory)\subtree

- template: subtree/templates/jobs-powershell-module.yml
  parameters:
    sourceRoot: $(Build.SourcesDirectory)\subtree
```

The `- template:` paths are relative to the consuming repository's root, since that is where the
entry pipeline lives. The `sourceRoot` values are spelled out rather than held in a variable, because
a variable referenced from a template's `variables:` block relies on nested macro expansion that is
easy to get subtly wrong.

To stamp a version instead, add a job that produces the tag and name it in `releaseTagJob`;
`azure-pipelines.yml` in this repository is the worked example of that arrangement.

## Editing these templates

The failure mode to watch for is a path that works at the repository root and only breaks in the
subtree, which this repository's own pipeline will not catch. When adding a task:

- Root anything inside the winget-cli tree at `${{ parameters.sourceRoot }}`.
- Never leave a `filePath`, `solution`, `restoreSolution`, `projects`, or `workingDirectory` value
  as a repo-relative path such as `src\...`. Those resolve against the *consuming* repository.
- Prefer `${{ parameters.sourceRoot }}` over `$(Build.SourcesDirectory)` for source files. The latter
  is the consuming repository's root, which is only the same thing at depth zero.
- Watch globs. `**/*.csproj` looks harmless but would restore the entire consuming repository.
- Keep MSBuild work inside `src\`, which is where winget-cli's `Directory.Build.props` and
  `Directory.Packages.props` sit. See below.

## The `src\` boundary

MSBuild and NuGet find `Directory.Build.props`, `Directory.Packages.props`, and `global.json` by
walking **up** from each project directory until they hit one. winget-cli has none at its root, so
that walk leaves the subtree and lands in the consuming repository, which silently applies its
settings to our projects. This is the one hazard `sourceRoot` cannot address: nothing here is a path.

In practice `src\Directory.Packages.props` does stop the walk for package versions, which is why the
`samples\` projects — which have no such file above them inside winget-cli — were the ones that broke
first, and why the restore glob above is scoped to `src\` rather than to `sourceRoot`. (They are in
no solution this pipeline builds, so nothing is lost.)

`src\Directory.Build.props` is different, and worth reading before assuming anything under `src\` is
insulated. Its first line is an explicit `GetPathOfFileAbove` import: *"Consume containing solution
build props if present."* It deliberately chains to whatever sits above winget-cli, so a consumer's
properties reach every project under `src\` by design, not by accident. That is what
`additionalMSBuildArgs` is for. `global.json` has no guard of any kind, which is what `preSteps` is
for.

Two consequences when editing: build only what lives under `src\`, and expect a consumer to hit all
of this before we do, since at depth zero the walk finds nothing and everything looks fine.
