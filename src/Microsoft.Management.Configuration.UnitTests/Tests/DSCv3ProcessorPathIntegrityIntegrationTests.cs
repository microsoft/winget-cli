// -----------------------------------------------------------------------------
// <copyright file="DSCv3ProcessorPathIntegrityIntegrationTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Text;
    using System.Threading.Tasks;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Integration tests for DSCv3 processor path integrity checks.
    /// These tests verify that hash verification catches mismatches when a custom DSC executable
    /// path is provided. Units run in-process (no elevation split) so limit mode is not involved.
    /// </summary>
    [Collection("UnitTestCollection")]
    [OutOfProc]
    public class DSCv3ProcessorPathIntegrityIntegrationTests : ConfigurationProcessorTestBase
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="DSCv3ProcessorPathIntegrityIntegrationTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public DSCv3ProcessorPathIntegrityIntegrationTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Verifies that providing a wrong hash for a custom processor path causes the
        /// apply to fail with a hash mismatch error.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task Apply_ProcessorPath_WrongHash_FailsWithHashMismatch()
        {
            using var tempFile = new TempFile(content: "test content for hash test");

            IConfigurationSetProcessorFactory dynamicFactory =
                await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync(
                    Helpers.Constants.DSCv3DynamicRuntimeHandlerIdentifier);

            var factoryMap = (IDictionary<string, string>)dynamicFactory;
            factoryMap["DscExecutablePath"] = tempFile.FullFileName;
            factoryMap["DscExecutablePathHash"] = new string('0', 64); // Wrong hash.
            factoryMap["DscExecutablePathIsAlias"] = "false";

            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.SchemaVersion = "0.3";
            configurationSet.Metadata.Add(Helpers.Constants.EnableDynamicFactoryTestMode, true);

            ConfigurationUnit unit = this.ConfigurationUnit();
            unit.Identifier = "testUnit";
            unit.Type = "TestResource/TestUnit";
            unit.Intent = ConfigurationUnitIntent.Apply;
            configurationSet.Units = new[] { unit };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);
            var ex = Assert.ThrowsAny<Exception>(() => processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None));
            Assert.Equal(Errors.WINGET_CONFIG_ERROR_PROCESSOR_HASH_MISMATCH, ex.HResult);
        }

        /// <summary>
        /// Verifies that omitting the hash for a custom processor path causes the apply
        /// to fail. The server requires a hash whenever a custom path is provided.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task Apply_ProcessorPath_MissingHash_ApplyFails()
        {
            using var tempFile = new TempFile(content: "test content");

            IConfigurationSetProcessorFactory dynamicFactory =
                await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync(
                    Helpers.Constants.DSCv3DynamicRuntimeHandlerIdentifier);

            var factoryMap = (IDictionary<string, string>)dynamicFactory;

            // DscExecutablePathHash intentionally omitted.
            factoryMap["DscExecutablePath"] = tempFile.FullFileName;

            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.SchemaVersion = "0.3";
            configurationSet.Metadata.Add(Helpers.Constants.EnableDynamicFactoryTestMode, true);

            ConfigurationUnit unit = this.ConfigurationUnit();
            unit.Identifier = "testUnit";
            unit.Type = "TestResource/TestUnit";
            unit.Intent = ConfigurationUnitIntent.Apply;
            configurationSet.Units = new[] { unit };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);
            Assert.ThrowsAny<Exception>(() => processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None));
        }

        /// <summary>
        /// Regression test for the case-sensitivity mismatch between C# and C++.
        ///
        /// When the auto-discovered DSC path is an app execution alias (the "Found" code path),
        /// the C# factory's <c>TryGetValue("FoundDscExecutablePathIsAlias")</c> used to return
        /// <c>"True"</c> (PascalCase from <c>bool.ToString()</c>), while the C++ side in
        /// <c>ConfigurationDynamicRuntimeFactory.cpp</c> compared it case-sensitively against
        /// <c>L"true"</c>. The mismatch caused <c>processorPathIsAlias</c> to be serialized as
        /// <c>false</c> in the JSON sent to the remoting server. The server then called
        /// <c>VerifyAndOpen</c> with <c>isAlias:false</c> on an APPEXECLINK reparse point, which
        /// fails with Win32 error 1920 (ERROR_CANT_ACCESS_FILE).
        ///
        /// This test exercises the full C++ dynamic factory → C# TryGetValue → JSON
        /// serialization → remoting server boundary, with wingetdev.exe as the injected
        /// "found" path (an app execution alias that is always registered when the test
        /// package is installed). The regression is detected as Win32 error 1920 on apply.
        /// </summary>
        /// <returns>A <see cref="Task"/> representing the asynchronous unit test.</returns>
        [Fact]
        public async Task Apply_FoundProcessorPath_IsAlias_DoesNotFailWithCantAccessFile()
        {
            // wingetdev.exe is an app execution alias registered when the test package is deployed.
            string wingetdevPath = Path.Combine(
                Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
                @"Microsoft\WindowsApps\wingetdev.exe");

            Assert.True(File.Exists(wingetdevPath), $"wingetdev.exe not found at '{wingetdevPath}'. The test package must be deployed before running this test.");

            // Obtain the factory via the C++ dynamic factory so that SerializeSetProperties,
            // which contains the Lookup("FoundDscExecutablePathIsAlias") → TryGetValue →
            // bool.ToString() path, is exercised.
            IConfigurationSetProcessorFactory dynamicFactory =
                await this.fixture.ConfigurationStatics.CreateConfigurationSetProcessorFactoryAsync(
                    Helpers.Constants.DSCv3DynamicRuntimeHandlerIdentifier);

            var factoryMap = (IDictionary<string, string>)dynamicFactory;

            // Inject wingetdev as the "found" path. This sets only the path in ProcessorSettings;
            // the hash and isAlias flag are computed automatically by EnsureFoundPathHashCached so
            // the real bool.ToString() stringification is exercised (not a hardcoded "true" string).
            // Deliberately do NOT set "DscExecutablePath" so the C++ takes the usingFoundPath=true
            // branch and calls Lookup("FoundDscExecutablePathIsAlias").
            factoryMap[Helpers.Constants.TestFoundDscExecutablePathPropertyName] = wingetdevPath;

            ConfigurationSet configurationSet = this.ConfigurationSet();
            configurationSet.SchemaVersion = "0.3";
            configurationSet.Metadata.Add(Helpers.Constants.EnableDynamicFactoryTestMode, true);
            configurationSet.Metadata.Add(Helpers.Constants.ForceHighIntegrityLevelUnitsTestGuid, true);

            ConfigurationUnit unit = this.ConfigurationUnit();
            unit.Identifier = "testUnit";
            unit.Type = "Microsoft.WinGet.Dev/TestJSON";
            configurationSet.Units = new[] { unit };

            ConfigurationProcessor processor = this.CreateConfigurationProcessorWithDiagnostics(dynamicFactory);
            ApplyConfigurationSetResult result = processor.ApplySet(configurationSet, ApplyConfigurationSetFlags.None);

            Assert.NotNull(result);
            Assert.Equal(1, result.UnitResults.Count);

            ApplyConfigurationUnitResult unitResult = result.UnitResults[0];
            Assert.NotNull(unitResult);

            string resultMessage = unitResult.ResultInformation?.ResultCode != null
                ? $"HResult=0x{unitResult.ResultInformation.ResultCode.HResult:X8}: {unitResult.ResultInformation.Description}"
                : "no error";
            this.log.WriteLine($"Unit result: {resultMessage}");

            // Before the fix: isAlias is incorrectly false, so VerifyAndOpen opens the APPEXECLINK
            // with GENERIC_READ which fails with Win32 error 1920, causing VerifyAndOpen to throw
            // InvalidOperationException (HResult = COR_E_INVALIDOPERATION = 0x80131509).
            // After the fix: the alias is opened correctly; any failure is unrelated to alias access
            // and will have a different HResult.
            const int corEInvalidOperation = unchecked((int)0x80131509);
            Assert.NotEqual(corEInvalidOperation, unitResult.ResultInformation?.ResultCode?.HResult ?? 0);
        }

        /// <summary>
        /// Returns all exception messages (including inner exceptions) as a single string.
        /// </summary>
        /// <param name="ex">The exception chain to inspect.</param>
        /// <returns>Concatenated exception messages.</returns>
        private static string GetFullExceptionMessage(Exception ex)
        {
            var sb = new StringBuilder();
            for (Exception? current = ex; current != null; current = current.InnerException)
            {
                sb.AppendLine(current.Message);
            }

            return sb.ToString();
        }
    }
}
