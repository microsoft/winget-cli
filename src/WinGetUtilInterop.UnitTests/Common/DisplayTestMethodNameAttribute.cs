// -----------------------------------------------------------------------------
// <copyright file="DisplayTestMethodNameAttribute.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGetUtil.UnitTests.Common.Logging
{
    using System;
    using System.Reflection;
    using Microsoft.Msix.Utils.Logger;
    using Xunit.Sdk;

    /// <summary>
    /// This will log the name the method in OWC utils logger.
    /// </summary>
    internal class DisplayTestMethodNameAttribute : BeforeAfterTestAttribute
    {
        /// <inheritdoc/>
        public override void Before(MethodInfo methodUnderTest)
        {
            Logger.Info("-----------------------------------------------------------------------");
            Logger.Info($"Starting test {methodUnderTest.Name}{Environment.NewLine}");
        }

        /// <inheritdoc/>
        public override void After(MethodInfo methodUnderTest)
        {
            Logger.Info($"{Environment.NewLine}Finish test {methodUnderTest.Name}");
            Logger.Info("-----------------------------------------------------------------------\n");
        }
    }
}
