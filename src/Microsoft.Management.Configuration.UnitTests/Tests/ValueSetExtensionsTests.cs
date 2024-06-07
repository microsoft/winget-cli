// -----------------------------------------------------------------------------
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
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Windows.Foundation.Collections;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// ValueSet extension tests.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
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

            var expectedList = hashtable[arrayKey] as IList<object>;
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

            var expectedList = hashtable[arrayKey] as IList<object>;
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
            string arrayValue4 = "arrayValue4";
            string arrayValue5 = "arrayValue5";
            string arrayValue6 = "arrayValue6";
            string arrayValue7 = "arrayValue7";
            string arrayValue8 = "arrayValue8";
            string arrayValue9 = "arrayValue9";
            string arrayValue10 = "arrayValue10";
            string arrayValue11 = "arrayValue11";
            string arrayValue12 = "arrayValue12";
            var valueSetArray = new ValueSet()
            {
                { "10", arrayValue10 },
                { "7", arrayValue7 },
                { "2", arrayValue2 },
                { "12", arrayValue12 },
                { "6", arrayValue6 },
                { "treatAsArray", true },
                { "3", arrayValue3 },
                { "1", arrayValue1 },
                { "9", arrayValue9 },
                { "0", arrayValue0 },
                { "4", arrayValue4 },
                { "11", arrayValue11 },
                { "8", arrayValue8 },
                { "5", arrayValue5 },
            };

            var result = valueSetArray.ToArray();
            Assert.NotNull(result);
            Assert.Equal(valueSetArray.Count - 1, result.Count);
            Assert.Equal(arrayValue0, result[0]);
            Assert.Equal(arrayValue1, result[1]);
            Assert.Equal(arrayValue2, result[2]);
            Assert.Equal(arrayValue3, result[3]);
            Assert.Equal(arrayValue4, result[4]);
            Assert.Equal(arrayValue5, result[5]);
            Assert.Equal(arrayValue6, result[6]);
            Assert.Equal(arrayValue7, result[7]);
            Assert.Equal(arrayValue8, result[8]);
            Assert.Equal(arrayValue9, result[9]);
            Assert.Equal(arrayValue10, result[10]);
            Assert.Equal(arrayValue11, result[11]);
            Assert.Equal(arrayValue12, result[12]);
        }

        /// <summary>
        /// Tests ConvertValueSetToArray.
        /// </summary>
        [Fact]
        public void ValueSet_InvalidArray()
        {
            string arrayValue0 = "arrayValue0";
            string arrayValue1 = "arrayValue1";
            string arrayValue2 = "arrayValue2";
            string arrayValue3 = "arrayValue3";
            string arrayValue4 = "arrayValue4";
            string arrayValue5 = "arrayValue5";
            string arrayValue6 = "arrayValue6";
            string arrayValue7 = "arrayValue7";
            string arrayValue8 = "arrayValue8";
            string arrayValue9 = "arrayValue9";
            string arrayValue10 = "arrayValue10";
            string arrayValue11 = "arrayValue11";
            string arrayValue12 = "arrayValue12";
            var valueSetArray = new ValueSet()
            {
                { "10", arrayValue10 },
                { "7", arrayValue7 },
                { "2", arrayValue2 },
                { "12", arrayValue12 },
                { "6", arrayValue6 },
                { "3", arrayValue3 },
                { "1", arrayValue1 },
                { "9", arrayValue9 },
                { "0", arrayValue0 },
                { "4", arrayValue4 },
                { "11", arrayValue11 },
                { "8", arrayValue8 },
                { "5", arrayValue5 },
            };

            Assert.Throws<InvalidOperationException>(() => valueSetArray.ToArray());
        }

        /// <summary>
        /// Tests ConvertValueSetToArray.
        /// </summary>
        [Fact]
        public void ValueSet_InvalidArrayKey()
        {
            string arrayValue0 = "arrayValue0";
            string arrayValue1 = "arrayValue1";
            string arrayValue2 = "arrayValue2";
            string arrayValue3 = "arrayValue3";
            string arrayValue4 = "arrayValue4";
            string arrayValue5 = "arrayValue5";
            string arrayValue6 = "arrayValue6";
            string arrayValue7 = "arrayValue7";
            string arrayValue8 = "arrayValue8";
            string arrayValue9 = "arrayValue9";
            string arrayValue10 = "arrayValue10";
            string arrayValue11 = "arrayValue11";
            string arrayValue12 = "arrayValue12";
            var valueSetArray = new ValueSet()
            {
                { "10", arrayValue10 },
                { "7", arrayValue7 },
                { "2", arrayValue2 },
                { "a", arrayValue12 },
                { "6", arrayValue6 },
                { "3", arrayValue3 },
                { "1", arrayValue1 },
                { "9", arrayValue9 },
                { "0", arrayValue0 },
                { "4", arrayValue4 },
                { "11", arrayValue11 },
                { "8", arrayValue8 },
                { "5", arrayValue5 },
            };

            Assert.Throws<InvalidOperationException>(() => valueSetArray.ToArray());
        }

        /// <summary>
        /// Tests ValueSet simple types content equals.
        /// </summary>
        [Fact]
        public void ValueSet_SimpleTypes_ContentEquals()
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

            // Same content different order
            var valueSetDifferentOrder = new ValueSet
            {
                { boolProperty, boolPropertyValue },
                { stringProperty, stringPropertyValue },
                { intProperty, intPropertyValue },
            };

            Assert.True(valueSet.ContentEquals(valueSetDifferentOrder));

            // Entry missing
            var valueSetEntryMissing = new ValueSet
            {
                { stringProperty, stringPropertyValue },
                { intProperty, intPropertyValue },
            };

            Assert.False(valueSet.ContentEquals(valueSetEntryMissing));

            // Different entry
            var valueSetEntryDifferent = new ValueSet
            {
                { stringProperty, stringPropertyValue },
                { intProperty, intPropertyValue },
                { "Another", "AnotherValue" },
            };

            Assert.False(valueSet.ContentEquals(valueSetEntryDifferent));

            // Different value
            var valueSetDifferentValue = new ValueSet
            {
                { boolProperty, boolPropertyValue },
                { stringProperty, stringPropertyValue },
                { intProperty, 0 },
            };

            Assert.False(valueSet.ContentEquals(valueSetDifferentValue));
        }

        /// <summary>
        /// Tests when a ValueSet has inner value sets.
        /// </summary>
        [Fact]
        public void ValueSet_NestedValueSets_ContentEquals()
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

            // Same content different order
            var valueSetDifferentOrder = new ValueSet()
            {
                { key2, valueSetInner2 },
                { key1, valueSetInner },
            };

            Assert.True(valueSet.ContentEquals(valueSetDifferentOrder));

            // Different nested content
            var valueSetDifferentContent = new ValueSet()
            {
                { key2, valueSetInner },
                { key1, valueSetInner2 },
            };

            Assert.False(valueSet.ContentEquals(valueSetDifferentContent));
        }
    }
}
