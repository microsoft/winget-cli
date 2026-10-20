// -----------------------------------------------------------------------------
// <copyright file="DeltaSource.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Tests the delta index against the deployed package mechanism.
    ///
    /// Unit tests cover the shape of a delta source and the merging of a delta with its baseline,
    /// but they run unpackaged, so they can only ever reach the local file store. Everything here
    /// is about the half they structurally cannot touch: deploying the packages, keeping three
    /// separate deployed identities apart, and the clean up that follows a removal.
    /// </summary>
    public class DeltaSource : BaseCommand
    {
        /// <summary>
        /// The experimental feature, as it is named in the settings file.
        /// </summary>
        private const string DeltaFeatureName = "deltaIndex";

        /// <summary>
        /// A package that is in the baseline, and so is not one the delta has to supply.
        /// </summary>
        private const string BaselinePackageIdentifier = Constants.PortableExePackageId;

        /// <summary>
        /// One time set up.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            WinGetSettingsHelper.ConfigureFeature(DeltaFeatureName, true);
        }

        /// <summary>
        /// One time tear down.
        /// </summary>
        [OneTimeTearDown]
        public void OneTimeTeardown()
        {
            RemoveDeltaSource();
            WinGetSettingsHelper.ConfigureFeature(DeltaFeatureName, false);
        }

        /// <summary>
        /// Test set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            RemoveDeltaSource();
            WinGetSettingsHelper.ConfigureFeature(DeltaFeatureName, true);
        }

        /// <summary>
        /// Adding a delta capable source acquires the delta and the baseline rather than the full
        /// index, and what it then presents is the two of them merged.
        /// </summary>
        [Test]
        public void DeltaSourceAdd()
        {
            AddDeltaSource();

            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceDeltaIdentityName), Is.True, "The delta package should be deployed");
            Assert.That(
                GetDeployedPackageVersion(Constants.DeltaTestSourceBaselineIdentityName),
                Is.EqualTo(Constants.DeltaTestSourceBaselineVersion),
                "The baseline should have been deployed at its own version");

            // The baseline is a package of its own, so a source on the delta holds no full index at
            // all. Were the two to share an identity this could not be asked.
            Assert.That(
                IsPackageDeployed(Constants.DeltaTestSourceIdentityName),
                Is.False,
                "The full index should not have been acquired");

            // This package was left out of the baseline, so it exists only in the delta.
            RequireFound(Constants.DeltaOnlyPackageIdentifier);

            // And a package that is in the baseline is still there, so the delta did not displace it.
            RequireFound(BaselinePackageIdentifier);
        }

        /// <summary>
        /// A source added while the feature was off holds the full index, and goes on holding it.
        ///
        /// Which packages a source is made of is established when it is added and recorded in its
        /// details; an update reads those details and never probes for anything else. So turning
        /// the feature on does not move an existing source onto the delta, and the source has to be
        /// re-added for that. This is deliberate -- see the no-migration decision -- and it is
        /// asserted here because nothing about it is obvious from the outside.
        /// </summary>
        [Test]
        public void DeltaSourceNotAdoptedByExistingFullIndexSource()
        {
            WinGetSettingsHelper.ConfigureFeature(DeltaFeatureName, false);

            AddDeltaSource();

            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceDeltaIdentityName), Is.False, "No delta should be involved while the feature is off");
            Assert.That(
                GetDeployedPackageVersion(Constants.DeltaTestSourceIdentityName),
                Is.EqualTo(Constants.DeltaTestSourceVersion),
                "The full index should be deployed at its own version");

            WinGetSettingsHelper.ConfigureFeature(DeltaFeatureName, true);

            var updateResult = TestCommon.RunAICLICommand("source update", $"-n {Constants.DeltaTestSourceName}");
            Assert.That(updateResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(updateResult.StdOut, Does.Contain("Done"));

            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceDeltaIdentityName), Is.False, "The update should not have acquired a delta");
            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceBaselineIdentityName), Is.False, "The update should not have acquired a baseline");
            Assert.That(
                GetDeployedPackageVersion(Constants.DeltaTestSourceIdentityName),
                Is.EqualTo(Constants.DeltaTestSourceVersion),
                "The source should still be served by the full index");

            // The full index holds everything, so what the source presents is unchanged.
            RequireFound(Constants.DeltaOnlyPackageIdentifier);
            RequireFound(BaselinePackageIdentifier);
        }

        /// <summary>
        /// Disabling delta after adding a source acquires the full index and keeps it usable.
        /// </summary>
        /// <param name="updateSource">Whether to explicitly update before searching.</param>
        [TestCase(false)]
        [TestCase(true)]
        public void DeltaSourceFeatureDisabled(bool updateSource)
        {
            AddDeltaSource();

            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceDeltaIdentityName), Is.True);
            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceBaselineIdentityName), Is.True);
            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceIdentityName), Is.False);
            RequireFound(Constants.DeltaOnlyPackageIdentifier);
            RequireFound(BaselinePackageIdentifier);

            WinGetSettingsHelper.ConfigureFeature(DeltaFeatureName, false);

            if (updateSource)
            {
                var updateResult = TestCommon.RunAICLICommand("source update", $"-n {Constants.DeltaTestSourceName}");
                Assert.That(updateResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
                Assert.That(updateResult.StdOut, Does.Contain("Done"));
            }

            RequireFound(Constants.DeltaOnlyPackageIdentifier);
            RequireFound(BaselinePackageIdentifier);
            Assert.That(
                GetDeployedPackageVersion(Constants.DeltaTestSourceIdentityName),
                Is.EqualTo(Constants.DeltaTestSourceVersion),
                "The source should be served by the full index with delta disabled");

            var removeResult = TestCommon.RunAICLICommand("source remove", $"-n {Constants.DeltaTestSourceName}");
            Assert.That(removeResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceIdentityName), Is.False);
            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceDeltaIdentityName), Is.False);
            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceBaselineIdentityName), Is.False);
        }

        /// <summary>
        /// Removing the source takes both deployed packages with it. A delta source leaves more
        /// behind than an ordinary one, so there is more to get wrong.
        /// </summary>
        [Test]
        public void DeltaSourceRemove()
        {
            AddDeltaSource();

            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceDeltaIdentityName), Is.True);
            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceBaselineIdentityName), Is.True);

            var removeResult = TestCommon.RunAICLICommand("source remove", $"-n {Constants.DeltaTestSourceName}");
            Assert.That(removeResult.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(removeResult.StdOut, Does.Contain("Done"));

            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceDeltaIdentityName), Is.False, "The delta package should have been removed");
            Assert.That(IsPackageDeployed(Constants.DeltaTestSourceBaselineIdentityName), Is.False, "The baseline package should have been removed");

            var listResult = TestCommon.RunAICLICommand("source list", $"-n {Constants.DeltaTestSourceName}");
            Assert.That(listResult.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_SOURCE_NAME_DOES_NOT_EXIST));
        }

        /// <summary>
        /// Adds the delta source, requiring that it succeed.
        /// </summary>
        private static void AddDeltaSource()
        {
            var result = TestCommon.RunAICLICommand("source add", $"{Constants.DeltaTestSourceName} {Constants.DeltaTestSourceUrl} --trust-level trusted");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Done"));
        }

        /// <summary>
        /// Removes the delta source, tolerating its not having been added.
        /// </summary>
        private static void RemoveDeltaSource()
        {
            TestCommon.RunAICLICommand("source remove", $"-n {Constants.DeltaTestSourceName}");
        }

        /// <summary>
        /// Requires that the delta source presents the given package.
        /// </summary>
        /// <param name="packageIdentifier">The package identifier.</param>
        private static void RequireFound(string packageIdentifier)
        {
            var result = TestCommon.RunAICLICommand("search", $"{packageIdentifier} --source {Constants.DeltaTestSourceName}");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK), $"{packageIdentifier} should be found");
            Assert.That(result.StdOut, Does.Contain(packageIdentifier));
        }

        /// <summary>
        /// Whether a package with the given identity name is deployed for the current user.
        /// </summary>
        /// <param name="identityName">The package identity name.</param>
        /// <returns>True if it is deployed.</returns>
        private static bool IsPackageDeployed(string identityName)
        {
            return !string.IsNullOrEmpty(GetDeployedPackageVersion(identityName));
        }

        /// <summary>
        /// Gets the version of a deployed package, or an empty string when it is not deployed.
        /// </summary>
        /// <param name="identityName">The package identity name.</param>
        /// <returns>The version, or an empty string.</returns>
        private static string GetDeployedPackageVersion(string identityName)
        {
            // The name has to match exactly, which matters here because the delta's identity name
            // has the full index's as a prefix.
            var result = TestCommon.RunCommandWithResult(
                "powershell",
                $"-Command \"(Get-AppxPackage | Where-Object {{ $_.Name -ceq '{identityName}' }} | Select-Object -First 1).Version\"");

            return result.StdOut.Trim();
        }
    }
}
