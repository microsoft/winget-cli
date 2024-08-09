// -----------------------------------------------------------------------------
// <copyright file="SearchCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Test search command.
    /// </summary>
    public class SearchCommand : BaseCommand
    {
        /// <summary>
        /// Test search without args.
        /// </summary>
        [Test]
        public void SearchWithoutArgs()
        {
            var result = TestCommon.RunAICLICommand("search", string.Empty);
            Assert.AreEqual(Constants.ErrorCode.ERROR_INVALID_CL_ARGUMENTS, result.ExitCode);
        }

        /// <summary>
        /// Test search with query.
        /// </summary>
        [Test]
        public void SearchQuery()
        {
            var result = TestCommon.RunAICLICommand("search", "TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test search with alias.
        /// </summary>
        public void SearchUsingAlias()
        {
            var result = TestCommon.RunAICLICommand("find", "TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test search with name.
        /// </summary>
        [Test]
        public void SearchWithName()
        {
            var result = TestCommon.RunAICLICommand("search", "--name testexampleinstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test search with Id.
        /// </summary>
        [Test]
        public void SearchWithID()
        {
            var result = TestCommon.RunAICLICommand("search", "--id appinstallertest.testexampleinstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test search with invalid name.
        /// </summary>
        [Test]
        public void SearchWithInvalidName()
        {
            var result = TestCommon.RunAICLICommand("search", "--name InvalidName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

        /// <summary>
        /// Test search where it returns multiple results.
        /// </summary>
        [Test]
        public void SearchReturnsMultiple()
        {
            // Search Microsoft should return multiple
            var result = TestCommon.RunAICLICommand("search", "AppInstallerTest");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExeInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestBurnInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test search with exact name.
        /// </summary>
        [Test]
        public void SearchWithExactName()
        {
            var result = TestCommon.RunAICLICommand("search", "--exact TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test search with exact ID.
        /// </summary>
        [Test]
        public void SearchWithExactID()
        {
            var result = TestCommon.RunAICLICommand("search", "--exact AppInstallerTest.TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        /// <summary>
        /// Test search with exact case-sensitive.
        /// </summary>
        [Test]
        public void SearchWithExactArgCaseSensitivity()
        {
            var result = TestCommon.RunAICLICommand("search", "--exact testexampleinstaller");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

        /// <summary>
        /// Test search with a failed source.
        /// </summary>
        [Test]
        public void SearchWithSingleSourceFailure()
        {
            TestCommon.RunAICLICommand("source add", "failSearch \"{ \"\"OpenHR\"\": \"\"0x80070002\"\" }\" Microsoft.Test.Configurable --header \"{}\"");

            try
            {
                var result = TestCommon.RunAICLICommand("search", "--exact AppInstallerTest.TestExampleInstaller");
                Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
                Assert.True(result.StdOut.Contains("Failed when searching source; results will not be included: failSearch"));
                Assert.True(result.StdOut.Contains("TestExampleInstaller"));
                Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
            }
            finally
            {
                this.ResetTestSource();
            }
        }

        /// <summary>
        /// Test search with bad pin.
        /// </summary>
        [Test]
        public void SearchStoreWithBadPin()
        {
            // Configure as close as possible to the real chain but use the test cert for everything
            // This will at least force the public key to be checked rather than simply failing based on chain length
            GroupPolicyHelper.EnableAdditionalSources.SetEnabledList(new GroupPolicyHelper.GroupPolicySource[]
            {
                    new GroupPolicyHelper.GroupPolicySource
                    {
                        Name = Constants.TestAlternateSourceName,
                        Arg = Constants.DefaultMSStoreSourceUrl,
                        Type = Constants.DefaultMSStoreSourceType,
                        Data = string.Empty,
                        Identifier = Constants.DefaultMSStoreSourceIdentifier,
                        CertificatePinning = new GroupPolicyHelper.GroupPolicyCertificatePinning
                        {
                            Chains = new GroupPolicyHelper.GroupPolicyCertificatePinningChain[]
                            {
                                new GroupPolicyHelper.GroupPolicyCertificatePinningChain
                                {
                                    Chain = new GroupPolicyHelper.GroupPolicyCertificatePinningDetails[]
                                    {
                                        new GroupPolicyHelper.GroupPolicyCertificatePinningDetails
                                        {
                                            Validation = new string[] { "publickey" },
                                            EmbeddedCertificate = TestCommon.GetTestServerCertificateHexString(),
                                        },
                                        new GroupPolicyHelper.GroupPolicyCertificatePinningDetails
                                        {
                                            Validation = new string[] { "subject", "issuer" },
                                            EmbeddedCertificate = TestCommon.GetTestServerCertificateHexString(),
                                        },
                                        new GroupPolicyHelper.GroupPolicyCertificatePinningDetails
                                        {
                                            Validation = new string[] { "subject", "issuer" },
                                            EmbeddedCertificate = TestCommon.GetTestServerCertificateHexString(),
                                        },
                                    },
                                },
                            },
                        },
                        TrustLevel = new string[] { "None" },
                        Explicit = false,
                    },
            });

            try
            {
                var result = TestCommon.RunAICLICommand("search", $"-s {Constants.TestAlternateSourceName} foo --verbose-logs");
                Assert.AreEqual(Constants.ErrorCode.ERROR_PINNED_CERTIFICATE_MISMATCH, result.ExitCode);
            }
            finally
            {
                this.ResetTestSource();
            }
        }
    }
}
