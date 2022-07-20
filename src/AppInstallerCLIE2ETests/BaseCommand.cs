// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using System.Threading;
    using Newtonsoft.Json.Linq;
    using NUnit.Framework;

    public class BaseCommand
    {
        [OneTimeSetUp]
        public void BaseSetup()
        {
            ResetTestSource();
        }

        [OneTimeTearDown]
        public void BaseTeardown()
        {
            TestCommon.RunAICLICommand("source reset", "--force");
        }

        public void ResetTestSource(bool useGroupPolicyForTestSource = false)
        {
            TestCommon.RunAICLICommand("source reset", "--force");
            TestCommon.RunAICLICommand("source remove", Constants.DefaultWingetSourceName);
            TestCommon.RunAICLICommand("source remove", Constants.DefaultMSStoreSourceName);

            // TODO: If/when cert pinning is implemented on the packaged index source, useGroupPolicyForTestSource should be set to default true
            //       to enable testing it by default.  Until then, leaving this here...
            if (useGroupPolicyForTestSource)
            {
                GroupPolicyHelper.EnableAdditionalSources.SetEnabledList(new GroupPolicySource[]
                {
                    new GroupPolicySource
                    {
                        Name = Constants.TestSourceName,
                        Arg = Constants.TestSourceUrl,
                        Type = Constants.TestSourceType,
                        Data = Constants.TestSourceIdentifier,
                        Identifier = Constants.TestSourceIdentifier,
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
                                        }
                                    }
                                }
                            }
                        }
                    }
                });
            }
            else
            {
                GroupPolicyHelper.EnableAdditionalSources.SetNotConfigured();
                TestCommon.RunAICLICommand("source add", $"{Constants.TestSourceName} {Constants.TestSourceUrl}");
            }

            Thread.Sleep(2000);
        }

        public void ConfigureFeature(string featureName, bool status)
        {
            string localAppDataPath = Environment.GetEnvironmentVariable(Constants.LocalAppData);
            JObject settingsJson = JObject.Parse(File.ReadAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath)));
            JObject experimentalFeatures = (JObject)settingsJson["experimentalFeatures"];
            experimentalFeatures[featureName] = status;

            File.WriteAllText(Path.Combine(localAppDataPath, TestCommon.SettingsJsonFilePath), settingsJson.ToString());
        }

        public void InitializeAllFeatures(bool status)
        {
            ConfigureFeature("experimentalArg", status);
            ConfigureFeature("experimentalCmd", status);
            ConfigureFeature("dependencies", status);
            ConfigureFeature("directMSI", status);
            ConfigureFeature("zipInstall", status);
        }
    }
}
