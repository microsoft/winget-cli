// -----------------------------------------------------------------------------
// <copyright file="AgreementResponse.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Response
{
    using ModelContextProtocol.Protocol;

    /// <summary>
    /// Contains reusable responses for agreements.
    /// </summary>
    internal static class AgreementResponse
    {
        /// <summary>
        /// Creates a response that asks the caller to present agreements to the user.
        /// </summary>
        /// <param name="agreements">The agreements to present.</param>
        /// <param name="parameterName">The name of the tool parameter that conveys the user's decision.</param>
        /// <param name="declineContinues">
        /// Whether declining allows the operation to continue without the things that require the agreements.
        /// </param>
        /// <returns>The response.</returns>
        public static CallToolResult ForAgreementsRequired(List<AgreementInfo> agreements, string parameterName, bool declineContinues)
        {
            string onDecline = declineContinues ?
                $"If the user declines, call this tool again with the same arguments and {parameterName} set to false to continue without the sources that require them." :
                "If the user declines, do not call this tool again for this request.";

            AgreementsRequiredResult result = new()
            {
                Message = "The user must accept these agreements before this operation can continue, and has not yet been shown them. " +
                    "Present every agreement to the user exactly as provided, including its label, text, and url, and ask whether they accept. " +
                    "Do not accept on the user's behalf. " +
                    $"Only if the user explicitly accepts, call this tool again with the same arguments and {parameterName} set to true. " +
                    onDecline,
                Agreements = agreements,
            };

            return ToolResponse.FromObject(result);
        }

        /// <summary>
        /// Creates a response for when every source was excluded because its agreements were not accepted.
        /// </summary>
        /// <param name="excludedSources">The sources that were excluded.</param>
        /// <returns>The response.</returns>
        public static CallToolResult ForAllSourcesExcluded(IReadOnlyCollection<string> excludedSources)
        {
            return ToolResponse.WithExcludedSources(
                new CallToolResult()
                {
                    IsError = true,
                    Content = [new TextContentBlock() { Text = "No package sources are available because the user has not accepted the agreements of the requested sources." }],
                },
                excludedSources);
        }

        /// <summary>
        /// Creates a response for when the user did not accept the agreements of a package.
        /// </summary>
        /// <param name="identifier">The identifier of the package.</param>
        /// <param name="agreements">The agreements that were not accepted.</param>
        /// <returns>The response.</returns>
        public static CallToolResult ForPackageAgreementsNotAccepted(string identifier, List<AgreementInfo> agreements)
        {
            AgreementsRequiredResult result = new()
            {
                Message = $"The package was not installed because the user did not accept its agreements: {identifier}",
                Agreements = agreements,
            };

            return ToolResponse.FromObject(result, isError: true);
        }
    }
}
