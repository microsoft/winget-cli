﻿// -----------------------------------------------------------------------------
// <copyright file="ValueSetExtensionsTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Windows.Foundation.Collections;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// ValueSet extension tests.
    /// </summary>
    [Collection("UnitTestCollection")]
    public class ValueSetExtensionsTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ValueSetExtensionsTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ValueSetExtensionsTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Tests PropertyExists.
        /// </summary>
        [Fact]
        public void ValueSet_SimpleTypes()
        {
            string stringProperty = "stringProperty";
            string stringPropertyValue = "string";

            string intProperty = "intProperty";
            long intPropertyValue = 64;

            string boolProperty = "boolProperty";
            bool boolPropertyValue = true;

            var valueSet = new ValueSet
            {
                { stringProperty, stringPropertyValue },
                { intProperty, intPropertyValue },
                { boolProperty, boolPropertyValue },
            };

            var resultHashtable = valueSet.ToHashtable();

            Assert.NotNull(resultHashtable);

            Assert.True(resultHashtable.ContainsKey(stringProperty));
            Assert.Equal(stringPropertyValue, resultHashtable[stringProperty]);

            Assert.True(resultHashtable.ContainsKey(intProperty));
            Assert.Equal(intPropertyValue, resultHashtable[intProperty]);

            Assert.True(resultHashtable.ContainsKey(boolProperty));
            Assert.Equal(boolPropertyValue, resultHashtable[boolProperty]);
        }

        /// <summary>
        /// Tests when a ValueSet has inner value sets.
        /// </summary>
        [Fact]
        public void ValueSet_NestedValueSets()
        {
            string boolPropertyInner = "boolPropertyInner";
            bool boolPropertyValueInner = true;
            var valueSetInner = new ValueSet()
            {
                { boolPropertyInner, boolPropertyValueInner },
            };

            string stringPropertyInnerInner = "stringPropertyInnerInner";
            string stringPropertyValueInnerInner = "stringInnerInner";
            var valueSetInnerInner = new ValueSet()
            {
                { stringPropertyInnerInner, stringPropertyValueInnerInner },
            };

            string inner2Key = "InnerKey2";
            var valueSetInner2 = new ValueSet()
            {
                { inner2Key, valueSetInnerInner },
            };

            string key1 = "key1";
            string key2 = "key2";
            var valueSet = new ValueSet()
            {
                { key1, valueSetInner },
                { key2, valueSetInner2 },
            };

            var hashtable = valueSet.ToHashtable();
            Assert.NotNull(hashtable);
            Assert.Equal(2, hashtable.Count);
            Assert.True(hashtable.ContainsKey(key1));
            Assert.True(hashtable.ContainsKey(key2));

            var key1Result = hashtable[key1] as Hashtable;
            Assert.NotNull(key1Result);
            Assert.Single(key1Result);
            Assert.True(key1Result.ContainsKey(boolPropertyInner));
            Assert.Equal(boolPropertyValueInner, key1Result[boolPropertyInner]);

            var key2Result = hashtable[key2] as Hashtable;
            Assert.NotNull(key2Result);
            Assert.Single(key2Result);
            Assert.True(key2Result.ContainsKey(inner2Key));
            var key2ResultInner = key2Result[inner2Key] as Hashtable;
            Assert.NotNull(key2ResultInner);
            Assert.True(key2ResultInner.ContainsKey(stringPropertyInnerInner));
            Assert.Equal(stringPropertyValueInnerInner, key2ResultInner[stringPropertyInnerInner]);
        }

        /// <summary>
        /// Test when ValueSet contains a ValueSet that is threated as an.
        /// </summary>
        [Fact]
        public void ValueSet_ArraySimpleTypes()
        {
            var valueSetArray = new ValueSet()
            {
                { "treatAsArray", true },
                { "0", "value1" },
                { "1", "value1" },
                { "2", "value1" },
                { "3", "value1" },
            };

            string arrayKey = "arrayKey";
            var valueSet = new ValueSet()
            {
                { arrayKey, valueSetArray },
            };

            var hashtable = valueSet.ToHashtable();
            Assert.NotNull(hashtable);
            Assert.True(hashtable.ContainsKey(arrayKey));

            var expectedList = hashtable[arrayKey] as List<object>;
            Assert.NotNull(expectedList);
            Assert.Equal(4, expectedList.Count);
            foreach (var element in expectedList)
            {
                Assert.IsType<string>(element);
            }
        }

        /// <summary>
        /// Test ValueSet with an array of value sets.
        /// </summary>
        [Fact]
        public void ValueSet_ArrayHashtable()
        {
            var arrayValue1 = new ValueSet()
            {
                { "key11", "value11" },
                { "key12", "value12" },
            };

            var arrayValue2 = new ValueSet()
            {
                { "key21", "value21" },
            };

            var valueSetArray = new ValueSet()
            {
                { "treatAsArray", true },
                { "0", arrayValue1 },
                { "1", arrayValue2 },
            };

            string arrayKey = "arrayKey";
            var valueSet = new ValueSet()
            {
                { arrayKey, valueSetArray },
            };

            var hashtable = valueSet.ToHashtable();
            Assert.NotNull(hashtable);
            Assert.True(hashtable.ContainsKey(arrayKey));

            var expectedList = hashtable[arrayKey] as List<object>;
            Assert.NotNull(expectedList);
            Assert.Equal(2, expectedList.Count);

            var resultValue1 = expectedList[0] as Hashtable;
            Assert.NotNull(resultValue1);
            Assert.Equal(2, resultValue1.Count);

            var resultValue2 = expectedList[1] as Hashtable;
            Assert.NotNull(resultValue2);
            Assert.Single(resultValue2);
        }

        /// <summary>
        /// Tests ConvertValueSetToArray.
        /// </summary>
        [Fact]
        public void ValueSet_ArrayOrder()
        {
            string arrayValue0 = "arrayValue0";
            string arrayValue1 = "arrayValue1";
            string arrayValue2 = "arrayValue2";
            string arrayValue3 = "arrayValue3";
            var valueSetArray = new ValueSet()
            {
                { "3", arrayValue3 },
                { "treatAsArray", true },
                { "1", arrayValue1 },
                { "2", arrayValue2 },
                { "0", arrayValue0 },
            };

            var result = valueSetArray.ToArray();
            Assert.NotNull(result);
            Assert.Equal(4, result.Count);
            Assert.Equal(arrayValue0, result[0]);
            Assert.Equal(arrayValue1, result[1]);
            Assert.Equal(arrayValue2, result[2]);
            Assert.Equal(arrayValue3, result[3]);
        }
    }
}
