// -----------------------------------------------------------------------------
// <copyright file="ShowCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test show command.
    /// </summary>
    public class ShowCommand : BaseCommand
    {
        /// <summary>
        /// Test show with no args.
        /// </summary>
        [Test]
        public void ShowWithNoArgs()
        {
            var result = TestCommon.RunAICLICommand("show", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.ERROR_INVALID_CL_ARGUMENTS, result.ExitCode);
        }

        /// <summary>
        /// Test show no match.
        /// </summary>
        [Test]
        public void ShowWithNoMatches()
        {
            // Show with 0 search match shows a "please refine input"
            var result = TestCommon.RunAICLICommand("show", $"DoesNotExist");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

        /// <summary>
        /// Test show with substring match.
        /// </summary>
        [Test]
        public void ShowWithSubstringMatch()
        {
            // Show with a substring match still returns 0 results
            var result = TestCommon.RunAICLICommand("show", $"AppInstallerTest");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

        /// <summary>
        /// Test show with name match.
        /// </summary>
        [Test]
        public void ShowWithNameMatch()
        {
            var result = TestCommon.RunAICLICommand("show", $"--name testexampleinstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Found TestExampleInstaller [AppInstallerTest.TestExampleInstaller]"));
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test show with id match.
        /// </summary>
        [Test]
        public void ShowWithIDMatch()
        {
            var result = TestCommon.RunAICLICommand("show", $"--id appinstallertest.testexampleinstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Found TestExampleInstaller [AppInstallerTest.TestExampleInstaller]"));
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test show versions.
        /// </summary>
        [Test]
        public void ShowWithVersions()
        {
            // Show with --versions list the versions
            var result = TestCommon.RunAICLICommand("show", $"TestExampleInstaller --versions");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("1.2.3.4"));
        }

        /// <summary>
        /// Test show with exact match name.
        /// </summary>
        [Test]
        public void ShowWithExactName()
        {
            var result = TestCommon.RunAICLICommand("show", $"--exact TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Found TestExampleInstaller [AppInstallerTest.TestExampleInstaller]"));
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test show with exact id.
        /// </summary>
        [Test]
        public void ShowWithExactID()
        {
            var result = TestCommon.RunAICLICommand("show", $"--exact AppInstallerTest.TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Found TestExampleInstaller [AppInstallerTest.TestExampleInstaller]"));
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test show with exact args.
        /// </summary>
        [Test]
        public void ShowWithExactArgCaseSensitivity()
        {
            var result = TestCommon.RunAICLICommand("show", $"--exact testexampleinstaller");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

        /// <summary>
        /// Test show with installer type.
        /// </summary>
        [Test]
        public void ShowWithInstallerTypeArg()
        {
            var result = TestCommon.RunAICLICommand("show", $"--id AppInstallerTest.TestMultipleInstallers --installer-type msi");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Found TestMultipleInstallers [AppInstallerTest.TestMultipleInstallers]"));
            Assert.True(result.StdOut.Contains("Installer Type: msi"));
        }

        /// <summary>
        /// Test show with an archive installer type.
        /// </summary>
        [Test]
        public void ShowWithZipInstallerTypeArg()
        {
            var result = TestCommon.RunAICLICommand("show", $"--id AppInstallerTest.TestMultipleInstallers --installer-type zip");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("Found TestMultipleInstallers [AppInstallerTest.TestMultipleInstallers]"));
            Assert.True(result.StdOut.Contains("Installer Type: exe (zip)"));
        }
    }
}