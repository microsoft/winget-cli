// -----------------------------------------------------------------------------
// <copyright file="ConfigurationSetProcessorTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.Set;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.PowerShell.Commands;
    using Moq;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for configuration processor tests.
    /// </summary>
    [Collection("UnitTestCollection")]
    public class ConfigurationSetProcessorTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationSetProcessorTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationSetProcessorTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Test CreateUnitProcessor. Happy path.
        /// </summary>
        [Fact]
        public void CreateUnitProcessor_ResourceExists()
        {
            string resourceName = "xResourceName";
            string moduleName = "xModuleName";
            Version version = new Version("1.0");

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                    m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(c => c.Unit.UnitName == resourceName)))
                .Returns(new DscResourceInfoInternal(resourceName, moduleName, version))
                .Verifiable();

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                UnitName = resourceName,
            };
            unit.Directives.Add("module", moduleName);
            unit.Directives.Add("version", version.ToString());

            var unitProcessor = configurationSetProcessor.CreateUnitProcessor(unit, null);
            Assert.NotNull(unitProcessor);
            Assert.Equal(unit.UnitName, unitProcessor.Unit.UnitName);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Test CreateUnitProcessor with no version directive.
        /// </summary>
        [Fact]
        public void CreateUnitProcessor_ResourceExists_NoVersionDirective()
        {
            string resourceName = "xResourceName";
            string moduleName = "xModuleName";
            Version version = new Version("1.0.0.0");

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                    m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(c => c.Unit.UnitName == resourceName)))
                .Returns(new DscResourceInfoInternal(resourceName, moduleName, version))
                .Verifiable();

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                UnitName = resourceName,
            };
            unit.Directives.Add("module", moduleName);

            var unitProcessor = configurationSetProcessor.CreateUnitProcessor(unit, null);
            Assert.NotNull(unitProcessor);
            Assert.Equal(unit.UnitName, unitProcessor.Unit.UnitName);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Test CreateUnitProcessor with no module directive.
        /// </summary>
        [Fact]
        public void CreateUnitProcessor_ResourceExists_NoModuleDirective()
        {
            string resourceName = "xResourceName";
            string moduleName = "xModuleName";
            Version version = new Version("1.0");

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                    m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(c => c.Unit.UnitName == resourceName)))
                .Returns(new DscResourceInfoInternal(resourceName, moduleName, version))
                .Verifiable();

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                UnitName = resourceName,
            };

            var unitProcessor = configurationSetProcessor.CreateUnitProcessor(unit, null);
            Assert.NotNull(unitProcessor);
            Assert.Equal(unit.UnitName, unitProcessor.Unit.UnitName);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Tests Creating a unit processor by downloading the resource.
        /// </summary>
        [Fact]
        public void CreateUnitProcessor_InstallResource()
        {
            string resourceName = "xResourceName";
            string moduleName = "xModuleName";
            Version version = new Version("1.0");

            DscResourceInfoInternal? nullResource = null;
            DscResourceInfoInternal dscResourceInfo = new DscResourceInfoInternal(resourceName, moduleName, version);
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.SetupSequence(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(c => c.Unit.UnitName == resourceName)))
                .Returns(nullResource)
                .Returns(dscResourceInfo);

            PSObject findDscResourceResult = new PSObject(processorEnvMock);
            processorEnvMock.Setup(
                m => m.FindDscResource(It.Is<ConfigurationUnitInternal>(c => c.Unit.UnitName == resourceName)))
                .Returns(findDscResourceResult)
                .Verifiable();

            processorEnvMock.Setup(
                m => m.InstallModule(findDscResourceResult))
                .Verifiable();

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                UnitName = resourceName,
            };
            unit.Directives.Add("module", moduleName);
            unit.Directives.Add("version", version.ToString());

            var unitProcessor = configurationSetProcessor.CreateUnitProcessor(unit, null);
            Assert.NotNull(unitProcessor);
            Assert.Equal(unit.UnitName, unitProcessor.Unit.UnitName);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Tests Creating a unit processor by downloading the resource.
        /// </summary>
        [Fact]
        public void CreateUnitProcessor_InstallResource_NotFoundAfterInstall()
        {
            string resourceName = "xResourceName";
            string moduleName = "xModuleName";
            Version version = new Version("1.0");

            DscResourceInfoInternal? nullResource = null;
            DscResourceInfoInternal dscResourceInfo = new DscResourceInfoInternal(resourceName, moduleName, version);
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(c => c.Unit.UnitName == resourceName)))
                .Returns(nullResource);

            PSObject findDscResourceResult = new PSObject(processorEnvMock);
            processorEnvMock.Setup(
                m => m.FindDscResource(It.Is<ConfigurationUnitInternal>(c => c.Unit.UnitName == resourceName)))
                .Returns(findDscResourceResult)
                .Verifiable();

            processorEnvMock.Setup(
                m => m.InstallModule(findDscResourceResult))
                .Verifiable();

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                UnitName = resourceName,
            };
            unit.Directives.Add("module", moduleName);
            unit.Directives.Add("version", version.ToString());

            Assert.Throws<GetDscResourceNotFoundException>(
                () => configurationSetProcessor.CreateUnitProcessor(unit, null));

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Tests Creating a unit processor by downloading the resource.
        /// </summary>
        [Fact]
        public void CreateUnitProcessor_InstallResource_NotFound()
        {
            string resourceName = "xResourceName";
            string moduleName = "xModuleName";
            Version version = new Version("1.0");

            DscResourceInfoInternal? nullResource = null;
            DscResourceInfoInternal dscResourceInfo = new DscResourceInfoInternal(resourceName, moduleName, version);
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(c => c.Unit.UnitName == resourceName)))
                .Returns(nullResource);

            PSObject? findDscResourceResult = null;
            processorEnvMock.Setup(
                m => m.FindDscResource(It.Is<ConfigurationUnitInternal>(c => c.Unit.UnitName == resourceName)))
                .Returns(findDscResourceResult)
                .Verifiable();

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                UnitName = resourceName,
            };
            unit.Directives.Add("module", moduleName);
            unit.Directives.Add("version", version.ToString());

            Assert.Throws<FindDscResourceNotFoundException>(
                () => configurationSetProcessor.CreateUnitProcessor(unit, null));

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Test GetUnitProcessorDetails Local Resource not found.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Local_NoFound()
        {
            string resourceName = "xResource";
            DscResourceInfoInternal? nullDscInfoInternal = null;

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(u => u.Unit.UnitName == resourceName)))
                .Returns(nullDscInfoInternal)
                .Verifiable();

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit()
            {
                UnitName = resourceName,
            };

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailLevel.Local);

            Assert.Null(configurationUnitProcessorDetails);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Test GetUnitProcessorDetails Local Found.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Local_NotInstalledByPowerShellGet()
        {
            var dscResourceInfoMock = new Mock<DscResourceInfoInternal>();

            var (unit, dscResourceInfo, psModuleInfo) = this.GetReasourceAndModuleInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(u => u.Unit.UnitName == dscResourceInfo.Name)))
                .Returns(dscResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetModule(It.Is<ModuleSpecification>(s => s.Name == dscResourceInfo.Name)))
                .Returns(psModuleInfo)
                .Verifiable();

            PSObject? nullPsModuleInfo = null;
            processorEnvMock.Setup(
                m => m.GetInstalledModule(It.Is<ModuleSpecification>(s => s.Name == dscResourceInfo.Name)))
                .Returns(nullPsModuleInfo)
                .Verifiable();

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailLevel.Local);

            Assert.NotNull(configurationUnitProcessorDetails);
            Assert.Equal(dscResourceInfo.Name, configurationUnitProcessorDetails.UnitName);

            processorEnvMock.Verify();
        }

        private (ConfigurationUnit unit, DscResourceInfoInternal dscResourceInfo, PSModuleInfo psModuleInfo) GetReasourceAndModuleInfo()
        {
            // This is easier than trying to mock sealed class from external code...
            var testEnv = this.fixture.PrepareTestProcessorEnvironment();

            var unit = new ConfigurationUnit();
            unit.UnitName = "SimpleFileResource";
            unit.Directives.Add("module", "xSimpleTestResource");
            unit.Directives.Add("version", "0.0.0.1");

            var dscResourceInfo = testEnv.GetDscResource(new ConfigurationUnitInternal(unit, null));
            var psModuleInfo = testEnv.GetModule(PowerShellHelpers.CreateModuleSpecification("xSimpleTestResource", "0.0.0.1"));

            if (dscResourceInfo is null || psModuleInfo is null)
            {
                throw new ArgumentNullException("Test processor environment not set correctly");
            }

            return (unit, dscResourceInfo, psModuleInfo);
        }
    }
}
