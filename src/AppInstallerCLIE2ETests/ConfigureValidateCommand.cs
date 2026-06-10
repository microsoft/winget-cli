// -----------------------------------------------------------------------------
// <copyright file="ConfigureValidateCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// `Configure validate` command tests.
    /// </summary>
    public class ConfigureValidateCommand
    {
        private const string Command = "configure validate";

        /// <summary>
        /// Set up.
        /// </summary>
        [OneTimeSetUp]
        public void BaseSetup()
        {
            TestCommon.SetupTestSource(false);
        }

        /// <summary>
        /// Tear down.
        /// </summary>
        [OneTimeTearDown]
        public void BaseTeardown()
        {
            TestCommon.TearDownTestSource();
        }

        /// <summary>
        /// The configuration file is empty.
        /// </summary>
        [Test]
        public void EmptyFile()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\Empty.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_INVALID_YAML, result.ExitCode);
        }

        /// <summary>
        /// The configuration file is not configuration YAML.
        /// </summary>
        [Test]
        public void NotConfigurationYAML()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\NotConfig.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_MISSING_FIELD, result.ExitCode);
            Assert.True(result.StdOut.Contains("$schema"));
            Assert.True(result.StdOut.Contains("missing"));
        }

        /// <summary>
        /// The configuration file does not specify the schema version.
        /// </summary>
        [Test]
        public void NoVersion()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\NoVersion.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_MISSING_FIELD, result.ExitCode);
            Assert.True(result.StdOut.Contains("configurationVersion"));
            Assert.True(result.StdOut.Contains("missing"));
        }

        /// <summary>
        /// The configuration file schema version is not known.
        /// </summary>
        [Test]
        public void UnknownVersion()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\UnknownVersion.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION, result.ExitCode);
            Assert.True(result.StdOut.Contains("Configuration file version"));
            Assert.True(result.StdOut.Contains("is not known."));
        }

        /// <summary>
        /// The resources node is not the correct type in YAML.
        /// </summary>
        [Test]
        public void ResourcesIsWrongType()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\ResourcesNotASequence.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_INVALID_FIELD_TYPE, result.ExitCode);
            Assert.True(result.StdOut.Contains("resources"));
            Assert.True(result.StdOut.Contains("wrong type"));
        }

        /// <summary>
        /// The unit node is not the correct type in YAML.
        /// </summary>
        [Test]
        public void UnitIsWrongType()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\UnitNotAMap.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_INVALID_FIELD_TYPE, result.ExitCode);
            Assert.True(result.StdOut.Contains("resources[0]"));
            Assert.True(result.StdOut.Contains("wrong type"));
        }

        /// <summary>
        /// The resource name is missing.
        /// </summary>
        [Test]
        public void NoResourceName()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\NoResourceName.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_INVALID_FIELD_VALUE, result.ExitCode);
            Assert.True(result.StdOut.Contains("resource"));
            Assert.True(result.StdOut.Contains("invalid value"));
            Assert.True(result.StdOut.Contains("Module/"));
        }

        /// <summary>
        /// The resource name module does not match the directives module.
        /// </summary>
        [Test]
        public void ModuleMismatch()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\ModuleMismatch.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_INVALID_FIELD_VALUE, result.ExitCode);
            Assert.True(result.StdOut.Contains("module"));
            Assert.True(result.StdOut.Contains("invalid value"));
            Assert.True(result.StdOut.Contains("DifferentModule"));
        }

        /// <summary>
        /// The configuration contains multiple resources with the same identifier.
        /// </summary>
        [Test]
        public void DuplicateIdentifiers()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\DuplicateIdentifiers.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_DUPLICATE_IDENTIFIER, result.ExitCode);
            Assert.True(result.StdOut.Contains("The configuration contains the identifier `same` multiple times."));
            Assert.False(result.StdOut.Contains("NotMentioned"));
        }

        /// <summary>
        /// The configuration does not contain the dependency.
        /// </summary>
        [Test]
        public void MissingDependency()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\MissingDependency.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_MISSING_DEPENDENCY, result.ExitCode);
            Assert.True(result.StdOut.Contains("The dependency `missing` was not found within the configuration."));
            Assert.False(result.StdOut.Contains("xE2ETestResource"));
        }

        /// <summary>
        /// The configuration contains a dependency cycle.
        /// </summary>
        [Test]
        public void DependencyCycle()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\DependencyCycle.yml"));
            Assert.AreEqual(Constants.ErrorCode.CONFIG_ERROR_SET_DEPENDENCY_CYCLE, result.ExitCode);
            Assert.True(result.StdOut.Contains("This configuration unit is part of a dependency cycle."));
            Assert.False(result.StdOut.Contains("NotMentioned"));
        }

        /// <summary>
        /// The configuration unit is not available in a public catalog.
        /// </summary>
        [Test]
        public void ResourceIsNotPublic()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\Configure_TestRepo.yml"));
            Assert.AreEqual(Constants.ErrorCode.S_FALSE, result.ExitCode);
            Assert.True(result.StdOut.Contains("not available publicly"));
        }

        /// <summary>
        /// The configuration unit is not found.
        /// </summary>
        [Test]
        public void ResourceIsNotFound()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\ResourceNotFound.yml"));
            Assert.AreEqual(Constants.ErrorCode.S_FALSE, result.ExitCode);
            Assert.True(result.StdOut.Contains("The configuration unit could not be found."));
        }

        /// <summary>
        /// The module was not provided.
        /// </summary>
        [Test]
        public void ModuleNotProvided()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\PSGallery_NoModule_NoSettings.yml"), timeOut: 120000);
            Assert.AreEqual(Constants.ErrorCode.S_FALSE, result.ExitCode);
            Assert.True(result.StdOut.Contains("The module was not provided."));
        }

        /// <summary>
        /// No issues detected (yet).
        /// </summary>
        [Test]
        public void NoIssuesDetected()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\PSGallery_NoSettings.yml"), timeOut: 120000);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Validation found no issues."));
        }

        /// <summary>
        /// No issues detected (yet) from https configuration file.
        /// </summary>
        [Test]
        public void NoIssuesDetected_HttpsConfigurationFile()
        {
            var result = TestCommon.RunAICLICommand(Command, $"{Constants.TestSourceUrl}/TestData/Configuration/PSGallery_NoSettings.yml", timeOut: 120000);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Validation found no issues."));
        }

        /// <summary>
        /// No issues detected from WinGet resource units.
        /// </summary>
        [Test]
        public void NoIssuesDetected_WinGetDscResource()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\WinGetDscResourceValidate_Good.yml"), timeOut: 120000);
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Validation found no issues."));
        }

        /// <summary>
        /// No issues detected from WinGet resource units.
        /// </summary>
        [Test]
        public void ValidateWinGetDscResource_DependencySourceMissing()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\WinGetDscResourceValidate_DependencySourceMissing.yml"), timeOut: 120000);
            Assert.AreEqual(Constants.ErrorCode.S_FALSE, result.ExitCode);
            Assert.True(result.StdOut.Contains("WinGetPackage configuration unit package depends on a third-party source not previously configured. Package Id: AppInstallerTest.TestExeInstaller; Source: TestSource"));
        }

        /// <summary>
        /// No issues detected from WinGet resource units.
        /// </summary>
        [Test]
        public void ValidateWinGetDscResource_PackageNotFound()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\WinGetDscResourceValidate_PackageNotFound.yml"), timeOut: 120000);
            Assert.AreEqual(Constants.ErrorCode.S_FALSE, result.ExitCode);
            Assert.True(result.StdOut.Contains("WinGetPackage configuration unit package cannot be validated. Package not found. Package Id: AppInstallerTest.DoesNotExist"));
        }

        /// <summary>
        /// No issues detected from WinGet resource units.
        /// </summary>
        [Test]
        public void ValidateWinGetDscResource_PackageVersionNotFound()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\WinGetDscResourceValidate_PackageVersionNotFound.yml"), timeOut: 120000);
            Assert.AreEqual(Constants.ErrorCode.S_FALSE, result.ExitCode);
            Assert.True(result.StdOut.Contains("WinGetPackage configuration unit package cannot be validated. Package version not found. Package Id: AppInstallerTest.TestExeInstaller; Version 101.0.101.0"));
        }

        /// <summary>
        /// No issues detected from WinGet resource units.
        /// </summary>
        [Test]
        public void ValidateWinGetDscResource_SourceOpenFailed()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\WinGetDscResourceValidate_SourceOpenFailed.yml"), timeOut: 120000);
            Assert.AreEqual(Constants.ErrorCode.S_FALSE, result.ExitCode);
            Assert.True(result.StdOut.Contains("WinGetPackage configuration unit package cannot be validated. Source open failed. Package Id: AppInstallerTest.TestExeInstaller; Source: TestSourceV2"));
        }

        /// <summary>
        /// No issues detected from WinGet resource units.
        /// </summary>
        [Test]
        public void ValidateWinGetDscResource_VersionSpecifiedWithOnlyOneVersionAvailable()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\WinGetDscResourceValidate_VersionSpecifiedWithOnlyOneVersionAvailable.yml"), timeOut: 120000);
            Assert.AreEqual(Constants.ErrorCode.S_FALSE, result.ExitCode);
            Assert.True(result.StdOut.Contains("WinGetPackage configuration unit package specified with a specific version while only one package version is available. Package Id: AppInstallerTest.TestValidManifest; Version: 1.0.0.0"));
        }

        /// <summary>
        /// No issues detected from WinGet resource units.
        /// </summary>
        [Test]
        public void ValidateWinGetDscResource_VersionSpecifiedWithUseLatest()
        {
            var result = TestCommon.RunAICLICommand(Command, TestCommon.GetTestDataFile("Configuration\\WinGetDscResourceValidate_VersionSpecifiedWithUseLatest.yml"), timeOut: 120000);
            Assert.AreEqual(Constants.ErrorCode.S_FALSE, result.ExitCode);
            Assert.True(result.StdOut.Contains("WinGetPackage declares both UseLatest and Version. Package Id: AppInstallerTest.TestExeInstaller"));
        }
    }
}
