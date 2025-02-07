// -----------------------------------------------------------------------------
// <copyright file="OpenConfigurationSetTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.DirectoryServices;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.VisualBasic;
    using Windows.Foundation.Collections;
    using Windows.Storage.Streams;
    using WinRT;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Unit tests for parsing configuration sets from streams.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
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
        /// Verifies that the configuration set (0.2) can be serialized and reopened correctly.
        /// </summary>
        [Fact]
        public void TestSet_Serialize_0_2()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult openResult = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.2
  assertions:
    - resource: FakeModule/FakeResource
      id: TestId
      directives:
        description: FakeDescription
        allowPrerelease: true
        securityContext: elevated
      settings:
        TestString: Hello
        TestBool: false
        TestInt: 1234  
  resources:
    - resource: FakeModule2/FakeResource2
      id: TestId2
      dependsOn:
        - TestId
        - dependency2
        - dependency3
      directives:
        description: FakeDescription2
        securityContext: elevated
      settings:
        TestString: Bye
        TestBool: true
        TestInt: 4321
        Mapping:
          Key: TestValue
"));

            // Serialize set.
            ConfigurationSet configurationSet = openResult.Set;
            InMemoryRandomAccessStream stream = new InMemoryRandomAccessStream();
            configurationSet.Serialize(stream);

            string yamlOutput = this.ReadStream(stream);

            // Reopen configuration set from serialized string and verify values.
            OpenConfigurationSetResult serializedSetResult = processor.OpenConfigurationSet(this.CreateStream(yamlOutput));
            Assert.Null(serializedSetResult.ResultCode);
            ConfigurationSet set = serializedSetResult.Set;
            Assert.NotNull(set);

            Assert.Equal("0.2", set.SchemaVersion);
            Assert.Equal(2, set.Units.Count);

            Assert.Equal("FakeResource", set.Units[0].Type);
            Assert.Equal(ConfigurationUnitIntent.Assert, set.Units[0].Intent);
            Assert.Equal("TestId", set.Units[0].Identifier);
            this.VerifyValueSet(set.Units[0].Metadata, new ("description", "FakeDescription"), new ("allowPrerelease", true), new ("module", "FakeModule"));
            this.VerifyValueSet(set.Units[0].Settings, new ("TestString", "Hello"), new ("TestBool", false), new ("TestInt", 1234));

            Assert.Equal("FakeResource2", set.Units[1].Type);
            Assert.Equal(ConfigurationUnitIntent.Apply, set.Units[1].Intent);
            Assert.Equal("TestId2", set.Units[1].Identifier);
            this.VerifyStringArray(set.Units[1].Dependencies, "TestId", "dependency2", "dependency3");
            this.VerifyValueSet(set.Units[1].Metadata, new ("description", "FakeDescription2"), new ("module", "FakeModule2"));

            ValueSet mapping = new ValueSet();
            mapping.Add("Key", "TestValue");
            this.VerifyValueSet(set.Units[1].Settings, new ("TestString", "Bye"), new ("TestBool", true), new ("TestInt", 4321), new ("Mapping", mapping));
        }

        /// <summary>
        /// Verifies that the configuration set (0.3) can be serialized and reopened correctly.
        /// </summary>
        [Fact]
        public void TestSet_Serialize_0_3()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult openResult = processor.OpenConfigurationSet(this.CreateStream(@"
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
metadata:
  description: FakeSetDescription
variables:
  var1: Test1
  var2: 42
parameters:
  param1:
    type: securestring
  param2:
    type: int
    defaultValue: 89
resources:
  - type: FakeModule/FakeResource
    name: TestId
    metadata:
      description: FakeDescription
      allowPrerelease: true
      myVal: mine
    properties:
      TestString: Hello
      TestBool: false
      TestInt: 1234  
  - type: FakeModule2/FakeResource2
    name: TestId2
    dependsOn:
      - TestId
      - dependency2
      - dependency3
    metadata:
      description: FakeDescription2
      myVal: yours
    properties:
      TestString: Bye
      TestBool: true
      TestInt: 4321
      Mapping:
        Key: TestValue
  - type: FakeModule/FakeResource3
    name: TestId3
    metadata:
      isGroup: true
    properties:
      other: value
      resources:
        - type: Grouped/Resource
          name: Child
          properties:
            b: c
"));

            // Serialize set.
            ConfigurationSet configurationSet = openResult.Set;
            InMemoryRandomAccessStream stream = new InMemoryRandomAccessStream();
            configurationSet.Serialize(stream);

            string yamlOutput = this.ReadStream(stream);

            // Reopen configuration set from serialized string and verify values.
            OpenConfigurationSetResult serializedSetResult = processor.OpenConfigurationSet(this.CreateStream(yamlOutput));
            Assert.Null(serializedSetResult.ResultCode);
            ConfigurationSet set = serializedSetResult.Set;
            Assert.NotNull(set);

            Assert.Equal("0.3", set.SchemaVersion);
            Assert.Equal(3, set.Units.Count);

            this.VerifyValueSet(set.Metadata, new KeyValuePair<string, object>("description", "FakeSetDescription"));
            this.VerifyValueSet(set.Variables, new ("var1", "Test1"), new ("var2", 42));

            Assert.Equal(2, set.Parameters.Count);
            this.VerifyParameter(set.Parameters[0], "param1", Windows.Foundation.PropertyType.String, true);
            this.VerifyParameter(set.Parameters[1], "param2", Windows.Foundation.PropertyType.Int64, false, 89);

            Assert.Equal("FakeModule/FakeResource", set.Units[0].Type);
            Assert.Equal("TestId", set.Units[0].Identifier);
            this.VerifyValueSet(set.Units[0].Metadata, new ("description", "FakeDescription"), new ("allowPrerelease", true), new ("myVal", "mine"));
            this.VerifyValueSet(set.Units[0].Settings, new ("TestString", "Hello"), new ("TestBool", false), new ("TestInt", 1234));

            Assert.Equal("FakeModule2/FakeResource2", set.Units[1].Type);
            Assert.Equal("TestId2", set.Units[1].Identifier);
            this.VerifyStringArray(set.Units[1].Dependencies, "TestId", "dependency2", "dependency3");
            this.VerifyValueSet(set.Units[1].Metadata, new ("description", "FakeDescription2"), new ("myVal", "yours"));

            ValueSet mapping = new ValueSet();
            mapping.Add("Key", "TestValue");
            this.VerifyValueSet(set.Units[1].Settings, new ("TestString", "Bye"), new ("TestBool", true), new ("TestInt", 4321), new ("Mapping", mapping));

            Assert.Equal("FakeModule/FakeResource3", set.Units[2].Type);
            Assert.Equal("TestId3", set.Units[2].Identifier);
            Assert.True(set.Units[2].IsGroup);

            ValueSet childResource = new ValueSet();
            childResource.Add("type", "Grouped/Resource");
            childResource.Add("name", "Child");
            ValueSet childResourceProperties = new ValueSet();
            childResourceProperties.Add("b", "c");
            childResource.Add("properties", childResourceProperties);

            ValueSet resourcesArray = new ValueSet();
            resourcesArray.Add("treatAsArray", true);
            resourcesArray.Add("0", childResource);

            this.VerifyValueSet(set.Units[2].Settings, new ("other", "value"), new ("resources", resourcesArray));

            var groupChildren = set.Units[2].Units;
            Assert.Single(groupChildren);

            Assert.Equal("Grouped/Resource", groupChildren[0].Type);
            Assert.Equal("Child", groupChildren[0].Identifier);
            this.VerifyValueSet(groupChildren[0].Settings, new KeyValuePair<string, object>("b", "c"));
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
            this.VerifyValueSet(set.Variables, new ("v1", "var1"), new ("v2", 42));

            Assert.Empty(set.Parameters);

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
        /// Test for the successful parsing of default value of a parameter.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <param name="expectedValue">The expected value.</param>
        /// <param name="expectedType">The expected type.</param>
        /// <param name="secure">The secure state.</param>
        [Theory]
        [InlineData("string", "abc", "abc", Windows.Foundation.PropertyType.String)]
        [InlineData("string", "'42'", "42", Windows.Foundation.PropertyType.String)]
        [InlineData("securestring", "abcdef", "abcdef", Windows.Foundation.PropertyType.String, true)]
        [InlineData("int", "42", 42, Windows.Foundation.PropertyType.Int64)]
        [InlineData("bool", "true", true, Windows.Foundation.PropertyType.Boolean)]
        [InlineData("object", "string", "string", Windows.Foundation.PropertyType.Inspectable)]
        [InlineData("object", "42", 42, Windows.Foundation.PropertyType.Inspectable)]
        [InlineData("secureobject", "string", "string", Windows.Foundation.PropertyType.Inspectable, true)]
        [InlineData("secureobject", "42", 42, Windows.Foundation.PropertyType.Inspectable, true)]
        public void Parameters_DefaultValue_Success(string type, string defaultValue, object expectedValue, object expectedType, bool secure = false)
        {
            this.TestParameterDefaultValue(type, defaultValue, expectedValue, Assert.IsType<Windows.Foundation.PropertyType>(expectedType), secure);
        }

        /// <summary>
        /// Test for the failed parsing of default value of a parameter.
        /// </summary>
        /// <param name="type">The type.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <param name="expectedValue">The expected value.</param>
        [Theory]
        [InlineData("string", "42")]
        [InlineData("int", "abc")]
        [InlineData("int", "'42'", "42")]
        [InlineData("bool", "'true'", "true")]
        public void Parameters_DefaultValue_Failure(string type, string defaultValue, object? expectedValue = null)
        {
            this.TestParameterDefaultValue(type, defaultValue, expectedValue);
        }

        /// <summary>
        /// Test to ensure that schema version and uri is working as expected.
        /// </summary>
        /// <param name="version">The version.</param>
        /// <param name="uri">The uri.</param>
        [Theory]
        [InlineData("0.1", null)]
        [InlineData("0.2", null)]
        [InlineData("0.3", "https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json")]
        public void Schema_Version_Uri(string version, string? uri)
        {
            ConfigurationSet set = this.ConfigurationSet();

            set.SchemaVersion = version;
            if (uri != null)
            {
                Assert.Equal(uri, set.SchemaUri.AbsoluteUri);
            }
            else
            {
                Assert.Null(set.SchemaUri);
            }

            if (!string.IsNullOrEmpty(uri))
            {
                set.SchemaUri = new Uri(uri);
                Assert.Equal(version, set.SchemaVersion);
            }
        }

        /// <summary>
        /// Verifies that the configuration set (0.2) with environments parses and serializes.
        /// </summary>
        [Fact]
        public void Environment_0_2()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult openResult = processor.OpenConfigurationSet(this.CreateStream(@"
properties:
  configurationVersion: 0.2
  resources:
    - resource: FakeModule/FakeResource
      id: elevated
      directives:
        description: FakeDescription
        allowPrerelease: true
        securityContext: elevated
      settings:
        TestString: Hello
    - resource: FakeModule2/FakeResource2
      id: restricted
      directives:
        description: FakeDescription2
        securityContext: restricted
      settings:
        TestString: Bye
    - resource: FakeModule2/FakeResource2
      id: current
      directives:
        securityContext: current
      settings:
        TestString: Bye
    - resource: FakeModule2/FakeResource2
      id: default
      settings:
        TestString: Bye
"));

            Dictionary<string, SecurityContext> expectedEnvironments = new Dictionary<string, SecurityContext>();
            expectedEnvironments.Add("elevated", SecurityContext.Elevated);
            expectedEnvironments.Add("restricted", SecurityContext.Restricted);
            expectedEnvironments.Add("current", SecurityContext.Current);
            expectedEnvironments.Add("default", SecurityContext.Current);

            this.ValidateSecurityContexts(openResult, expectedEnvironments);

            // Shuffle security contexts, serialize, parse and validate again
            expectedEnvironments["elevated"] = SecurityContext.Restricted;
            expectedEnvironments["restricted"] = SecurityContext.Current;
            expectedEnvironments["current"] = SecurityContext.Restricted;
            expectedEnvironments["default"] = SecurityContext.Elevated;

            var units = openResult.Set.Units;
            foreach (var unit in units)
            {
                SecurityContext newContext = SecurityContext.Current;
                Assert.True(expectedEnvironments.TryGetValue(unit.Identifier, out newContext));
                unit.Environment.Context = newContext;
            }

            // Serialize set.
            InMemoryRandomAccessStream stream = new InMemoryRandomAccessStream();
            openResult.Set.Serialize(stream);

            string yamlOutput = this.ReadStream(stream);

            // Reopen configuration set from serialized string and verify values.
            OpenConfigurationSetResult serializedSetResult = processor.OpenConfigurationSet(this.CreateStream(yamlOutput));

            this.ValidateSecurityContexts(serializedSetResult, expectedEnvironments);
        }

        /// <summary>
        /// Verifies that the configuration set (0.3) inherits set environment.
        /// </summary>
        [Fact]
        public void SetMetadataEnvironmentInheritance_0_3()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult openResult = processor.OpenConfigurationSet(this.CreateStream(@"
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
metadata:
  winget:
    securityContext: elevated
    processor:
      identifier: pwsh
      properties:
        a: b
resources:
  - name: first
    type: Module/Resource
    properties:
      c: 3
  - name: second
    type: Module/Resource2
    properties:
      l: '10'
"));

            Dictionary<string, string> environmentProperties = new Dictionary<string, string>();
            environmentProperties.Add("a", "b");
            ConfigurationEnvironmentData setEnvironment = new ConfigurationEnvironmentData() { Context = SecurityContext.Elevated, ProcessorIdentifier = "pwsh", ProcessorProperties = environmentProperties };

            Dictionary<string, ConfigurationEnvironmentData> expectedEnvironments = new Dictionary<string, ConfigurationEnvironmentData>();
            expectedEnvironments.Add("first", new ConfigurationEnvironmentData());
            expectedEnvironments.Add("second", new ConfigurationEnvironmentData());

            this.ValidateEnvironments(openResult, setEnvironment, expectedEnvironments);

            // Serialize set.
            InMemoryRandomAccessStream stream = new InMemoryRandomAccessStream();
            openResult.Set.Serialize(stream);

            string yamlOutput = this.ReadStream(stream);

            // Reopen configuration set from serialized string and verify values.
            OpenConfigurationSetResult serializedSetResult = processor.OpenConfigurationSet(this.CreateStream(yamlOutput));

            this.ValidateEnvironments(serializedSetResult, setEnvironment, expectedEnvironments);
        }

        /// <summary>
        /// Verifies that the configuration set (0.3) inherits set environment.
        /// </summary>
        [Fact]
        public void SetMetadataEnvironmentInheritance_ProcessorOverridden_0_3()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult openResult = processor.OpenConfigurationSet(this.CreateStream(@"
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
metadata:
  winget:
    securityContext: elevated
    processor:
      identifier: pwsh
      properties:
        a: b
resources:
  - name: first
    type: Module/Resource
    properties:
      c: 3
  - name: second
    type: Module/Resource2
    properties:
      l: '10'
    metadata:
      winget:
        processor: not-pwsh
"));

            Dictionary<string, string> environmentProperties = new Dictionary<string, string>();
            environmentProperties.Add("a", "b");
            ConfigurationEnvironmentData setEnvironment = new ConfigurationEnvironmentData() { Context = SecurityContext.Elevated, ProcessorIdentifier = "pwsh", ProcessorProperties = environmentProperties };

            Dictionary<string, ConfigurationEnvironmentData> expectedEnvironments = new Dictionary<string, ConfigurationEnvironmentData>();
            expectedEnvironments.Add("first", new ConfigurationEnvironmentData());
            expectedEnvironments.Add("second", new ConfigurationEnvironmentData() { ProcessorIdentifier = "not-pwsh" });

            this.ValidateEnvironments(openResult, setEnvironment, expectedEnvironments);

            // Serialize set.
            InMemoryRandomAccessStream stream = new InMemoryRandomAccessStream();
            openResult.Set.Serialize(stream);

            string yamlOutput = this.ReadStream(stream);

            // Reopen configuration set from serialized string and verify values.
            OpenConfigurationSetResult serializedSetResult = processor.OpenConfigurationSet(this.CreateStream(yamlOutput));

            this.ValidateEnvironments(serializedSetResult, setEnvironment, expectedEnvironments);
        }

        /// <summary>
        /// Verifies that the configuration set (0.3) inherits set environment.
        /// </summary>
        [Fact]
        public void SetMetadataEnvironmentInheritance_ContextOverridden_0_3()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult openResult = processor.OpenConfigurationSet(this.CreateStream(@"
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
metadata:
  winget:
    securityContext: elevated
    processor:
      identifier: pwsh
      properties:
        a: b
resources:
  - name: first
    type: Module/Resource
    properties:
      c: 3
  - name: second
    type: Module/Resource2
    properties:
      l: '10'
    metadata:
      winget:
        securityContext: restricted
"));

            Dictionary<string, string> environmentProperties = new Dictionary<string, string>();
            environmentProperties.Add("a", "b");
            ConfigurationEnvironmentData setEnvironment = new ConfigurationEnvironmentData() { Context = SecurityContext.Elevated, ProcessorIdentifier = "pwsh", ProcessorProperties = environmentProperties };

            Dictionary<string, ConfigurationEnvironmentData> expectedEnvironments = new Dictionary<string, ConfigurationEnvironmentData>();
            expectedEnvironments.Add("first", new ConfigurationEnvironmentData());
            expectedEnvironments.Add("second", new ConfigurationEnvironmentData() { Context = SecurityContext.Restricted });

            this.ValidateEnvironments(openResult, setEnvironment, expectedEnvironments);

            // Serialize set.
            InMemoryRandomAccessStream stream = new InMemoryRandomAccessStream();
            openResult.Set.Serialize(stream);

            string yamlOutput = this.ReadStream(stream);

            // Reopen configuration set from serialized string and verify values.
            OpenConfigurationSetResult serializedSetResult = processor.OpenConfigurationSet(this.CreateStream(yamlOutput));

            this.ValidateEnvironments(serializedSetResult, setEnvironment, expectedEnvironments);
        }

        /// <summary>
        /// Verifies that the configuration set (0.3) serializes common environment to the set metadata.
        /// </summary>
        [Fact]
        public void CommonEnvironmentElevatedToSetMetadata_0_3()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult openResult = processor.OpenConfigurationSet(this.CreateStream(@"
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
resources:
  - name: first
    type: Module/Resource
    metadata:
      winget:
        securityContext: elevated
        processor: pwsh
    properties:
      c: 3
  - name: second
    type: Module/Resource2
    properties:
      l: '10'
    metadata:
      winget:
        securityContext: elevated
        processor:
          identifier: pwsh
"));

            ConfigurationEnvironmentData setEnvironment = new ConfigurationEnvironmentData();

            Dictionary<string, ConfigurationEnvironmentData> expectedEnvironments = new Dictionary<string, ConfigurationEnvironmentData>();
            expectedEnvironments.Add("first", new ConfigurationEnvironmentData() { Context = SecurityContext.Elevated, ProcessorIdentifier = "pwsh" });
            expectedEnvironments.Add("second", new ConfigurationEnvironmentData() { Context = SecurityContext.Elevated, ProcessorIdentifier = "pwsh" });

            this.ValidateEnvironments(openResult, setEnvironment, expectedEnvironments);

            // Serialize set.
            InMemoryRandomAccessStream stream = new InMemoryRandomAccessStream();
            openResult.Set.Serialize(stream);

            string yamlOutput = this.ReadStream(stream);

            // Reopen configuration set from serialized string and verify values.
            OpenConfigurationSetResult serializedSetResult = processor.OpenConfigurationSet(this.CreateStream(yamlOutput));

            this.ValidateEnvironments(serializedSetResult, setEnvironment, expectedEnvironments);
        }

        /// <summary>
        /// Verifies that the configuration set (0.3) serializes common environment to the set metadata.
        /// </summary>
        [Fact]
        public void CommonProcessorElevatedToSetMetadata_0_3()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult openResult = processor.OpenConfigurationSet(this.CreateStream(@"
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
resources:
  - name: first
    type: Module/Resource
    metadata:
      winget:
        securityContext: elevated
        processor: pwsh
    properties:
      c: 3
  - name: second
    type: Module/Resource2
    properties:
      l: '10'
    metadata:
      winget:
        securityContext: restricted
        processor:
          identifier: pwsh
"));

            ConfigurationEnvironmentData setEnvironment = new ConfigurationEnvironmentData();

            Dictionary<string, ConfigurationEnvironmentData> expectedEnvironments = new Dictionary<string, ConfigurationEnvironmentData>();
            expectedEnvironments.Add("first", new ConfigurationEnvironmentData() { Context = SecurityContext.Elevated, ProcessorIdentifier = "pwsh" });
            expectedEnvironments.Add("second", new ConfigurationEnvironmentData() { Context = SecurityContext.Restricted, ProcessorIdentifier = "pwsh" });

            this.ValidateEnvironments(openResult, setEnvironment, expectedEnvironments);

            // Serialize set.
            InMemoryRandomAccessStream stream = new InMemoryRandomAccessStream();
            openResult.Set.Serialize(stream);

            string yamlOutput = this.ReadStream(stream);

            // Reopen configuration set from serialized string and verify values.
            OpenConfigurationSetResult serializedSetResult = processor.OpenConfigurationSet(this.CreateStream(yamlOutput));

            this.ValidateEnvironments(serializedSetResult, setEnvironment, expectedEnvironments);
        }

        /// <summary>
        /// Verifies that the configuration set (0.3) environments work with group units.
        /// </summary>
        [Fact]
        public void EnvironmentsWithGroups_0_3()
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult openResult = processor.OpenConfigurationSet(this.CreateStream(@"
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
metadata:
  winget:
    securityContext: elevated
    processor:
      identifier: pwsh
      properties:
        a: b
resources:
  - name: non-group
    type: Module/Resource2
    properties:
      l: '10'
  - name: group
    type: Module/Resource
    metadata:
      isGroup: true
      winget:
        securityContext: restricted
    properties:
      resources:
        - name: inherit
          type: Module/Resource
          properties:
            a: b
        - name: override
          type: Module/Resource
          properties:
            c: d
          metadata:
            winget:
              processor: not-pwsh
"));

            Dictionary<string, string> environmentProperties = new Dictionary<string, string>();
            environmentProperties.Add("a", "b");
            ConfigurationEnvironmentData setEnvironment = new ConfigurationEnvironmentData() { Context = SecurityContext.Elevated, ProcessorIdentifier = "pwsh", ProcessorProperties = environmentProperties };

            Dictionary<string, ConfigurationEnvironmentData> expectedEnvironments = new Dictionary<string, ConfigurationEnvironmentData>();
            expectedEnvironments.Add("non-group", new ConfigurationEnvironmentData());
            expectedEnvironments.Add("group", new ConfigurationEnvironmentData() { Context = SecurityContext.Restricted });

            Dictionary<string, ConfigurationEnvironmentData> groupExpectedEnvironments = new Dictionary<string, ConfigurationEnvironmentData>();
            groupExpectedEnvironments.Add("inherit", new ConfigurationEnvironmentData());
            groupExpectedEnvironments.Add("override", new ConfigurationEnvironmentData() { ProcessorIdentifier = "not-pwsh" });

            this.ValidateEnvironments(openResult, setEnvironment, expectedEnvironments, "group", groupExpectedEnvironments);

            // Serialize set.
            InMemoryRandomAccessStream stream = new InMemoryRandomAccessStream();
            openResult.Set.Serialize(stream);

            string yamlOutput = this.ReadStream(stream);

            // Reopen configuration set from serialized string and verify values.
            OpenConfigurationSetResult serializedSetResult = processor.OpenConfigurationSet(this.CreateStream(yamlOutput));

            this.ValidateEnvironments(openResult, setEnvironment, expectedEnvironments, "group", groupExpectedEnvironments);
        }

        private void ValidateEnvironments(OpenConfigurationSetResult openResult, ConfigurationEnvironmentData setEnvironment, Dictionary<string, ConfigurationEnvironmentData> expectedEnvironments, string? groupToCheck = null, Dictionary<string, ConfigurationEnvironmentData>? groupExpectedEnvironments = null)
        {
            Assert.Null(openResult.ResultCode);
            Assert.NotNull(openResult.Set);
            ConfigurationSet configurationSet = openResult.Set;

            this.ValidateEnvironment(setEnvironment, configurationSet.Environment);

            var units = configurationSet.Units;
            this.ValidateEnvironments(units, expectedEnvironments, groupToCheck, groupExpectedEnvironments);
        }

        private void ValidateEnvironments(IList<ConfigurationUnit> units, Dictionary<string, ConfigurationEnvironmentData> expectedEnvironments, string? groupToCheck = null, Dictionary<string, ConfigurationEnvironmentData>? groupExpectedEnvironments = null)
        {
            Assert.Equal(expectedEnvironments.Count, units.Count);
            foreach (var unit in units)
            {
                ConfigurationEnvironmentData? expectedEnvironment = null;
                Assert.True(expectedEnvironments.TryGetValue(unit.Identifier, out expectedEnvironment));
                this.ValidateEnvironment(expectedEnvironment, unit.Environment);

                if (unit.Identifier == groupToCheck)
                {
                    Assert.True(unit.IsGroup);
                    Assert.NotNull(groupExpectedEnvironments);
                    var groupUnits = unit.Units;
                    this.ValidateEnvironments(groupUnits, groupExpectedEnvironments);
                }
            }
        }

        private void ValidateEnvironment(ConfigurationEnvironmentData? expectedEnvironment, ConfigurationEnvironment? actualEnvironment)
        {
            Assert.NotNull(expectedEnvironment);
            Assert.NotNull(actualEnvironment);
            Assert.Equal(expectedEnvironment.Context, actualEnvironment.Context);
            Assert.Equal(expectedEnvironment.ProcessorIdentifier, actualEnvironment.ProcessorIdentifier);
            Assert.True(expectedEnvironment.PropertiesEqual(actualEnvironment.ProcessorProperties));
        }

        private void ValidateSecurityContexts(OpenConfigurationSetResult openResult, Dictionary<string, SecurityContext> expectedContexts)
        {
            Assert.Null(openResult.ResultCode);
            Assert.NotNull(openResult.Set);
            ConfigurationSet configurationSet = openResult.Set;

            var units = configurationSet.Units;
            Assert.Equal(expectedContexts.Count, units.Count);
            foreach (var unit in units)
            {
                SecurityContext expectedContext = SecurityContext.Current;
                Assert.True(expectedContexts.TryGetValue(unit.Identifier, out expectedContext));
                Assert.Equal(expectedContext, unit.Environment.Context);
            }
        }

        private void TestParameterDefaultValue(string type, string defaultValue, object? expectedValue = null, Windows.Foundation.PropertyType? expectedType = null, bool secure = false)
        {
            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics();

            OpenConfigurationSetResult result = processor.OpenConfigurationSet(this.CreateStream(string.Format(
                @"
$schema: https://raw.githubusercontent.com/PowerShell/DSC/main/schemas/2023/08/config/document.json
parameters:
  {0}:
    type: {0}
    defaultValue: {1}
",
                type,
                defaultValue)));

            if (expectedType != null)
            {
                Assert.Null(result.ResultCode);
                Assert.NotNull(result.Set);
                Assert.Equal(string.Empty, result.Field);
                Assert.Equal(string.Empty, result.Value);
                Assert.Equal(0U, result.Line);
                Assert.Equal(0U, result.Column);

                var parameters = result.Set.Parameters;
                Assert.NotNull(parameters);
                Assert.Single(parameters);

                Assert.Equal(type, parameters[0].Name);
                Assert.Equal(expectedType, parameters[0].Type);
                Assert.Equal(secure, parameters[0].IsSecure);

                Assert.NotNull(expectedValue);
                this.VerifyObject(type, expectedValue, parameters[0].DefaultValue);
            }
            else
            {
                Assert.NotNull(result.ResultCode);
                Assert.Equal(Errors.WINGET_CONFIG_ERROR_INVALID_FIELD_VALUE, result.ResultCode.HResult);
                Assert.Null(result.Set);
                Assert.Equal("defaultValue", result.Field);
                Assert.Equal(expectedValue?.ToString() ?? defaultValue, result.Value);
                Assert.NotEqual(0U, result.Line);
                Assert.NotEqual(0U, result.Column);
            }
        }

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
                Assert.True(values.ContainsKey(expectation.Key), $"Not Found {expectation.Key}");
                object value = values[expectation.Key];

                this.VerifyObject(expectation.Key, expectation.Value, value);
            }
        }

        private void VerifyStringArray(IList<string> strings, params string[] expected)
        {
            Assert.NotNull(strings);
            Assert.Equal(expected.Length, strings.Count);

            foreach (var expectation in expected)
            {
                bool found = false;
                foreach (var value in strings)
                {
                    if (!found)
                    {
                        found = expectation == value;
                    }
                }

                Assert.True(found, $"Did not find {expectation} in string array");
            }
        }

        private void VerifyParameter(ConfigurationParameter parameter, string name, Windows.Foundation.PropertyType type, bool secure, object? defaultValue = null)
        {
            Assert.Equal(name, parameter.Name);
            Assert.Equal(type, parameter.Type);
            Assert.Equal(secure, parameter.IsSecure);
            this.VerifyObject(name, defaultValue, parameter.DefaultValue);
        }

        private void VerifyObject(string name, object? expectedValue, object? actualValue)
        {
            if (expectedValue != null)
            {
                Assert.NotNull(actualValue);

                switch (expectedValue)
                {
                    case int i:
                        Assert.True(i == (int)(long)actualValue, $"{name}: expected[{i}], actual[{(int)(long)actualValue}]");
                        break;
                    case string s:
                        Assert.True(s == (string)actualValue, $"{name}: expected[{s}], actual[{(string)actualValue}]");
                        break;
                    case bool b:
                        Assert.True(b == (bool)actualValue, $"{name}: expected[{b}], actual[{(bool)actualValue}]");
                        break;
                    case ValueSet v:
                        var actualValueSet = actualValue.As<ValueSet>();
                        Assert.True(v.ContentEquals(actualValueSet), $"ValueSets not equal: {name}\n---expected---:\n{v.ToYaml()}\n---actual---:\n{actualValueSet.ToYaml()}");
                        break;
                    default:
                        Assert.Fail($"Add expected type `{expectedValue.GetType().Name}` to switch statement for {name}.");
                        break;
                }
            }
        }
    }
}
