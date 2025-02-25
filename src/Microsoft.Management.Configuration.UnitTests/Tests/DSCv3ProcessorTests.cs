// -----------------------------------------------------------------------------
// <copyright file="DSCv3ProcessorTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using Microsoft.Management.Configuration.Processor;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Tests for the DSCv3 processor.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class DSCv3ProcessorTests : ConfigurationProcessorTestBase
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="DSCv3ProcessorTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public DSCv3ProcessorTests(UnitTestFixture fixture, ITestOutputHelper log)
            : base(fixture, log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Tests for the unit details caching.
        /// </summary>
        [Fact]
        public void Set_UnitPropertyDetailsCached()
        {
            var (factory, dsc) = CreateTestFactory();
            var set = this.ConfigurationSet();
            string type1 = "Type1";
            string type2 = "Type2";
            var unit1 = this.ConfigurationUnit().Assign(new { Type = type1 });
            var unit2 = this.ConfigurationUnit().Assign(new { Type = type2 });

            var setProcessor = factory.CreateSetProcessor(set);

            // Initially, no details
            var details = setProcessor.GetUnitProcessorDetails(unit1, ConfigurationUnitDetailFlags.Local);
            Assert.Null(details);

            // Null result not cached
            dsc.GetResourceByTypeResult = new TestResourceListItem() { Type = type1 };
            details = setProcessor.GetUnitProcessorDetails(unit1, ConfigurationUnitDetailFlags.Local);
            Assert.NotNull(details);
            Assert.Equal(type1, details.UnitType);

            // Not-null result cached
        }

        private static (DSCv3ConfigurationSetProcessorFactory, TestDSCv3) CreateTestFactory()
        {
            DSCv3ConfigurationSetProcessorFactory factory = new DSCv3ConfigurationSetProcessorFactory();
            TestDSCv3 dsc = new TestDSCv3();
            factory.Settings.DSCv3 = dsc;

            return (factory, dsc);
        }
    }
}
