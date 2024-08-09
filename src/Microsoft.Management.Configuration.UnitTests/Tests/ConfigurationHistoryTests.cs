// -----------------------------------------------------------------------------
// <copyright file="ConfigurationHistoryTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
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
    [InProc]
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
        [OutOfProc]
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

        /// <summary>
        /// Checks that the history matches the applied set.
        /// </summary>
        [Fact]
        [OutOfProc]
        public void ApplySet_HistoryMatches_0_2()
        {
            this.RunApplyHistoryMatchTest(
                @"
properties:
  configurationVersion: 0.2
  assertions:
    - resource: Module/Assert
      id: AssertIdentifier1
      settings:
        Setting1: '1'
        Setting2: 2
    - resource: Module/Assert
      id: AssertIdentifier2
      dependsOn:
        - AssertIdentifier1
      directives:
        description: Describe!
      settings:
        Setting1:
          Setting2: 2
  parameters:
    - resource: Module2/Inform
      id: InformIdentifier1
      settings:
        Setting1:
          Setting2:
            Setting3: 3
  resources:
    - resource: Apply
", new string[] { "AssertIdentifier2" });
        }

        /// <summary>
        /// Checks that the history matches the applied set.
        /// </summary>
        [Fact]
        public void ApplySet_HistoryMatches_0_3()
        {
            this.RunApplyHistoryMatchTest(
                @"
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
metadata:
  a: 1
  b: '2'
variables:
  v1: var1
  v2: 42
resources:
  - name: Name
    type: Module/Resource
    metadata:
      e: '5'
      f: 6
    properties:
      c: 3
      d: '4'
  - name: Name2
    type: Module/Resource2
    dependsOn:
      - Name
    properties:
      l: '10'
    metadata:
      i: '7'
      j: 8
      q: 42
  - name: Group
    type: Module2/Resource
    metadata:
      isGroup: true
    properties:
      resources:
        - name: Child1
          type: Module3/Resource
          metadata:
            e: '5'
            f: 6
          properties:
            c: 3
            d: '4'
        - name: Child2
          type: Module4/Resource2
          properties:
            l: '10'
          metadata:
            i: '7'
            j: 8
            q: 42
");
        }

        /// <summary>
        /// Applies a set, reads the history, changes the read set and reapplies it.
        /// </summary>
        [Fact]
        [OutOfProc]
        public void ApplySet_ChangeHistory()
        {
            string disabledIdentifier = "AssertIdentifier2";

            ConfigurationSet returnedSet = this.RunApplyHistoryMatchTest(
                @"
properties:
  configurationVersion: 0.2
  assertions:
    - resource: Module/Assert
      id: AssertIdentifier1
      settings:
        Setting1: '1'
        Setting2: 2
    - resource: Module/Assert
      id: AssertIdentifier2
      dependsOn:
        - AssertIdentifier1
      directives:
        description: Describe!
      settings:
        Setting1:
          Setting2: 2
  parameters:
    - resource: Module2/Inform
      id: InformIdentifier1
      settings:
        Setting1:
          Setting2:
            Setting3: 3
  resources:
    - resource: Apply
", new string[] { disabledIdentifier });

            foreach (ConfigurationUnit unit in returnedSet.Units)
            {
                if (unit.Identifier == disabledIdentifier)
                {
                    unit.IsActive = true;
                }
            }

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ApplyConfigurationSetResult result = processor.ApplySet(returnedSet, ApplyConfigurationSetFlags.None);
            Assert.NotNull(result);
            Assert.Null(result.ResultCode);

            ConfigurationSet? historySet = null;

            foreach (ConfigurationSet set in processor.GetConfigurationHistory())
            {
                if (set.InstanceIdentifier == returnedSet.InstanceIdentifier)
                {
                    historySet = set;
                }
            }

            this.AssertSetsEqual(returnedSet, historySet);
        }

        /// <summary>
        /// Applies a set, reads the history and removes it.
        /// </summary>
        [Fact]
        [OutOfProc]
        public void ApplySet_RemoveHistory()
        {
            ConfigurationSet returnedSet = this.RunApplyHistoryMatchTest(
                @"
properties:
  configurationVersion: 0.2
  assertions:
    - resource: Module/Assert
      id: AssertIdentifier1
      settings:
        Setting1: '1'
        Setting2: 2
    - resource: Module/Assert
      id: AssertIdentifier2
      dependsOn:
        - AssertIdentifier1
      directives:
        description: Describe!
      settings:
        Setting1:
          Setting2: 2
  parameters:
    - resource: Module2/Inform
      id: InformIdentifier1
      settings:
        Setting1:
          Setting2:
            Setting3: 3
  resources:
    - resource: Apply
");

            returnedSet.Remove();

            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            ConfigurationSet? historySet = null;

            foreach (ConfigurationSet set in processor.GetConfigurationHistory())
            {
                if (set.InstanceIdentifier == returnedSet.InstanceIdentifier)
                {
                    historySet = set;
                }
            }

            Assert.Null(historySet);
        }

        [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1011:Closing square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/2927")]
        private ConfigurationSet RunApplyHistoryMatchTest(string contents, string[]? inactiveIdentifiers = null)
        {
            TestConfigurationProcessorFactory factory = new TestConfigurationProcessorFactory();
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            OpenConfigurationSetResult configurationSetResult = processor.OpenConfigurationSet(this.CreateStream(contents));
            ConfigurationSet configurationSet = configurationSetResult.Set;
            Assert.NotNull(configurationSet);

            TestConfigurationSetProcessor setProcessor = factory.CreateTestProcessor(configurationSet);
            setProcessor.EnableDefaultGroupProcessorCreation = true;

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
            this.AssertResultsEqual(result, historySet);
            return historySet;
        }

        private void AssertSetsEqual(ConfigurationSet expectedSet, [NotNull] ConfigurationSet? actualSet)
        {
            Assert.NotNull(actualSet);
            Assert.Equal(expectedSet.Name, actualSet.Name);
            Assert.Equal(expectedSet.Origin, actualSet.Origin);
            Assert.Equal(expectedSet.Path, actualSet.Path);

            Assert.Equal(ConfigurationSetState.Completed, actualSet.State);

            this.AssertTimeNotZero(actualSet.FirstApply);
            this.AssertTimeNotZero(actualSet.ApplyBegun);
            this.AssertTimeNotZero(actualSet.ApplyEnded);
            Assert.True(actualSet.FirstApply <= actualSet.ApplyBegun);
            Assert.True(actualSet.ApplyBegun <= actualSet.ApplyEnded);

            Assert.Equal(expectedSet.SchemaVersion, actualSet.SchemaVersion);
            Assert.Equal(expectedSet.SchemaUri, actualSet.SchemaUri);
            Assert.True(expectedSet.Metadata.ContentEquals(actualSet.Metadata));

            this.AssertUnitsListEqual(expectedSet.Units, actualSet.Units);
        }

        private void AssertTimeNotZero(DateTimeOffset actualTime)
        {
            Assert.NotEqual(DateTimeOffset.UnixEpoch, actualTime);
            Assert.NotEqual(DateTimeOffset.MinValue, actualTime);
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

        private void AssertResultsEqual(ApplyConfigurationSetResult expected, ConfigurationSet actualSet)
        {
            List<ConfigurationUnit> actualUnitList = new List<ConfigurationUnit>();

            foreach (ConfigurationUnit unit in actualSet.Units)
            {
                this.AccumulateUnits(actualUnitList, unit);
            }

            foreach (ApplyConfigurationUnitResult expectedUnitResult in expected.UnitResults)
            {
                ConfigurationUnit? actualUnit = null;
                foreach (ConfigurationUnit historyUnit in actualUnitList)
                {
                    if (historyUnit.InstanceIdentifier == expectedUnitResult.Unit.InstanceIdentifier)
                    {
                        actualUnit = historyUnit;
                    }
                }

                this.AssertUnitResultsEqual(expectedUnitResult, actualUnit);
            }
        }

        private void AccumulateUnits(List<ConfigurationUnit> unitList, ConfigurationUnit unit)
        {
            unitList.Add(unit);
            if (unit.IsGroup)
            {
                foreach (ConfigurationUnit child in unit.Units)
                {
                    this.AccumulateUnits(unitList, child);
                }
            }
        }

        private void AssertUnitResultsEqual(ApplyConfigurationUnitResult expectedResult, ConfigurationUnit? actualUnit)
        {
            Assert.NotNull(actualUnit);
            Assert.Equal(expectedResult.State, actualUnit.State);

            var expectedResultInformation = expectedResult.ResultInformation;
            if (expectedResultInformation != null)
            {
                var actualResultInformation = actualUnit.ResultInformation;
                Assert.NotNull(actualResultInformation);

                Assert.Equal(expectedResultInformation.ResultCode == null, actualResultInformation.ResultCode == null);
                if (expectedResultInformation.ResultCode != null)
                {
                    Assert.Equal(expectedResultInformation.ResultCode.HResult, actualResultInformation.ResultCode!.HResult);
                }

                Assert.Equal(expectedResultInformation.Description, actualResultInformation.Description);
                Assert.Equal(expectedResultInformation.Details, actualResultInformation.Details);
                Assert.Equal(expectedResultInformation.ResultSource, actualResultInformation.ResultSource);
            }
        }
    }
}
