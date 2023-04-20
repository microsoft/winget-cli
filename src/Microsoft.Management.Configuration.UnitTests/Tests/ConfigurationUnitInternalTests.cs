// -----------------------------------------------------------------------------
// <copyright file="ConfigurationUnitInternalTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using System.Management.Automation;
    using Microsoft.Management.Configuration;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Tests ConfigurationUnitExtensionsTests.
    /// </summary>
    [Collection("UnitTestCollection")]
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
            string overlayDescription = "beep beep boop i am an overlay text";

            string anotherDirective = "another";
            string overlayAnother = "insert another text";

            var unit = new ConfigurationUnit();
            unit.Directives.Add(moduleDirective, unitModule);
            unit.Directives.Add(versionDirective, unitVersion);
            unit.Directives.Add(descriptionDirective, unitDescription);

            var overlays = new Dictionary<string, object>()
            {
                { descriptionDirective, overlayDescription },
                { anotherDirective, overlayAnother },
            };

            var unitInternal = new ConfigurationUnitInternal(unit, overlays);

            var description = unitInternal.GetDirective(descriptionDirective);
            Assert.Equal(description, overlayDescription);

            var another = unitInternal.GetDirective(anotherDirective);
            Assert.Equal(another, overlayAnother);

            var fake = unitInternal.GetDirective("fake");
            Assert.Null(fake);

            var description2 = unitInternal.GetDirective("DESCRIPTION");
            Assert.Equal(description2, overlayDescription);

            Assert.Equal(unitModule, unitInternal.Module!.Name);

            Assert.Equal(Version.Parse(unitVersion), unitInternal.Module!.RequiredVersion);
        }

        /// <summary>
        /// Tests GetVersion with a bad version.
        /// </summary>
        [Fact]
        public void GetVersion_BadVersion()
        {
            var unit = new ConfigurationUnit();
            unit.Directives.Add("module", "module");
            unit.Directives.Add("version", "not a version");

            Assert.Throws<PSInvalidCastException>(
                () => new ConfigurationUnitInternal(unit, null));
        }
    }
}
