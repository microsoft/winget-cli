// -----------------------------------------------------------------------------
// <copyright file="DscModuleV2Tests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.IO;
    using System.Management.Automation;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscModules;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.PowerShell.Commands;
    using Windows.Foundation.Collections;
    using Xunit;
    using Xunit.Abstractions;
    using static Microsoft.Management.Configuration.UnitTests.Helpers.PowerShellTestsConstants;

    /// <summary>
    /// Tests DscModuleV2 with really simple resources.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class DscModuleV2Tests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="DscModuleV2Tests"/> class.
        /// </summary>
        /// <param name="fixture">Fixture.</param>
        /// <param name="log">log.</param>
        public DscModuleV2Tests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Tests GetAllDscResources.
        /// </summary>
        [Fact]
        public void GetAllDscResources_Test()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var resources = dscModule.GetAllDscResources(pwsh);

            Assert.True(resources.Count > 0);
            Assert.Contains(resources, r => r.Name == TestModule.SimpleFileResourceName);
        }

        /// <summary>
        /// Tests GetDscResourcesInModule.
        /// </summary>
        /// <param name="module">Module.</param>
        /// <param name="expectedResources">Expected DSC resources.</param>
        [Theory]
        [InlineData(TestModule.SimpleTestResourceModuleName, 5)]
        [InlineData("MyReallyFakeModule", 0)]
        public void GetDscResourcesInModule_Test(string module, int expectedResources)
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var resources = dscModule.GetDscResourcesInModule(
                pwsh,
                PowerShellHelpers.CreateModuleSpecification(module));
            Assert.Equal(expectedResources, resources.Count);
        }

        /// <summary>
        /// Tests GetDscResourcesInModule with versions.
        /// </summary>
        [Fact]
        public void GetDscResourcesInModule_VersionTest()
        {
            string newVersion = "1.0.0.0";
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            // Get duplicated resources by creating a new directory and copy our modules.
            // Change version and add them to the PSModulePath.
            using var tmpDir = new TempDirectory();
            tmpDir.CopyDirectory(this.fixture.TestModulesPath);
            var manifestFile = Path.Combine(
                tmpDir.FullDirectoryPath,
                TestModule.SimpleTestResourceModuleName,
                TestModule.SimpleTestResourceManifestFileName);
            File.WriteAllText(
                manifestFile,
                File.ReadAllText(manifestFile).Replace("0.0.0.1", newVersion));
            testEnvironment.AppendPSModulePath(tmpDir.FullDirectoryPath);

            var dscModule = new DscModuleV2();

            // This doesn't work on v2
            ////var allResources = dscModule.GetDscResourcesInModule(
            ////    testEnvironment.Runspace,
            ////    PowerShellHelpers.CreateModuleSpecification(TestModule.SimpleTestResourceModuleName));
            ////Assert.Equal(8, allResources.Count);
            {
                using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
                var ogResources = dscModule.GetDscResourcesInModule(
                pwsh,
                PowerShellHelpers.CreateModuleSpecification(
                    TestModule.SimpleTestResourceModuleName,
                    version: TestModule.SimpleTestResourceVersion));
                Assert.Equal(5, ogResources.Count);
            }

            {
                using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
                var newVersionResources = dscModule.GetDscResourcesInModule(
                    pwsh,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName,
                        version: newVersion));
                Assert.Equal(5, newVersionResources.Count);
            }

            {
                using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
                var badVersionResources = dscModule.GetDscResourcesInModule(
                    pwsh,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName,
                        version: "1.2.3.4"));
                Assert.Equal(0, badVersionResources.Count);
            }
        }

        /// <summary>
        /// Tests GetDscResource. Should return a resource.
        /// </summary>
        [Fact]
        public void GetDscResource_ResourceExists()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var resource = dscModule.GetDscResource(
                pwsh,
                TestModule.SimpleTestResourceName,
                PowerShellHelpers.CreateModuleSpecification(TestModule.SimpleTestResourceModuleName));

            Assert.NotNull(resource);
        }

        /// <summary>
        /// Test GetDscResource for a resource that doesn't exist.
        /// </summary>
        [Fact]
        public void GetDscResource_ResourceDoesntExist()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var resource = dscModule.GetDscResource(
                pwsh,
                "FakeResourceName",
                PowerShellHelpers.CreateModuleSpecification(
                    TestModule.SimpleTestResourceModuleName));
            Assert.Null(resource);
        }

        /// <summary>
        /// Test GetDscResource when the same module is in different paths.
        /// </summary>
        [Fact]
        public void GetDscResource_Conflict()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            // Get duplicated resources by creating a new directory and copy our modules.
            // Then add it to the PSModulePath.
            using var tmpDir = new TempDirectory();
            tmpDir.CopyDirectory(this.fixture.TestModulesPath);
            testEnvironment.AppendPSModulePath(tmpDir.FullDirectoryPath);

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            Assert.Throws<GetDscResourceModuleConflict>(
                () => dscModule.GetDscResource(
                    pwsh,
                    TestModule.SimpleTestResourceName,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName)));
        }

        /// <summary>
        /// Tests GetDscResource when there are multiple versions of a resource.
        /// </summary>
        [Fact]
        public void GetDscResource_DiffVersions()
        {
            string newVersion = "2.0.0.0";
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            // Get duplicated resources by creating a new directory and copy our modules.
            // Change version and add them to the PSModulePath.
            using var tmpDir = new TempDirectory();
            tmpDir.CopyDirectory(this.fixture.TestModulesPath);
            var manifestFile = Path.Combine(
                tmpDir.FullDirectoryPath,
                TestModule.SimpleTestResourceModuleName,
                TestModule.SimpleTestResourceManifestFileName);
            File.WriteAllText(
                manifestFile,
                File.ReadAllText(manifestFile).Replace("0.0.0.1", newVersion));
            testEnvironment.AppendPSModulePath(tmpDir.FullDirectoryPath);

            var dscModule = new DscModuleV2();

            // specific version.
            {
                using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
                var resource = dscModule.GetDscResource(
                    pwsh,
                    TestModule.SimpleTestResourceName,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName,
                        TestModule.SimpleTestResourceVersion));
                Assert.NotNull(resource);
            }

            {
                using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
                var dsc = dscModule.GetDscResource(
                    pwsh,
                    TestModule.SimpleTestResourceName,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName,
                        version: newVersion));

                Assert.NotNull(dsc);
                Assert.NotNull(dsc.Version);
                Assert.Equal(newVersion, dsc.Version.ToString());
            }
        }

        /// <summary>
        /// Calls Invoke-DscResource Get.
        /// </summary>
        [Fact]
        public void InvokeGetResource_Test()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var getResult = dscModule.InvokeGetResource(
                pwsh,
                new ValueSet(),
                TestModule.SimpleTestResourceName,
                PowerShellHelpers.CreateModuleSpecification(
                    TestModule.SimpleTestResourceModuleName));

            Assert.True(getResult.ContainsKey("key"));
            Assert.True(getResult.TryGetValue("key", out object keyValue));
            Assert.Equal("SimpleTestResourceKey", keyValue as string);
        }

        /// <summary>
        /// Calls Invoke-DscResource Get. Resource Get throws.
        /// </summary>
        [Fact]
        public void InvokeGetResource_ResourceThrows()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();

            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var exception = Assert.Throws<InvokeDscResourceException>(() =>
                dscModule.InvokeGetResource(
                    pwsh,
                    new ValueSet(),
                    TestModule.SimpleTestResourceThrowsName,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName)));

            Assert.IsType<RuntimeException>(exception.InnerException);
        }

        /// <summary>
        /// Calls Invoke-DscResource Get. Resource writes error.
        /// </summary>
        [Fact(Skip = "Not supported in PSDesiredStateConfiguration 2.0.7")]
        public void InvokeGetResource_ResourceError()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            Assert.Throws<WriteErrorException>(() =>
                dscModule.InvokeGetResource(
                    pwsh,
                    new ValueSet(),
                    TestModule.SimpleTestResourceErrorName,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName)));
        }

        /// <summary>
        /// Calls Invoke-DscResource Get. Resource does not exist.
        /// </summary>
        [Fact]
        public void InvokeGetResource_ResourceDoesntExist()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var exception = Assert.Throws<InvokeDscResourceException>(
                () => dscModule.InvokeGetResource(
                    pwsh,
                    new ValueSet(),
                    "FakeResourceName",
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName)));

            Assert.IsType<RuntimeException>(exception.InnerException);
        }

        /// <summary>
        /// Calls Invoke-DscResource Test.
        /// </summary>
        /// <param name="value">Setting value.</param>
        /// <param name="expectedResult">Expected result.</param>
        [Theory]
        [InlineData("4815162342", true)]
        [InlineData("notalostreference", false)]
        public void InvokeTestResource_Test(string value, bool expectedResult)
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();

            var settings = new ValueSet()
            {
                { "secretCode", value },
            };

            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var testResult = dscModule.InvokeTestResource(
                pwsh,
                settings,
                TestModule.SimpleTestResourceName,
                PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName));

            Assert.Equal(expectedResult, testResult);
        }

        /// <summary>
        /// Calls Invoke-DscResource Test. Resource throws.
        /// </summary>
        [Fact]
        public void InvokeTestResource_Throws()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var exception = Assert.Throws<InvokeDscResourceException>(() =>
                dscModule.InvokeTestResource(
                    pwsh,
                    new ValueSet(),
                    TestModule.SimpleTestResourceThrowsName,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName)));

            Assert.IsType<RuntimeException>(exception.InnerException);
        }

        /// <summary>
        /// Calls Invoke-DscResource Test. Resource writes error.
        /// </summary>
        [Fact(Skip = "Not supported in PSDesiredStateConfiguration 2.0.7")]
        public void InvokeTestResource_ResourceError()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            Assert.Throws<WriteErrorException>(() =>
                dscModule.InvokeTestResource(
                    pwsh,
                    new ValueSet(),
                    TestModule.SimpleTestResourceErrorName,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName)));
        }

        /// <summary>
        /// Calls Invoke-DscResource Test. Resource does not exist.
        /// </summary>
        [Fact]
        public void InvokeTestResource_ResourceDoesntExist()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var exception = Assert.Throws<InvokeDscResourceException>(() =>
                _ = dscModule.InvokeTestResource(
                    pwsh,
                    new ValueSet(),
                    "FakeResourceName",
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName)));

            Assert.IsType<RuntimeException>(exception.InnerException);
        }

        /// <summary>
        /// Calls Invoke-DscResource Set.
        /// </summary>
        /// <param name="value">Setting value.</param>
        /// <param name="rebootRequired">Expected reboot required.</param>
        [Fact]
        public void InvokeSetResource_Test()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();

            var settings = new ValueSet()
            {
                { "secretCode", "4815162342" },
            };

            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var testResult = dscModule.InvokeSetResource(
                pwsh,
                settings,
                TestModule.SimpleTestResourceName,
                PowerShellHelpers.CreateModuleSpecification(
                    TestModule.SimpleTestResourceModuleName));

            // TODO: Verify reboot required when is supported for class resources.
            ////Assert.Equal(rebootRequired, testResult);
        }

        /// <summary>
        /// Calls Invoke-DscResource Set. Resource throws.
        /// </summary>
        [Fact]
        public void InvokeSetResource_Throws()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var exception = Assert.Throws<InvokeDscResourceException>(() =>
                dscModule.InvokeSetResource(
                    pwsh,
                    new ValueSet(),
                    TestModule.SimpleTestResourceThrowsName,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName)));

            Assert.IsType<RuntimeException>(exception.InnerException);
        }

        /// <summary>
        /// Calls Invoke-DscResource Set. Resource writes error.
        /// </summary>
        [Fact(Skip = "Not supported in PSDesiredStateConfiguration 2.0.7")]
        public void InvokeSetResource_ResourceError()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            Assert.Throws<WriteErrorException>(() =>
                dscModule.InvokeSetResource(
                    pwsh,
                    new ValueSet(),
                    TestModule.SimpleTestResourceErrorName,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName)));
        }

        /// <summary>
        /// Calls Invoke-DscResource Set. Resource does not exist.
        /// </summary>
        [Fact]
        public void InvokeSetResource_ResourceDoesntExist()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var exception = Assert.Throws<InvokeDscResourceException>(() =>
                dscModule.InvokeSetResource(
                    pwsh,
                    new ValueSet(),
                    "FakeResourceName",
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName)));

            Assert.IsType<RuntimeException>(exception.InnerException);
        }

        /// <summary>
        /// Test calling Invoke-DscResource when a resource has multiple versions.
        /// </summary>
        [Fact]
        public void InvokeResource_MultipleVersions()
        {
            string newVersion = "0.0.2.0";
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            // Get duplicated resources by creating a new directory and copy our modules.
            // Change version and add them to the PSModulePath.
            using var tmpDir = new TempDirectory();
            tmpDir.CopyDirectory(this.fixture.TestModulesPath);
            var manifestFile = Path.Combine(
                tmpDir.FullDirectoryPath,
                TestModule.SimpleTestResourceModuleName,
                TestModule.SimpleTestResourceManifestFileName);
            File.WriteAllText(
                manifestFile,
                File.ReadAllText(manifestFile).Replace("0.0.0.1", newVersion));
            testEnvironment.AppendPSModulePath(tmpDir.FullDirectoryPath);

            var dscModule = new DscModuleV2();
            {
                using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
                dscModule.InvokeSetResource(
                    pwsh,
                    new ValueSet(),
                    TestModule.SimpleTestResourceName,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName,
                        TestModule.SimpleTestResourceVersion));
            }

            {
                using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
                dscModule.InvokeSetResource(
                    pwsh,
                    new ValueSet(),
                    TestModule.SimpleTestResourceName,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName,
                        version: newVersion));
            }
        }

        /// <summary>
        /// Calls Invoke-DscResource invalid arguments.
        /// </summary>
        [Fact]
        public void InvokeSetResource_InvalidArguments()
        {
            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();

            var dscModule = new DscModuleV2();

            var settings = new ValueSet()
            {
                { "Fake", "please dont add it" },
            };

            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var e = Assert.Throws<InvokeDscResourceException>(() => dscModule.InvokeSetResource(
                pwsh,
                settings,
                TestModule.SimpleTestResourceName,
                PowerShellHelpers.CreateModuleSpecification(
                    TestModule.SimpleTestResourceModuleName)));

            Assert.Contains("The property 'Fake' cannot be found on this object.", e.Message);
            Assert.Equal(ConfigurationUnitResultSource.ConfigurationSet, e.ResultSource);
        }

        /// <summary>
        /// Tests GetDscResourcesInModule with versions.
        /// </summary>
        [Fact]
        public void InvokeSetResource_ModulePathSpaces()
        {
            // Copy test module to a directory with spaces.
            using var tmpDir = new TempDirectory(directoryName: Path.Combine(Guid.NewGuid().ToString(), "Path With Spaces"));
            tmpDir.CopyDirectory(this.fixture.TestModulesPath);
            var manifestFile = Path.Combine(
                tmpDir.FullDirectoryPath,
                TestModule.SimpleTestResourceModuleName,
                TestModule.SimpleTestResourceManifestFileName);

            var testEnvironment = this.fixture.PrepareTestProcessorEnvironment();
            testEnvironment.CleanupPSModulePath(this.fixture.TestModulesPath);
            testEnvironment.AppendPSModulePath(tmpDir.FullDirectoryPath);

            var dscModule = new DscModuleV2();

            var settings = new ValueSet()
            {
                { "secretCode", "4815162342" },
            };

            using PowerShell pwsh = PowerShell.Create(testEnvironment.Runspace);
            var testResult = dscModule.InvokeSetResource(
                pwsh,
                settings,
                TestModule.SimpleTestResourceName,
                PowerShellHelpers.CreateModuleSpecification(
                    TestModule.SimpleTestResourceModuleName));
        }
    }
}
