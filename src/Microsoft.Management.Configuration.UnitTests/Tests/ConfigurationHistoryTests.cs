// -----------------------------------------------------------------------------
// <copyright file="ConfigurationHistoryTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for configuration history.
    /// </summary>
    [Collection("UnitTestCollection")]
    [OutOfProc]
    public class ConfigurationHistoryTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationHistoryTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationHistoryTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
        }

        /// <summary>
        /// Checks that the history matches the applied set.
        /// </summary>
        [Fact]
        public void ApplySet_HistoryMatches_0_1()
        {
            this.RunApplyHistoryMatchTest(
                @"
properties:
  configurationVersion: 0.1
  assertions:
    - resource: Assert
      id: AssertIdentifier1
      directives:
        module: Module
      settings:
        Setting1: '1'
        Setting2: 2
    - resource: Assert
      id: AssertIdentifier2
      dependsOn:
        - AssertIdentifier1
      directives:
        module: Module
      settings:
        Setting1:
          Setting2: 2
  parameters:
    - resource: Inform
      id: InformIdentifier1
      directives:
        module: Module2
      settings:
        Setting1:
          Setting2:
            Setting3: 3
  resources:
    - resource: Apply
", new string[] { "AssertIdentifier2" });
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1011:Closing square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/2927")]
        private void RunApplyHistoryMatchTest(string contents, string[]? inactiveIdentifiers = null)
        {
            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            OpenConfigurationSetResult configurationSetResult = processor.OpenConfigurationSet(this.CreateStream(contents));
            ConfigurationSet configurationSet = configurationSetResult.Set;
            Assert.NotNull(configurationSet);

            configurationSet.Name = "Test Name";
            configurationSet.Origin = "Test Origin";
            configurationSet.Path = "Test Path";

            if (inactiveIdentifiers != null)
            {
                foreach (string identifier in inactiveIdentifiers)
                {
                    foreach (ConfigurationUnit unit in configurationSet.Units)
                    {
                        if (unit.Identifier == identifier)
                        {
                            unit.IsActive = false;
                        }
                    }
                }
            }

            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.Null(result.ResultCode);

            ConfigurationSet? historySet = null;

            foreach (ConfigurationSet set in processor.GetConfigurationHistory())
            {
                if (set.InstanceIdentifier == configurationSet.InstanceIdentifier)
                {
                    historySet = set;
                }
            }

            this.AssertSetsEqual(configurationSet, historySet);
        }

        private void AssertSetsEqual(ConfigurationSet expectedSet, ConfigurationSet? actualSet)
        {
            Assert.NotNull(actualSet);
            Assert.Equal(expectedSet.Name, actualSet.Name);
            Assert.Equal(expectedSet.Origin, actualSet.Origin);
            Assert.Equal(expectedSet.Path, actualSet.Path);
            Assert.NotEqual(DateTimeOffset.UnixEpoch, actualSet.FirstApply);
            Assert.Equal(expectedSet.SchemaVersion, actualSet.SchemaVersion);
            Assert.Equal(expectedSet.SchemaUri, actualSet.SchemaUri);
            Assert.True(expectedSet.Metadata.ContentEquals(actualSet.Metadata));

            this.AssertUnitsListEqual(expectedSet.Units, actualSet.Units);
        }

        private void AssertUnitsListEqual(IList<ConfigurationUnit> expectedUnits, IList<ConfigurationUnit> actualUnits)
        {
            Assert.Equal(expectedUnits.Count, actualUnits.Count);

            foreach (ConfigurationUnit expectedUnit in expectedUnits)
            {
                ConfigurationUnit? actualUnit = null;
                foreach (ConfigurationUnit historyUnit in actualUnits)
                {
                    if (historyUnit.InstanceIdentifier == expectedUnit.InstanceIdentifier)
                    {
                        actualUnit = historyUnit;
                    }
                }

                this.AssertUnitsEqual(expectedUnit, actualUnit);
            }
        }

        private void AssertUnitsEqual(ConfigurationUnit expectedUnit, ConfigurationUnit? actualUnit)
        {
            Assert.NotNull(actualUnit);
            Assert.Equal(expectedUnit.Type, actualUnit.Type);
            Assert.Equal(expectedUnit.Identifier, actualUnit.Identifier);
            Assert.Equal(expectedUnit.Intent, actualUnit.Intent);
            Assert.Equal(expectedUnit.Dependencies, actualUnit.Dependencies);
            Assert.True(expectedUnit.Metadata.ContentEquals(actualUnit.Metadata));
            Assert.True(expectedUnit.Settings.ContentEquals(actualUnit.Settings));
            Assert.Equal(expectedUnit.IsActive, actualUnit.IsActive);
            Assert.Equal(expectedUnit.IsGroup, actualUnit.IsGroup);

            if (expectedUnit.IsGroup)
            {
                this.AssertUnitsListEqual(expectedUnit.Units, actualUnit.Units);
            }
        }
    }
}
