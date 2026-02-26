// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitInternalTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Management.Automation;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Tests ConfigurationUnitExtensionsTests.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class ConfigurationUnitInternalTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ConfigurationUnitInternalTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ConfigurationUnitInternalTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Test GetDirectives.
        /// </summary>
        [Fact]
        public void GetDirectivesTest()
        {
            string moduleDirective = "module";
            string unitModule = "xModule";

            string versionDirective = "version";
            string unitVersion = "1.0.0.0";

            string descriptionDirective = "description";
            string unitDescription = "beep beep boop i am a text";

            string boolDirective = "boolDirective";
            bool boolDirectiveValue = true;

            string boolDirective2 = "boolDirective2";
            bool boolDirective2Value = false;

            var unit = new ConfigurationUnit().Assign(new { Type = $"{unitModule}/unitResource" });
            unit.Metadata.Add(moduleDirective, unitModule);
            unit.Metadata.Add(versionDirective, unitVersion);
            unit.Metadata.Add(descriptionDirective, unitDescription);
            unit.Metadata.Add(boolDirective, boolDirectiveValue);
            unit.Metadata.Add(boolDirective2, boolDirective2Value);

            var unitInternal = new ConfigurationUnitAndModule(unit, string.Empty);

            var description = unitInternal.GetDirective<string>(descriptionDirective);
            Assert.Equal(description, unitDescription);

            var fake = unitInternal.GetDirective<string>("fake");
            Assert.Null(fake);

            var description2 = unitInternal.GetDirective<string>("DESCRIPTION");
            Assert.Equal(description2, unitDescription);

            Assert.Equal(unitModule, unitInternal.Module!.Name);

            Assert.Equal(Version.Parse(unitVersion), unitInternal.Module!.RequiredVersion);

            Assert.Equal(boolDirectiveValue, unitInternal.GetDirective(boolDirective));
            Assert.Equal(boolDirective2Value, unitInternal.GetDirective(boolDirective2));
            Assert.Null(unitInternal.GetDirective("fakeBool"));
        }

        /// <summary>
        /// Tests GetVersion with a bad version.
        /// </summary>
        [Fact]
        public void GetVersion_BadVersion()
        {
            var unit = new ConfigurationUnit();
            unit.Metadata.Add("module", "module");
            unit.Metadata.Add("version", "not a version");

            Assert.Throws<ArgumentException>(
                () => new ConfigurationUnitInternal(unit, string.Empty));
        }

        /// <summary>
        /// Verifies expansion of ConfigRoot.
        /// </summary>
        [Fact]
        public void GetExpandedSettings_ConfigRoot()
        {
            using var tmpFile = new TempFile("fakeConfigFile.yml", content: "content");

            var unit = new ConfigurationUnit().Assign(new { Type = "unitModule/unitResource" });
            unit.Settings.Add("var1", @"$WinGetConfigRoot\this\is\a\path.txt");
            unit.Settings.Add("var2", @"${WinGetConfigRoot}\this\is\a\path.txt");
            unit.Settings.Add("var3", @"this\is\a\$WINGETCONFIGROOT\path.txt");
            unit.Settings.Add("var4", @"this\is\a\${WINGETCONFIGROOT}\path.txt");
            unit.Settings.Add("var5", @"this\is\a\path\$wingetconfigroot");
            unit.Settings.Add("var6", @"this\is\a\path\${wingetconfigroot}");

            string configPath = tmpFile.FullFileName;
            string? expectedPath = Path.GetDirectoryName(configPath);
            var unitInternal = new ConfigurationUnitInternal(unit, configPath);

            var expandedSettings = unitInternal.GetExpandedSettings();

            var var1 = expandedSettings["var1"];
            Assert.Equal(@"$WinGetConfigRoot\this\is\a\path.txt", var1 as string);

            var var2 = expandedSettings["var2"];
            Assert.Equal($@"{expectedPath}\this\is\a\path.txt", var2 as string);

            var var3 = expandedSettings["var3"];
            Assert.Equal(@"this\is\a\$WINGETCONFIGROOT\path.txt", var3 as string);

            var var4 = expandedSettings["var4"];
            Assert.Equal($@"this\is\a\{expectedPath}\path.txt", var4 as string);

            var var5 = expandedSettings["var5"];
            Assert.Equal(@"this\is\a\path\$wingetconfigroot", var5 as string);

            var var6 = expandedSettings["var6"];
            Assert.Equal($@"this\is\a\path\{expectedPath}", var6 as string);
        }

        /// <summary>
        /// Verifies throws when config root is not set.
        /// </summary>
        [Fact]
        public void GetExpandedSetting_ConfigRoot_Throw()
        {
            var unit = new ConfigurationUnit().Assign(new { Type = "unitModule/unitResource" });
            unit.Settings.Add("var2", @"${WinGetConfigRoot}\this\is\a\path.txt");

            var unitInternal = new ConfigurationUnitInternal(unit, null!);
            Assert.Throws<UnitSettingConfigRootException>(() => unitInternal.GetExpandedSettings());
        }
    }
}
