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
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.PowerShell.Commands;
    using Moq;
    using Windows.Security.Cryptography.Certificates;
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

            Assert.Throws<InstallDscResourceException>(
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
        /// Test GetUnitProcessorDetails Local Found. Module not installed by PowerShellGet.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Local_NotInstalledByPowerShellGet()
        {
            var unit = this.CreteConfigurationUnit();
            var (dscResourceInfo, psModuleInfo) = this.GetResourceAndModuleInfo(unit);
            PSObject? nullPsModuleInfo = null;

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(u => u.Unit.UnitName == dscResourceInfo.Name)))
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

        /// <summary>
        /// Test GetUnitProcessorDetails locally found. Do not include Load.
        /// </summary>
        /// <param name="detailLevel">Detail level.</param>
        [Theory]
        [InlineData(ConfigurationUnitDetailLevel.Local)]
        [InlineData(ConfigurationUnitDetailLevel.Catalog)]
        [InlineData(ConfigurationUnitDetailLevel.Download)]
        public void GetUnitProcessorDetails_Local(ConfigurationUnitDetailLevel detailLevel)
        {
            var unit = this.CreteConfigurationUnit();
            var (dscResourceInfo, psModuleInfo) = this.GetResourceAndModuleInfo(unit);
            var getModuleInfo = this.CreateGetModuleInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(u => u.Unit.UnitName == dscResourceInfo.Name)))
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

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                detailLevel);

            Assert.NotNull(configurationUnitProcessorDetails);
            Assert.Equal(dscResourceInfo.Name, configurationUnitProcessorDetails.UnitName);

            processorEnvMock.Verify();
            processorEnvMock.Verify(m => m.FindDscResource(It.IsAny<ConfigurationUnitInternal>()), Times.Never());
            processorEnvMock.Verify(m => m.ImportModule(It.IsAny<ModuleSpecification>()), Times.Never());
        }

        /// <summary>
        /// Test GetUnitProcessorDetails locally found and load.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Local_Load()
        {
            var unit = this.CreteConfigurationUnit();
            var (dscResourceInfo, psModuleInfo) = this.GetResourceAndModuleInfo(unit);
            var getModuleInfo = this.CreateGetModuleInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(u => u.Unit.UnitName == dscResourceInfo.Name)))
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

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailLevel.Load);

            Assert.NotNull(configurationUnitProcessorDetails);
            Assert.Equal(dscResourceInfo.Name, configurationUnitProcessorDetails.UnitName);

            processorEnvMock.Verify();
            processorEnvMock.Verify(m => m.FindDscResource(It.IsAny<ConfigurationUnitInternal>()), Times.Never());
        }

        /// <summary>
        /// Test GetUnitProcessorDetails Catalog Not Found.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Catalog_NotFound()
        {
            var unit = this.CreteConfigurationUnit();
            DscResourceInfoInternal? nullDscResourceInfo = null;
            PSObject? nullPsModuleInfo = null;

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(u => u.Unit.UnitName == unit.UnitName)))
                .Returns(nullDscResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.FindDscResource(It.Is<ConfigurationUnitInternal>(c => unit.UnitName == unit.UnitName)))
                .Returns(nullPsModuleInfo)
                .Verifiable();

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailLevel.Catalog);

            Assert.Null(configurationUnitProcessorDetails);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Test GetUnitProcessorDetails Catalog Found.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Catalog()
        {
            var unit = this.CreteConfigurationUnit();
            DscResourceInfoInternal? nullDscResourceInfo = null;
            var getFindResourceInfo = this.CreateFindResourceInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(u => u.Unit.UnitName == unit.UnitName)))
                .Returns(nullDscResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.FindDscResource(It.Is<ConfigurationUnitInternal>(c => unit.UnitName == unit.UnitName)))
                .Returns(getFindResourceInfo)
                .Verifiable();

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailLevel.Catalog);

            Assert.NotNull(configurationUnitProcessorDetails);
            Assert.Equal("SimpleFileResource", configurationUnitProcessorDetails.UnitName);

            processorEnvMock.Verify();
        }

        /// <summary>
        /// Test GetUnitProcessorDetails downloading module.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Download()
        {
            var unit = this.CreteConfigurationUnit();
            DscResourceInfoInternal? nullDscResourceInfo = null;
            var (_, psModuleInfo) = this.GetResourceAndModuleInfo(unit);
            var getFindResourceInfo = this.CreateFindResourceInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(u => u.Unit.UnitName == unit.UnitName)))
                .Returns(nullDscResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.FindDscResource(It.Is<ConfigurationUnitInternal>(c => unit.UnitName == unit.UnitName)))
                .Returns(getFindResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.SaveModule(getFindResourceInfo, It.IsAny<string>()))
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetAvailableModule(It.Is<string>(s => s.EndsWith("xSimpleTestResource"))))
                .Returns(psModuleInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.GetCertsOfValidSignedFiles(It.IsAny<string[]>()))
                .Returns(new List<Certificate>())
                .Verifiable();

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailLevel.Download);

            Assert.NotNull(configurationUnitProcessorDetails);
            Assert.Equal("SimpleFileResource", configurationUnitProcessorDetails.UnitName);

            processorEnvMock.Verify();

            processorEnvMock.Verify(m => m.InstallModule(It.IsAny<PSObject>()), Times.Never());
        }

        /// <summary>
        /// Tests GetUnitProcessorDetails install module, but resource not found anyway.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Load_NotFoundAfterInstall()
        {
            var unit = this.CreteConfigurationUnit();
            DscResourceInfoInternal? nullDscResourceInfo = null;
            var (_, psModuleInfo) = this.GetResourceAndModuleInfo(unit);
            var getFindResourceInfo = this.CreateFindResourceInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(u => u.Unit.UnitName == unit.UnitName)))
                .Returns(nullDscResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.FindDscResource(It.Is<ConfigurationUnitInternal>(c => unit.UnitName == unit.UnitName)))
                .Returns(getFindResourceInfo)
                .Verifiable();
            processorEnvMock.Setup(
                m => m.InstallModule(getFindResourceInfo))
                .Verifiable();

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            Assert.Throws<InstallDscResourceException>(() => configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailLevel.Load));

            processorEnvMock.Verify();
            processorEnvMock.Verify(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(u => u.Unit.UnitName == unit.UnitName)),
                Times.Exactly(2));
        }

        /// <summary>
        /// Tests GetUnitProcessorDetails install module.
        /// </summary>
        [Fact]
        public void GetUnitProcessorDetails_Load()
        {
            var unit = this.CreteConfigurationUnit();
            DscResourceInfoInternal? nullDscResourceInfo = null;
            var (dscResourceInfo, psModuleInfo) = this.GetResourceAndModuleInfo(unit);
            var getFindResourceInfo = this.CreateFindResourceInfo();

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.SetupSequence(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(u => u.Unit.UnitName == unit.UnitName)))
                .Returns(nullDscResourceInfo)
                .Returns(dscResourceInfo);
            processorEnvMock.Setup(
                m => m.FindDscResource(It.Is<ConfigurationUnitInternal>(c => unit.UnitName == unit.UnitName)))
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

            var configurationSetProcessor = new ConfigurationSetProcessor(
                processorEnvMock.Object,
                new ConfigurationSet());

            var configurationUnitProcessorDetails = configurationSetProcessor.GetUnitProcessorDetails(
                unit,
                ConfigurationUnitDetailLevel.Load);

            Assert.NotNull(configurationUnitProcessorDetails);
            Assert.Equal(dscResourceInfo.Name, configurationUnitProcessorDetails.UnitName);

            processorEnvMock.Verify();
            processorEnvMock.Verify(
                m => m.GetDscResource(It.Is<ConfigurationUnitInternal>(u => u.Unit.UnitName == unit.UnitName)),
                Times.Exactly(2));
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
            var testEnv = this.fixture.PrepareTestProcessorEnvironment(true);
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
