// -----------------------------------------------------------------------------
// <copyright file="DscResourceMapTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.Processor.PowerShell.DscResourcesInfo;
    using Microsoft.Management.Configuration.Processor.PowerShell.Helpers;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// DscResourceMap tests.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class DscResourceMapTests
    {
        private const string ResourceZoro = "xResourceZoro";

        private const string ModuleMugiwara = "xMugiwaraModule";
        private const string ModuleMugiwaraResourceLuffy = "xResourceLuffy";

        private const string ModuleOni = "xModuleOni";
        private const string ModuleOniResourceKaido = "xResourceKaido";
        private const string ModuleOniResourceYamato = "xResourceYamato";

        private static readonly Version VersionZoro = new Version("1.0.0.0");

        private static readonly Version VersionLuffyGear4 = new Version("4.0.0.1");
        private static readonly Version VersionLuffyGear5 = new Version("5.0.0.0");

        private static readonly Version VersionKaido = new Version("0.0.0.4");
        private static readonly Version VersionYamato = new Version("0.0.0.11");

        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="DscResourceMapTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log.</param>
        public DscResourceMapTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Test GetResource for resources in the map.
        /// </summary>
        [Fact]
        public void DscResourcesMap_GetResource()
        {
            var dscResourceMap = new DscResourcesMap(this.CreateDscResourceInfo());

            // Get just by name.
            var zoroResult = dscResourceMap.GetResource(ResourceZoro, null, null);
            Assert.NotNull(zoroResult);
            Assert.Equal(ResourceZoro, zoroResult.Name);
            Assert.Equal(VersionZoro, zoroResult.Version);

            // Name not normalized.
            var zoroResultNormalize = dscResourceMap.GetResource("XRESOURCEZORO", null, null);
            Assert.NotNull(zoroResultNormalize);
            Assert.Equal(ResourceZoro, zoroResultNormalize.Name);
            Assert.Equal(VersionZoro, zoroResultNormalize.Version);

            var yamatoResult = dscResourceMap.GetResource(ModuleOniResourceYamato, null, null);
            Assert.NotNull(yamatoResult);
            Assert.Equal(ModuleOniResourceYamato, yamatoResult.Name);
            Assert.Equal(ModuleOni, yamatoResult.ModuleName);
            Assert.Equal(VersionYamato, yamatoResult.Version);

            // Just by name and module get latest.
            var luffyResult5 = dscResourceMap.GetResource(ModuleMugiwaraResourceLuffy, ModuleMugiwara, null);
            Assert.NotNull(luffyResult5);
            Assert.Equal(ModuleMugiwaraResourceLuffy, luffyResult5.Name);
            Assert.Equal(ModuleMugiwara, luffyResult5.ModuleName);
            Assert.Equal(VersionLuffyGear5, luffyResult5.Version);

            // Specific version.
            var luffyResult4 = dscResourceMap.GetResource(ModuleMugiwaraResourceLuffy, ModuleMugiwara, VersionLuffyGear4);
            Assert.NotNull(luffyResult4);
            Assert.Equal(ModuleMugiwaraResourceLuffy, luffyResult4.Name);
            Assert.Equal(ModuleMugiwara, luffyResult4.ModuleName);
            Assert.Equal(VersionLuffyGear4, luffyResult4.Version);

            // Module name not normalized
            var luffyResult4Normalized = dscResourceMap.GetResource(ModuleMugiwaraResourceLuffy, "XMUGIWARAMODULE", VersionLuffyGear4);
            Assert.NotNull(luffyResult4Normalized);
            Assert.Equal(ModuleMugiwaraResourceLuffy, luffyResult4Normalized.Name);
            Assert.Equal(ModuleMugiwara, luffyResult4Normalized.ModuleName);
            Assert.Equal(VersionLuffyGear4, luffyResult4Normalized.Version);
        }

        /// <summary>
        /// Tests GetResource when resources are not in the maps.
        /// </summary>
        [Fact]
        public void DscResourcesMap_GetResource_NoResource()
        {
            var dscResourceMap = new DscResourcesMap(this.CreateDscResourceInfo());

            // Zoro is lost.
            var zoroResult = dscResourceMap.GetResource(ResourceZoro, ModuleMugiwara, null);
            Assert.Null(zoroResult);

            // Yamato didn't join (spoilers)
            var yamatoResult = dscResourceMap.GetResource(ModuleOniResourceYamato, ModuleMugiwara, VersionYamato);
            Assert.Null(yamatoResult);

            // Gear 6 is not a thing (?)
            var luffyResult = dscResourceMap.GetResource(ModuleMugiwaraResourceLuffy, ModuleMugiwara, Version.Parse("6.0.0.0"));
            Assert.Null(luffyResult);

            // Wrong universe.
            var gokuResult = dscResourceMap.GetResource("xResourceGoku", null, null);
            Assert.Null(gokuResult);
        }

        /// <summary>
        /// Test Exists for resources in the map.
        /// </summary>
        [Fact]
        public void DscResourcesMap_Exists()
        {
            var dscResourceMap = new DscResourcesMap(this.CreateDscResourceInfo());

            Assert.True(dscResourceMap.Exists(ResourceZoro, null, null));
            Assert.True(dscResourceMap.Exists(ModuleOniResourceYamato, null, null));
            Assert.True(dscResourceMap.Exists(ModuleMugiwaraResourceLuffy, ModuleMugiwara, null));
            Assert.True(dscResourceMap.Exists(ModuleMugiwaraResourceLuffy, ModuleMugiwara, VersionLuffyGear4));
        }

        /// <summary>
        /// Tests Exists when resources are not in the maps.
        /// </summary>
        [Fact]
        public void DscResourcesMap_Exists_NoResource()
        {
            var dscResourceMap = new DscResourcesMap(this.CreateDscResourceInfo());

            Assert.False(dscResourceMap.Exists(ResourceZoro, ModuleMugiwara, null));
            Assert.False(dscResourceMap.Exists(ModuleOniResourceYamato, ModuleMugiwara, VersionYamato));
            Assert.False(dscResourceMap.Exists(ModuleMugiwaraResourceLuffy, ModuleMugiwara, Version.Parse("6.0.0.0")));
            Assert.False(dscResourceMap.Exists("xResourceGoku", null, null));
        }

        /// <summary>
        /// Creates 5 fake resources.
        /// 1 resource without module name.
        /// 2 resources module A same resource v1 v2.
        /// 2 resources module B different resources.
        /// </summary>
        /// <returns>Fake resources.</returns>
        private IReadOnlyList<DscResourceInfoInternal> CreateDscResourceInfo()
        {
            return new List<DscResourceInfoInternal>()
            {
                new DscResourceInfoInternal(ResourceZoro, null, VersionZoro),

                new DscResourceInfoInternal(ModuleMugiwaraResourceLuffy, ModuleMugiwara, VersionLuffyGear4),
                new DscResourceInfoInternal(ModuleMugiwaraResourceLuffy, ModuleMugiwara, VersionLuffyGear5),

                new DscResourceInfoInternal(ModuleOniResourceKaido, ModuleOni, VersionKaido),
                new DscResourceInfoInternal(ModuleOniResourceYamato, ModuleOni, VersionYamato),
            };
        }
    }
}
