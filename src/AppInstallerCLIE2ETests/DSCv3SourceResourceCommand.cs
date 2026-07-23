// -----------------------------------------------------------------------------
// <copyright file="DSCv3SourceResourceCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System.Collections.Generic;
    using System.Text.Json.Serialization;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// `Configure` command tests.
    /// </summary>
    [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1010:Opening square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
    [System.Diagnostics.CodeAnalysis.SuppressMessage("StyleCop.CSharp.SpacingRules", "SA1011:Closing square brackets should be spaced correctly", Justification = "https://github.com/DotNetAnalyzers/StyleCopAnalyzers/issues/3687 pending SC 1.2 release")]
    public class DSCv3SourceResourceCommand : DSCv3ResourceTestBase
    {
        private const string DefaultSourceName = "SourceResourceTestSource";
        private const string DefaultSourceType = "Microsoft.Test.Configurable";
        private const string DefaultTrustLevel = "none";
        private const string TrustedTrustLevel = "trusted";
        private const bool DefaultExplicitState = false;
        private const int DefaultPriority = 0;
        private const string SourceResource = "source";
        private const string ArgumentPropertyName = "argument";
        private const string TypePropertyName = "type";
        private const string TrustLevelPropertyName = "trustLevel";
        private const string ExplicitPropertyName = "explicit";
        private const string PriorityPropertyName = "priority";

        private static string DefaultSourceArgForCmdLine
        {
            get { return CreateSourceArgument(true); }
        }

        private static string NonDefaultSourceArgForCmdLine
        {
            get { return CreateSourceArgument(true, 1, 1); }
        }

        private static string DefaultSourceArgDirect
        {
            get { return CreateSourceArgument(false); }
        }

        private static string NonDefaultSourceArgDirect
        {
            get { return CreateSourceArgument(false, 1, 1); }
        }

        /// <summary>
        /// Setup done once before all the tests here.
        /// </summary>
        [OneTimeSetUp]
        public void OneTimeSetup()
        {
            TestCommon.SetupTestSource();
            EnsureTestResourcePresence();
        }

        /// <summary>
        /// Teardown done once after all the tests here.
        /// </summary>
        [OneTimeTearDown]
        public void OneTimeTeardown()
        {
            RemoveTestSource();
            TestCommon.TearDownTestSource();
        }

        /// <summary>
        /// Set up.
        /// </summary>
        [SetUp]
        public void Setup()
        {
            RemoveTestSource();
            WinGetSettingsHelper.ConfigureFeature("sourcePriority", true);
        }

        /// <summary>
        /// Calls `get` on the `source` resource with the value not present.
        /// </summary>
        [Test]
        public void Source_Get_NotPresent()
        {
            SourceResourceData resourceData = new SourceResourceData() { Name = DefaultSourceName };

            var result = RunDSCv3Command(SourceResource, GetFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            SourceResourceData output = GetSingleOutputLineAs<SourceResourceData>(result.StdOut);
            Assert.That(output, Is.Not.Null);
            Assert.That(output.Exist, Is.False);
            Assert.That(output.Name, Is.EqualTo(resourceData.Name));
        }

        /// <summary>
        /// Calls `get` on the `source` resource with the value present.
        /// </summary>
        [Test]
        public void Source_Get_Present()
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {DefaultSourceArgForCmdLine} --type {DefaultSourceType} --explicit");
            Assert.That(setup.ExitCode, Is.Zero);

            SourceResourceData resourceData = new SourceResourceData() { Name = DefaultSourceName };

            var result = RunDSCv3Command(SourceResource, GetFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            SourceResourceData output = GetSingleOutputLineAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, DefaultSourceArgDirect, DefaultTrustLevel, true);
        }

        /// <summary>
        /// Calls `test` on the `source` resource with the value not present.
        /// </summary>
        [Test]
        public void Source_Test_NotPresent()
        {
            SourceResourceData resourceData = new SourceResourceData() { Name = DefaultSourceName };

            var result = RunDSCv3Command(SourceResource, TestFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            Assert.That(output, Is.Not.Null);
            Assert.That(output.Exist, Is.False);
            Assert.That(output.Name, Is.EqualTo(resourceData.Name));
            Assert.That(output.InDesiredState, Is.False);

            AssertDiffState(diff, [ ExistPropertyName ]);
        }

        /// <summary>
        /// Calls `test` on the `source` resource with the value present.
        /// </summary>
        [Test]
        public void Source_Test_SimplePresent()
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {DefaultSourceArgForCmdLine} --type {DefaultSourceType}");
            Assert.That(setup.ExitCode, Is.Zero);

            SourceResourceData resourceData = new SourceResourceData() { Name = DefaultSourceName };

            var result = RunDSCv3Command(SourceResource, TestFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, DefaultSourceArgDirect, DefaultTrustLevel, DefaultExplicitState);
            Assert.That(output.InDesiredState, Is.True);

            AssertDiffState(diff, []);
        }

        /// <summary>
        /// Calls `test` on the `source` resource with an argument that matches.
        /// </summary>
        /// <param name="useDefaultArgument">The argument to use when adding the existing source.</param>
        /// <param name="trustLevel">The trust level to use when adding the existing source.</param>
        /// <param name="isExplicit">The explicit state to use when adding the existing source.</param>
        /// <param name="priority">The priority to use when adding the existing source.</param>
        /// <param name="targetProperty">The property to target for the test.</param>
        [TestCase(false, DefaultTrustLevel, true, 42, ArgumentPropertyName)]
        [TestCase(true, DefaultTrustLevel, false, 14, TypePropertyName)]
        [TestCase(false, TrustedTrustLevel, false, 42, TrustLevelPropertyName)]
        [TestCase(true, DefaultTrustLevel, true, 39, ExplicitPropertyName)]
        [TestCase(true, DefaultTrustLevel, true, 1, PriorityPropertyName)]
        public void Source_Test_PropertyMatch(bool useDefaultArgument, string trustLevel, bool isExplicit, int priority, string targetProperty)
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {(useDefaultArgument ? DefaultSourceArgForCmdLine : NonDefaultSourceArgForCmdLine)} --type {DefaultSourceType} --trust-level {trustLevel} {(isExplicit ? "--explicit" : string.Empty)} --priority {priority}");
            Assert.That(setup.ExitCode, Is.Zero);

            SourceResourceData resourceData = new SourceResourceData() { Name = DefaultSourceName };

            switch (targetProperty)
            {
                case ArgumentPropertyName:
                    resourceData.Argument = useDefaultArgument ? DefaultSourceArgDirect : NonDefaultSourceArgDirect;
                    break;
                case TypePropertyName:
                    resourceData.Type = DefaultSourceType;
                    break;
                case TrustLevelPropertyName:
                    resourceData.TrustLevel = trustLevel;
                    break;
                case ExplicitPropertyName:
                    resourceData.Explicit = isExplicit;
                    break;
                case PriorityPropertyName:
                    resourceData.Priority = priority;
                    break;
                default:
                    Assert.Fail($"{targetProperty} is not a handled case.");
                    break;
            }

            var result = RunDSCv3Command(SourceResource, TestFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, useDefaultArgument ? DefaultSourceArgDirect : NonDefaultSourceArgDirect, trustLevel, isExplicit);
            Assert.That(output.InDesiredState, Is.True);

            AssertDiffState(diff, []);
        }

        /// <summary>
        /// Calls `test` on the `source` resource with a argument that does not match.
        /// </summary>
        /// <param name="useDefaultArgument">The argument to use when adding the existing source.</param>
        /// <param name="trustLevel">The trust level to use when adding the existing source.</param>
        /// <param name="isExplicit">The explicit state to use when adding the existing source.</param>
        /// <param name="priority">The priority to use when adding the existing source.</param>
        /// <param name="targetProperty">The property to target for the test.</param>
        /// <param name="testValue">The value to test against.</param>
        [TestCase(false, DefaultTrustLevel, true, 2, ArgumentPropertyName, true)]
        [TestCase(false, DefaultTrustLevel, false, 13, TrustLevelPropertyName, TrustedTrustLevel)]
        [TestCase(true, DefaultTrustLevel, true, 42, ExplicitPropertyName, false)]
        [TestCase(true, DefaultTrustLevel, true, 8, PriorityPropertyName, 76)]
        public void Source_Test_PropertyMismatch(bool useDefaultArgument, string trustLevel, bool isExplicit, int priority, string targetProperty, object testValue)
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {(useDefaultArgument ? DefaultSourceArgForCmdLine : NonDefaultSourceArgForCmdLine)} --type {DefaultSourceType} --trust-level {trustLevel} {(isExplicit ? "--explicit" : string.Empty)} --priority {priority}");
            Assert.That(setup.ExitCode, Is.Zero);

            SourceResourceData resourceData = new SourceResourceData() { Name = DefaultSourceName };

            switch (targetProperty)
            {
                case ArgumentPropertyName:
                    resourceData.Argument = (bool)testValue ? DefaultSourceArgDirect : NonDefaultSourceArgDirect;
                    break;
                case TrustLevelPropertyName:
                    resourceData.TrustLevel = (string)testValue;
                    break;
                case ExplicitPropertyName:
                    resourceData.Explicit = (bool)testValue;
                    break;
                case PriorityPropertyName:
                    resourceData.Priority = (int)testValue;
                    break;
                default:
                    Assert.Fail($"{targetProperty} is not a handled case.");
                    break;
            }

            var result = RunDSCv3Command(SourceResource, TestFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, useDefaultArgument ? DefaultSourceArgDirect : NonDefaultSourceArgDirect, trustLevel, isExplicit);
            Assert.That(output.InDesiredState, Is.False);

            AssertDiffState(diff, [ targetProperty ]);
        }

        /// <summary>
        /// Calls `test` on the `source` resource with all properties matching.
        /// </summary>
        [Test]
        public void Source_Test_AllMatch()
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {NonDefaultSourceArgForCmdLine} --type {DefaultSourceType} --trust-level {TrustedTrustLevel} --explicit --priority 42");
            Assert.That(setup.ExitCode, Is.Zero);

            SourceResourceData resourceData = new SourceResourceData()
            {
                Name = DefaultSourceName,
                Argument = NonDefaultSourceArgDirect,
                Type = DefaultSourceType,
                TrustLevel = TrustedTrustLevel,
                Explicit = true,
                Priority = 42,
            };

            var result = RunDSCv3Command(SourceResource, TestFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, resourceData);
            Assert.That(output.InDesiredState, Is.True);

            AssertDiffState(diff, []);
        }

        /// <summary>
        /// Calls `set` on the `source` resource when the source is not present, and again afterward.
        /// </summary>
        [Test]
        public void Source_Set_SimpleRepeated()
        {
            SourceResourceData resourceData = new SourceResourceData()
            {
                Name = DefaultSourceName,
                Argument = NonDefaultSourceArgDirect,
                Type = DefaultSourceType,
            };

            var result = RunDSCv3Command(SourceResource, SetFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, resourceData);

            AssertDiffState(diff, [ ExistPropertyName ]);

            // Set again should be a no-op
            result = RunDSCv3Command(SourceResource, SetFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (output, diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, resourceData);

            AssertDiffState(diff, []);
        }

        /// <summary>
        /// Calls `set` on the `source` resource to ensure that it is not present.
        /// </summary>
        [Test]
        public void Source_Set_Remove()
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {DefaultSourceArgForCmdLine} --type {DefaultSourceType} --explicit");
            Assert.That(setup.ExitCode, Is.Zero);

            SourceResourceData resourceData = new SourceResourceData()
            {
                Name = DefaultSourceName,
                Exist = false,
            };

            var result = RunDSCv3Command(SourceResource, SetFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            Assert.That(output, Is.Not.Null);
            Assert.That(output.Exist, Is.False);
            Assert.That(output.Name, Is.EqualTo(resourceData.Name));

            AssertDiffState(diff, [ ExistPropertyName ]);

            // Call `get` to ensure the result
            SourceResourceData resourceDataForGet = new SourceResourceData()
            {
                Name = DefaultSourceName,
            };

            result = RunDSCv3Command(SourceResource, GetFunction, resourceDataForGet);
            AssertSuccessfulResourceRun(ref result);

            output = GetSingleOutputLineAs<SourceResourceData>(result.StdOut);
            Assert.That(output, Is.Not.Null);
            Assert.That(output.Exist, Is.False);
            Assert.That(output.Name, Is.EqualTo(resourceDataForGet.Name));
        }

        /// <summary>
        /// Calls `set` on the `source` resource with an existing item, replacing it.
        /// </summary>
        [Test]
        public void Source_Set_Replace()
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {DefaultSourceArgForCmdLine} --type {DefaultSourceType}");
            Assert.That(setup.ExitCode, Is.Zero);

            SourceResourceData resourceData = new SourceResourceData()
            {
                Name = DefaultSourceName,
                Argument = DefaultSourceArgDirect,
                Type = DefaultSourceType,
                TrustLevel = TrustedTrustLevel,
                Explicit = true,
            };

            var result = RunDSCv3Command(SourceResource, SetFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, resourceData);

            AssertDiffState(diff, [ TrustLevelPropertyName, ExplicitPropertyName ]);

            // Call `get` to ensure the result
            SourceResourceData resourceDataForGet = new SourceResourceData()
            {
                Name = DefaultSourceName,
            };

            result = RunDSCv3Command(SourceResource, GetFunction, resourceDataForGet);
            AssertSuccessfulResourceRun(ref result);

            output = GetSingleOutputLineAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, resourceData);
        }

        /// <summary>
        /// Calls `set` on the `source` resource with an existing item, editing it due to only changing editable properties.
        /// </summary>
        [Test]
        public void Source_Set_Replace_Edit()
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {DefaultSourceArgForCmdLine} --type {DefaultSourceType}");
            Assert.That(setup.ExitCode, Is.Zero);

            SourceResourceData resourceData = new SourceResourceData()
            {
                Name = DefaultSourceName,
                Explicit = true,
                Priority = 42,
            };

            var result = RunDSCv3Command(SourceResource, SetFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, resourceData);

            AssertDiffState(diff, [ExplicitPropertyName, PriorityPropertyName]);

            // Call `get` to ensure the result
            SourceResourceData resourceDataForGet = new SourceResourceData()
            {
                Name = DefaultSourceName,
            };

            result = RunDSCv3Command(SourceResource, GetFunction, resourceDataForGet);
            AssertSuccessfulResourceRun(ref result);

            output = GetSingleOutputLineAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, resourceData);
        }

        /// <summary>
        /// Calls `export` on the `source` resource without providing any input.
        /// </summary>
        [Test]
        public void Source_Export_NoInput()
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {DefaultSourceArgForCmdLine} --type {DefaultSourceType}");
            Assert.That(setup.ExitCode, Is.Zero);

            var result = RunDSCv3Command(SourceResource, ExportFunction, " ");
            AssertSuccessfulResourceRun(ref result);

            List<SourceResourceData> output = GetOutputLinesAs<SourceResourceData>(result.StdOut);

            bool foundDefaultSource = false;
            foreach (SourceResourceData item in output)
            {
                if (item.Name == DefaultSourceName)
                {
                    foundDefaultSource = true;
                    Assert.That(item.Name, Is.EqualTo(DefaultSourceName));
                    Assert.That(item.Argument, Is.EqualTo(DefaultSourceArgDirect));
                    Assert.That(item.Type, Is.EqualTo(DefaultSourceType));
                    Assert.That(item.TrustLevel, Is.EqualTo(DefaultTrustLevel));
                    Assert.That(item.Explicit, Is.EqualTo(DefaultExplicitState));
                    Assert.That(item.Priority, Is.EqualTo(DefaultPriority));
                    break;
                }
            }

            Assert.That(foundDefaultSource, Is.True);
        }

        private static void RemoveTestSource()
        {
            SourceResourceData resourceData = new SourceResourceData()
            {
                Name = DefaultSourceName,
                Exist = false,
            };

            var result = RunDSCv3Command(SourceResource, SetFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);
        }

        private static void AssertExistingSourceResourceData(SourceResourceData output, SourceResourceData input)
        {
            AssertExistingSourceResourceData(output, input.Argument, input.TrustLevel, input.Explicit, input.Priority);
        }

        private static void AssertExistingSourceResourceData(SourceResourceData output, string argument, string trustLevel = null, bool? isExplicit = null, int? priority = null)
        {
            Assert.That(output, Is.Not.Null);
            Assert.That(output.Exist, Is.True);
            Assert.That(output.Name, Is.EqualTo(DefaultSourceName));

            if (argument != null)
            {
                Assert.That(output.Argument, Is.EqualTo(argument));
            }

            Assert.That(output.Type, Is.EqualTo(DefaultSourceType));

            if (trustLevel != null)
            {
                Assert.That(output.TrustLevel, Is.EqualTo(trustLevel));
            }

            if (isExplicit != null)
            {
                Assert.That(output.Explicit, Is.EqualTo(isExplicit));
            }

            if (priority != null)
            {
                Assert.That(output.Priority, Is.EqualTo(priority));
            }
        }

        private static string CreateSourceArgument(bool forCommandLine = false, int openHR = 0, int searchHR = 0)
        {
            const string CommandLineFormat = @"""{{""""OpenHR"""": {0}, """"SearchHR"""": {1} }}""";
            const string DirectFormat = @"{{""OpenHR"": {0}, ""SearchHR"": {1} }}";
            return string.Format(forCommandLine ? CommandLineFormat : DirectFormat, openHR, searchHR);
        }

        private class SourceResourceData
        {
            [JsonPropertyName(ExistPropertyName)]
            public bool? Exist { get; set; }

            [JsonPropertyName(InDesiredStatePropertyName)]
            public bool? InDesiredState { get; set; }

            public string Name { get; set; }

            public string Argument { get; set; }

            public string Type { get; set; }

            public string TrustLevel { get; set; }

            public bool? Explicit { get; set; }

            public bool? AcceptAgreements { get; set; }

            public int? Priority { get; set; }
        }
    }
}
