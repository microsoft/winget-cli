// -----------------------------------------------------------------------------
// <copyright file="HashtableExtensionsTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System.Collections;
    using Microsoft.Management.Configuration.Processor.Exceptions;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Windows.Foundation.Collections;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Hashtable extension tests.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class HashtableExtensionsTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="HashtableExtensionsTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public HashtableExtensionsTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Tests ToValueSet with simple types.
        /// </summary>
        [Fact]
        public void ToValueSet_Test()
        {
            var ht = new Hashtable()
            {
                { "key1", "value1" },
                { "key2", 2 },
                { "key3", true },
            };

            var valueSet = ht.ToValueSet();

            Assert.True(valueSet.ContainsKey("key1"));
            Assert.Equal("value1", (string)valueSet["key1"]);

            Assert.True(valueSet.ContainsKey("key2"));
            Assert.Equal(2, (int)valueSet["key2"]);

            Assert.True(valueSet.ContainsKey("key3"));
            Assert.True((bool)valueSet["key3"]);
        }

        /// <summary>
        /// Test for inner hashtables.
        /// </summary>
        [Fact]
        public void ToValueSet_InnerHashtable()
        {
            var ht = new Hashtable()
            {
                {
                    "hashtableKey", new Hashtable()
                    {
                        { "key1", "value1" },
                        { "key2", 2 },
                        { "key3", true },
                    }
                },
            };

            var valueSet = ht.ToValueSet();

            Assert.True(valueSet.ContainsKey("hashtableKey"));
            var resultValueSet = (ValueSet)valueSet["hashtableKey"];

            Assert.True(resultValueSet.ContainsKey("key1"));
            Assert.Equal("value1", (string)resultValueSet["key1"]);

            Assert.True(resultValueSet.ContainsKey("key2"));
            Assert.Equal(2, (int)resultValueSet["key2"]);

            Assert.True(resultValueSet.ContainsKey("key3"));
            Assert.True((bool)resultValueSet["key3"]);
        }

        /// <summary>
        /// Test for inner arrays.
        /// </summary>
        [Fact]
        public void ToValueSet_InnerArray()
        {
            var ht = new Hashtable()
            {
                {
                    "arrayKey", new string[]
                    {
                        "s1",
                        "s2",
                        "s3",
                    }
                },
            };

            var valueSet = ht.ToValueSet();

            Assert.True(valueSet.ContainsKey("arrayKey"));
            var resultValueSet = (ValueSet)valueSet["arrayKey"];
            Assert.True(resultValueSet.ContainsKey("treatAsArray"));
            Assert.Equal(4, resultValueSet.Count);
        }

        /// <summary>
        /// Test when a key is not a string.
        /// </summary>
        [Fact]
        public void ToValueSet_KeyNotString()
        {
            var ht = new Hashtable()
            {
                { 1, "value" },
            };

            Assert.Throws<UnitPropertyUnsupportedException>(() => ht.ToValueSet());
        }
    }
}
