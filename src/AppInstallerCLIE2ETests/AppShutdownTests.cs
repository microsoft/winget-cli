// -----------------------------------------------------------------------------
// <copyright file="ConfigureShowCommand.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.IO;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Xml;
    using NUnit.Framework;

    /// <summary>
    /// `test appshutdown` command tests.
    /// </summary>
    public class AppShutdownTests : BaseCommand
    {
        /// <summary>
        /// Runs winget test appshutdown and register the application to force a WM_QUERYENDSESSION message.
        /// </summary>
        [Test]
        public void RegisterApplicationTest()
        {
            if (!TestCommon.PackagedContext)
            {
                return;
            }

            if (string.IsNullOrEmpty(TestCommon.AICLIPackagePath))
            {
                throw new NullReferenceException("AICLIPackagePath");
            }

            var appxManifest = Path.Combine(TestCommon.AICLIPackagePath, "AppxManifest.xml");
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

            var versionAtr = identityNode.Attributes["Version"];
            if (versionAtr == null)
            {
                throw new NullReferenceException("Version attribute");
            }

            var ogVersion = new Version(versionAtr.Value);
            var newVersion = new Version(ogVersion.Major, ogVersion.Minor, ogVersion.Build, ogVersion.Revision + 1);
            versionAtr.Value = newVersion.ToString();
            xmlDoc.Save(appxManifest);

            // This just waits for the app termination event.
            var testCmdTask = new Task<TestCommon.RunCommandResult>(() =>
            {
                return TestCommon.RunAICLICommandViaInvokeCommandInDesktopPackage("test", "appshutdown", timeOut: 60000, throwOnTimeout: false);
            });

            // Register the app with the updated version.
            var registerTask = new Task<bool>(() =>
            {
                return TestCommon.InstallMsixRegister(TestCommon.AICLIPackagePath, true, false);
            });

            // Give it a little time.
            testCmdTask.Start();
            Thread.Sleep(10000);
            registerTask.Start();

            Task.WaitAll(new Task[] { testCmdTask, registerTask }, 120000);

            Assert.True(registerTask.Result);
            TestContext.Out.Write(testCmdTask.Result.StdOut);

            // The ctrl-c command terminates the batch file before the exit code file gets created.
            // Look for the output.
            Assert.True(testCmdTask.Result.StdOut.Contains("Succeeded waiting for app shutdown event"));
        }
    }
}