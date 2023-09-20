﻿// -----------------------------------------------------------------------------
// <copyright file="OpenConfigurationSetTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.Linq;
    using System.Runtime.InteropServices;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Windows.Foundation.Collections;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for parsing configuration sets from streams.
    /// </summary>
    [Collection("UnitTestCollection")]
    [OutOfProc]
    public class OpenConfigurationSetTests : ConfigurationProcessorTestBase
    {
        /// <summary>
        /// The directives key for the module property.
        /// </summary>
        internal const string ModuleDirective = "module";

        /// <summary>
        /// Initializes a new instance of the <see cref="OpenConfigurationSetTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public OpenConfigurationSetTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
        }

        /// <summary>
        /// Passes a null stream as input.
        /// </summary>
        [Fact]
        public void NullStream()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(null);
            Assert.Null(result.Set);
            Assert.IsType<NullReferenceException>(result.ResultCode);
            Assert.Equal(string.Empty, result.Field);
        }

        /// <summary>
        /// Passes an empty stream as input.
        /// </summary>
        [Fact]
        public void EmptyStream()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(string.Empty));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_YAML, result.ResultCode.HResult);
            Assert.Equal(string.Empty, result.Field);
            Assert.Equal(0U, result.Line);
            Assert.Equal(0U, result.Column);
        }

        /// <summary>
        /// Passes a stream with a single null byte in it.
        /// </summary>
        [Fact]
        public void NullByteStream()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream("\0"));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_YAML, result.ResultCode.HResult);
            Assert.NotEqual(string.Empty, result.Field);
            Assert.Equal(0U, result.Line);
            Assert.Equal(0U, result.Column);
        }

        /// <summary>
        /// Passes YAML, but it isn't anything like a configuration file.
        /// </summary>
        [Fact]
        public void NotConfigYAML()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream("yaml: yep"));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_MISSING_FIELD, result.ResultCode.HResult);
            Assert.Equal("$schema", result.Field);
            Assert.Equal(0U, result.Line);
            Assert.Equal(0U, result.Column);
        }

        /// <summary>
        /// Passes YAML without a schema version.
        /// </summary>
        [Fact]
        public void NoConfigVersion()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  thing: 1
"));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_MISSING_FIELD, result.ResultCode.HResult);
            Assert.Equal("configurationVersion", result.Field);
            Assert.Equal(0U, result.Line);
            Assert.Equal(0U, result.Column);
        }

        /// <summary>
        /// Passes YAML that appears to be from the distant future.
        /// </summary>
        [Fact]
        public void UnknownConfigVersion()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 99999999
"));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_UNKNOWN_CONFIGURATION_FILE_VERSION, result.ResultCode.HResult);
            Assert.Equal("configurationVersion", result.Field);
            Assert.Equal("99999999", result.Value);
            Assert.Equal(0U, result.Line);
            Assert.Equal(0U, result.Column);
        }

        /// <summary>
        /// Has one of each type of intent to ensure that it is set properly.
        /// </summary>
        [Fact]
        public void EnsureIntent()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.1
  assertions:
    - resource: Assert
  parameters:
    - resource: Inform
  resources:
    - resource: Apply
"));

            Assert.NotNull(result.Set);
            Assert.Null(result.ResultCode);
            Assert.Equal(string.Empty, result.Field);

            var units = result.Set.Units;
            Assert.Equal(3, units.Count);
            bool sawAssert = false;
            bool sawInform = false;
            bool sawApply = false;

            foreach (var unit in units)
            {
                Assert.Equal(unit.Type, unit.Intent.ToString());
                switch (unit.Intent)
                {
                    case ConfigurationUnitIntent.Assert: sawAssert = true; break;
                    case ConfigurationUnitIntent.Inform: sawInform = true; break;
                    case ConfigurationUnitIntent.Apply: sawApply = true; break;
                    default: Assert.Fail("Unknown intent"); break;
                }
            }

            Assert.True(sawAssert);
            Assert.True(sawInform);
            Assert.True(sawApply);
        }

        /// <summary>
        /// Passes YAML with resources being something other than a sequence.
        /// </summary>
        [Fact]
        public void NonSequenceUnits()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.1
  resources: 1
"));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, result.ResultCode.HResult);
            Assert.Equal("resources", result.Field);
            Assert.Equal(4U, result.Line);
            Assert.NotEqual(0U, result.Column);
        }

        /// <summary>
        /// Passes YAML with a resource being something other than a map.
        /// </summary>
        [Fact]
        public void NonMapSequenceUnits()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.1
  resources:
    - string
"));
            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_FIELD_TYPE, result.ResultCode.HResult);
            Assert.Equal("resources[0]", result.Field);
            Assert.Equal(5U, result.Line);
            Assert.NotEqual(0U, result.Column);
        }

        /// <summary>
        /// Passes YAML with all values present.
        /// </summary>
        [Fact]
        public void CheckAllUnitProperties()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.1
  resources:
    - resource: Resource
      id: Identifier
      dependsOn:
        - Dependency1
        - Dependency2
      directives:
        Directive1: A
        Directive2: B
      settings:
        Setting1: '1'
        Setting2: 2
"));
            Assert.NotNull(result.Set);
            Assert.Null(result.ResultCode);
            Assert.Equal(string.Empty, result.Field);

            Assert.NotEqual(Guid.Empty, result.Set.InstanceIdentifier);

            var units = result.Set.Units;
            Assert.NotNull(units);
            Assert.Equal(1, units.Count);

            ConfigurationUnit unit = units[0];
            Assert.NotNull(unit);
            Assert.Equal("Resource", unit.Type);
            Assert.NotEqual(Guid.Empty, unit.InstanceIdentifier);
            Assert.Equal("Identifier", unit.Identifier);
            Assert.Equal(ConfigurationUnitIntent.Apply, unit.Intent);

            var dependencies = unit.Dependencies;
            Assert.NotNull(dependencies);
            Assert.Equal(2, dependencies.Count);
            Assert.Contains("Dependency1", dependencies);
            Assert.Contains("Dependency2", dependencies);

            var directives = unit.Metadata;
            Assert.NotNull(directives);
            Assert.Equal(2, directives.Count);
            Assert.Contains("Directive1", directives);
            Assert.Equal("A", directives["Directive1"]);
            Assert.Contains("Directive2", directives);
            Assert.Equal("B", directives["Directive2"]);

            var settings = unit.Settings;
            Assert.NotNull(settings);
            Assert.Equal(2, settings.Count);
            Assert.Contains("Setting1", settings);
            Assert.Equal("1", settings["Setting1"]);
            Assert.Contains("Setting2", settings);
            Assert.Equal(2L, settings["Setting2"]);

            Assert.Null(unit.Details);
            Assert.Equal(ConfigurationUnitState.Unknown, unit.State);
            Assert.Null(unit.ResultInformation);
            Assert.True(unit.IsActive);
        }

        /// <summary>
        /// Test type of scalar nodes.
        /// </summary>
        [Fact]
        public void CheckUnitScalarTypes()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.1
  resources:
    - resource: Resource
      id: Identifier
      settings:
        SettingInt: 1
        SettingString: '1' 
        SettingBool: false
        SettingStringBool: 'false'
"));
            Assert.NotNull(result.Set);
            Assert.Null(result.ResultCode);

            var units = result.Set.Units;
            Assert.NotNull(units);
            Assert.Equal(1, units.Count);

            ConfigurationUnit unit = units[0];
            Assert.NotNull(unit);

            var settings = unit.Settings;
            Assert.NotNull(settings);
            Assert.Equal(4, settings.Count);
            Assert.Contains("SettingInt", settings);
            Assert.Equal(1L, settings["SettingInt"]);
            Assert.Contains("SettingString", settings);
            Assert.Equal("1", settings["SettingString"]);
            Assert.Contains("SettingBool", settings);
            Assert.Equal(false, settings["SettingBool"]);
            Assert.Contains("SettingStringBool", settings);
            Assert.Equal("false", settings["SettingStringBool"]);
        }

        /// <summary>
        /// Test that module gets left in resource name in 0.1.
        /// </summary>
        [Fact]
        public void ModuleInResourceName_NotFor0_1()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.1
  resources:
    - resource: Module/Resource
      id: Identifier
      settings:
        SettingInt: 1
"));

            Assert.NotNull(result.Set);
            Assert.Null(result.ResultCode);

            Assert.Equal("0.1", result.Set.SchemaVersion);
            Assert.Single(result.Set.Units);

            var unit = result.Set.Units[0];
            Assert.NotNull(unit);
            Assert.Equal("Module/Resource", unit.Type);
            Assert.Empty(unit.Metadata);
        }

        /// <summary>
        /// Test that module gets parsed out of resource name in 0.2.
        /// </summary>
        [Fact]
        public void ModuleInResourceName()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.2
  resources:
    - resource: Module/Resource
      id: Identifier
      directives:
        module: Module
      settings:
        SettingInt: 1
"));

            Assert.NotNull(result.Set);
            Assert.Null(result.ResultCode);

            Assert.Equal("0.2", result.Set.SchemaVersion);
            Assert.Single(result.Set.Units);

            var unit = result.Set.Units[0];
            Assert.NotNull(unit);
            Assert.Equal("Resource", unit.Type);
            Assert.Single(unit.Metadata);
            Assert.True(unit.Metadata.ContainsKey(ModuleDirective));
            Assert.Equal("Module", unit.Metadata[ModuleDirective]);
        }

        /// <summary>
        /// Test that module is in the resource name and the directives and are different.
        /// </summary>
        [Fact]
        public void ModuleInResourceName_DirectiveDifferent()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.2
  resources:
    - resource: Module/Resource
      id: Identifier
      directives:
        module: DifferentModule
      settings:
        SettingInt: 1
"));

            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE, result.ResultCode.HResult);
            Assert.Equal(ModuleDirective, result.Field);
            Assert.Equal("DifferentModule", result.Value);
            Assert.Equal(5U, result.Line);
            Assert.NotEqual(0U, result.Column);
        }

        /// <summary>
        /// Test that providing only the module in the qualified name is an error.
        /// </summary>
        [Fact]
        public void EmptyResourceWithModule()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.2
  resources:
    - resource: Module/
      id: Identifier
      settings:
        SettingInt: 1
"));

            Assert.Null(result.Set);
            Assert.NotNull(result.ResultCode);
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE, result.ResultCode.HResult);
            Assert.Equal("resource", result.Field);
            Assert.Equal("Module/", result.Value);
            Assert.Equal(5U, result.Line);
            Assert.NotEqual(0U, result.Column);
        }

        /// <summary>
        /// Test for using version 0.3 schema.
        /// </summary>
        [Fact]
        public void BasicVersion_0_3()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
metadata:
  a: 1
  b: '2'
resources:
  - name: Name
    type: Module/Resource
    metadata:
      e: '5'
      f: 6
    properties:
      c: 3
      d: '4'
    dependsOn:
      - g
      - h
  - name: Name2
    type: Module/Resource2
    dependsOn:
      - m
    properties:
      l: '10'
    metadata:
      i: '7'
      j: 8
      q: 42
"));

            Assert.Null(result.ResultCode);
            Assert.NotNull(result.Set);
            Assert.Equal(string.Empty, result.Field);
            Assert.Equal(string.Empty, result.Value);
            Assert.Equal(0U, result.Line);
            Assert.Equal(0U, result.Column);

            ConfigurationSet set = result.Set;

            Assert.Equal("0.3", set.SchemaVersion);
            Assert.NotNull(set.SchemaUri);
            Assert.Equal("https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json", set.SchemaUri.ToString());

            this.VerifyValueSet(set.Metadata, new ("a", 1), new ("b", "2"));

            Assert.Empty(set.Parameters);
            Assert.Empty(set.Variables);

            Assert.Equal(2, set.Units.Count);

            this.VerifyUnitProperties(set.Units[0], "Name", "Module/Resource");
            this.VerifyValueSet(set.Units[0].Metadata, new ("e", "5"), new ("f", 6));
            this.VerifyValueSet(set.Units[0].Settings, new ("c", 3), new ("d", "4"));
            this.VerifyStringArray(set.Units[0].Dependencies, "g", "h");

            this.VerifyUnitProperties(set.Units[1], "Name2", "Module/Resource2");
            this.VerifyValueSet(set.Units[1].Metadata, new ("i", "7"), new ("j", 8), new ("q", 42));
            this.VerifyValueSet(set.Units[1].Settings, new KeyValuePair<string, object>("l", "10"));
            this.VerifyStringArray(set.Units[1].Dependencies, "m");
        }

        /// <summary>
        /// Test for using version 0.3 schema parameters.
        /// </summary>
        [Fact]
        public void Parameters_0_3()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(@"
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
metadata:
  a: 1
  b: '2'
parameters:

resources:
  - name: Name
    type: Module/Resource
    metadata:
      e: '5'
      f: 6
    properties:
      c: 3
      d: '4'
    dependsOn:
      - g
      - h
  - name: Name2
    type: Module/Resource2
    dependsOn:
      - m
    properties:
      l: '10'
    metadata:
      i: '7'
      j: 8
      q: 42
"));
        }

            // PARAMETERS
            // VARIABLES

            private void VerifyUnitProperties(ConfigurationUnit unit, string identifier, string type)
        {
            Assert.NotNull(unit);
            Assert.Equal(identifier, unit.Identifier);
            Assert.Equal(type, unit.Type);
        }

        private void VerifyValueSet(ValueSet values, params KeyValuePair<string, object>[] expected)
        {
            Assert.NotNull(values);
            Assert.Equal(expected.Length, values.Count);

            foreach (var expectation in expected)
            {
                Assert.True(values.ContainsKey(expectation.Key));
                object value = values[expectation.Key];

                switch (expectation.Value)
                {
                    case int i:
                        Assert.Equal(i, (int)(long)value);
                        break;
                    case string s:
                        Assert.Equal(s, (string)value);
                        break;
                    default:
                        Assert.Fail($"Add expected type `{expectation.Value.GetType().Name}` to switch statement.");
                        break;
                }
            }
        }

        private void VerifyStringArray(IList<string> strings, params string[] expected)
        {
            Assert.NotNull(strings);
            Assert.Equal(expected.Length, strings.Count);
        }
    }
}
