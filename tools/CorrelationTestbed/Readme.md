# E2E correlation testing
This directory holds a few scripts and a test project, all centered around enabling end-to-end validation of our correlation between system artifacts and packages in external sources.

The test project uses the COM API to first install a package, then attempts to check for correlation in two ways (directions):
1. When the remote package identity is known, the caller will usually look it up via that remote identity.  This path can enable additional information to be retrieved to make the correlation with.
2. When listing all of the local packages, the remote identity must be determined for each one. We can use only data known locally to make the correlation.

The primary script, `Test-CorrelationInSandbox.ps1`, is based off of the sandbox test script in winget-pkgs (thanks to many people).  It sets up the sandbox and initial script to run there for each package to test, then waits for a sentinel file to be created in the output location.  The results of all of running the test project exe, as well as the ARP differences and winget logs are put in that output location, then the sandbox is destroyed for the next package to run.

```
Test-CorrelationInSandbox.ps1
-- Required --
[[-PackageIdentifiers] <string[]>] :: A set of package ids to test
[[-Source] <string>] :: The name of the source that the packages are from, ex. "winget"

-- Optional --
[-ExePath <string>] :: Path to the test exe; defaults to the Release output location
[-UseDev] :: Switch to use the local dev winget build
[-DevPackagePath <string>] :: Path to the local dev *Release* winget build; defaults to the normal location
[-ResultsPath <string>] :: Path to output the results to; defaults to a temp directory
[-RegFileDirectory <string>] :: Path to a directory containing .reg files to insert before the test, creating noise for correlation
```

Once testing is done, `Process-CorrelationResults.ps1` will take all of the results JSON files and put them into `results.csv` in the directory.  If any tests failed to run, they will be in `failed.csv`.  There are correlation columns in the CSV that can be averaged in Excel to get the correlation percentage.

## Running a test
### Setup
First you must have built the test exe located in `InstallAndCheckCorrelation`. By default the Release x64 version is picked up by the script, so it is easiest to build that one.

If you want to run against the local dev build of the winget COM server, the default is again to use Release x64.  The sandbox will not run the debug build for unknown reasons currently.

### Run the test pass
Run the `Test-CorrelationInSandbox.ps1` script, then wait for a while since it is going to download and install every package serially, with a little bit of overhead in between.

A simple example call is:
```
Test-CorrelationInSandbox.ps1 -PackageIdentifiers @("Microsoft.VisualStudioCode") -Source winget
```
This will use the latest version of winget available on github.  Adding `-UseDev` should be enough to use the local build instead if Release x64 is already deployed onto the host machine.

### Collate the results
Running `Process-CorrelationResults.ps1` on the directory output at the end of `Test-CorrelationInSandbox.ps1` will place a CSV file with the results combined together.  These results can be inspected for correctness and an overall correlation score determined from the different correlation paths.