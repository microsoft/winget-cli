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
        private const string SourceResource = "source";
        private const string ArgumentPropertyName = "argument";
        private const string TypePropertyName = "type";
        private const string TrustLevelPropertyName = "trustLevel";
        private const string ExplicitPropertyName = "explicit";

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
            Assert.IsNotNull(output);
            Assert.False(output.Exist);
            Assert.AreEqual(resourceData.Name, output.Name);
        }

        /// <summary>
        /// Calls `get` on the `source` resource with the value present.
        /// </summary>
        [Test]
        public void Source_Get_Present()
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {DefaultSourceArgForCmdLine} --type {DefaultSourceType} --explicit");
            Assert.AreEqual(0, setup.ExitCode);

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
            Assert.IsNotNull(output);
            Assert.False(output.Exist);
            Assert.AreEqual(resourceData.Name, output.Name);
            Assert.False(output.InDesiredState);

            AssertDiffState(diff, [ ExistPropertyName ]);
        }

        /// <summary>
        /// Calls `test` on the `source` resource with the value present.
        /// </summary>
        [Test]
        public void Source_Test_SimplePresent()
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {DefaultSourceArgForCmdLine} --type {DefaultSourceType}");
            Assert.AreEqual(0, setup.ExitCode);

            SourceResourceData resourceData = new SourceResourceData() { Name = DefaultSourceName };

            var result = RunDSCv3Command(SourceResource, TestFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, DefaultSourceArgDirect, DefaultTrustLevel, DefaultExplicitState);
            Assert.True(output.InDesiredState);

            AssertDiffState(diff, []);
        }

        /// <summary>
        /// Calls `test` on the `source` resource with a argument that matches.
        /// </summary>
        /// <param name="useDefaultArgument">The argument to use when adding the existing source.</param>
        /// <param name="trustLevel">The trust level to use when adding the existing source.</param>
        /// <param name="isExplicit">The explicit state to use when adding the existing source.</param>
        /// <param name="targetProperty">The property to target for the test.</param>
        [TestCase(false, DefaultTrustLevel, true, ArgumentPropertyName)]
        [TestCase(true, DefaultTrustLevel, false, TypePropertyName)]
        [TestCase(false, TrustedTrustLevel, false, TrustLevelPropertyName)]
        [TestCase(true, DefaultTrustLevel, true, ExplicitPropertyName)]
        public void Source_Test_PropertyMatch(bool useDefaultArgument, string trustLevel, bool isExplicit, string targetProperty)
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {(useDefaultArgument ? DefaultSourceArgForCmdLine : NonDefaultSourceArgForCmdLine)} --type {DefaultSourceType} --trust-level {trustLevel} {(isExplicit ? "--explicit" : string.Empty)}");
            Assert.AreEqual(0, setup.ExitCode);

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
                default:
                    Assert.Fail($"{targetProperty} is not a handled case.");
                    break;
            }

            var result = RunDSCv3Command(SourceResource, TestFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, useDefaultArgument ? DefaultSourceArgDirect : NonDefaultSourceArgDirect, trustLevel, isExplicit);
            Assert.True(output.InDesiredState);

            AssertDiffState(diff, []);
        }

        /// <summary>
        /// Calls `test` on the `source` resource with a argument that does not match.
        /// </summary>
        /// <param name="useDefaultArgument">The argument to use when adding the existing source.</param>
        /// <param name="trustLevel">The trust level to use when adding the existing source.</param>
        /// <param name="isExplicit">The explicit state to use when adding the existing source.</param>
        /// <param name="targetProperty">The property to target for the test.</param>
        /// <param name="testValue">The value to test against.</param>
        [TestCase(false, DefaultTrustLevel, true, ArgumentPropertyName, true)]
        [TestCase(false, DefaultTrustLevel, false, TrustLevelPropertyName, TrustedTrustLevel)]
        [TestCase(true, DefaultTrustLevel, true, ExplicitPropertyName, false)]
        public void Source_Test_PropertyMismatch(bool useDefaultArgument, string trustLevel, bool isExplicit, string targetProperty, object testValue)
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {(useDefaultArgument ? DefaultSourceArgForCmdLine : NonDefaultSourceArgForCmdLine)} --type {DefaultSourceType} --trust-level {trustLevel} {(isExplicit ? "--explicit" : string.Empty)}");
            Assert.AreEqual(0, setup.ExitCode);

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
                default:
                    Assert.Fail($"{targetProperty} is not a handled case.");
                    break;
            }

            var result = RunDSCv3Command(SourceResource, TestFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, useDefaultArgument ? DefaultSourceArgDirect : NonDefaultSourceArgDirect, trustLevel, isExplicit);
            Assert.False(output.InDesiredState);

            AssertDiffState(diff, [ targetProperty ]);
        }

        /// <summary>
        /// Calls `test` on the `source` resource with all properties matching.
        /// </summary>
        [Test]
        public void Source_Test_AllMatch()
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {NonDefaultSourceArgForCmdLine} --type {DefaultSourceType} --trust-level {TrustedTrustLevel} --explicit");
            Assert.AreEqual(0, setup.ExitCode);

            SourceResourceData resourceData = new SourceResourceData()
            {
                Name = DefaultSourceName,
                Argument = NonDefaultSourceArgDirect,
                Type = DefaultSourceType,
                TrustLevel = TrustedTrustLevel,
                Explicit = true,
            };

            var result = RunDSCv3Command(SourceResource, TestFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            AssertExistingSourceResourceData(output, resourceData);
            Assert.True(output.InDesiredState);

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
            Assert.AreEqual(0, setup.ExitCode);

            SourceResourceData resourceData = new SourceResourceData()
            {
                Name = DefaultSourceName,
                Exist = false,
            };

            var result = RunDSCv3Command(SourceResource, SetFunction, resourceData);
            AssertSuccessfulResourceRun(ref result);

            (SourceResourceData output, List<string> diff) = GetSingleOutputLineAndDiffAs<SourceResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.False(output.Exist);
            Assert.AreEqual(resourceData.Name, output.Name);

            AssertDiffState(diff, [ ExistPropertyName ]);

            // Call `get` to ensure the result
            SourceResourceData resourceDataForGet = new SourceResourceData()
            {
                Name = DefaultSourceName,
            };

            result = RunDSCv3Command(SourceResource, GetFunction, resourceDataForGet);
            AssertSuccessfulResourceRun(ref result);

            output = GetSingleOutputLineAs<SourceResourceData>(result.StdOut);
            Assert.IsNotNull(output);
            Assert.False(output.Exist);
            Assert.AreEqual(resourceDataForGet.Name, output.Name);
        }

        /// <summary>
        /// Calls `set` on the `source` resource with an existing item, replacing it.
        /// </summary>
        [Test]
        public void Source_Set_Replace()
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {DefaultSourceArgForCmdLine} --type {DefaultSourceType}");
            Assert.AreEqual(0, setup.ExitCode);

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
        /// Calls `export` on the `source` resource without providing any input.
        /// </summary>
        [Test]
        public void Source_Export_NoInput()
        {
            var setup = TestCommon.RunAICLICommand("source add", $"--name {DefaultSourceName} --arg {DefaultSourceArgForCmdLine} --type {DefaultSourceType}");
            Assert.AreEqual(0, setup.ExitCode);

            var result = RunDSCv3Command(SourceResource, ExportFunction, " ");
            AssertSuccessfulResourceRun(ref result);

            List<SourceResourceData> output = GetOutputLinesAs<SourceResourceData>(result.StdOut);

            bool foundDefaultSource = false;
            foreach (SourceResourceData item in output)
            {
                if (item.Name == DefaultSourceName)
                {
                    foundDefaultSource = true;
                    Assert.AreEqual(DefaultSourceName, item.Name);
                    Assert.AreEqual(DefaultSourceArgDirect, item.Argument);
                    Assert.AreEqual(DefaultSourceType, item.Type);
                    Assert.AreEqual(DefaultTrustLevel, item.TrustLevel);
                    Assert.AreEqual(DefaultExplicitState, item.Explicit);
                    break;
                }
            }

            Assert.IsTrue(foundDefaultSource);
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
            AssertExistingSourceResourceData(output, input.Argument, input.TrustLevel, input.Explicit);
        }

        private static void AssertExistingSourceResourceData(SourceResourceData output, string argument, string trustLevel = null, bool? isExplicit = null)
        {
            Assert.IsNotNull(output);
            Assert.True(output.Exist);
            Assert.AreEqual(DefaultSourceName, output.Name);
            Assert.AreEqual(argument, output.Argument);
            Assert.AreEqual(DefaultSourceType, output.Type);

            if (trustLevel != null)
            {
                Assert.AreEqual(trustLevel, output.TrustLevel);
            }

            if (isExplicit != null)
            {
                Assert.AreEqual(isExplicit, output.Explicit);
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
        }
    }
}
