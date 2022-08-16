// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;

    public class SearchCommand : BaseCommand
    {
        [Test]
        public void SearchWithoutArgs()
        {
            var result = TestCommon.RunAICLICommand("search", "");
            Assert.AreEqual(Constants.ErrorCode.ERROR_INVALID_CL_ARGUMENTS, result.ExitCode);
        }

        [Test]
        public void SearchQuery()
        {
            var result = TestCommon.RunAICLICommand("search", "TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        public void SearchUsingAlias()
        {
            var result = TestCommon.RunAICLICommand("find", "TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void SearchWithName()
        {
            var result = TestCommon.RunAICLICommand("search", "--name testexampleinstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void SearchWithID()
        {
            var result = TestCommon.RunAICLICommand("search", "--id appinstallertest.testexampleinstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void SearchWithInvalidName()
        {
            var result = TestCommon.RunAICLICommand("search", "--name InvalidName");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

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

        [Test]
        public void SearchWithExactName()
        {
            var result = TestCommon.RunAICLICommand("search", "--exact TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void SearchWithExactID()
        {
            var result = TestCommon.RunAICLICommand("search", "--exact AppInstallerTest.TestExampleInstaller");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("TestExampleInstaller"));
            Assert.True(result.StdOut.Contains("AppInstallerTest.TestExampleInstaller"));
        }

        [Test]
        public void SearchWithExactArgCaseSensitivity()
        {
            var result = TestCommon.RunAICLICommand("search", "--exact testexampleinstaller");
            Assert.AreEqual(Constants.ErrorCode.ERROR_NO_APPLICATIONS_FOUND, result.ExitCode);
            Assert.True(result.StdOut.Contains("No package found matching input criteria."));
        }

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
                ResetTestSource();
            }
        }

        [Test]
        public void SearchStoreWithBadPin()
        {
            // Configure as close as possible to the real chain but use the test cert for everything
            // This will at least force the public key to be checked rather than simply failing based on chain length
            GroupPolicyHelper.EnableAdditionalSources.SetEnabledList(new GroupPolicySource[]
            {
                    new GroupPolicySource
                    {
                        Name = Constants.TestAlternateSourceName,
                        Arg = Constants.DefaultMSStoreSourceUrl,
                        Type = Constants.DefaultMSStoreSourceType,
                        Data = "",
                        Identifier = Constants.DefaultMSStoreSourceIdentifier,
                        CertificatePinning = new GroupPolicyCertificatePinning
                        {
                            Chains = new GroupPolicyCertificatePinningChain[] {
                                new GroupPolicyCertificatePinningChain
                                {
                                    Chain = new GroupPolicyCertificatePinningDetails[]
                                    {
                                        new GroupPolicyCertificatePinningDetails
                                        {
                                            Validation = new string[] { "publickey" },
                                            EmbeddedCertificate = TestCommon.GetTestServerCertificateHexString()
                                        },
                                        new GroupPolicyCertificatePinningDetails
                                        {
                                            Validation = new string[] { "subject", "issuer" },
                                            EmbeddedCertificate = TestCommon.GetTestServerCertificateHexString()
                                        },
                                        new GroupPolicyCertificatePinningDetails
                                        {
                                            Validation = new string[] { "subject", "issuer" },
                                            EmbeddedCertificate = TestCommon.GetTestServerCertificateHexString()
                                        }
                                    }
                                }
                            }
                        }
                    }
            });

            try
            {
                var result = TestCommon.RunAICLICommand("search", $"-s {Constants.TestAlternateSourceName} foo --verbose-logs");
                Assert.AreEqual(Constants.ErrorCode.ERROR_PINNED_CERTIFICATE_MISMATCH, result.ExitCode);
            }
            finally
            {
                ResetTestSource();
            }
        }
    }
}
