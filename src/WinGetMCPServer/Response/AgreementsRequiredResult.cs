// -----------------------------------------------------------------------------
// <copyright file="AgreementsRequiredResult.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Response
{
    /// <summary>
    /// Contains agreements that the user must accept before the operation can continue.
    /// </summary>
    internal class AgreementsRequiredResult
    {
        public required string Message { get; init; }

        public List<AgreementInfo> Agreements { get; init; } = new List<AgreementInfo>();
    }
}
