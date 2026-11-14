// -----------------------------------------------------------------------------
// <copyright file="WingetCLIWrapperTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.UnitTests
{
    using System;
    using System.IO;
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Xunit;

    /// <summary>
    /// Tests for <see cref="WingetCLIWrapper"/>.
    /// </summary>
    public class WingetCLIWrapperTests
    {
        private const string FileName = "winget.exe";

        /// <summary>
        /// Tests that the first PATH directory containing the file is returned.
        /// </summary>
        [Fact]
        public void FindInPathReturnsFirstMatch()
        {
            string root = Directory.CreateDirectory(Path.Combine(Path.GetTempPath(), Guid.NewGuid().ToString())).FullName;

            try
            {
                string empty = Directory.CreateDirectory(Path.Combine(root, "empty")).FullName;
                string first = Directory.CreateDirectory(Path.Combine(root, "first")).FullName;
                string second = Directory.CreateDirectory(Path.Combine(root, "second")).FullName;
                File.WriteAllText(Path.Combine(first, FileName), string.Empty);
                File.WriteAllText(Path.Combine(second, FileName), string.Empty);

                string envPath = string.Join(Path.PathSeparator, empty, first, second);

                Assert.Equal(Path.Combine(first, FileName), WingetCLIWrapper.FindInPath(envPath, FileName));
            }
            finally
            {
                Directory.Delete(root, true);
            }
        }

        /// <summary>
        /// Tests that PATH entries that resolve to the current directory are ignored.
        /// </summary>
        [Fact]
        public void FindInPathIgnoresCurrentDirectory()
        {
            string fileName = $"{Guid.NewGuid()}.exe";
            string currentDirectory = Environment.CurrentDirectory;
            string plantedFile = Path.Combine(currentDirectory, fileName);
            File.WriteAllText(plantedFile, string.Empty);

            try
            {
                string envPath = string.Join(
                    Path.PathSeparator,
                    string.Empty,
                    ".",
                    ".\\",
                    currentDirectory.Substring(0, 2),
                    currentDirectory.Substring(2));

                Assert.Null(WingetCLIWrapper.FindInPath(envPath, fileName));
            }
            finally
            {
                File.Delete(plantedFile);
            }
        }
    }
}
