// -----------------------------------------------------------------------------
// <copyright file="DSCv3ProcessorTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using Microsoft.Management.Configuration.Processor;
    using Microsoft.Management.Configuration.Processor.DSCv3.Model;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using System.Collections.Generic;
    using System.Linq;
    using Windows.Foundation.Collections;
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
            dsc.GetResourceByTypeResult = null;
            dsc.GetResourceByTypeDelegate = s => throw new System.Exception("Shouldn't be called");
            details = setProcessor.GetUnitProcessorDetails(unit1, ConfigurationUnitDetailFlags.Local);
            Assert.NotNull(details);
            Assert.Equal(type1, details.UnitType);

            // Different type, no details
            dsc.GetResourceByTypeDelegate = null;
            details = setProcessor.GetUnitProcessorDetails(unit2, ConfigurationUnitDetailFlags.Local);
            Assert.Null(details);

            // Null result not cached
            dsc.GetResourceByTypeResult = new TestResourceListItem() { Type = type2 };
            details = setProcessor.GetUnitProcessorDetails(unit2, ConfigurationUnitDetailFlags.Local);
            Assert.NotNull(details);
            Assert.Equal(type2, details.UnitType);

            // First type is still first type
            dsc.GetResourceByTypeResult = null;
            dsc.GetResourceByTypeDelegate = s => throw new System.Exception("Shouldn't be called");
            details = setProcessor.GetUnitProcessorDetails(unit1, ConfigurationUnitDetailFlags.Local);
            Assert.NotNull(details);
            Assert.Equal(type1, details.UnitType);
        }

        /// <summary>
        /// Test for unit processor creation requiring resource to be found.
        /// </summary>
        [Fact]
        public void Set_ResourceNotFoundIsError()
        {
            var (factory, dsc) = CreateTestFactory();
            var set = this.ConfigurationSet();
            string type1 = "Type1";
            var unit1 = this.ConfigurationUnit().Assign(new { Type = type1 });

            var setProcessor = factory.CreateSetProcessor(set);

            // Not found is error
            Assert.Throws<FindDscResourceNotFoundException>(() => setProcessor.CreateUnitProcessor(unit1));

            // Found is not error
            dsc.GetResourceByTypeResult = new TestResourceListItem() { Type = type1 };
            var unitProcessor = setProcessor.CreateUnitProcessor(unit1);
            Assert.NotNull(unitProcessor);
            Assert.Equal(type1, unitProcessor.Unit.Type);
        }

        /// <summary>
        /// Test for settings export.
        /// </summary>
        [Fact]
        public void GetAllSettings_Expected()
        {
            var (factory, dsc) = CreateTestFactory();
            var processor = this.CreateConfigurationProcessorWithDiagnostics(factory);

            string type1 = "Type1";
            var unit1 = this.ConfigurationUnit().Assign(new { Type = type1 });

            dsc.GetResourceByTypeDelegate = (type) =>
            {
                Assert.Equal(type1, type);
                return new TestResourceListItem() { Type = type1 };
            };

            ValueSet set1 = new ValueSet();
            set1.Add("key1", "val1");

            ValueSet set2 = new ValueSet();
            set2.Add("key2", "val2");

            dsc.ExportResourceResult = new List<IResourceExportItem>()
            {
                new TestResourceExportItem() { Type = type1, Name = "1", Settings = set1 },
                new TestResourceExportItem() { Type = type1, Name = "2", Settings = set2 },
            };

            var result = processor.GetAllUnitSettings(unit1);

            Assert.NotNull(result);
            Assert.NotNull(result.ResultInformation);
            Assert.Null(result.ResultInformation.ResultCode);

            Assert.Equal(2, result.Settings.Count);
            Assert.NotNull(result.Settings.Single(set => set.Contains(set1.First())));
            Assert.NotNull(result.Settings.Single(set => set.Contains(set2.First())));
        }

        // TODO: GetAllSettings_DifferentType, GetAllUnits_SameType, GetAllUnits_MultipleTypes (with rest of Exporter support)

        private static (DSCv3ConfigurationSetProcessorFactory, TestDSCv3) CreateTestFactory()
        {
            DSCv3ConfigurationSetProcessorFactory factory = new DSCv3ConfigurationSetProcessorFactory();
            TestDSCv3 dsc = new TestDSCv3();
            factory.Settings.DSCv3 = dsc;
            factory.Settings.DscExecutablePath = "Test-Path-Not-Used.txt";

            return (factory, dsc);
        }
    }
}
