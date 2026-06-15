// -----------------------------------------------------------------------------
// <copyright file="ManifestEqualityUnitTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetUtilInterop.UnitTests.ManifestUnitTest
{
    using System.IO;
    using System.Reflection;
    using Microsoft.WinGetUtil.Models.Preview;
    using Microsoft.WinGetUtil.UnitTests.Common.Logging;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Manifest equality tests.
    /// </summary>
    public class ManifestEqualityUnitTests
    {
        private ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ManifestEqualityUnitTests"/> class.
        /// </summary>
        /// <param name="log">Output Helper.</param>
        public ManifestEqualityUnitTests(ITestOutputHelper log)
        {
            this.log = log;
        }

        /// <summary>
        /// All manifest are equal but some manifest are more equal than others.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void ManifestEquals()
        {
            // All equality properties.
            Manifest allEquality = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.AllEquality));
            Manifest otherAllEquality = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.AllEquality));
            AssertEquivalence(allEquality, allEquality);
            AssertEquivalence(allEquality, otherAllEquality);

            // With all equality properties and non equality properties.
            Manifest allEqualityWithDescription = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.AllEqualityWithDescription));
            AssertEquivalence(allEquality, allEqualityWithDescription);

            // With some equality properties.
            Manifest someEquality = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.SomeEquality));
            Manifest otherSomeEquality = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.SomeEquality));
            AssertEquivalence(someEquality, otherSomeEquality);

            // With some equality properties and some non equality properties
            Manifest someEqualityWithLocalization = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.SomeEqualityWithLocalization));
            AssertEquivalence(someEquality, someEqualityWithLocalization);

            // Equality where a Switch object is not serialized
            Manifest someEqualityWithoutSwitch = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.SomeEqualityWithoutSwitches));
            Manifest otherSomeEqualityWithoutSwitch = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.SomeEqualityWithoutSwitches));
            AssertEquivalence(someEqualityWithoutSwitch, otherSomeEqualityWithoutSwitch);

            // Equality where Installers list is not serialized
            Manifest someEqualityWithoutInstaller = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.SomeEqualityWithoutInstallers));
            Manifest otherSomeEqualityWithoutInstaller = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.SomeEqualityWithoutInstallers));
            AssertEquivalence(someEqualityWithoutInstaller, otherSomeEqualityWithoutInstaller);
        }

        /// <summary>
        /// Every manifest is unique in its own way.
        /// </summary>
        [Fact]
        [DisplayTestMethodName]
        public void ManifestNotEquals()
        {
            // All equality properties.
            Manifest allEquality = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.AllEquality));
            Assert.False(allEquality == null);

            Manifest allEqualityOther = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.DifferentId));
            AssertNotEquivalent(allEquality, allEqualityOther);

            Manifest someEquality = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.SomeEquality));
            Manifest withoutSwitches = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.SomeEqualityWithoutSwitches));
            AssertNotEquivalent(someEquality, withoutSwitches);

            Manifest withoutInstaller = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.SomeEqualityWithoutSwitches));
            AssertNotEquivalent(someEquality, withoutInstaller);

            Manifest oneInstaller = Manifest.CreateManifestFromString(ReadFile(ManifestStrings.OneInstaller));
            AssertNotEquivalent(allEquality, oneInstaller);
        }

        private static string ReadFile(string fileName)
        {
            string location = Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location);
            return File.ReadAllText(Path.Combine(location, "TestCollateral", fileName));
        }

        private static void AssertEquivalence(Manifest first, Manifest second)
        {
            Assert.True(first.IsManifestEquivalent(second));
        }

        private static void AssertNotEquivalent(Manifest first, Manifest second)
        {
            Assert.False(first.IsManifestEquivalent(second));
        }

        /// <summary>
        /// Helper class with manifest strings.
        /// </summary>
        internal class ManifestStrings
        {
            /// <summary>
            /// Manifest with all properties that provide equality.
            /// </summary>
            public const string AllEquality = "AllEquality.yaml";

            /// <summary>
            /// Manifest with all properties that provide equality.
            /// </summary>
            public const string AllEqualityWithDescription = "AllEqualityWithDescription.yaml";

            /// <summary>
            /// Manifest with some properties that provide equality.
            /// </summary>
            public const string SomeEquality = "SomeEquality.yaml";

            /// <summary>
            /// Manifest with some properties that provide equality.
            /// </summary>
            public const string SomeEqualityWithLocalization = "SomeEqualityWithLocalization.yaml";

            /// <summary>
            /// Manifest with some properties specifically without switches.
            /// </summary>
            public const string SomeEqualityWithoutSwitches = "SomeEqualityWithoutSwitches.yaml";

            /// <summary>
            /// Manifest with some properties specifically without installers.
            /// </summary>
            public const string SomeEqualityWithoutInstallers = "SomeEqualityWithoutInstallers.yaml";

            /// <summary>
            /// Manifest with all properties that provide equality.
            /// </summary>
            public const string DifferentId = "DifferentId.yaml";

            /// <summary>
            /// Manifest with all properties that provide equality.
            /// </summary>
            public const string OneInstaller = "OneInstaller.yaml";
        }
    }
}
