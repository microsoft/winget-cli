// -----------------------------------------------------------------------------
// <copyright file="UnitTestFixture.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Fixtures
{
    using Xunit.Abstractions;

    /// <summary>
    /// Unit test fixture.
    /// </summary>
    public class UnitTestFixture
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="UnitTestFixture"/> class.
        /// </summary>
        /// <param name="messageSink">The message sink for the fixture.</param>
        public UnitTestFixture(IMessageSink messageSink)
        {
            this.MessageSink = messageSink;
        }

        /// <summary>
        /// Gets the message sink for the fixture.
        /// </summary>
        public IMessageSink MessageSink { get; private set; }
    }
}
