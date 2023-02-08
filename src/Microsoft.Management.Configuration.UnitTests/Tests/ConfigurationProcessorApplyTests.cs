// -----------------------------------------------------------------------------
// <copyright file="ConfigurationProcessorApplyTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.IO;
    using System.Linq;
    using System.Runtime.InteropServices;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Microsoft.VisualStudio.TestPlatform.ObjectModel;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for running test on the processor.
    /// </summary>
    [Collection("UnitTestCollection")]
    public class ConfigurationProcessorApplyTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationProcessorApplyTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationProcessorApplyTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
        }

        /// <summary>
        /// An error creating the set processor results in an error for the function.
        /// </summary>
        [Fact]
        public void ApplySet_SetProcessorError()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            factory.Exceptions.Add(configurationSet, new FileNotFoundException());

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            Assert.Throws<FileNotFoundException>(() => processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None));
        }

        /// <summary>
        /// Multiple configuration units with the same identifier.
        /// </summary>
        [Fact]
        public void ApplySet_DuplicateIdentifiers()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnit1 = new ConfigurationUnit();
            ConfigurationUnit configurationUnit2 = new ConfigurationUnit();
            ConfigurationUnit configurationUnitDifferentIdentifier = new ConfigurationUnit();
            string sharedIdentifier = "SameIdentifier";
            configurationUnit1.Identifier = sharedIdentifier;
            configurationUnit2.Identifier = sharedIdentifier;
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnit1, configurationUnit2, configurationUnitDifferentIdentifier };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER, result.ResultCode.HResult);
            Assert.Equal(3, result.UnitResults.Count);

            foreach (var configurationUnit in new ConfigurationUnit[] { configurationUnit1, configurationUnit2 })
            {
                ApplyConfigurationUnitResult unitResult = result.UnitResults.First(x => x.Unit == configurationUnit);
                Assert.NotNull(unitResult);
                Assert.False(unitResult.PreviouslyInDesiredState);
                Assert.False(unitResult.RebootRequired);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.NotNull(unitResult.ResultInformation.ResultCode);
                Assert.Equal(Errors.WINGET_CONFIG_ERROR_DUPLICATE_IDENTIFIER, unitResult.ResultInformation.ResultCode.HResult);
            }

            ApplyConfigurationUnitResult unitResultDifferentIdentifier = result.UnitResults.First(x => x.Unit == configurationUnitDifferentIdentifier);
            Assert.NotNull(unitResultDifferentIdentifier);
            Assert.False(unitResultDifferentIdentifier.PreviouslyInDesiredState);
            Assert.False(unitResultDifferentIdentifier.RebootRequired);
            Assert.NotNull(unitResultDifferentIdentifier.ResultInformation);
            Assert.Null(unitResultDifferentIdentifier.ResultInformation.ResultCode);
        }

        /// <summary>
        /// A configuration unit has a dependency that is not in the set.
        /// </summary>
        [Fact]
        public void ApplySet_MissingDependency()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnit = new ConfigurationUnit();
            ConfigurationUnit configurationUnitMissingDependency = new ConfigurationUnit();
            configurationUnit.Identifier = "Identifier";
            configurationUnitMissingDependency.Dependencies = new string[] { "Dependency" };
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnit, configurationUnitMissingDependency };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_MISSING_DEPENDENCY, result.ResultCode.HResult);
            Assert.Equal(2, result.UnitResults.Count);

            ApplyConfigurationUnitResult unitResult = result.UnitResults.First(x => x.Unit == configurationUnit);
            Assert.NotNull(unitResult);
            Assert.False(unitResult.PreviouslyInDesiredState);
            Assert.False(unitResult.RebootRequired);
            Assert.NotNull(unitResult.ResultInformation);
            Assert.Null(unitResult.ResultInformation.ResultCode);

            unitResult = result.UnitResults.First(x => x.Unit == configurationUnitMissingDependency);
            Assert.NotNull(unitResult);
            Assert.False(unitResult.PreviouslyInDesiredState);
            Assert.False(unitResult.RebootRequired);
            Assert.NotNull(unitResult.ResultInformation);
            Assert.NotNull(unitResult.ResultInformation.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_MISSING_DEPENDENCY, unitResult.ResultInformation.ResultCode.HResult);
        }

        /// <summary>
        /// The configuration set has a dependency cycle.
        /// </summary>
        [Fact]
        public void ApplySet_DependencyCycle()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnit1 = new ConfigurationUnit();
            ConfigurationUnit configurationUnit2 = new ConfigurationUnit();
            ConfigurationUnit configurationUnit3 = new ConfigurationUnit();
            configurationUnit1.Identifier = "Identifier1";
            configurationUnit2.Identifier = "Identifier2";
            configurationUnit3.Identifier = "Identifier3";
            configurationUnit1.Dependencies = new string[] { "Identifier3" };
            configurationUnit2.Dependencies = new string[] { "Identifier1" };
            configurationUnit3.Dependencies = new string[] { "Identifier2" };
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnit1, configurationUnit2, configurationUnit3 };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED, result.ResultCode.HResult);
            Assert.Equal(3, result.UnitResults.Count);

            foreach (var unitResult in result.UnitResults)
            {
                Assert.NotNull(unitResult);
                Assert.False(unitResult.PreviouslyInDesiredState);
                Assert.False(unitResult.RebootRequired);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.NotNull(unitResult.ResultInformation.ResultCode);
                Assert.Equal(Errors.WINGET_CONFIG_ERROR_DEPENDENCY_UNSATISFIED, unitResult.ResultInformation.ResultCode.HResult);
            }
        }

        /// <summary>
        /// Checks that the intent for configuration units is handled properly.
        /// </summary>
        [Fact]
        public void ApplySet_IntentRespected()
        {
            ConfigurationSet configurationSet = new ConfigurationSet();
            ConfigurationUnit configurationUnitAssert = new ConfigurationUnit { Intent = ConfigurationUnitIntent.Assert };
            ConfigurationUnit configurationUnitInform = new ConfigurationUnit { Intent = ConfigurationUnitIntent.Inform };
            ConfigurationUnit configurationUnitApply = new ConfigurationUnit { Intent = ConfigurationUnitIntent.Apply };
            configurationSet.ConfigurationUnits = new ConfigurationUnit[] { configurationUnitInform, configurationUnitApply, configurationUnitAssert };

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            TestConfigurationUnitProcessor unitProcessorAssert = setProcessor.CreateTestProcessor(configurationUnitAssert);
            TestConfigurationUnitProcessor unitProcessorInform = setProcessor.CreateTestProcessor(configurationUnitInform);
            TestConfigurationUnitProcessor unitProcessorApply = setProcessor.CreateTestProcessor(configurationUnitApply);
            unitProcessorApply.TestSettingsDelegate = () => new TestSettingsResult { TestResult = ConfigurationTestResult.Negative };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.Null(result.ResultCode);
            Assert.Equal(3, result.UnitResults.Count);

            foreach (var unitResult in result.UnitResults)
            {
                Assert.NotNull(unitResult);
                Assert.False(unitResult.PreviouslyInDesiredState);
                Assert.False(unitResult.RebootRequired);
                Assert.NotNull(unitResult.ResultInformation);
                Assert.Null(unitResult.ResultInformation.ResultCode);
            }

            Assert.Equal(1, unitProcessorAssert.TestSettingsCalls);
            Assert.Equal(0, unitProcessorAssert.GetSettingsCalls);
            Assert.Equal(0, unitProcessorAssert.ApplySettingsCalls);

            Assert.Equal(0, unitProcessorInform.TestSettingsCalls);
            Assert.Equal(1, unitProcessorInform.GetSettingsCalls);
            Assert.Equal(0, unitProcessorInform.ApplySettingsCalls);

            Assert.Equal(1, unitProcessorApply.TestSettingsCalls);
            Assert.Equal(0, unitProcessorApply.GetSettingsCalls);
            Assert.Equal(1, unitProcessorApply.ApplySettingsCalls);
        }

        /// <summary>
        /// An assertion fails to run.
        /// </summary>
        [Fact]
        public void ApplySet_AssertionFailure()
        {
        }

        /// <summary>
        /// An assertion is found to be false.
        /// </summary>
        [Fact]
        public void ApplySet_AssertionNegative()
        {
        }

        /// <summary>
        /// A unit in the correct state is not applied again.
        /// </summary>
        [Fact]
        public void ApplySet_UnitAlreadyInCorrectState()
        {
        }

        /// <summary>
        /// Checks the progress reporting.
        /// </summary>
        [Fact]
        public void ApplySet_Progress()
        {
        }
    }
}
