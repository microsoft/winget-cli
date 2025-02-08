// -----------------------------------------------------------------------------
// <copyright file="PowerShellHelperTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// PowerShell helper tests.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class PowerShellHelperTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="PowerShellHelperTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public PowerShellHelperTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Tests CreateModuleSpecification.
        /// </summary>
        [Fact]
        public void CreateModuleSpecification_Module_Test()
        {
            string moduleName = "MyModule";

            var moduleSpecification = PowerShellHelpers.CreateModuleSpecification(moduleName);
            Assert.Equal(moduleName, moduleSpecification.Name);
            Assert.Null(moduleSpecification.RequiredVersion);
            Assert.Null(moduleSpecification.Version);
            Assert.Null(moduleSpecification.MaximumVersion);
            Assert.Null(moduleSpecification.Guid);
        }

        /// <summary>
        /// Tests CreateModuleSpecification with arguments. use version.
        /// </summary>
        [Fact]
        public void CreateModuleSpecification_Required_Test()
        {
            string moduleName = "MyModule";
            string version = "0.1.0";
            string guid = Guid.NewGuid().ToString();

            var moduleSpecification = PowerShellHelpers.CreateModuleSpecification(
                moduleName,
                version,
                null,
                null,
                guid);
            Assert.Equal(moduleName, moduleSpecification.Name);
            Assert.NotNull(moduleSpecification.RequiredVersion);
            Assert.Equal(version, moduleSpecification.RequiredVersion.ToString());
            Assert.Null(moduleSpecification.Version);
            Assert.Null(moduleSpecification.MaximumVersion);
            Assert.NotNull(moduleSpecification.Guid);
            Assert.Equal(guid, moduleSpecification.Guid.ToString());
        }

        /// <summary>
        /// Tests CreateModuleSpecification with arguments. Use min version.
        /// </summary>
        [Fact]
        public void CreateModuleSpecification_MinMax_Test()
        {
            string moduleName = "MyModule";
            string minVersion = "0.0.1";
            string maxVersion = "1.0.0";
            string guid = Guid.NewGuid().ToString();

            var moduleSpecification = PowerShellHelpers.CreateModuleSpecification(
                moduleName,
                null,
                minVersion,
                maxVersion,
                guid);
            Assert.Equal(moduleName, moduleSpecification.Name);
            Assert.Null(moduleSpecification.RequiredVersion);
            Assert.NotNull(moduleSpecification.Version);
            Assert.Equal(minVersion, moduleSpecification.Version.ToString());
            Assert.NotNull(moduleSpecification.MaximumVersion);
            Assert.Equal(maxVersion, moduleSpecification.MaximumVersion.ToString());
            Assert.NotNull(moduleSpecification.Guid);
            Assert.Equal(guid, moduleSpecification.Guid.ToString());
        }

        /// <summary>
        /// Tests CreateModuleSpecification with prerelease versions.
        /// </summary>
        [Fact]
        public void CreateModuleSpecification_PrereleaseVersions_Required_Test()
        {
            string moduleName = "MyModule";
            string preVersion = "0.1.0-pre";
            string version = "0.1.0";

            var moduleSpecification = PowerShellHelpers.CreateModuleSpecification(
                moduleName,
                preVersion);
            Assert.Equal(moduleName, moduleSpecification.Name);
            Assert.NotNull(moduleSpecification.RequiredVersion);
            Assert.Equal(version, moduleSpecification.RequiredVersion.ToString());
            Assert.Null(moduleSpecification.Version);
            Assert.Null(moduleSpecification.MaximumVersion);
            Assert.Null(moduleSpecification.Guid);
        }

        /// <summary>
        /// Tests CreateModuleSpecification with prerelease versions.
        /// </summary>
        [Fact]
        public void CreateModuleSpecification_PrereleaseVersions_Test()
        {
            string moduleName = "MyModule";
            string preMinVersion = "0.0.1-pre";
            string minVersion = "0.0.1";
            string preMaxVersion = "1.0.0-pre";
            string maxVersion = "1.0.0";

            var moduleSpecification = PowerShellHelpers.CreateModuleSpecification(
                moduleName,
                null,
                preMinVersion,
                preMaxVersion);
            Assert.Equal(moduleName, moduleSpecification.Name);
            Assert.Null(moduleSpecification.RequiredVersion);
            Assert.NotNull(moduleSpecification.Version);
            Assert.Equal(minVersion, moduleSpecification.Version.ToString());
            Assert.NotNull(moduleSpecification.MaximumVersion);
            Assert.Equal(maxVersion, moduleSpecification.MaximumVersion.ToString());
            Assert.Null(moduleSpecification.Guid);
        }

        /// <summary>
        /// Tests CreateModuleSpecification with arguments. Don't use minVersion and version.
        /// </summary>
        [Fact]
        public void CreateModuleSpecification_Args_Throws_Test()
        {
            string moduleName = "MyModule";
            string version = "0.1.0";
            string minVersion = "0.0.1";
            string maxVersion = "1.0.0";

            Assert.Throws<ArgumentException>(() => PowerShellHelpers.CreateModuleSpecification(
                moduleName,
                version,
                minVersion,
                null,
                null));

            Assert.Throws<ArgumentException>(() => PowerShellHelpers.CreateModuleSpecification(
                moduleName,
                version,
                null,
                maxVersion,
                null));
        }
    }
}
