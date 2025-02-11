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
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.PowerShell.Set;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.PowerShell.Commands;
    using Moq;
    using Windows.Foundation.Collections;
    using Windows.Security.Cryptography.Certificates;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for configuration processor tests.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
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
                    m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(new DscResourceInfoInternal(resourceName, moduleName, version))
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                Type = resourceName,
            };
            unit.Metadata.Add("module", moduleName);
            unit.Metadata.Add("version", version.ToString());

            var unitProcessor = configurationSetProcessor.CreateUnitProcessor(unit);
            Assert.NotNull(unitProcessor);
            Assert.Equal(unit.Type, unitProcessor.Unit.Type);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Test CreateUnitProcessor case-insensitive.
        /// </summary>
        [Fact]
        public void CreateUnitProcessor_CaseInsensitive()
        {
            string resourceName = "name";
            string moduleName = "xModuleName";
            Version version = new Version("1.0");

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                    m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type.Equals("Name", StringComparison.OrdinalIgnoreCase))))
                .Returns(new DscResourceInfoInternal("Name", moduleName, version))
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                Type = resourceName,
            };
            unit.Metadata.Add("module", moduleName);
            unit.Metadata.Add("version", version.ToString());

            var unitProcessor = configurationSetProcessor.CreateUnitProcessor(unit);
            Assert.NotNull(unitProcessor);
            Assert.Equal(unit.Type, unitProcessor.Unit.Type);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Test CreateUnitProcessor case-insensitive.
        /// </summary>
        [Fact]
        public void CreateUnitProcessor_ResourceNameMismatch()
        {
            string resourceName = "name";
            string moduleName = "xModuleName";
            Version version = new Version("1.0");

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                    m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(new DscResourceInfoInternal("OtherName", moduleName, version))
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                Type = resourceName,
            };
            unit.Metadata.Add("module", moduleName);
            unit.Metadata.Add("version", version.ToString());

            Assert.Throws<ArgumentException>(() => configurationSetProcessor.CreateUnitProcessor(unit));

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
                    m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(new DscResourceInfoInternal(resourceName, moduleName, version))
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                Type = resourceName,
            };
            unit.Metadata.Add("module", moduleName);

            var unitProcessor = configurationSetProcessor.CreateUnitProcessor(unit);
            Assert.NotNull(unitProcessor);
            Assert.Equal(unit.Type, unitProcessor.Unit.Type);

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
                    m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(new DscResourceInfoInternal(resourceName, moduleName, version))
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                Type = resourceName,
            };

            var unitProcessor = configurationSetProcessor.CreateUnitProcessor(unit);
            Assert.NotNull(unitProcessor);
            Assert.Equal(unit.Type, unitProcessor.Unit.Type);

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
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(nullResource)
                .Returns(dscResourceInfo);

            PSObject findDscResourceResult = new PSObject(processorEnvMock);
            processorEnvMock.Setup(
                m => m.FindModule(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(findDscResourceResult)
                .Verifiable();

            processorEnvMock.Setup(
                m => m.InstallModule(findDscResourceResult))
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                Type = resourceName,
            };
            unit.Metadata.Add("module", moduleName);
            unit.Metadata.Add("version", version.ToString());

            var unitProcessor = configurationSetProcessor.CreateUnitProcessor(unit);
            Assert.NotNull(unitProcessor);
            Assert.Equal(unit.Type, unitProcessor.Unit.Type);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Tests Creating a unit processor by downloading the resource.
        /// </summary>
        [Fact]
        public void CreateUnitProcessor_InstallResource_WithoutModule()
        {
            string resourceName = "SimpleFileResource";
            Version version = new Version("0.0.0.1");

            DscResourceInfoInternal? nullResource = null;
            DscResourceInfoInternal dscResourceInfo = new DscResourceInfoInternal(resourceName, null, version);
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.SetupSequence(
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(nullResource)
                .Returns(dscResourceInfo);

            PSObject findDscResourceResult = this.CreateFindResourceInfo();
            processorEnvMock.Setup(
                m => m.FindDscResource(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(findDscResourceResult)
                .Verifiable();

            PSObject moduleInfo = ((dynamic)findDscResourceResult).PSGetModuleInfo;
            processorEnvMock.Setup(
                m => m.InstallModule(moduleInfo))
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                Type = resourceName,
            };
            unit.Metadata.Add("version", version.ToString());

            var unitProcessor = configurationSetProcessor.CreateUnitProcessor(unit);
            Assert.NotNull(unitProcessor);
            Assert.Equal(unit.Type, unitProcessor.Unit.Type);

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
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(nullResource);

            PSObject findDscResourceResult = new PSObject(processorEnvMock);
            processorEnvMock.Setup(
                m => m.FindModule(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(findDscResourceResult)
                .Verifiable();

            processorEnvMock.Setup(
                m => m.InstallModule(findDscResourceResult))
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                Type = resourceName,
            };
            unit.Metadata.Add("module", moduleName);
            unit.Metadata.Add("version", version.ToString());

            Assert.Throws<InstallDscResourceException>(
                () => configurationSetProcessor.CreateUnitProcessor(unit));

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
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(nullResource);

            PSObject? findDscResourceResult = null;
            processorEnvMock.Setup(
                m => m.FindModule(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(findDscResourceResult)
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                Type = resourceName,
            };
            unit.Metadata.Add("module", moduleName);
            unit.Metadata.Add("version", version.ToString());

            Assert.Throws<FindDscResourceNotFoundException>(
                () => configurationSetProcessor.CreateUnitProcessor(unit));

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
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(u => u.Unit.Type == resourceName)))
                .Returns(nullDscInfoInternal)
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var unit = new ConfigurationUnit()
            {
                Type = resourceName,
            };

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailFlags.Local);

            Assert.Null(configurationUnitProcessorDetails);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Test GetUnitProcessorDetails Local Found. Module not installed by PowerShellGet.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Local_NotInstalledByPowerShellGet()
        {
            var unit = this.CreateConfigurationUnit();
            var (dscResourceInfo, psModuleInfo) = this.GetResourceAndModuleInfo(unit);
            PSObject? nullPsModuleInfo = null;

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(u => u.Unit.Type == dscResourceInfo.Name)))
                .Returns(dscResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetAvailableModule(It.Is<ModuleSpecification>(s => s.Name == dscResourceInfo.ModuleName)))
                .Returns(psModuleInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetInstalledModule(It.Is<ModuleSpecification>(s => s.Name == dscResourceInfo.ModuleName)))
                .Returns(nullPsModuleInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetCertsOfValidSignedFiles(It.IsAny<string[]>()))
                .Returns(new List<Certificate>())
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailFlags.Local);

            Assert.NotNull(configurationUnitProcessorDetails);
            Assert.Equal(dscResourceInfo.Name, configurationUnitProcessorDetails.UnitType);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Test GetUnitProcessorDetails locally found. Do not include Load.
        /// </summary>
        /// <param name="detailFlags">Detail flags.</param>
        [Theory]
        [InlineData(ConfigurationUnitDetailFlags.Local)]
        [InlineData(ConfigurationUnitDetailFlags.ReadOnly)]
        [InlineData(ConfigurationUnitDetailFlags.Download)]
        public void GetUnitProcessorDetails_Local(object detailFlags)
        {
            var unit = this.CreateConfigurationUnit();
            var (dscResourceInfo, psModuleInfo) = this.GetResourceAndModuleInfo(unit);
            var getModuleInfo = this.CreateGetModuleInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(u => u.Unit.Type == dscResourceInfo.Name)))
                .Returns(dscResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetAvailableModule(It.Is<ModuleSpecification>(s => s.Name == dscResourceInfo.ModuleName)))
                .Returns(psModuleInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetInstalledModule(It.Is<ModuleSpecification>(s => s.Name == dscResourceInfo.ModuleName)))
                .Returns(getModuleInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetCertsOfValidSignedFiles(It.IsAny<string[]>()))
                .Returns(new List<Certificate>())
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                Assert.IsType<ConfigurationUnitDetailFlags>(detailFlags));

            Assert.NotNull(configurationUnitProcessorDetails);
            Assert.Equal(dscResourceInfo.Name, configurationUnitProcessorDetails.UnitType);

            processorEnvMock.Verify();
            processorEnvMock.Verify(m => m.FindDscResource(It.IsAny<ConfigurationUnitAndModule>()), Times.Never());
            processorEnvMock.Verify(m => m.ImportModule(It.IsAny<ModuleSpecification>()), Times.Never());
        }

        /// <summary>
        /// Test GetUnitProcessorDetails locally found and load.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Local_Load()
        {
            var unit = this.CreateConfigurationUnit();
            var (dscResourceInfo, psModuleInfo) = this.GetResourceAndModuleInfo(unit);
            var getModuleInfo = this.CreateGetModuleInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(u => u.Unit.Type == dscResourceInfo.Name)))
                .Returns(dscResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetAvailableModule(It.Is<ModuleSpecification>(s => s.Name == dscResourceInfo.ModuleName)))
                .Returns(psModuleInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetInstalledModule(It.Is<ModuleSpecification>(s => s.Name == dscResourceInfo.ModuleName)))
                .Returns(getModuleInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.ImportModule(It.Is<ModuleSpecification>(s => s.Name == dscResourceInfo.ModuleName)))
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetCertsOfValidSignedFiles(It.IsAny<string[]>()))
                .Returns(new List<Certificate>())
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailFlags.Load);

            Assert.NotNull(configurationUnitProcessorDetails);
            Assert.Equal(dscResourceInfo.Name, configurationUnitProcessorDetails.UnitType);

            processorEnvMock.Verify();
            processorEnvMock.Verify(m => m.FindDscResource(It.IsAny<ConfigurationUnitAndModule>()), Times.Never());
        }

        /// <summary>
        /// Test GetUnitProcessorDetails Catalog Not Found.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Catalog_NotFound()
        {
            var unit = this.CreateConfigurationUnit();
            DscResourceInfoInternal? nullDscResourceInfo = null;
            PSObject? nullPsModuleInfo = null;

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(u => u.Unit.Type == unit.Type)))
                .Returns(nullDscResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.FindModule(It.Is<ConfigurationUnitAndModule>(c => unit.Type == unit.Type)))
                .Returns(nullPsModuleInfo)
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailFlags.ReadOnly);

            Assert.Null(configurationUnitProcessorDetails);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Test GetUnitProcessorDetails Catalog Found.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Catalog()
        {
            var unit = this.CreateConfigurationUnit();
            DscResourceInfoInternal? nullDscResourceInfo = null;
            var getFindResourceInfo = this.CreateFindResourceInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(u => u.Unit.Type == unit.Type)))
                .Returns(nullDscResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.FindModule(It.Is<ConfigurationUnitAndModule>(c => unit.Type == unit.Type)))
                .Returns(getFindResourceInfo)
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailFlags.ReadOnly);

            Assert.NotNull(configurationUnitProcessorDetails);
            Assert.Equal("SimpleFileResource", configurationUnitProcessorDetails.UnitType);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Test GetUnitProcessorDetails downloading module.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Download()
        {
            var unit = this.CreateConfigurationUnit();
            DscResourceInfoInternal? nullDscResourceInfo = null;
            var (_, psModuleInfo) = this.GetResourceAndModuleInfo(unit);
            var getFindModuleInfo = this.CreateGetModuleInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(u => u.Unit.Type == unit.Type)))
                .Returns(nullDscResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.FindModule(It.Is<ConfigurationUnitAndModule>(c => unit.Type == unit.Type)))
                .Returns(getFindModuleInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.SaveModule(getFindModuleInfo, It.IsAny<string>()))
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetAvailableModule(It.Is<string>(s => s.EndsWith("xSimpleTestResource"))))
                .Returns(psModuleInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetCertsOfValidSignedFiles(It.IsAny<string[]>()))
                .Returns(new List<Certificate>())
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailFlags.Download);

            Assert.NotNull(configurationUnitProcessorDetails);
            Assert.Equal("SimpleFileResource", configurationUnitProcessorDetails.UnitType);

            processorEnvMock.Verify();

            processorEnvMock.Verify(m => m.InstallModule(It.IsAny<PSObject>()), Times.Never());
        }

        /// <summary>
        /// Tests GetUnitProcessorDetails install module, but resource not found anyway.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Load_NotFoundAfterInstall()
        {
            var unit = this.CreateConfigurationUnit();
            DscResourceInfoInternal? nullDscResourceInfo = null;
            var (_, psModuleInfo) = this.GetResourceAndModuleInfo(unit);
            var getFindResourceInfo = this.CreateFindResourceInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(u => u.Unit.Type == unit.Type)))
                .Returns(nullDscResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.FindModule(It.Is<ConfigurationUnitAndModule>(c => unit.Type == unit.Type)))
                .Returns(getFindResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.InstallModule(getFindResourceInfo))
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            Assert.Throws<InstallDscResourceException>(() => configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailFlags.Load));

            processorEnvMock.Verify();
            processorEnvMock.Verify(
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(u => u.Unit.Type == unit.Type)),
                Times.Exactly(2));
        }

        /// <summary>
        /// Tests GetUnitProcessorDetails install module.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Load()
        {
            var unit = this.CreateConfigurationUnit();
            DscResourceInfoInternal? nullDscResourceInfo = null;
            var (dscResourceInfo, psModuleInfo) = this.GetResourceAndModuleInfo(unit);
            var getFindResourceInfo = this.CreateFindResourceInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.SetupSequence(
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(u => u.Unit.Type == unit.Type)))
                .Returns(nullDscResourceInfo)
                .Returns(dscResourceInfo);
            processorEnvMock.Setup(
                m => m.FindModule(It.Is<ConfigurationUnitAndModule>(c => unit.Type == unit.Type)))
                .Returns(getFindResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.InstallModule(getFindResourceInfo))
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetInstalledModule(It.Is<ModuleSpecification>(s => s.Name == dscResourceInfo.ModuleName)))
                .Returns(getFindResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.ImportModule(It.Is<ModuleSpecification>(s => s.Name == dscResourceInfo.ModuleName)))
                .Verifiable();

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailFlags.Load);

            Assert.NotNull(configurationUnitProcessorDetails);
            Assert.Equal(dscResourceInfo.Name, configurationUnitProcessorDetails.UnitType);

            processorEnvMock.Verify();
            processorEnvMock.Verify(
                m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(u => u.Unit.Type == unit.Type)),
                Times.Exactly(2));
        }

        /// <summary>
        /// This tests uses SimpleTestResourceTypes Test to validate the resource got the correct types
        /// from the processor.
        /// </summary>
        [Fact]
        public void CreateUnitProcessor_TestTypes()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            var setProcessor = new PowerShellConfigurationSetProcessor(processorEnv, new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                Type = "SimpleTestResourceTypes",
                Intent = ConfigurationUnitIntent.Assert,
            };

            unit.Metadata.Add("module", "xSimpleTestResource");
            unit.Metadata.Add("version", "0.0.0.1");

            var hashtableProperty = new ValueSet
            {
                { "secretStringKey", "secretCode" },
                { "secretIntKey", "123456" },
            };

            unit.Settings.Add("boolProperty", true);
            unit.Settings.Add("intProperty", 3);
            unit.Settings.Add("doubleProperty", -9.876);
            unit.Settings.Add("charProperty", 'f');
            unit.Settings.Add("hashtableProperty", hashtableProperty);

            var unitProcessor = setProcessor.CreateUnitProcessor(unit);

            unitProcessor.TestSettings();
        }

        /// <summary>
        /// Tests a module that requires admin is loaded from non admin.
        /// </summary>
        [FactSkipIfCI]
        public void CreateUnitProcessor_ModuleRequiresAdmin()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            var setProcessor = new PowerShellConfigurationSetProcessor(processorEnv, new ConfigurationSet());

            var unit = new ConfigurationUnit
            {
                Type = "AdminResource",
                Intent = ConfigurationUnitIntent.Assert,
            };
            unit.Metadata.Add("module", "xAdminTestResource");
            unit.Metadata.Add("version", "0.0.0.1");

            unit.Settings.Add("key", "key");

            var importModuleException = Assert.Throws<ImportModuleException>(() => setProcessor.CreateUnitProcessor(unit));
            Assert.Equal(ErrorCodes.WinGetConfigUnitImportModuleAdmin, importModuleException.HResult);
        }

        /// <summary>
        /// Test CreateUnitProcessor. Limit mode.
        /// </summary>
        [Fact]
        public void CreateUnitProcessor_LimitMode()
        {
            string resourceName = "xResourceName";
            string moduleName = "xModuleName";
            Version version = new Version("1.0");

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                    m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(new DscResourceInfoInternal(resourceName, moduleName, version))
                .Verifiable();

            var limitSet = new ConfigurationSet();
            var limitUnit = new ConfigurationUnit
            {
                Type = resourceName,
                Intent = ConfigurationUnitIntent.Apply,
            };
            limitUnit.Metadata.Add("module", moduleName);
            limitUnit.Metadata.Add("version", version.ToString());
            limitSet.Units = new List<ConfigurationUnit> { limitUnit };

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                limitSet,
                true);

            // Calling with unit different from limit set should throw.
            var unitDifferentContent = new ConfigurationUnit
            {
                Type = "differentResourceName",
                Intent = ConfigurationUnitIntent.Apply,
            };

            Assert.Throws<System.InvalidOperationException>(() => configurationSetProcessor.CreateUnitProcessor(unitDifferentContent));
            Assert.Throws<System.InvalidOperationException>(() => configurationSetProcessor.GetUnitProcessorDetails(unitDifferentContent, ConfigurationUnitDetailFlags.Load));

            // Calling with unit matching limit set.
            var unitProcessor = configurationSetProcessor.CreateUnitProcessor(limitUnit);
            Assert.NotNull(unitProcessor);
            Assert.Equal(limitUnit.Type, unitProcessor.Unit.Type);
            var processorDetails = configurationSetProcessor.GetUnitProcessorDetails(limitUnit, ConfigurationUnitDetailFlags.Load);
            Assert.NotNull(processorDetails);
            Assert.Equal(moduleName, processorDetails.ModuleName);

            // Calling CreateProcessor again should thow. Calling GetProcessorDetails multiple times is ok.
            Assert.Throws<System.InvalidOperationException>(() => configurationSetProcessor.CreateUnitProcessor(limitUnit));
            var processorDetails2 = configurationSetProcessor.GetUnitProcessorDetails(limitUnit, ConfigurationUnitDetailFlags.Load);
            Assert.NotNull(processorDetails2);
            Assert.Equal(moduleName, processorDetails2.ModuleName);
        }

        /// <summary>
        /// Test CreateUnitProcessor. Limit mode. Duplicate units in limit set.
        /// </summary>
        [Fact]
        public void CreateUnitProcessor_LimitMode_DuplicateUnits()
        {
            string resourceName = "xResourceName";
            string moduleName = "xModuleName";
            Version version = new Version("1.0");

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                    m => m.GetDscResource(It.Is<ConfigurationUnitAndModule>(c => c.Unit.Type == resourceName)))
                .Returns(new DscResourceInfoInternal(resourceName, moduleName, version))
                .Verifiable();

            var limitSet = new ConfigurationSet();
            var limitUnit = new ConfigurationUnit
            {
                Type = resourceName,
                Intent = ConfigurationUnitIntent.Apply,
            };
            limitUnit.Metadata.Add("module", moduleName);
            limitUnit.Metadata.Add("version", version.ToString());
            limitSet.Units = new List<ConfigurationUnit> { limitUnit, limitUnit };

            var configurationSetProcessor = new PowerShellConfigurationSetProcessor(
                processorEnvMock.Object,
                limitSet,
                true);

            // Calling with unit different from limit set should throw.
            var unitDifferentContent = new ConfigurationUnit
            {
                Type = "differentResourceName",
                Intent = ConfigurationUnitIntent.Apply,
            };

            Assert.Throws<System.InvalidOperationException>(() => configurationSetProcessor.CreateUnitProcessor(unitDifferentContent));

            // Calling with unit matching limit set.
            var unitProcessor = configurationSetProcessor.CreateUnitProcessor(limitUnit);
            Assert.NotNull(unitProcessor);
            Assert.Equal(limitUnit.Type, unitProcessor.Unit.Type);

            // Calling again should also not thow.
            var unitProcessor2 = configurationSetProcessor.CreateUnitProcessor(limitUnit);
            Assert.NotNull(unitProcessor2);
            Assert.Equal(limitUnit.Type, unitProcessor2.Unit.Type);

            // Calling third time should throw.
            Assert.Throws<System.InvalidOperationException>(() => configurationSetProcessor.CreateUnitProcessor(limitUnit));
        }

        private ConfigurationUnit CreateConfigurationUnit()
        {
            var unit = new ConfigurationUnit();
            unit.Type = "SimpleFileResource";
            unit.Metadata.Add("module", "xSimpleTestResource");
            unit.Metadata.Add("version", "0.0.0.1");

            return unit;
        }

        private (DscResourceInfoInternal dscResourceInfo, PSModuleInfo psModuleInfo) GetResourceAndModuleInfo(ConfigurationUnit unit)
        {
            // This is easier than trying to mock sealed class from external code...
            var testEnv = this.fixture.PrepareTestProcessorEnvironment(true);
            var dscResourceInfo = testEnv.GetDscResource(new ConfigurationUnitAndModule(unit, string.Empty));
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
                IconUri = "https://github.com/microsoft/winget-cli",
                Name = "xSimpleTestResource",
                Description = "PowerShell module with DSC resources for unit tests",
                RepositorySourceLocation = "https://github.com/microsoft/winget-cli",
                Version = "0.0.0.1",
                Author = "Luffytaro",
                CompanyName = "Microsoft Corporation",
            });
        }

        private PSObject CreateFindResourceInfo()
        {
            var getModuleInfo = this.CreateGetModuleInfo();
            return new PSObject(new
            {
                Name = "SimpleFileResource",
                ModuleName = "xSimpleTestResource",
                Version = "0.0.0.1",
                PSGetModuleInfo = getModuleInfo,
            });
        }
    }
}
