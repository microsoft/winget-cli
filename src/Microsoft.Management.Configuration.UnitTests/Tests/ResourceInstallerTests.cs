// -----------------------------------------------------------------------------
// <copyright file="ResourceInstallerTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using Microsoft.Management.Configuration.Processor.Constants;
    using Microsoft.Management.Configuration.Processor.DscModule;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.Runspaces;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.PowerShell.Commands;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Test module installer.
    /// This test uses internet. It should go in an E2E project eventually.
    /// </summary>
    [Collection("UnitTestCollection")]
    public class ResourceInstallerTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ResourceInstallerTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ResourceInstallerTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Find the module for the resource.
        /// </summary>
        [Fact]
        public void FindDscResource_Test()
        {
            var hostedEnvironment = new HostedEnvironment(new DscModuleV2(true));
            var unitInternal = this.CreateUnitInternal();

            var resourceInstaller = new ResourceInstaller(hostedEnvironment.Runspace, unitInternal);

            var resourceModule = resourceInstaller.FindDscResource();

            Assert.NotNull(resourceModule);
        }

        /// <summary>
        /// Find the module for the resource.
        /// </summary>
        [Fact]
        public void FindDscResource_Test_NotFound()
        {
            var hostedEnvironment = new HostedEnvironment(new DscModuleV2(true));
            var configurationUnit = new ConfigurationUnit
            {
                UnitName = $"{Guid.NewGuid()}{Guid.NewGuid()}",
            };

            var resourceInstaller = new ResourceInstaller(
                hostedEnvironment.Runspace,
                new ConfigurationUnitInternal(configurationUnit, null));

            Assert.Throws<FindDscResourceNotFoundException>(() => resourceInstaller.FindDscResource());
        }

        /// <summary>
        /// Test verify module.
        /// TODO: XmlFileContentResource doesn't contain signed modules. Find one that does.
        /// </summary>
        [Fact]
        public void VerifyModule_Test()
        {
            var tmpDir = new TempDirectory();
            var hostedEnvironment = new HostedEnvironment(new DscModuleV2(true));
            var unitInternal = this.CreateUnitInternal();

            var resourceInstaller = new ResourceInstaller(hostedEnvironment.Runspace, unitInternal);

            var resourceModule = resourceInstaller.FindDscResource();
            Assert.NotNull(resourceModule);

            Assert.Throws<InvalidOperationException>(
                () => resourceInstaller.ValidateModule(resourceModule, tmpDir.FullDirectoryPath));
        }

        /// <summary>
        /// Test to verify signed files.
        /// TODO: Create a helper that will use the PSModule resource with ensure absent before
        /// running this.
        /// </summary>
        [Fact]
        public void InstallModule_Test()
        {
            var hostedEnvironment = new HostedEnvironment(new DscModuleV2(true));
            var unitInternal = this.CreateUnitInternal();

            var resourceInstaller = new ResourceInstaller(hostedEnvironment.Runspace, unitInternal);

            var resourceModule = resourceInstaller.FindDscResource();
            Assert.NotNull(resourceModule);

            resourceInstaller.InstallModule(resourceModule);
        }

        /// <summary>
        /// Creates a unit for XmlFileContentResource.
        /// </summary>
        /// <returns>Unit internal.</returns>
        private ConfigurationUnitInternal CreateUnitInternal()
        {
            var configurationUnit = new ConfigurationUnit
            {
                UnitName = "XmlFileContentResource",
            };

            configurationUnit.Directives.Add(DirectiveConstants.Module, "xmlcontentdsc");
            configurationUnit.Directives.Add(DirectiveConstants.Version, "0.0.1");

            return new ConfigurationUnitInternal(configurationUnit, null);
        }
    }
}
