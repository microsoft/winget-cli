// -----------------------------------------------------------------------------
// <copyright file="AgreementsRestSource.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace AppInstallerCLIE2ETests.Helpers
{
    using System.IO;
    using NUnit.Framework;

    /// <summary>
    /// A REST source with source agreements, served from a generated information file under the static file root.
    /// Only the information endpoint is served, so the source cannot be searched.
    /// </summary>
    public class AgreementsRestSource
    {
        /// <summary>
        /// The label of the source agreement.
        /// </summary>
        public const string AgreementLabel = "Test Agreement";

        /// <summary>
        /// The text of the source agreement.
        /// </summary>
        public const string AgreementText = "Test agreement text";

        /// <summary>
        /// The URL of the source agreement.
        /// </summary>
        public const string AgreementUrl = "https://localhost:5001/TestKit/agreement";

        /// <summary>
        /// Initializes a new instance of the <see cref="AgreementsRestSource"/> class.
        /// </summary>
        /// <param name="name">The name of the source, also used as its directory under the static file root.</param>
        public AgreementsRestSource(string name)
        {
            this.Name = name;
        }

        /// <summary>
        /// Gets the name of the source.
        /// </summary>
        public string Name { get; }

        /// <summary>
        /// Gets the URL of the source.
        /// </summary>
        public string Url => "https://localhost:5001/TestKit/" + this.Name;

        private string InformationFilePath => Path.Combine(TestSetup.Parameters.StaticFileRootPath, this.Name, "information");

        /// <summary>
        /// Writes the information served by the source.
        /// </summary>
        /// <param name="agreementsIdentifier">The agreements identifier, or null for a source without agreements.</param>
        /// <param name="requiresMarket">Whether the source requires the market, which is an implicit agreement.</param>
        public void WriteInformation(string agreementsIdentifier, bool requiresMarket)
        {
            string agreements = agreementsIdentifier == null ? string.Empty : $@",
    ""SourceAgreements"": {{
      ""AgreementsIdentifier"": ""{agreementsIdentifier}"",
      ""Agreements"": [{{
        ""AgreementLabel"": ""{AgreementLabel}"",
        ""Agreement"": ""{AgreementText}"",
        ""AgreementUrl"": ""{AgreementUrl}""
      }}]
    }}";

            string market = requiresMarket ? @",
    ""RequiredPackageMatchFields"": [ ""Market"" ]" : string.Empty;

            string information = $@"{{
  ""Data"": {{
    ""SourceIdentifier"": ""{this.Name}"",
    ""ServerSupportedVersions"": [ ""1.0.0"" ]{agreements}{market}
  }}
}}";

            Directory.CreateDirectory(Path.GetDirectoryName(this.InformationFilePath));
            File.WriteAllText(this.InformationFilePath, information);
        }

        /// <summary>
        /// Adds the source with the CLI and then changes its agreements, so that they are not accepted.
        /// Adding a source accepts its current agreements.
        /// </summary>
        /// <param name="requiresMarket">Whether the source requires the market, which is an implicit agreement.</param>
        public void AddWithUnacceptedAgreements(bool requiresMarket = false)
        {
            this.WriteInformation("AgreementsV1", requiresMarket);
            var result = TestCommon.RunAICLICommand("source add", $"{this.Name} {this.Url} --type {Constants.RestTestSourceType} --trust-level trusted --accept-source-agreements");
            Assert.That(result.ExitCode, Is.EqualTo(Constants.ErrorCode.S_OK), result.StdOut);

            this.WriteInformation("AgreementsV2", requiresMarket);
        }

        /// <summary>
        /// Removes the source and its information file.
        /// </summary>
        public void Remove()
        {
            TestCommon.RunAICLICommand("source remove", this.Name);

            var directory = Path.GetDirectoryName(this.InformationFilePath);
            if (Directory.Exists(directory))
            {
                Directory.Delete(directory, true);
            }
        }
    }
}
