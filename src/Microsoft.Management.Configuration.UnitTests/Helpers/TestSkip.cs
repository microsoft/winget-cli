// -----------------------------------------------------------------------------
// <copyright file="TestSkip.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.UnitTests.Helpers
{
    using System;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Security.Principal;
    using Xunit;

    /// <summary>
    /// Preconditions that a test needs in order to be able to perform its function, and which
    /// cannot be determined until the test runs.
    /// <para>
    /// A test whose environment does not meet its preconditions reports as skipped rather than as
    /// passed or failed. Reporting it as passed hides the fact that nothing was verified, and
    /// reporting it as failed blames the environment for a product that may be perfectly fine.
    /// </para>
    /// <para>
    /// The test method must use <see cref="SkippableFactAttribute"/> or
    /// <see cref="SkippableTheoryAttribute"/> (or one of the <c>SkipIfCI</c> attributes, which are
    /// skippable as well) for these calls to result in a skip.
    /// </para>
    /// </summary>
    public static class TestSkip
    {
        private static readonly Lazy<bool> CanCreateSymbolicLinksValue = new (ProbeCanCreateSymbolicLinks);
        private static readonly Lazy<bool> IsElevatedValue = new (ProbeIsElevated);

        /// <summary>
        /// Gets a value indicating whether symbolic links can be created by this process.
        /// Creating them requires developer mode to be enabled or administrator rights.
        /// </summary>
        public static bool CanCreateSymbolicLinks
        {
            get { return CanCreateSymbolicLinksValue.Value; }
        }

        /// <summary>
        /// Gets a value indicating whether the current process is running elevated.
        /// </summary>
        public static bool IsElevated
        {
            get { return IsElevatedValue.Value; }
        }

        /// <summary>
        /// Gets a value indicating whether the tests are running in a CI build.
        /// </summary>
        public static bool IsCIBuild
        {
            get { return Environment.GetEnvironmentVariable("BUILD_BUILDNUMBER") is not null; }
        }

        /// <summary>
        /// Skips the test if symbolic links cannot be created.
        /// </summary>
        public static void IfCannotCreateSymbolicLinks()
        {
            Skip.IfNot(CanCreateSymbolicLinks, "Creating symbolic links requires developer mode to be enabled or administrator rights.");
        }

        /// <summary>
        /// Skips the test if the current process is elevated.
        /// </summary>
        public static void IfElevated()
        {
            Skip.If(IsElevated, "This test can only perform its function when running unelevated.");
        }

        /// <summary>
        /// Skips the test if the current process is not elevated.
        /// </summary>
        public static void IfNotElevated()
        {
            Skip.IfNot(IsElevated, "This test can only perform its function when running elevated.");
        }

        /// <summary>
        /// Skips the test if the tests are running in a CI build.
        /// </summary>
        public static void IfCIBuild()
        {
            Skip.If(IsCIBuild, "This test cannot perform its function in CI builds.");
        }

        /// <summary>
        /// Skips the test if a file that it requires is not present.
        /// </summary>
        /// <param name="path">The file that the test requires.</param>
        /// <param name="reason">Why the file is required, and how to make it available.</param>
        public static void IfFileNotFound(string path, string reason)
        {
            Skip.IfNot(File.Exists(path), $"The file '{path}' is not present. {reason}");
        }

        /// <summary>
        /// Skips the test if a directory that it requires is not present.
        /// </summary>
        /// <param name="path">The directory that the test requires.</param>
        /// <param name="reason">Why the directory is required, and how to make it available.</param>
        public static void IfDirectoryNotFound(string path, string reason)
        {
            Skip.IfNot(Directory.Exists(path), $"The directory '{path}' is not present. {reason}");
        }

        /// <summary>
        /// Creates a symbolic link to a file, skipping the test if symbolic links cannot be created.
        /// </summary>
        /// <param name="path">The link to create.</param>
        /// <param name="target">The target of the link.</param>
        public static void CreateSymbolicLinkOrSkip(string path, string target)
        {
            IfCannotCreateSymbolicLinks();

            try
            {
                File.CreateSymbolicLink(path, target);
            }
            catch (Exception e) when (IsSymbolicLinkPermissionError(e))
            {
                throw new SkipException($"Unable to create the symbolic link '{path}': {e.Message}");
            }
        }

        /// <summary>
        /// Creates a symbolic link to a directory, skipping the test if symbolic links cannot be created.
        /// </summary>
        /// <param name="path">The link to create.</param>
        /// <param name="target">The target of the link.</param>
        public static void CreateDirectorySymbolicLinkOrSkip(string path, string target)
        {
            IfCannotCreateSymbolicLinks();

            try
            {
                Directory.CreateSymbolicLink(path, target);
            }
            catch (Exception e) when (IsSymbolicLinkPermissionError(e))
            {
                throw new SkipException($"Unable to create the symbolic link '{path}': {e.Message}");
            }
        }

        private static bool IsSymbolicLinkPermissionError(Exception e)
        {
            return e is IOException || e is UnauthorizedAccessException;
        }

        private static bool ProbeIsElevated()
        {
            if (!RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            {
                return false;
            }

            using WindowsIdentity identity = WindowsIdentity.GetCurrent();
            return new WindowsPrincipal(identity).IsInRole(WindowsBuiltInRole.Administrator);
        }

        private static bool ProbeCanCreateSymbolicLinks()
        {
            // Whether links can be created depends on the privileges of the process and on
            // developer mode, neither of which can be reliably determined without trying.
            string probeDirectory = Path.Combine(Path.GetTempPath(), "winget_tests_" + Guid.NewGuid().ToString("N"));

            try
            {
                Directory.CreateDirectory(probeDirectory);

                string target = Path.Combine(probeDirectory, "target.txt");
                File.WriteAllText(target, "target");

                File.CreateSymbolicLink(Path.Combine(probeDirectory, "link.txt"), target);
                return true;
            }
            catch (Exception e) when (IsSymbolicLinkPermissionError(e))
            {
                return false;
            }
            finally
            {
                try
                {
                    Directory.Delete(probeDirectory, true);
                }
                catch (IOException)
                {
                }
                catch (UnauthorizedAccessException)
                {
                }
            }
        }
    }
}
