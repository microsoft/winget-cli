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
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_INVALID_CL_ARGUMENTS));
        }

        /// <summary>
        /// Test show no match.
        /// </summary>
        [Test]
        public void ShowWithNoMatches()
        {
            // Show with 0 search match shows a "please refine input"
            var result = TestCommon.RunAICLICommand("show", $"DoesNotExist");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));
            Assert.That(result.StdOut, Does.Contain("No package found matching input criteria."));
        }

        /// <summary>
        /// Test show with substring match.
        /// </summary>
        [Test]
        public void ShowWithSubstringMatch()
        {
            // Show with a substring match still returns 0 results
            var result = TestCommon.RunAICLICommand("show", $"AppInstallerTest");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));
            Assert.That(result.StdOut, Does.Contain("No package found matching input criteria."));
        }

        /// <summary>
        /// Test show with name match.
        /// </summary>
        [Test]
        public void ShowWithNameMatch()
        {
            var result = TestCommon.RunAICLICommand("show", $"--name testexampleinstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Found TestExampleInstaller [AppInstallerTest.TestExampleInstaller]"));
            Assert.That(result.StdOut, Does.Contain("TestExampleInstaller"));
            Assert.That(result.StdOut, Does.Contain("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test show with id match.
        /// </summary>
        [Test]
        public void ShowWithIDMatch()
        {
            var result = TestCommon.RunAICLICommand("show", $"--id appinstallertest.testexampleinstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Found TestExampleInstaller [AppInstallerTest.TestExampleInstaller]"));
            Assert.That(result.StdOut, Does.Contain("TestExampleInstaller"));
            Assert.That(result.StdOut, Does.Contain("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test show versions.
        /// </summary>
        [Test]
        public void ShowWithVersions()
        {
            // Show with --versions list the versions
            var result = TestCommon.RunAICLICommand("show", $"TestExampleInstaller --versions");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("TestExampleInstaller"));
            Assert.That(result.StdOut, Does.Contain("AppInstallerTest.TestExampleInstaller"));
            Assert.That(result.StdOut, Does.Contain("1.2.3.4"));
        }

        /// <summary>
        /// Test show with exact match name.
        /// </summary>
        [Test]
        public void ShowWithExactName()
        {
            var result = TestCommon.RunAICLICommand("show", $"--exact TestExampleInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Found TestExampleInstaller [AppInstallerTest.TestExampleInstaller]"));
            Assert.That(result.StdOut, Does.Contain("TestExampleInstaller"));
            Assert.That(result.StdOut, Does.Contain("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test show with exact id.
        /// </summary>
        [Test]
        public void ShowWithExactID()
        {
            var result = TestCommon.RunAICLICommand("show", $"--exact AppInstallerTest.TestExampleInstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Found TestExampleInstaller [AppInstallerTest.TestExampleInstaller]"));
            Assert.That(result.StdOut, Does.Contain("TestExampleInstaller"));
            Assert.That(result.StdOut, Does.Contain("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test show with exact args.
        /// </summary>
        [Test]
        public void ShowWithExactArgCaseSensitivity()
        {
            var result = TestCommon.RunAICLICommand("show", $"--exact testexampleinstaller");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND));
            Assert.That(result.StdOut, Does.Contain("No package found matching input criteria."));
        }

        /// <summary>
        /// Test show with installer type.
        /// </summary>
        [Test]
        public void ShowWithInstallerTypeArg()
        {
            var result = TestCommon.RunAICLICommand("show", $"--id AppInstallerTest.TestMultipleInstallers --installer-type msi");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Found TestMultipleInstallers [AppInstallerTest.TestMultipleInstallers]"));
            Assert.That(result.StdOut, Does.Contain("Installer Type: msi"));
        }

        /// <summary>
        /// Test show with an archive installer type.
        /// </summary>
        [Test]
        public void ShowWithZipInstallerTypeArg()
        {
            var result = TestCommon.RunAICLICommand("show", $"--id AppInstallerTest.TestMultipleInstallers --installer-type zip");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK));
            Assert.That(result.StdOut, Does.Contain("Found TestMultipleInstallers [AppInstallerTest.TestMultipleInstallers]"));
            Assert.That(result.StdOut, Does.Contain("Installer Type: exe (zip)"));
        }
    }
}