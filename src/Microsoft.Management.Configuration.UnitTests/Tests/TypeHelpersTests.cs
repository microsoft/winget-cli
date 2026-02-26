// -----------------------------------------------------------------------------
// <copyright file="TypeHelpersTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System.Collections;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.Processor.Helpers;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Windows.Foundation.Collections;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// TypeHelpers tests.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class TypeHelpersTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="TypeHelpersTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public TypeHelpersTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        private enum TestEnum
        {
            Value,
        }

        /// <summary>
        /// Tests PropertyExists.
        /// </summary>
        [Fact]
        public void PropertyExistsTests()
        {
            dynamic obj = new
            {
                Property1 = "value",
            };

            Assert.True(TypeHelpers.PropertyExists(obj, "Property1"));
            Assert.False(TypeHelpers.PropertyExists(obj, "Property2"));
        }

        /// <summary>
        /// Tests PropertyWithTypeExists.
        /// </summary>
        [Fact]
        public void PropertyWithTypeExistsTests()
        {
            dynamic obj = new
            {
                Property1 = "value",
                Property2 = 42,
            };

            Assert.True(TypeHelpers.PropertyWithTypeExists<string>(obj, "Property1"));
            Assert.True(TypeHelpers.PropertyWithTypeExists<int>(obj, "Property2"));

            Assert.False(TypeHelpers.PropertyWithTypeExists<int>(obj, "Property1"));
            Assert.False(TypeHelpers.PropertyWithTypeExists<string>(obj, "Property2"));
        }

        /// <summary>
        /// Test PropertyExistsAndIsEnum.
        /// </summary>
        [Fact]
        public void PropertyExistsAndIsEnumTests()
        {
            dynamic obj = new
            {
                Property1 = TestEnum.Value,
                Property2 = "string",
            };

            Assert.True(TypeHelpers.PropertyExistsAndIsEnum(obj, "Property1"));
            Assert.False(TypeHelpers.PropertyExistsAndIsEnum(obj, "Property2"));
        }

        /// <summary>
        /// Tests PropertyExistsAndIsList.
        /// </summary>
        [Fact]
        public void PropertyExistsAndIsListTests()
        {
            dynamic obj = new
            {
                Property1 = new List<string>() { "value" },
                Property2 = "string",
            };

            Assert.True(TypeHelpers.PropertyExistsAndIsList(obj, "Property1"));
            Assert.False(TypeHelpers.PropertyExistsAndIsList(obj, "Property2"));
        }

        /// <summary>
        /// Tests GetAllPropertiesValues.
        /// </summary>
        [Fact]
        public void GetAllPropertiesValuesTests()
        {
            string s = "value";
            int i = 42;
            TestEnum e = TestEnum.Value;
            dynamic obj = new
            {
                Property1 = s,
                Property2 = i,
                Property3 = e,
            };

            ValueSet set = TypeHelpers.GetAllPropertiesValues(obj);
            Assert.Equal(3, set.Count);

            Assert.True(set.ContainsKey("Property1"));
            Assert.True(set.TryGetValue("Property1", out object v1));
            Assert.Equal(s, v1 as string);

            Assert.True(set.ContainsKey("Property2"));
            Assert.True(set.TryGetValue("Property2", out object v2));
            Assert.Equal(i, (int)v2);

            Assert.True(set.ContainsKey("Property3"));
            Assert.True(set.TryGetValue("Property3", out object v3));
            Assert.Equal(e.ToString(), v3);
        }

        /// <summary>
        /// Verifies when a property is a Hashtable. It must be converted to a ValueSet.
        /// </summary>
        [Fact]
        public void GetAllPropertiesValuesTest_Hashtable()
        {
            string k1 = "key1";
            string k2 = "key2";
            int v1 = 7;
            string v2 = "value2";
            dynamic obj = new
            {
                Property1 = new Hashtable
                {
                    { k1, v1 },
                    { k2, v2 },
                },
            };

            ValueSet set = TypeHelpers.GetAllPropertiesValues(obj);
            Assert.Single(set);

            Assert.True(set.ContainsKey("Property1"));
            Assert.True(set.TryGetValue("Property1", out object valueSetResultObj));

            ValueSet? valueSetResult = valueSetResultObj as ValueSet;
            Assert.NotNull(valueSetResult);
            Assert.Equal(2, valueSetResult.Count);
            Assert.True(valueSetResult.ContainsKey(k1));
            Assert.Equal(v1, (int)valueSetResult[k1]);
            Assert.True(valueSetResult.ContainsKey(k2));
            Assert.Equal(v2, (string)valueSetResult[k2]);
        }

        /// <summary>
        /// Verifies when a property is an array. It must generate a ValueSet
        /// where the keys are the index and a key treatAsArray means the value
        /// must be treated an array.
        /// </summary>
        [Fact]
        public void GetAllPropertiesValuesTest_Array()
        {
            dynamic obj = new
            {
                Property1 = new int[]
                {
                    1,
                    2,
                    3,
                    4,
                },
            };

            ValueSet set = TypeHelpers.GetAllPropertiesValues(obj);
            Assert.Single(set);

            Assert.True(set.ContainsKey("Property1"));
            Assert.True(set.TryGetValue("Property1", out object valueSetResultObj));

            ValueSet? valueSetResult = valueSetResultObj as ValueSet;
            Assert.NotNull(valueSetResult);
            Assert.Equal(5, valueSetResult.Count);

            Assert.True(valueSetResult.ContainsKey("treatAsArray"));
            Assert.True(valueSetResult.ContainsKey("0"));
            Assert.True(valueSetResult.ContainsKey("1"));
            Assert.True(valueSetResult.ContainsKey("2"));
            Assert.True(valueSetResult.ContainsKey("3"));
        }
    }
}
