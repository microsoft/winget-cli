// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitProcessorTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Management.Automation;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.ProcessorEnvironments;
    using Microsoft.Management.Configuration.Processor.PowerShell.Unit;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.PowerShell.Commands;
    using Moq;
    using Windows.Foundation.Collections;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Configuration unit processor tests.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class ConfigurationUnitProcessorTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationUnitProcessorTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationUnitProcessorTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Call GetSettings with all intents should succeed.
        /// </summary>
        /// <param name="intent">Intent.</param>
        [Theory]
        [InlineData(ConfigurationUnitIntent.Inform)]
        [InlineData(ConfigurationUnitIntent.Assert)]
        [InlineData(ConfigurationUnitIntent.Apply)]
        public void GetSettings_Test(object intent)
        {
            string theKey = "key";
            string theValue = "value";
            var valueGetResult = new ValueSet
            {
                { theKey, theValue },
            };

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(m => m.InvokeGetResource(
                It.IsAny<ValueSet>(),
                It.IsAny<string>(),
                It.IsAny<ModuleSpecification?>()))
                .Returns(valueGetResult)
                .Verifiable();

            var unitResource = this.CreateUnitResource(Assert.IsType<ConfigurationUnitIntent>(intent));

            var unitProcessor = new PowerShellConfigurationUnitProcessor(processorEnvMock.Object, unitResource);

            var result = unitProcessor.GetSettings();

            processorEnvMock.Verify();

            Assert.True(result.Settings.Count == 1);
            Assert.True(result.Settings.ContainsKey(theKey));
            Assert.True(result.Settings.TryGetValue(theKey, out object keyValue));
            Assert.Equal(theValue, keyValue as string);
        }

        /// <summary>
        /// Tests GetSettings when a System.Management.Automation.RuntimeException is thrown.
        /// </summary>
        [Fact]
        public void GetSettings_Throws_Pwsh_RuntimeException()
        {
            var thrownException = new RuntimeException("a message");
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(m => m.InvokeGetResource(
                It.IsAny<ValueSet>(),
                It.IsAny<string>(),
                It.IsAny<ModuleSpecification?>()))
                .Throws(() => thrownException)
                .Verifiable();

            var unitResource = this.CreateUnitResource(ConfigurationUnitIntent.Inform);

            var unitProcessor = new PowerShellConfigurationUnitProcessor(processorEnvMock.Object, unitResource);

            var result = unitProcessor.GetSettings();

            processorEnvMock.Verify();

            // Do not check for the type.
            Assert.Equal(thrownException.HResult, result.ResultInformation.ResultCode.HResult);
            Assert.True(!string.IsNullOrWhiteSpace(result.ResultInformation.Description));
            Assert.Equal(ConfigurationUnitResultSource.Internal, result.ResultInformation.ResultSource);
        }

        /// <summary>
        /// Tests GetSettings when a Microsoft.PowerShell.Commands.WriteErrorException is thrown.
        /// </summary>
        [Fact]
        public void GetSettings_Throws_Pwsh_WriteErrorException()
        {
            var thrownException = new WriteErrorException("a message");
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(m => m.InvokeGetResource(
                It.IsAny<ValueSet>(),
                It.IsAny<string>(),
                It.IsAny<ModuleSpecification?>()))
                .Throws(() => thrownException)
                .Verifiable();

            var unitResource = this.CreateUnitResource(ConfigurationUnitIntent.Inform);

            var unitProcessor = new PowerShellConfigurationUnitProcessor(processorEnvMock.Object, unitResource);

            var result = unitProcessor.GetSettings();

            processorEnvMock.Verify();

            // Do not check for the type.
            Assert.Equal(thrownException.HResult, result.ResultInformation.ResultCode.HResult);
            Assert.True(!string.IsNullOrWhiteSpace(result.ResultInformation.Description));
            Assert.Equal(ConfigurationUnitResultSource.Internal, result.ResultInformation.ResultSource);
        }

        /// <summary>
        /// Call TestSettings with Inform intent is not allowed.
        /// </summary>
        [Fact]
        public void TestSettings_InformIntent()
        {
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            var unitResource = this.CreateUnitResource(ConfigurationUnitIntent.Inform);

            var unitProcessor = new PowerShellConfigurationUnitProcessor(processorEnvMock.Object, unitResource);

            Assert.Throws<NotSupportedException>(() => unitProcessor.TestSettings());
        }

        /// <summary>
        /// Call TestSettings with Assert and Apply should work.
        /// </summary>
        /// <param name="intent">Intent.</param>
        /// <param name="invokeTestResult">Invoke test result.</param>
        [Theory]
        [InlineData(ConfigurationUnitIntent.Assert, false)]
        [InlineData(ConfigurationUnitIntent.Apply, false)]
        [InlineData(ConfigurationUnitIntent.Assert, true)]
        [InlineData(ConfigurationUnitIntent.Apply, true)]
        public void TestSettings_TestSucceeded(object intent, bool invokeTestResult)
        {
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(m => m.InvokeTestResource(
                It.IsAny<ValueSet>(),
                It.IsAny<string>(),
                It.IsAny<ModuleSpecification?>()))
                .Returns(invokeTestResult)
                .Verifiable();

            var unitResource = this.CreateUnitResource(Assert.IsType<ConfigurationUnitIntent>(intent));

            var unitProcessor = new PowerShellConfigurationUnitProcessor(processorEnvMock.Object, unitResource);

            var testResult = unitProcessor.TestSettings();

            processorEnvMock.Verify();

            var expectedConfigTestResult = ConfigurationTestResult.Negative;
            if (invokeTestResult)
            {
                expectedConfigTestResult = ConfigurationTestResult.Positive;
            }

            Assert.Equal(expectedConfigTestResult, testResult.TestResult);
        }

        /// <summary>
        /// Tests TestSettings when a System.Management.Automation.RuntimeException is thrown.
        /// </summary>
        [Fact]
        public void TestSettings_Throws_Pwsh_RuntimeException()
        {
            var thrownException = new RuntimeException("a message");
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(m => m.InvokeTestResource(
                It.IsAny<ValueSet>(),
                It.IsAny<string>(),
                It.IsAny<ModuleSpecification?>()))
                .Throws(() => thrownException)
                .Verifiable();

            var unitResource = this.CreateUnitResource(ConfigurationUnitIntent.Assert);

            var unitProcessor = new PowerShellConfigurationUnitProcessor(processorEnvMock.Object, unitResource);

            var result = unitProcessor.TestSettings();

            processorEnvMock.Verify();

            Assert.Equal(ConfigurationTestResult.Failed, result.TestResult);

            // Do not check for the type.
            Assert.Equal(thrownException.HResult, result.ResultInformation.ResultCode.HResult);
            Assert.True(!string.IsNullOrWhiteSpace(result.ResultInformation.Description));
            Assert.Equal(ConfigurationUnitResultSource.Internal, result.ResultInformation.ResultSource);
        }

        /// <summary>
        /// Tests TestSettings when a Microsoft.PowerShell.Commands.WriteErrorException is thrown.
        /// </summary>
        [Fact]
        public void TestSettings_Throws_Pwsh_WriteErrorException()
        {
            var thrownException = new WriteErrorException("a message");
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(m => m.InvokeTestResource(
                It.IsAny<ValueSet>(),
                It.IsAny<string>(),
                It.IsAny<ModuleSpecification?>()))
                .Throws(() => thrownException)
                .Verifiable();

            var unitResource = this.CreateUnitResource(ConfigurationUnitIntent.Assert);

            var unitProcessor = new PowerShellConfigurationUnitProcessor(processorEnvMock.Object, unitResource);

            var result = unitProcessor.TestSettings();

            processorEnvMock.Verify();

            Assert.Equal(ConfigurationTestResult.Failed, result.TestResult);

            // Do not check for the type.
            Assert.Equal(thrownException.HResult, result.ResultInformation.ResultCode.HResult);
            Assert.True(!string.IsNullOrWhiteSpace(result.ResultInformation.Description));
            Assert.Equal(ConfigurationUnitResultSource.Internal, result.ResultInformation.ResultSource);
        }

        /// <summary>
        /// Call ApplySettings with invalid intents.
        /// </summary>
        /// <param name="intent">Intent.</param>
        [Theory]
        [InlineData(ConfigurationUnitIntent.Inform)]
        [InlineData(ConfigurationUnitIntent.Assert)]
        public void ApplySettings_InvalidIntent(object intent)
        {
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            var unitResource = this.CreateUnitResource(Assert.IsType<ConfigurationUnitIntent>(intent));

            var unitProcessor = new PowerShellConfigurationUnitProcessor(processorEnvMock.Object, unitResource);

            Assert.Throws<NotSupportedException>(() => unitProcessor.ApplySettings());
        }

        /// <summary>
        /// Call ApplySettings.
        /// </summary>
        /// <param name="rebootRequired">Reboot required.</param>
        [Theory]
        [InlineData(true)]
        [InlineData(false)]
        public void ApplySettings_Test(bool rebootRequired)
        {
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(m => m.InvokeSetResource(
                It.IsAny<ValueSet>(),
                It.IsAny<string>(),
                It.IsAny<ModuleSpecification?>()))
                .Returns(rebootRequired)
                .Verifiable();

            var unitResource = this.CreateUnitResource(ConfigurationUnitIntent.Apply);

            var unitProcessor = new PowerShellConfigurationUnitProcessor(processorEnvMock.Object, unitResource);

            var result = unitProcessor.ApplySettings();

            Assert.Equal(rebootRequired, result.RebootRequired);
        }

        /// <summary>
        /// Tests ApplySettings when a System.Management.Automation.RuntimeException is thrown.
        /// </summary>
        [Fact]
        public void ApplySettings_Throws_Pwsh_RuntimeException()
        {
            var thrownException = new RuntimeException("a message");
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(m => m.InvokeSetResource(
                It.IsAny<ValueSet>(),
                It.IsAny<string>(),
                It.IsAny<ModuleSpecification?>()))
                .Throws(() => thrownException)
                .Verifiable();

            var unitResource = this.CreateUnitResource(ConfigurationUnitIntent.Apply);

            var unitProcessor = new PowerShellConfigurationUnitProcessor(processorEnvMock.Object, unitResource);

            var result = unitProcessor.ApplySettings();

            processorEnvMock.Verify();

            // Do not check for the type.
            Assert.Equal(thrownException.HResult, result.ResultInformation.ResultCode.HResult);
            Assert.True(!string.IsNullOrWhiteSpace(result.ResultInformation.Description));
            Assert.Equal(ConfigurationUnitResultSource.Internal, result.ResultInformation.ResultSource);
        }

        /// <summary>
        /// Tests ApplySettings when a Microsoft.PowerShell.Commands.WriteErrorException is thrown.
        /// </summary>
        [Fact]
        public void ApplySettings_Throws_Pwsh_WriteErrorException()
        {
            var thrownException = new RuntimeException("a message");
            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(m => m.InvokeSetResource(
                It.IsAny<ValueSet>(),
                It.IsAny<string>(),
                It.IsAny<ModuleSpecification?>()))
                .Throws(() => thrownException)
                .Verifiable();

            var unitResource = this.CreateUnitResource(ConfigurationUnitIntent.Apply);

            var unitProcessor = new PowerShellConfigurationUnitProcessor(processorEnvMock.Object, unitResource);

            var result = unitProcessor.ApplySettings();

            processorEnvMock.Verify();

            // Do not check for the type.
            Assert.Equal(thrownException.HResult, result.ResultInformation.ResultCode.HResult);
            Assert.True(!string.IsNullOrWhiteSpace(result.ResultInformation.Description));
            Assert.Equal(ConfigurationUnitResultSource.Internal, result.ResultInformation.ResultSource);
        }

        /// <summary>
        /// Tests ApplySettings in limit mode.
        /// </summary>
        [Fact]
        public void ApplySettings_Test_LimitMode()
        {
            string theKey = "key";
            string theValue = "value";
            var valueGetResult = new ValueSet
            {
                { theKey, theValue },
            };

            var processorEnvMock = new Mock<IProcessorEnvironment>();
            processorEnvMock.Setup(m => m.InvokeGetResource(
                It.IsAny<ValueSet>(),
                It.IsAny<string>(),
                It.IsAny<ModuleSpecification?>()))
                .Returns(valueGetResult)
                .Verifiable();

            var unitResource = this.CreateUnitResource(ConfigurationUnitIntent.Apply);

            var unitProcessor = new PowerShellConfigurationUnitProcessor(processorEnvMock.Object, unitResource, true);

            // GetSettings can be called multiple times.
            var getResult = unitProcessor.GetSettings();
            getResult = unitProcessor.GetSettings();

            // TestSettings can be called only once.
            var testResult = unitProcessor.TestSettings();
            Assert.Throws<System.InvalidOperationException>(() => unitProcessor.TestSettings());

            // ApplySettings can be called only once.
            var applyResult = unitProcessor.ApplySettings();
            Assert.Throws<System.InvalidOperationException>(() => unitProcessor.ApplySettings());
        }

        private ConfigurationUnitAndResource CreateUnitResource(ConfigurationUnitIntent intent)
        {
            string resourceName = "xResourceName";
            return new ConfigurationUnitAndResource(
                new ConfigurationUnitAndModule(
                    new ConfigurationUnit
                    {
                        Type = resourceName,
                        Intent = intent,
                    },
                    string.Empty),
                new DscResourceInfoInternal(resourceName, null, null));
        }
    }
}
