// -----------------------------------------------------------------------------
// <copyright file="ProcessorEnvironmentTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Moq;
    using Xunit;
    using Xunit.Abstractions;
    using static Microsoft.Management.Configuration.Processor.PowerShell.Constants.PowerShellConstants;

    /// <summary>
    /// HostedEnvironment tests, that is more ProcessorEnvironmentBase tests for non forwarding functions.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class ProcessorEnvironmentTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProcessorEnvironmentTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log.</param>
        public ProcessorEnvironmentTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Test SetVariable and SetVariable methods.
        /// </summary>
        [Fact]
        public void HostedEnvironment_Variables()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            // As string.
            string var1Name = "var1";
            string var1 = "This is a string";
            processorEnv.SetVariable(var1Name, var1);
            string var1Result = processorEnv.GetVariable<string>(var1Name);
            Assert.Equal(var1, var1Result);

            // As int.
            string var2Name = "var2";
            int var2 = 42;
            processorEnv.SetVariable(var2Name, var2);
            int var2Result = processorEnv.GetVariable<int>(var2Name);

            // Wrong type.
            Assert.Throws<System.InvalidCastException>(() => processorEnv.GetVariable<int>(var1Name));
        }

        /// <summary>
        /// Tests SetPsModulePath.
        /// </summary>
        [Fact]
        public void HostedEnvironment_SetPSModulePath()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            string psModulePathInput = "SetPSModulePathModulePath";
            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.NotEqual(psModulePathInput, psModulePath);

            processorEnv.SetPSModulePath(psModulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.Equal(psModulePathInput, psModulePath);
        }

        /// <summary>
        /// Tests SetPsModulePaths.
        /// </summary>
        [Fact]
        public void HostedEnvironment_SetPSModulePaths()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            var psModulePathInput = new List<string>()
            {
                "Path1",
                "Path2",
                "Path3",
                "Path4",
            };
            string psModulePathExpected = "Path1;Path2;Path3;Path4";

            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.NotEqual(psModulePathExpected, psModulePath);

            processorEnv.SetPSModulePaths(psModulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.Equal(psModulePathExpected, psModulePath);
        }

        /// <summary>
        /// Tests AppendPSModulePath.
        /// </summary>
        [Fact]
        public void HostedEnvironment_AppendPSModulePath()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            string psModulePathInput = "AppendPSModulePathModulePath";
            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.False(psModulePath.EndsWith($";{psModulePathInput}"));

            processorEnv.AppendPSModulePath(psModulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.EndsWith($";{psModulePathInput}", psModulePath);

            // No duplicates
            processorEnv.AppendPSModulePath(psModulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.False(psModulePath.EndsWith($";{psModulePathInput};{psModulePathInput}"));
            Assert.EndsWith($";{psModulePathInput}", psModulePath);
        }

        /// <summary>
        /// Tests AppendPSModulePaths.
        /// </summary>
        [Fact]
        public void HostedEnvironment_AppendPSModulePaths()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            var psModulePathInput = new List<string>()
            {
                "AppendPSModulePathsPath1",
                "AppendPSModulePathsPath2",
                "AppendPSModulePathsPath3",
                "AppendPSModulePathsPath4",
            };
            string psModulePathExpected = "AppendPSModulePathsPath1;AppendPSModulePathsPath2;AppendPSModulePathsPath3;AppendPSModulePathsPath4";

            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.False(psModulePath.EndsWith($";{psModulePathExpected}"));

            processorEnv.AppendPSModulePaths(psModulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.EndsWith($";{psModulePathExpected}", psModulePath);

            // No duplicates
            psModulePathInput = new List<string>()
            {
                "AppendPSModulePathsPath1",
                "AppendPSModulePathsPath5",
                "AppendPSModulePathsPath2",
            };
            processorEnv.AppendPSModulePaths(psModulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.EndsWith($";{psModulePathExpected};AppendPSModulePathsPath5", psModulePath);
        }

        /// <summary>
        /// Tests PrependPSModulePath.
        /// </summary>
        [Fact]
        public void HostedEnvironment_PrependPSModulePath()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            string psModulePathInput = "PrependPSModulePathModulePath";
            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.False(psModulePath.StartsWith($";{psModulePathInput}"));

            processorEnv.PrependPSModulePath(psModulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.StartsWith($"{psModulePathInput};", psModulePath);

            // No duplicates
            processorEnv.PrependPSModulePath(psModulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.False(psModulePath.StartsWith($"{psModulePathInput};{psModulePathInput};"));
            Assert.StartsWith($"{psModulePathInput};", psModulePath);
        }

        /// <summary>
        /// Tests PrependPSModulePaths.
        /// </summary>
        [Fact]
        public void HostedEnvironment_PrependPSModulePaths()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();

            var psModulePathInput = new List<string>()
            {
                "PrependPSModulePathsPath1",
                "PrependPSModulePathsPath2",
                "PrependPSModulePathsPath3",
                "PrependPSModulePathsPath4",
            };
            string psModulePathExpected = "PrependPSModulePathsPath1;PrependPSModulePathsPath2;PrependPSModulePathsPath3;PrependPSModulePathsPath4";

            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.False(psModulePath.StartsWith($";{psModulePathExpected}"));

            processorEnv.PrependPSModulePaths(psModulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.StartsWith($"{psModulePathExpected};", psModulePath);

            // No duplicates
            psModulePathInput = new List<string>()
            {
                "PrependPSModulePathsPath1",
                "PrependPSModulePathsPath5",
                "PrependPSModulePathsPath2",
            };
            processorEnv.PrependPSModulePaths(psModulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.StartsWith($"PrependPSModulePathsPath5;{psModulePathExpected};", psModulePath);
        }

        /// <summary>
        /// Test CleanupPSModulePath.
        /// </summary>
        [Fact]
        public void HostedEnvironment_CleanupPSModulePath()
        {
            var processorEnv = this.fixture.PrepareTestProcessorEnvironment();
            var psModulePathInput = new List<string>()
            {
                "CleanupPSModulePathPath1",
                "CleanupPSModulePathPath2",
                "CleanupPSModulePathPath3",
                "CleanupPSModulePathPath1",
                "CleanupPSModulePathPath1",
            };

            string psModulePathExpected = "CleanupPSModulePathPath1;CleanupPSModulePathPath2;CleanupPSModulePathPath3;CleanupPSModulePathPath1;CleanupPSModulePathPath1";
            processorEnv.SetPSModulePaths(psModulePathInput);
            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.Equal(psModulePathExpected, psModulePath);

            processorEnv.CleanupPSModulePath("CleanupPSModulePathPath1");

            psModulePathExpected = "CleanupPSModulePathPath2;CleanupPSModulePathPath3";
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.Equal(psModulePathExpected, psModulePath);
        }
    }
}
