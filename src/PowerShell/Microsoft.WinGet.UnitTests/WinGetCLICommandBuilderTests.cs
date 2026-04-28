// -----------------------------------------------------------------------------
// <copyright file="WinGetCLICommandBuilderTests.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.UnitTests
{
    using Microsoft.WinGet.Client.Engine.Helpers;
    using Xunit;

    /// <summary>
    /// Tests for <see cref="WinGetCLICommandBuilder"/>.
    /// </summary>
    public class WinGetCLICommandBuilderTests
    {
        private const string MockCommand = "mock";
        private const string TestOption = "test";

        /// <summary>
        /// Tests that options are correctly escaped.
        /// </summary>
        /// <param name="optionValue">The option value to test.</param>
        /// <param name="escapedOptionValue">The expected escaped option value.</param>
        [Theory]
        [InlineData("winget", "\"winget\"")] // mock --test winget
        [InlineData("winget create", "\"winget create\"")] // mock --test "winget create"
        [InlineData("winget\tcreate", "\"winget\tcreate\"")] // mock --test "winget<tab>create"
        [InlineData("winget\ncreate", "\"winget\ncreate\"")] // mock --test "winget<newline>create"
        [InlineData("", "\"\"")] // mock --test ""
        [InlineData(@"\\", "\"\\\\\\\\\"")] // mock --test "\\\\"
        [InlineData("winget \"create\"", "\"winget \\\"create\\\"\"")] // mock --test "winget \"create\""
        [InlineData(@"C:\PATH_A\PATH_B", "\"C:\\PATH_A\\PATH_B\"")] // mock --test C:\PATH_A\PATH_B
        [InlineData(@"C:\PATH_A\PATH_B\", "\"C:\\PATH_A\\PATH_B\\\\\"")] // mock --test "C:\PATH_A\PATH_B\\"
        [InlineData("C:\\PATH_A\\\"PATH_B\"", "\"C:\\PATH_A\\\\\\\"PATH_B\\\"\"")] // mock --test "C:\PATH_A\\\"PATH_B\""
        public void GeneratesCorrectlyEscapedParameters(string optionValue, string escapedOptionValue)
        {
            WinGetCLICommandBuilder builder = new (MockCommand);
            builder.AppendOption(TestOption, optionValue);

            var expectedCommand = MockCommand;
            var expectedParameters = $"--{TestOption} {escapedOptionValue}";

            Assert.Equal(expectedCommand, builder.Command);
            Assert.Equal(expectedParameters, builder.Parameters);
            Assert.Equal($"{expectedCommand} {expectedParameters}", builder.ToString());
        }

        /// <summary>
        /// Tests that multiple options are appended correctly.
        /// </summary>
        [Fact]
        public void AppendMultipleOptions_GeneratesCorrectParameters()
        {
            WinGetCLICommandBuilder builder = new (MockCommand);
            builder.AppendOption("option1", "value1");
            builder.AppendOption("option2", "value2");
            builder.AppendOption("option3", "value with spaces");

            var expectedParameters = "--option1 \"value1\" --option2 \"value2\" --option3 \"value with spaces\"";

            Assert.Equal(MockCommand, builder.Command);
            Assert.Equal(expectedParameters, builder.Parameters);
            Assert.Equal($"{MockCommand} {expectedParameters}", builder.ToString());
        }

        /// <summary>
        /// Tests that switches are appended correctly.
        /// </summary>
        [Fact]
        public void AppendSwitch_GeneratesParameterWithoutValue()
        {
            WinGetCLICommandBuilder builder = new (MockCommand);
            builder.AppendSwitch("verbose");
            builder.AppendOption("name", "test");

            var expectedParameters = "--verbose --name \"test\"";

            Assert.Equal(expectedParameters, builder.Parameters);
        }

        /// <summary>
        /// Tests that an empty builder returns only the command.
        /// </summary>
        [Fact]
        public void EmptyBuilder_ReturnsOnlyCommand()
        {
            WinGetCLICommandBuilder builder = new (MockCommand);

            Assert.Equal(MockCommand, builder.Command);
            Assert.Equal(string.Empty, builder.Parameters);
            Assert.Equal(MockCommand, builder.ToString());
        }

        /// <summary>
        /// Tests that appending an option with a null value does not append anything.
        /// </summary>
        [Fact]
        public void AppendOption_WithNullValue_DoesNotAppendOption()
        {
            WinGetCLICommandBuilder builder = new (MockCommand);
            builder.AppendOption("test", null);

            Assert.Equal(string.Empty, builder.Parameters);
        }

        /// <summary>
        /// Tests that mixed parameters are generated in the correct order.
        /// </summary>
        [Fact]
        public void MixedParameters_GeneratesCorrectOrder()
        {
            WinGetCLICommandBuilder builder = new (MockCommand);
            builder.AppendSwitch("silent");
            builder.AppendOption("source", "winget");
            builder.AppendSwitch("force");
            builder.AppendOption("id", "Microsoft.PowerShell");

            var expectedParameters = "--silent --source \"winget\" --force --id \"Microsoft.PowerShell\"";

            Assert.Equal(expectedParameters, builder.Parameters);
        }

        /// <summary>
        /// Tests that different commands are set correctly.
        /// </summary>
        /// <param name="command">The command to test.</param>
        [Theory]
        [InlineData("install")]
        [InlineData("search")]
        [InlineData("upgrade")]
        public void DifferentCommands_ReturnCorrectCommand(string command)
        {
            WinGetCLICommandBuilder builder = new (command);

            Assert.Equal(command, builder.Command);
        }

        /// <summary>
        /// Tests a complex scenario with multiple parameter types.
        /// </summary>
        [Fact]
        public void ComplexScenario_WithMultipleParameterTypes()
        {
            WinGetCLICommandBuilder builder = new ("install");
            builder.AppendOption("id", "Microsoft.VisualStudioCode");
            builder.AppendSwitch("silent");
            builder.AppendOption("override", "/VERYSILENT /NORESTART");
            builder.AppendOption("location", @"C:\Program Files\VSCode");
            builder.AppendSwitch("interactive");

            var expectedCommand = "install";
            var expectedParameters = "--id \"Microsoft.VisualStudioCode\" --silent --override \"/VERYSILENT /NORESTART\" --location \"C:\\Program Files\\VSCode\" --interactive";

            Assert.Equal(expectedCommand, builder.Command);
            Assert.Equal(expectedParameters, builder.Parameters);
            Assert.Equal($"{expectedCommand} {expectedParameters}", builder.ToString());
        }

        /// <summary>
        /// Tests that subcommands are handled correctly.
        /// </summary>
        [Fact]
        public void Subcommand_GeneratesCorrectCommandStructure()
        {
            WinGetCLICommandBuilder builder = new ("source");
            builder.AppendSubCommand("add");
            builder.AppendOption("name", "mysource");
            builder.AppendOption("arg", "https://mock");

            var expectedParameters = "--name \"mysource\" --arg \"https://mock\"";

            Assert.Equal("source add", builder.Command);
            Assert.Equal(expectedParameters, builder.Parameters);
            Assert.Equal($"source add {expectedParameters}", builder.ToString());
        }
    }
}
