// -----------------------------------------------------------------------------
// <copyright file="AgreementConsent.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Agreements
{
    using System.Collections.Concurrent;
    using System.Text;
    using System.Text.Json;
    using ModelContextProtocol.Protocol;
    using ModelContextProtocol.Server;
    using WinGetMCPServer.Response;

    /// <summary>
    /// The outcome of asking the user to accept a set of agreements.
    /// </summary>
    internal enum ConsentDecision
    {
        /// <summary>
        /// The user accepted the agreements.
        /// </summary>
        Accepted,

        /// <summary>
        /// The user explicitly declined the agreements.
        /// </summary>
        Declined,

        /// <summary>
        /// The user dismissed the request without making a choice.
        /// </summary>
        Dismissed,

        /// <summary>
        /// The user could not be asked directly; the agreements must be returned to the caller to present.
        /// </summary>
        Pending,
    }

    /// <summary>
    /// Obtains the user's consent to agreements, using elicitation when the client supports it and
    /// otherwise falling back to returning the agreements to the caller.
    /// </summary>
    internal static class AgreementConsent
    {
        /// <summary>
        /// The name of the confirmation field in the elicitation form.
        /// </summary>
        private const string AcceptFieldName = "accept";

        /// <summary>
        /// Agreement text longer than this is truncated in the elicitation message, as forms have no way
        /// to render long or rich content.
        /// </summary>
        private const int MaximumAgreementTextLength = 2000;

        /// <summary>
        /// The agreement sets that have been returned to a caller that cannot be elicited from. Without
        /// elicitation the server cannot observe the user, so this is the strongest available guard
        /// against a caller claiming acceptance of agreements it was never shown. Tools are constructed
        /// per call, so this state must be static; it is never persisted.
        /// </summary>
        private static readonly ConcurrentDictionary<string, byte> PresentedAgreements = new ConcurrentDictionary<string, byte>(StringComparer.Ordinal);

        /// <summary>
        /// Determines whether the client supports form mode elicitation.
        /// </summary>
        /// <param name="server">The server.</param>
        /// <returns>True if form mode elicitation can be used.</returns>
        public static bool SupportsFormElicitation(McpServer server)
        {
            return server.ClientCapabilities?.Elicitation?.Form != null;
        }

        /// <summary>
        /// Obtains the user's decision on a set of agreements.
        /// </summary>
        /// <param name="server">The server.</param>
        /// <param name="introduction">Text that introduces the agreements to the user.</param>
        /// <param name="agreements">The agreements.</param>
        /// <param name="question">The question asked of the user, used as the label of the confirmation field.</param>
        /// <param name="acceptAgreements">The decision provided by the caller, if any.</param>
        /// <param name="cancellationToken">The cancellation token.</param>
        /// <returns>The decision.</returns>
        public static async Task<ConsentDecision> ObtainAsync(
            McpServer server,
            string introduction,
            IReadOnlyList<AgreementInfo> agreements,
            string question,
            bool? acceptAgreements,
            CancellationToken cancellationToken)
        {
            if (acceptAgreements == false)
            {
                return ConsentDecision.Declined;
            }

            // Ask the user directly whenever possible, even if the caller claims acceptance, so that the
            // decision is never made by the model on the user's behalf.
            if (SupportsFormElicitation(server))
            {
                try
                {
                    ElicitResult result = await server.ElicitAsync(CreateRequest(introduction, agreements, question), cancellationToken);

                    if (result.IsAccepted &&
                        result.Content != null &&
                        result.Content.TryGetValue(AcceptFieldName, out JsonElement value) &&
                        value.ValueKind == JsonValueKind.True)
                    {
                        return ConsentDecision.Accepted;
                    }

                    // Submitting the form without checking the box is a refusal.
                    return result.IsAccepted || string.Equals(result.Action, "decline", StringComparison.OrdinalIgnoreCase) ?
                        ConsentDecision.Declined :
                        ConsentDecision.Dismissed;
                }
                catch (OperationCanceledException) when (cancellationToken.IsCancellationRequested)
                {
                    throw;
                }
                catch (Exception)
                {
                    // Elicitation failed; fall back to having the caller present the agreements.
                }
            }

            string key = CreatePresentationKey(agreements);
            if (acceptAgreements == true && PresentedAgreements.ContainsKey(key))
            {
                return ConsentDecision.Accepted;
            }

            PresentedAgreements.TryAdd(key, 0);
            return ConsentDecision.Pending;
        }

        /// <summary>
        /// Creates a form mode elicitation request with a single confirmation field. Forms carry only flat
        /// primitive fields, so the agreements themselves must be conveyed in the message.
        /// </summary>
        /// <param name="introduction">Text that introduces the agreements.</param>
        /// <param name="agreements">The agreements.</param>
        /// <param name="question">The question asked of the user, used as the label of the confirmation field.</param>
        /// <returns>The elicitation request.</returns>
        private static ElicitRequestParams CreateRequest(string introduction, IReadOnlyList<AgreementInfo> agreements, string question)
        {
            StringBuilder message = new StringBuilder();
            message.AppendLine(introduction);

            foreach (AgreementInfo agreement in agreements)
            {
                message.AppendLine();

                if (!string.IsNullOrWhiteSpace(agreement.Label))
                {
                    message.AppendLine(agreement.Label);
                }

                if (!string.IsNullOrWhiteSpace(agreement.Text))
                {
                    if (agreement.Text.Length > MaximumAgreementTextLength)
                    {
                        // The full text remains available at the URL, which follows when present.
                        message.Append(agreement.Text, 0, MaximumAgreementTextLength);
                        message.AppendLine("\u2026");
                    }
                    else
                    {
                        message.AppendLine(agreement.Text);
                    }
                }

                if (!string.IsNullOrWhiteSpace(agreement.Url))
                {
                    message.AppendLine(agreement.Url);
                }
            }

            return new ElicitRequestParams()
            {
                Message = message.ToString().TrimEnd(),
                RequestedSchema = new ElicitRequestParams.RequestSchema()
                {
                    Properties =
                    {
                        [AcceptFieldName] = new ElicitRequestParams.BooleanSchema()
                        {
                            Title = question,
                            Default = false,
                        },
                    },
                    Required = [AcceptFieldName],
                },
            };
        }

        private static string CreatePresentationKey(IReadOnlyList<AgreementInfo> agreements)
        {
            return JsonSerializer.Serialize(agreements, ToolResponse.GetDefaultJsonOptions());
        }
    }
}
