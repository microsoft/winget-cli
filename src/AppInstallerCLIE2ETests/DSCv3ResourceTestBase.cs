// -----------------------------------------------------------------------------
// <copyright file="DSCv3ResourceTestBase.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests
{
    using System;
    using System.Collections.Generic;
    using System.IO;
    using System.Text.Json;
    using System.Text.Json.Serialization;
    using AppInstallerCLIE2ETests.Helpers;
    using NUnit.Framework;

    /// <summary>
    /// Provides common functionality for DSC v3 resource tests.
    /// </summary>
    public class DSCv3ResourceTestBase
    {
        /// <summary>
        /// The string for the `get` function.
        /// </summary>
        public const string GetFunction = "get";

        /// <summary>
        /// The string for the `test` function.
        /// </summary>
        public const string TestFunction = "test";

        /// <summary>
        /// The string for the `set` function.
        /// </summary>
        public const string SetFunction = "set";

        /// <summary>
        /// The string for the `export` function.
        /// </summary>
        public const string ExportFunction = "export";

        /// <summary>
        /// The string for the `_exist` property name.
        /// </summary>
        public const string ExistPropertyName = "_exist";

        /// <summary>
        /// The string for the `_inDesiredState` property name.
        /// </summary>
        public const string InDesiredStatePropertyName = "_inDesiredState";

        /// <summary>
        /// Write the resource manifests out to the WindowsApps alias directory.
        /// </summary>
        public static void EnsureTestResourcePresence()
        {
            string outputDirectory = Path.Join(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Microsoft\\WindowsApps");
            Assert.IsNotEmpty(outputDirectory);

            var result = TestCommon.RunAICLICommand($"dscv3", $"--manifest -o {outputDirectory}");
            Assert.AreEqual(0, result.ExitCode);
        }

        /// <summary>
        /// Runs a DSC v3 resource command.
        /// </summary>
        /// <param name="resource">The resource to target.</param>
        /// <param name="function">The resource function to run.</param>
        /// <param name="input">Input for the function; supports null, direct string, or JSON serialization of complex objects.</param>
        /// <param name="timeOut">The maximum time to wait in milliseconds.</param>
        /// <param name="throwOnTimeout">Whether to throw on a timeout or simply return the incomplete result.</param>
        /// <returns>A RunCommandResult containing the process exit code and output and error streams.</returns>
        protected static TestCommon.RunCommandResult RunDSCv3Command(string resource, string function, object input, int timeOut = 60000, bool throwOnTimeout = true)
        {
            return TestCommon.RunAICLICommand($"dscv3 {resource}", $"--{function}", ConvertToJSON(input), timeOut, throwOnTimeout);
        }

        /// <summary>
        /// Asserts that a RunCommandResult contains a success for a DSC v3 resource command run.
        /// </summary>
        /// <param name="result">The result of a DSC v3 resource command run.</param>
        protected static void AssertSuccessfulResourceRun(ref TestCommon.RunCommandResult result)
        {
            Assert.AreEqual(0, result.ExitCode);
            Assert.IsNotEmpty(result.StdOut);
        }

        /// <summary>
        /// Gets the output as lines.
        /// </summary>
        /// <param name="output">The output stream from a DSC v3 resource command.</param>
        /// <returns>The lines of the output.</returns>
        protected static string[] GetOutputLines(string output)
        {
            return output.TrimEnd().Split(Environment.NewLine);
        }

        /// <summary>
        /// Asserts that the output is a single line and deserializes that line as JSON.
        /// </summary>
        /// <typeparam name="T">The type to deserialize from JSON.</typeparam>
        /// <param name="output">The output stream from a DSC v3 resource command.</param>
        /// <returns>The object as deserialized.</returns>
        protected static T GetSingleOutputLineAs<T>(string output)
        {
            string[] lines = GetOutputLines(output);
            Assert.AreEqual(1, lines.Length);

            return JsonSerializer.Deserialize<T>(lines[0], GetDefaultJsonOptions());
        }

        /// <summary>
        /// Asserts that the output is two lines and deserializes them as a JSON object and JSON string array.
        /// </summary>
        /// <typeparam name="T">The type to deserialize from JSON.</typeparam>
        /// <param name="output">The output stream from a DSC v3 resource command.</param>
        /// <returns>The object as deserialized and the contents of the string array.</returns>
        protected static (T, List<string>) GetSingleOutputLineAndDiffAs<T>(string output)
        {
            string[] lines = GetOutputLines(output);
            Assert.AreEqual(2, lines.Length);

            var options = GetDefaultJsonOptions();
            return (JsonSerializer.Deserialize<T>(lines[0], options), JsonSerializer.Deserialize<List<string>>(lines[1], options));
        }

        /// <summary>
        /// Deserializes all lines as JSON objects.
        /// </summary>
        /// <typeparam name="T">The type to deserialize from JSON.</typeparam>
        /// <param name="output">The output stream from a DSC v3 resource command.</param>
        /// <returns>A List of objects as deserialized.</returns>
        protected static List<T> GetOutputLinesAs<T>(string output)
        {
            List<T> result = new List<T>();
            string[] lines = GetOutputLines(output);
            var options = GetDefaultJsonOptions();

            foreach (string line in lines)
            {
                result.Add(JsonSerializer.Deserialize<T>(line, options));
            }

            return result;
        }

        /// <summary>
        /// Requires that the diff from a resource command contain the same set of strings as expected.
        /// </summary>
        /// <param name="diff">The diff from a resource command.</param>
        /// <param name="expected">The expected strings.</param>
        protected static void AssertDiffState(List<string> diff, IList<string> expected)
        {
            Assert.IsNotNull(diff);
            Assert.AreEqual(expected.Count, diff.Count);

            foreach (string item in expected)
            {
                Assert.Contains(item, diff);
            }
        }

        private static JsonSerializerOptions GetDefaultJsonOptions()
        {
            return new JsonSerializerOptions()
            {
                DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
                PropertyNamingPolicy = JsonNamingPolicy.CamelCase,
                Converters =
                {
                    new JsonStringEnumConverter(),
                },
            };
        }

        private static string ConvertToJSON(object value) => value switch
        {
            string s => s,
            null => null,
            _ => JsonSerializer.Serialize(value, GetDefaultJsonOptions()),
        };
    }
}
