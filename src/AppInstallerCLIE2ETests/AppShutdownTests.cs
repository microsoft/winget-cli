// -----------------------------------------------------------------------------
// <copyright file="AppShutdownTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Xml;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// `test appshutdown` command tests.
    /// </summary>
    public class AppShutdownTests
    {
        /// <summary>
        /// Runs winget test appshutdown and register the application to force a WM_QUERYENDSESSION message.
        /// </summary>
        [Test]
        public void RegisterApplicationTest()
        {
            if (!TestSetup.Parameters.PackagedContext)
            {
                Assert.Ignore("Not packaged context.");
            }

            if (!TestCommon.ExecutingAsAdministrator && TestCommon.IsCIEnvironment)
            {
                Assert.Ignore("This test won't work on Window Server as non-admin");
            }

            if (!Environment.Is64BitProcess)
            {
                // My guess is that HAM terminates us faster after the CTRL-C on x86...
                Assert.Ignore("This test is flaky when run as x86.");
            }

            if (string.IsNullOrEmpty(TestSetup.Parameters.AICLIPackagePath))
            {
                throw new NullReferenceException("AICLIPackagePath");
            }

            var appxManifest = Path.Combine(TestSetup.Parameters.AICLIPackagePath, "AppxManifest.xml");
            if (!File.Exists(appxManifest))
            {
                throw new FileNotFoundException(appxManifest);
            }

            // In order to registering the application we need a higher version number and pass the force app shutdown flag.
            // Doing it the long way.
            var xmlDoc = new XmlDocument();
            XmlNamespaceManager namespaces = new XmlNamespaceManager(xmlDoc.NameTable);
            namespaces.AddNamespace("n", "http://schemas.microsoft.com/appx/manifest/foundation/windows10");
            xmlDoc.Load(appxManifest);
            var identityNode = xmlDoc.SelectSingleNode("/n:Package/n:Identity", namespaces);
            if (identityNode == null)
            {
                throw new NullReferenceException("Identity node");
            }

            var versionAttribute = identityNode.Attributes["Version"];
            if (versionAttribute == null)
            {
                throw new NullReferenceException("Version attribute");
            }

            var ogVersion = new Version(versionAttribute.Value);
            var newVersion = new Version(ogVersion.Major, ogVersion.Minor, ogVersion.Build, ogVersion.Revision + 1);
            versionAttribute.Value = newVersion.ToString();
            xmlDoc.Save(appxManifest);

            // This just waits for the app termination event.
            var testCmdTask = new Task<TestCommon.RunCommandResult>(() =>
            {
                return TestCommon.RunAICLICommand("test", "appshutdown --verbose", timeOut: 300000, throwOnTimeout: false);
            });

            // Register the app with the updated version.
            var registerTask = new Task<bool>(() =>
            {
                return TestCommon.InstallMsixRegister(TestSetup.Parameters.AICLIPackagePath, true, false);
            });

            // Give it a little time.
            testCmdTask.Start();
            Thread.Sleep(30000);
            registerTask.Start();

            Task.WaitAll(new Task[] { testCmdTask, registerTask }, 360000);

            // Assert.True(registerTask.Result);
            TestContext.Out.Write(testCmdTask.Result.StdOut);

            // The ctrl-c command terminates the batch file before the exit code file gets created.
            // Look for the output.
            Assert.True(testCmdTask.Result.StdOut.Contains("Succeeded waiting for app shutdown event"));
        }

        /// <summary>
        /// Runs winget test can-unload-now expecting that it cannot be unloaded.
        /// </summary>
        [Test]
        public void CanUnloadNowTest()
        {
            var result = TestCommon.RunAICLICommand("test", "can-unload-now --verbose");

            var lines = result.StdOut.Split('\n', StringSplitOptions.TrimEntries | StringSplitOptions.RemoveEmptyEntries);

            Assert.AreEqual(5, lines.Length);
            Assert.True(lines[0].Contains("Internal objects:"));
            Assert.False(lines[0].Contains("Internal objects: 0"));
            Assert.True(lines[1].Contains("External objects: 0"));
            Assert.True(lines[2].Contains("DllCanUnloadNow"));
            Assert.True(lines[3].Contains("Internal objects: 0"));
            Assert.True(lines[4].Contains("External objects: 0"));
        }
    }
}
