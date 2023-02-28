// -----------------------------------------------------------------------------
// <copyright file="ConfigurationDetailsTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Management.Automation;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.Unit;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Windows.Security.Cryptography.Certificates;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Tests for ConfigurationUnitProcessorDetails and ConfigurationUnitSettingDetails.
    /// </summary>
    [Collection("UnitTestCollection")]
    public class ConfigurationDetailsTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationDetailsTests"/> class.
        /// </summary>
        public ConfigurationDetailsTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Tests creating ConfigurationUnitProcessorDetails and ConfigurationUnitSettingDetails with different inputs.
        /// </summary>
        /// <param name="hasDscInfo">Has dsc info.</param>
        /// <param name="hasPSModuleInfo">Has ps module info.</param>
        /// <param name="hasGetModuleInfo">Has get module info.</param>
        /// <param name="hasCerts">Has certs.</param>
        [Theory]
        [InlineData(false, false, false, false)]
        [InlineData(false, false, false, true)]
        [InlineData(false, false, true, false)]
        [InlineData(false, false, true, true)]
        [InlineData(false, true, false, false)]
        [InlineData(false, true, false, true)]
        [InlineData(false, true, true, false)]
        [InlineData(false, true, true, true)]
        [InlineData(true, false, false, false)]
        [InlineData(true, false, false, true)]
        [InlineData(true, false, true, false)]
        [InlineData(true, false, true, true)]
        [InlineData(true, true, false, false)]
        [InlineData(true, true, false, true)]
        [InlineData(true, true, true, false)]
        [InlineData(true, true, true, true)]
        public void ConfigurationUnitProcessorDetails_CreationTest(bool hasDscInfo, bool hasPSModuleInfo, bool hasGetModuleInfo, bool hasCerts)
        {
            List<Certificate>? certsInput = null;
            if (hasCerts)
            {
                certsInput = new List<Certificate>();
            }

            if (!hasDscInfo && !hasPSModuleInfo && !hasGetModuleInfo)
            {
                Assert.Throws<ArgumentException>(
                    () => new ConfigurationUnitProcessorDetails("unitName", null, null, null, certsInput));
            }
            else
            {
                var unit = this.CreteConfigurationUnit();
                var (dscResourceInfo, psModuleInfo) = this.GetResourceAndModuleInfo(unit);

                DscResourceInfoInternal? dscResourceInfoInput = null;
                if (hasDscInfo)
                {
                    dscResourceInfoInput = dscResourceInfo;
                }

                PSModuleInfo? psModuleInfoInput = null;
                if (hasPSModuleInfo)
                {
                    psModuleInfoInput = psModuleInfo;
                }

                PSObject? getModuleInfo = null;
                if (hasGetModuleInfo)
                {
                    getModuleInfo = this.CreateGetModuleInfo();
                }

                var details = new ConfigurationUnitProcessorDetails(unit.UnitName, dscResourceInfoInput, psModuleInfoInput, getModuleInfo, certsInput);

                Assert.Equal(unit.UnitName, details.UnitName);

                if (hasDscInfo)
                {
                    Assert.True(details.IsLocal);
                    Assert.Equal("xSimpleTestResource", details.ModuleName);
                    Assert.Equal("0.0.0.1", details.Version);
                    Assert.NotNull(details.Settings);
                    Assert.True(details.Settings.Count == 5);

                    var pathSetting = details.Settings.Where(s => s.Name == "Path").FirstOrDefault();
                    Assert.NotNull(pathSetting);
                    Assert.Equal("[string]", pathSetting.Type);
                    Assert.True(pathSetting.IsRequired);
                    Assert.Equal(string.Empty, pathSetting.Semantics);

                    var contentSetting = details.Settings.Where(s => s.Name == "Content").FirstOrDefault();
                    Assert.NotNull(contentSetting);
                    Assert.Equal("[string]", contentSetting.Type);
                    Assert.False(contentSetting.IsRequired);
                    Assert.Equal(string.Empty, contentSetting.Semantics);

                    var dependsOnSetting = details.Settings.Where(s => s.Name == "DependsOn").FirstOrDefault();
                    Assert.NotNull(dependsOnSetting);
                    Assert.Equal("[string[]]", dependsOnSetting.Type);
                    Assert.False(dependsOnSetting.IsRequired);
                    Assert.Equal(string.Empty, dependsOnSetting.Semantics);

                    var ensureSetting = details.Settings.Where(s => s.Name == "Ensure").FirstOrDefault();
                    Assert.NotNull(ensureSetting);
                    Assert.Equal("[string]", ensureSetting.Type);
                    Assert.False(ensureSetting.IsRequired);
                    Assert.Equal("Absent, Present", ensureSetting.Semantics);

                    var psDscRunAsCredentialSetting = details.Settings.Where(s => s.Name == "PsDscRunAsCredential").FirstOrDefault();
                    Assert.NotNull(psDscRunAsCredentialSetting);
                    Assert.Equal("[PSCredential]", psDscRunAsCredentialSetting.Type);
                    Assert.False(psDscRunAsCredentialSetting.IsRequired);
                    Assert.Equal(string.Empty, psDscRunAsCredentialSetting.Semantics);
                }

                if (hasPSModuleInfo)
                {
                    Assert.Equal(new Uri("https://www.contoso.com/help"), details.ModuleDocumentationUri);
                    Assert.Equal(new Uri("https://www.contoso.com/icons/icon.png"), details.UnitIconUri);
                    Assert.Equal("xSimpleTestResource", details.ModuleName);
                    Assert.Equal(ModuleType.Script.ToString(), details.ModuleType);
                    Assert.Equal("PowerShell module with DSC resources for unit tests", details.ModuleDescription);
                    Assert.Equal(new Uri("https://github.com/microsoft/winget-cli"), details.PublishedModuleUri);
                    Assert.Equal("0.0.0.1", details.Version);
                    Assert.Equal("Luffytaro", details.Author);
                    Assert.Equal("Microsoft Corporation", details.Publisher);
                }

                if (hasGetModuleInfo)
                {
                    Assert.Equal("PSGallery", details.ModuleSource);
                    Assert.Equal(new DateTimeOffset(new DateTime(2017, 12, 10)), details.PublishedDate);
                    Assert.Equal(new Uri("https://www.contoso.com/icons/icon.png"), details.UnitIconUri);
                    Assert.Equal("xSimpleTestResource", details.ModuleName);
                    Assert.Equal("PowerShell module with DSC resources for unit tests", details.ModuleDescription);
                    Assert.Equal("0.0.0.1", details.Version);
                    Assert.Equal("Luffytaro", details.Author);
                    Assert.Equal("Microsoft Corporation", details.Publisher);
                }

                if (hasCerts)
                {
                    Assert.NotNull(details.SigningInformation);
                }
            }
        }

        private ConfigurationUnit CreteConfigurationUnit()
        {
            var unit = new ConfigurationUnit();
            unit.UnitName = "SimpleFileResource";
            unit.Directives.Add("module", "xSimpleTestResource");
            unit.Directives.Add("version", "0.0.0.1");

            return unit;
        }

        private (DscResourceInfoInternal dscResourceInfo, PSModuleInfo psModuleInfo) GetResourceAndModuleInfo(ConfigurationUnit unit)
        {
            // This is easier than trying to mock sealed class from external code...
            var testEnv = this.fixture.PrepareTestProcessorEnvironment();
            testEnv.ValidateRunspace();
            var dscResourceInfo = testEnv.GetDscResource(new ConfigurationUnitInternal(unit, null));
            var psModuleInfo = testEnv.GetAvailableModule(PowerShellHelpers.CreateModuleSpecification("xSimpleTestResource", "0.0.0.1"));

            if (dscResourceInfo is null || psModuleInfo is null)
            {
                throw new ArgumentNullException("Test processor environment not set correctly");
            }

            return (dscResourceInfo, psModuleInfo);
        }

        private PSObject CreateGetModuleInfo()
        {
            return new PSObject(new
            {
                Repository = "PSGallery",
                PublishedDate = new DateTime(2017, 12, 10),
                IconUri = "https://www.contoso.com/icons/icon.png",
                Name = "xSimpleTestResource",
                Description = "PowerShell module with DSC resources for unit tests",
                RepositorySourceLocation = "https://github.com/microsoft/winget-cli",
                Version = "0.0.0.1",
                Author = "Luffytaro",
                CompanyName = "Microsoft Corporation",
            });
        }
    }
}
