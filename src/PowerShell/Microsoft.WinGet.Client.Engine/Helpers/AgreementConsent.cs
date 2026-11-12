// -----------------------------------------------------------------------------
// <copyright file="AgreementConsent.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.Collections.Generic;
    using System.Runtime.InteropServices;
    using System.Text;
    using Microsoft.Management.Deployment;
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Obtains the user's consent to source and package agreements.
    /// </summary>
    internal static class AgreementConsent
    {
        /// <summary>
        /// The name of the parameter that accepts source agreements.
        /// </summary>
        public const string AcceptSourceAgreementsParameter = "-AcceptSourceAgreements";

        /// <summary>
        /// The name of the parameter that accepts package agreements.
        /// </summary>
        public const string AcceptPackageAgreementsParameter = "-AcceptPackageAgreements";

        private const int ENoInterface = unchecked((int)0x80004002);

        /// <summary>
        /// The result of obtaining consent.
        /// </summary>
        public enum Decision
        {
            /// <summary>
            /// The user accepted the agreements, either by parameter or when prompted.
            /// Acceptance may be recorded.
            /// </summary>
            Accepted,

            /// <summary>
            /// The agreements were accepted because the user could not be prompted.
            /// Acceptance must not be recorded.
            /// </summary>
            AcceptedWithoutPrompt,

            /// <summary>
            /// The user declined the agreements.
            /// </summary>
            Declined,
        }

        /// <summary>
        /// Determines whether an exception indicates that the WinGet server does not implement a member.
        /// </summary>
        /// <param name="e">The exception.</param>
        /// <returns>True if the server is older than the member.</returns>
        public static bool IsMissingContract(Exception e)
        {
            return e is InvalidCastException || (e is COMException && e.HResult == ENoInterface);
        }

        /// <summary>
        /// Formats the agreements of a source.
        /// </summary>
        /// <param name="reference">The source.</param>
        /// <param name="includeImplicitFields">Whether to include the implicit agreement fields; requires contract 31.</param>
        /// <returns>The formatted agreements, or null if there are none.</returns>
        public static string? FormatSourceAgreements(PackageCatalogReference reference, bool includeImplicitFields)
        {
            StringBuilder builder = new StringBuilder();

            // Enumerating WinRT collections through IEnumerable fails with the embedded projection, so index instead.
            var agreements = reference.SourceAgreements;
            for (int i = 0; i < agreements.Count; ++i)
            {
                SourceAgreement agreement = agreements[i];
                AppendAgreement(builder, agreement.Label, agreement.Text, agreement.Url);
            }

            if (includeImplicitFields && (reference.ImplicitAgreementFields & ImplicitAgreementFields.Market) != 0)
            {
                AppendAgreement(builder, null, Resources.SourceAgreementsMarketMessage, null);
            }

            if (builder.Length == 0)
            {
                return null;
            }

            return string.Format(Resources.SourceAgreementsTitle, reference.Info.Name) + Environment.NewLine + builder.ToString();
        }

        /// <summary>
        /// Formats the agreements of a package.
        /// </summary>
        /// <param name="package">The package description.</param>
        /// <param name="agreements">The agreements.</param>
        /// <returns>The formatted agreements, or null if there are none.</returns>
        public static string? FormatPackageAgreements(string package, IReadOnlyList<PackageAgreement>? agreements)
        {
            if (agreements == null)
            {
                return null;
            }

            StringBuilder builder = new StringBuilder();
            for (int i = 0; i < agreements.Count; ++i)
            {
                PackageAgreement agreement = agreements[i];
                AppendAgreement(builder, agreement.Label, agreement.Text, agreement.Url);
            }

            if (builder.Length == 0)
            {
                return null;
            }

            return string.Format(Resources.PackageAgreementsTitle, package) + Environment.NewLine + builder.ToString();
        }

        /// <summary>
        /// Obtains consent to a set of agreements.
        /// </summary>
        /// <param name="cmdlet">The cmdlet.</param>
        /// <param name="agreements">The formatted agreements.</param>
        /// <param name="prompt">The question to ask the user.</param>
        /// <param name="caption">The caption of the prompt.</param>
        /// <param name="acceptedByParameter">Whether the user accepted the agreements with a parameter.</param>
        /// <param name="parameterName">The name of the parameter that accepts the agreements.</param>
        /// <returns>The decision.</returns>
        public static Decision Obtain(
            PowerShellCmdlet cmdlet,
            string agreements,
            string prompt,
            string caption,
            bool acceptedByParameter,
            string parameterName)
        {
            if (acceptedByParameter)
            {
                cmdlet.Write(StreamType.Warning, agreements + Environment.NewLine + string.Format(Resources.AgreementsAcceptedByParameter, parameterName));
                return Decision.Accepted;
            }

            // The agreements are part of the query so that they are shown with the prompt; warnings
            // written from the background thread may not be displayed until after it.
            bool? answer = cmdlet.ShouldContinue(agreements + Environment.NewLine + prompt, caption);
            if (answer == null)
            {
                cmdlet.Write(StreamType.Warning, agreements + Environment.NewLine + string.Format(Resources.AgreementsAcceptedNonInteractive, parameterName));
                return Decision.AcceptedWithoutPrompt;
            }

            return answer.Value ? Decision.Accepted : Decision.Declined;
        }

        private static void AppendAgreement(StringBuilder builder, string? label, string? text, string? url)
        {
            if (!string.IsNullOrWhiteSpace(label))
            {
                builder.AppendLine(label);
            }

            if (!string.IsNullOrWhiteSpace(text))
            {
                builder.AppendLine(text);
            }

            if (!string.IsNullOrWhiteSpace(url))
            {
                builder.AppendLine(url);
            }
        }
    }
}
