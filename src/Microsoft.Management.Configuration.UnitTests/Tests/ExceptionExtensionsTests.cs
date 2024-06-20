// -----------------------------------------------------------------------------
// <copyright file="ExceptionExtensionsTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Tests
{
    using System;
    using Microsoft.Management.Configuration.Processor.Extensions;
    using Microsoft.Management.Configuration.UnitTests.Fixtures;
    using Microsoft.Management.Configuration.UnitTests.Helpers;
    using Microsoft.PowerShell.Commands;
    using Xunit;
    using Xunit.Abstractions;

    /// <summary>
    /// Exception extension tests.
    /// </summary>
    [Collection("UnitTestCollection")]
    [InProc]
    public class ExceptionExtensionsTests
    {
        private readonly UnitTestFixture fixture;
        private readonly ITestOutputHelper log;

        /// <summary>
        /// Initializes a new instance of the <see cref="ExceptionExtensionsTests"/> class.
        /// </summary>
        /// <param name="fixture">Unit test fixture.</param>
        /// <param name="log">Log helper.</param>
        public ExceptionExtensionsTests(UnitTestFixture fixture, ITestOutputHelper log)
        {
            this.fixture = fixture;
            this.log = log;
        }

        /// <summary>
        /// Tests GetMostInnerException.
        /// </summary>
        [Fact]
        public void GetMostInnerException_Test()
        {
            var exception = new Exception(
                "message1",
                new WriteErrorException(
                    "WriteException",
                    new ArgumentNullException()));

            var mostInner = exception.GetMostInnerException();
            Assert.IsType<ArgumentNullException>(mostInner);

            var exception2 = new Exception(
                "message2",
                new WriteErrorException(
                    "WriteException2"));

            mostInner = exception2.GetMostInnerException();
            Assert.IsType<WriteErrorException>(mostInner);

            var exception3 = new ArgumentOutOfRangeException("message2");
            mostInner = exception3.GetMostInnerException();
            Assert.IsType<ArgumentOutOfRangeException>(mostInner);
        }
    }
}
