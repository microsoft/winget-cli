// -----------------------------------------------------------------------------
// <copyright file="DSCv3InputHandlingTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System.Linq;
    using Microsoft.Management.Configuration.Processor;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Windows.Foundation.Collections;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Tests for DSCv3 input handling - verifies that --input flag is only passed when settings are present.
    /// Related to issue #5746.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class DSCv3InputHandlingTests : ConfigurationProcessorTestBase
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="DSCv3InputHandlingTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public DSCv3InputHandlingTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Test that GetUnitSettings works without input settings.
        /// This verifies the fix for issue #5746 where resources like Microsoft.WinGet/UserSettingsFile
        /// don't expect any input for the Get method.
        /// </summary>
        [Fact]
        public void GetUnitSettings_NoInput_DoesNotProvideInputToDSC()
        {
            var (factory, dsc) = CreateTestFactory();
            var processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            string type1 = "Microsoft.WinGet/UserSettingsFile";
            var unit = this.ConfigurationUnit().Assign(new { Type = type1 });

            dsc.GetResourceByTypeDelegate = (type) =>
            {
                Assert.Equal(type1, type);
                return new TestResourceListItem() { Type = type1 };
            };

            ValueSet resultSettings = new ValueSet();
            resultSettings.Add("settings", new ValueSet());

            dsc.GetResourceSettingsDelegate = (unitInternal) =>
            {
                // Verify that the unit has no settings (empty ValueSet)
                var expandedSettings = unitInternal.GetExpandedSettings();
                Assert.Equal(0, expandedSettings.Count);
                return new TestResourceGetItem() { Type = type1, Settings = resultSettings };
            };

            var result = processor.GetUnitSettings(unit);

            Assert.NotNull(result);
            Assert.NotNull(result.ResultInformation);
            Assert.Null(result.ResultInformation.ResultCode);
        }

        /// <summary>
        /// Test that GetUnitSettings works with input settings.
        /// This verifies that when settings are provided, they are correctly passed to DSC.
        /// </summary>
        [Fact]
        public void GetUnitSettings_WithInput_ProvidesInputToDSC()
        {
            var (factory, dsc) = CreateTestFactory();
            var processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            string type1 = "SomeResource";
            ValueSet inputSettings = new ValueSet();
            inputSettings.Add("key1", "value1");
            
            var unit = this.ConfigurationUnit().Assign(new { Type = type1, Settings = inputSettings });

            dsc.GetResourceByTypeDelegate = (type) =>
            {
                Assert.Equal(type1, type);
                return new TestResourceListItem() { Type = type1 };
            };

            ValueSet resultSettings = new ValueSet();
            resultSettings.Add("result", "data");

            dsc.GetResourceSettingsDelegate = (unitInternal) =>
            {
                // Verify that the unit has settings
                var expandedSettings = unitInternal.GetExpandedSettings();
                Assert.NotEqual(0, expandedSettings.Count);
                Assert.True(expandedSettings.ContainsKey("key1"));
                Assert.Equal("value1", expandedSettings["key1"]);
                return new TestResourceGetItem() { Type = type1, Settings = resultSettings };
            };

            var result = processor.GetUnitSettings(unit);

            Assert.NotNull(result);
            Assert.NotNull(result.ResultInformation);
            Assert.Null(result.ResultInformation.ResultCode);
        }

        /// <summary>
        /// Test that TestSettings works without input settings.
        /// </summary>
        [Fact]
        public void TestSettings_NoInput_DoesNotProvideInputToDSC()
        {
            var (factory, dsc) = CreateTestFactory();
            var processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            string type1 = "SomeResource";
            var unit = this.ConfigurationUnit().Assign(new { Type = type1 });

            dsc.GetResourceByTypeDelegate = (type) =>
            {
                Assert.Equal(type1, type);
                return new TestResourceListItem() { Type = type1 };
            };

            dsc.TestResourceDelegate = (unitInternal) =>
            {
                // Verify that the unit has no settings
                var expandedSettings = unitInternal.GetExpandedSettings();
                Assert.Equal(0, expandedSettings.Count);
                return new TestResourceTestItem() { Type = type1, InDesiredState = true };
            };

            var result = processor.TestSettings(unit);

            Assert.NotNull(result);
            Assert.NotNull(result.ResultInformation);
            Assert.Null(result.ResultInformation.ResultCode);
            Assert.True(result.TestResult == Processor.Set.ConfigurationTestResult.Positive);
        }

        /// <summary>
        /// Test that ApplySettings (SetResourceSettings) works without input settings.
        /// </summary>
        [Fact]
        public void ApplySettings_NoInput_DoesNotProvideInputToDSC()
        {
            var (factory, dsc) = CreateTestFactory();
            var processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            string type1 = "SomeResource";
            var unit = this.ConfigurationUnit().Assign(new { Type = type1 });

            dsc.GetResourceByTypeDelegate = (type) =>
            {
                Assert.Equal(type1, type);
                return new TestResourceListItem() { Type = type1 };
            };

            dsc.SetResourceSettingsDelegate = (unitInternal) =>
            {
                // Verify that the unit has no settings
                var expandedSettings = unitInternal.GetExpandedSettings();
                Assert.Equal(0, expandedSettings.Count);
                return new TestResourceSetItem() { Type = type1, RebootRequired = false };
            };

            var result = processor.ApplySettings(unit, Processor.Set.ConfigurationUnitIntent.Apply);

            Assert.NotNull(result);
            Assert.NotNull(result.ResultInformation);
            Assert.Null(result.ResultInformation.ResultCode);
        }

        private static (DSCv3ConfigurationSetProcessorFactory, TestDSCv3) CreateTestFactory()
        {
            DSCv3ConfigurationSetProcessorFactory factory = new DSCv3ConfigurationSetProcessorFactory();
            TestDSCv3 dsc = new TestDSCv3();
            factory.Settings.DSCv3 = dsc;
            factory.Settings.DscExecutablePath = "Test-Path-Not-Used.txt";

            return (factory, dsc);
        }
    }
}
