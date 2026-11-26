// -----------------------------------------------------------------------------
// <copyright file="PipedEncodingTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using System.Security;
    using System.Text;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test the encoding of human-readable output that is redirected to a pipe.
    /// </summary>
    public class PipedEncodingTests : BaseCommand
    {
        // The id of the test package contains a character that code page 866 and UTF-8 encode
        // differently, so decoding the output with the wrong code page is visible in the text.
        private const string TestPackageId = "AppInstallerTest.TëstExeInstaller";

        // Code page 866 has no character for ë, so the conversion writes the closest equivalent
        // that the code page has for it. The UTF-8 test asserts the original id instead.
        private const string ConsoleTestPackageId = "AppInstallerTest.TestExeInstaller";

        // The console output code page of Russian and Ukrainian systems, where the garbled output
        // that the conversion addresses was reported.
        private const int ConsoleCodePage = 866;

        private static readonly Encoding ConsoleEncoding = CodePagesEncodingProvider.Instance.GetEncoding(ConsoleCodePage);

        private Encoding previousOutputEncoding;

        /// <summary>
        /// Sets the console output code page that the test process and its children use.
        /// </summary>
        [SetUp]
        public void SetUp()
        {
            this.previousOutputEncoding = null;

            try
            {
                this.previousOutputEncoding = Console.OutputEncoding;
                Console.OutputEncoding = ConsoleEncoding;
            }
            catch (IOException)
            {
                Assert.Ignore("There is no console to change the output code page of.");
            }
            catch (SecurityException)
            {
                Assert.Ignore("The console output code page cannot be changed.");
            }
        }

        /// <summary>
        /// Restores the console output code page.
        /// </summary>
        [TearDown]
        public void TearDown()
        {
            if (this.previousOutputEncoding != null)
            {
                Console.OutputEncoding = this.previousOutputEncoding;
            }
        }

        /// <summary>
        /// Test that the default setting writes the output in the code page that the console host
        /// decodes it with.
        /// </summary>
        [Test]
        public void PipedOutputUsesConsoleCodePage()
        {
            var result = TestCommon.RunAICLICommand("show", TestPackageId, standardOutputEncoding: ConsoleEncoding);

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain(ConsoleTestPackageId));
        }

        /// <summary>
        /// Test that the argument writes UTF-8 for one invocation, whatever the console code page is.
        /// </summary>
        [Test]
        public void PipedEncodingArgumentWritesUtf8()
        {
            var result = TestCommon.RunAICLICommand("show", $"{TestPackageId} --piped-encoding utf8", standardOutputEncoding: Encoding.UTF8);

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain(TestPackageId));
        }

        /// <summary>
        /// Test that the argument refuses a value that is not one of the encodings it accepts.
        /// </summary>
        [Test]
        public void PipedEncodingArgumentRejectsUnknownValue()
        {
            var result = TestCommon.RunAICLICommand("show", $"{TestPackageId} --piped-encoding utf-16");

            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_INVALID_CL_ARGUMENTS));
        }
    }
}
