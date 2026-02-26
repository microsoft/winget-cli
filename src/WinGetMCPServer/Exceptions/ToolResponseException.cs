// -----------------------------------------------------------------------------
// <copyright file="ToolResponseException.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace WinGetMCPServer.Exceptions
{
    using ModelContextProtocol.Protocol;

    /// <summary>
    /// An exception that contains a tool response.
    /// </summary>
    internal class ToolResponseException : Exception
    {
        public ToolResponseException(CallToolResult toolResponse)
        {
            this.Response = toolResponse;
        }

        public CallToolResult Response { get; private set; }
    }
}
