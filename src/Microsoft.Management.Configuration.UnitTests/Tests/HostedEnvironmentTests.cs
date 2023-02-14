// -----------------------------------------------------------------------------
// <copyright file="HostedEnvironmentTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System.Collections.Generic;
    using System.Management.Automation.Runspaces;
    using Microsoft.Management.Configuration.Processor.DscModule;
    using Microsoft.Management.Configuration.Processor.Runspaces;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Moq;
    using Xunit;
    using Xunit.Abstractions;
    using static Microsoft.Management.Configuration.Processor.Constants.PowerShellConstants;

    /// <summary>
    /// HostedEnvironment tests, that is more ProcessorEnvironmentBase tests for non forwarding functions.
    /// </summary>
    [Collection("UnitTestCollection")]
    public class HostedEnvironmentTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;
        private readonly Mock<IDscModule> dscModuleMock;

        /// <summary>
        /// Initializes a new instance of the <see cref="HostedEnvironmentTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log.</param>
        public HostedEnvironmentTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;

            this.dscModuleMock = new Mock<IDscModule>();
            this.dscModuleMock.Setup(
                m => m.ValidateModule(It.IsAny<Runspace>()));
            this.dscModuleMock.Setup(
                m => m.ModuleName)
                .Returns(Modules.PSDesiredStateConfiguration);
        }

        /// <summary>
        /// Test SetVariable and SetVariable methods.
        /// </summary>
        [Fact]
        public void HostedEnvironment_Variables()
        {
            var processorEnv = new HostedEnvironment(this.dscModuleMock.Object);

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
            var processorEnv = new HostedEnvironment(this.dscModuleMock.Object);

            string psmodulePathInput = "SetPSModulePathModulePath";
            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.NotEqual(psmodulePathInput, psModulePath);

            processorEnv.SetPSModulePath(psmodulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.Equal(psmodulePathInput, psModulePath);
        }

        /// <summary>
        /// Tests SetPsModulePaths.
        /// </summary>
        [Fact]
        public void HostedEnvironment_SetPSModulePaths()
        {
            var processorEnv = new HostedEnvironment(this.dscModuleMock.Object);

            var psmodulePathInput = new List<string>()
            {
                "Path1",
                "Path2",
                "Path3",
                "Path4",
            };
            string psmodulePathExpected = "Path1;Path2;Path3;Path4";

            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.NotEqual(psmodulePathExpected, psModulePath);

            processorEnv.SetPSModulePaths(psmodulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.Equal(psmodulePathExpected, psModulePath);
        }

        /// <summary>
        /// Tests AppendPSModulePath.
        /// </summary>
        [Fact]
        public void HostedEnvironment_AppendPSModulePath()
        {
            var processorEnv = new HostedEnvironment(this.dscModuleMock.Object);

            string psmodulePathInput = "AppendPSModulePathModulePath";
            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.False(psModulePath.EndsWith($";{psmodulePathInput}"));

            processorEnv.AppendPSModulePath(psmodulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.EndsWith($";{psmodulePathInput}", psModulePath);
        }

        /// <summary>
        /// Tests AppendPSModulePaths.
        /// </summary>
        [Fact]
        public void HostedEnvironment_AppendPSModulePaths()
        {
            var processorEnv = new HostedEnvironment(this.dscModuleMock.Object);

            var psmodulePathInput = new List<string>()
            {
                "AppendPSModulePathsPath1",
                "AppendPSModulePathsPath2",
                "AppendPSModulePathsPath3",
                "AppendPSModulePathsPath4",
            };
            string psmodulePathExpected = "AppendPSModulePathsPath1;AppendPSModulePathsPath2;AppendPSModulePathsPath3;AppendPSModulePathsPath4";

            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.False(psModulePath.EndsWith($";{psmodulePathExpected}"));

            processorEnv.AppendPSModulePaths(psmodulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.EndsWith($";{psmodulePathExpected}", psModulePath);
        }

        /// <summary>
        /// Tests PrependPSModulePath.
        /// </summary>
        [Fact]
        public void HostedEnvironment_PrependPSModulePath()
        {
            var processorEnv = new HostedEnvironment(this.dscModuleMock.Object);

            string psmodulePathInput = "PrependPSModulePathModulePath";
            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.False(psModulePath.StartsWith($";{psmodulePathInput}"));

            processorEnv.PrependPSModulePath(psmodulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.StartsWith($"{psmodulePathInput};", psModulePath);
        }

        /// <summary>
        /// Tests PrependPSModulePaths.
        /// </summary>
        [Fact]
        public void HostedEnvironment_PrependPSModulePaths()
        {
            var processorEnv = new HostedEnvironment(this.dscModuleMock.Object);

            var psmodulePathInput = new List<string>()
            {
                "PrependPSModulePathsPath1",
                "PrependPSModulePathsPath2",
                "PrependPSModulePathsPath3",
                "PrependPSModulePathsPath4",
            };
            string psmodulePathExpected = "PrependPSModulePathsPath1;PrependPSModulePathsPath2;PrependPSModulePathsPath3;PrependPSModulePathsPath4";

            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.False(psModulePath.StartsWith($";{psmodulePathExpected}"));

            processorEnv.PrependPSModulePaths(psmodulePathInput);
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.StartsWith($"{psmodulePathExpected};", psModulePath);
        }

        /// <summary>
        /// Test CleanupPSModulePath.
        /// </summary>
        [Fact]
        public void HostedEnvironment_CleanupPSModulePath()
        {
            var processorEnv = new HostedEnvironment(this.dscModuleMock.Object);
            var psmodulePathInput = new List<string>()
            {
                "CleanupPSModulePathPath1",
                "CleanupPSModulePathPath2",
                "CleanupPSModulePathPath3",
                "CleanupPSModulePathPath1",
                "CleanupPSModulePathPath1",
            };

            string psmodulePathExpected = "CleanupPSModulePathPath1;CleanupPSModulePathPath2;CleanupPSModulePathPath3;CleanupPSModulePathPath1;CleanupPSModulePathPath1";
            processorEnv.SetPSModulePaths(psmodulePathInput);
            string psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.Equal(psmodulePathExpected, psModulePath);

            processorEnv.CleanupPSModulePath("CleanupPSModulePathPath1");

            psmodulePathExpected = "CleanupPSModulePathPath2;CleanupPSModulePathPath3";
            psModulePath = processorEnv.GetVariable<string>(Variables.PSModulePath);
            Assert.Equal(psModulePath, psModulePath);
        }
    }
}
