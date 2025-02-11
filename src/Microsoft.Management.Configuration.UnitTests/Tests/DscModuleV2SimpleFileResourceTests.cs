// -----------------------------------------------------------------------------
// <copyright file="DscModuleV2SimpleFileResourceTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System.IO;
    using System.Management.Automation;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscModules;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Windows.Foundation.Collections;
    using Xunit;
    using Xunit.Abstractions;
    using static Microsoft.Management.Configuration.UnitTests.Helpers.PowerShellTestsConstants;

    /// <summary>
    /// Class that tests a little not that complex resource.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class DscModuleV2SimpleFileResourceTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="DscModuleV2SimpleFileResourceTests"/> class.
        /// </summary>
        /// <param name="fixture">Fixture.</param>
        /// <param name="log">log.</param>
        public DscModuleV2SimpleFileResourceTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Test SimpleFileResource Ensure Present and Absent when file doesn't exist.
        /// </summary>
        /// <param name="ensureValue">Ensure value.</param>
        /// <param name="expectedResult">Expected result.</param>
        [Theory]
        [InlineData("Absent", true)]
        [InlineData("Present", false)]
        public void SimpleFileResource_FileAbsent(string ensureValue, bool expectedResult)
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            // Doesn't create a file.
            using var tmpFile = new TempFile();

            var settings = new ValueSet
            {
                { "Path", tmpFile.FullFileName },
                { "Ensure", ensureValue },
            };

            var dscModule = new DscModuleV2();

            using PowerShell pwsh = PowerShell.Create(processorEnv.Runspace);
            Assert.Equal(
                expectedResult,
                dscModule.InvokeTestResource(
                    pwsh,
                    settings,
                    TestModule.SimpleFileResourceName,
                    PowerShellHelpers.CreateModuleSpecification(
                        TestModule.SimpleTestResourceModuleName)));
        }

        /// <summary>
        /// Test SimpleFileResource when the file exists.
        /// </summary>
        [Fact]
        public void SimpleFileResource_FilePresent()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            using var tmpFile = new TempFile();
            tmpFile.CreateFile();

            var settings = new ValueSet
            {
                { "Path", tmpFile.FullFileName },
                { "Ensure", "Present" },
            };

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(processorEnv.Runspace);
            Assert.True(dscModule.InvokeTestResource(
                pwsh,
                settings,
                TestModule.SimpleFileResourceName,
                PowerShellHelpers.CreateModuleSpecification(
                    TestModule.SimpleTestResourceModuleName)));
        }

        /// <summary>
        /// Test SimpleFileResource with different content.
        /// </summary>
        [Fact]
        public void SimpleFileResource_DifferentContent()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            using var tmpFile = new TempFile(content: "All work and no play makes Ruben a dull boy");

            var settings = new ValueSet
            {
                { "Path", tmpFile.FullFileName },
                { "Ensure", "Present" },
                { "Content", "Is that a from somewhere?" },
            };

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(processorEnv.Runspace);
            Assert.False(dscModule.InvokeTestResource(
                pwsh,
                settings,
                TestModule.SimpleFileResourceName,
                PowerShellHelpers.CreateModuleSpecification(
                    TestModule.SimpleTestResourceModuleName)));
        }

        /// <summary>
        /// Test SimpleFileResource with same context.
        /// </summary>
        [Fact]
        public void SimpleFileResource_SameContent()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            string content = "This is literally the same, dont fail";
            using var tmpFile = new TempFile(content: content);

            var settings = new ValueSet
            {
                { "Path", tmpFile.FullFileName },
                { "Ensure", "Present" },
                { "Content", content },
            };

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(processorEnv.Runspace);
            Assert.True(dscModule.InvokeTestResource(
                pwsh,
                settings,
                TestModule.SimpleFileResourceName,
                PowerShellHelpers.CreateModuleSpecification(
                    TestModule.SimpleTestResourceModuleName)));
        }

        /// <summary>
        /// Test SimpleFileResource Set creates the file.
        /// </summary>
        /// <param name="ensureValue">Ensure value.</param>
        /// <param name="preCondition">If file exists before calling set.</param>
        /// <param name="postCondition">If file exists after calling set.</param>
        [Theory]
        [InlineData("Absent", false, false)]
        [InlineData("Present", false, true)]
        [InlineData("Absent", true, false)]
        [InlineData("Present", true, true)]
        public void SimpleFileResource_Set_Ensure(string ensureValue, bool preCondition, bool postCondition)
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            // Doesn't create a file.
            using var tmpFile = new TempFile();

            var settings = new ValueSet
            {
                { "Path", tmpFile.FullFileName },
                { "Ensure", ensureValue },
            };

            if (preCondition)
            {
                tmpFile.CreateFile();
            }

            Assert.Equal(
                preCondition,
                File.Exists(tmpFile.FullFileName));

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(processorEnv.Runspace);
            dscModule.InvokeSetResource(
                pwsh,
                settings,
                TestModule.SimpleFileResourceName,
                PowerShellHelpers.CreateModuleSpecification(
                    TestModule.SimpleTestResourceModuleName));

            Assert.Equal(
                postCondition,
                File.Exists(tmpFile.FullFileName));
        }

        /// <summary>
        /// Tests SimpleFileResource Set writes to the file.
        /// </summary>
        /// <param name="preSetContent">The text in the file before calling Set.</param>
        /// <param name="postSetContent">The test in the file after calling Set.</param>
        [Theory]
        [InlineData(null, "after content")]
        [InlineData("i am a content", "and im another")]
        [InlineData("copy paste", "copy paste")]
        public void SimpleFileResource_Set_Content(string? preSetContent, string postSetContent)
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            // Doesn't create a file.
            using var tmpFile = new TempFile();
            if (preSetContent is not null)
            {
                tmpFile.CreateFile(preSetContent);
            }

            var settings = new ValueSet
            {
                { "Path", tmpFile.FullFileName },
                { "Ensure", "Present" },
                { "Content", postSetContent },
            };

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(processorEnv.Runspace);
            dscModule.InvokeSetResource(
                pwsh,
                settings,
                TestModule.SimpleFileResourceName,
                PowerShellHelpers.CreateModuleSpecification(
                    TestModule.SimpleTestResourceModuleName));

            Assert.Equal(
                postSetContent,
                File.ReadAllText(tmpFile.FullFileName));
        }

        /// <summary>
        /// Test SimpleFileResource Get.
        /// </summary>
        [Fact]
        public void SimpleFileResource_Get()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            string content = "I'm out of ideas";
            using var tmpFile = new TempFile(content: content);

            var settings = new ValueSet
            {
                { "Path", tmpFile.FullFileName },
            };

            var dscModule = new DscModuleV2();
            using PowerShell pwsh = PowerShell.Create(processorEnv.Runspace);
            var properties = dscModule.InvokeGetResource(
                                pwsh,
                                settings,
                                TestModule.SimpleFileResourceName,
                                PowerShellHelpers.CreateModuleSpecification(
                                    TestModule.SimpleTestResourceModuleName));

            Assert.True(properties.ContainsKey("Path"));
            Assert.True(properties.TryGetValue("Path", out object pathResult));
            Assert.Equal(tmpFile.FullFileName, pathResult as string);

            // Present is just the default value.
            Assert.True(properties.ContainsKey("Ensure"));
            Assert.True(properties.TryGetValue("Ensure", out object ensureResult));
            Assert.Equal("Present", ensureResult as string);

            Assert.True(properties.ContainsKey("Content"));
            Assert.True(properties.TryGetValue("Content", out object contentResult));
            Assert.Equal(content, contentResult);
        }
    }
}
