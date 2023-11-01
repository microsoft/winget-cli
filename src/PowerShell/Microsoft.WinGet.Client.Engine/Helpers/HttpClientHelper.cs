// -----------------------------------------------------------------------------
// <copyright file="HttpClientHelper.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Engine.Helpers
{
    using System;
    using System.Diagnostics;
    using System.IO;
    using System.Management.Automation;
    using System.Net.Http;
    using System.Threading.Tasks;
    using Microsoft.WinGet.Common.Command;
    using Microsoft.WinGet.Resources;

    /// <summary>
    /// Helper class for HttpClient calls.
    /// </summary>
    internal class HttpClientHelper
    {
        /// <summary>
        /// The user agent of this module.
        /// </summary>
        public const string UserAgent = "winget-powershell";

        private static readonly HttpClient Client;

        static HttpClientHelper()
        {
            Client = new HttpClient();
        }

        /// <summary>
        /// Downloads a file from a url.
        /// </summary>
        /// <param name="url">Url.</param>
        /// <param name="fileName">File name.</param>
        /// /// <param name="pwshCmdlet">PowershellCmdlet.</param>
        /// <returns>A <see cref="Task"/> representing the asynchronous operation.</returns>
        public async Task DownloadUrlWithProgressAsync(string url, string fileName, PowerShellCmdlet pwshCmdlet)
        {
            pwshCmdlet.Write(StreamType.Verbose, $"Downloading {url}");
            using var request = new HttpRequestMessage(HttpMethod.Get, url);
            request.Headers.Add("User-Agent", UserAgent);

            var cancellationToken = pwshCmdlet.GetCancellationToken();
            using var response = await Client.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, cancellationToken);
            response.EnsureSuccessStatusCode();

            try
            {
                long? contentLength = response.Content.Headers.ContentLength;
                var responseStream = await response.Content.ReadAsStreamAsync();

                using var fileStream = File.Open(fileName, FileMode.OpenOrCreate);

                if (contentLength.HasValue)
                {
                    pwshCmdlet.Write(StreamType.Verbose, $"Size {contentLength} bytes");

                    const int BufferSize = 1024 * 1024; // 1MB
                    byte[] buffer = new byte[BufferSize];
                    int bytesRead, totalBytes = 0;

                    var activityId = pwshCmdlet.GetNewProgressActivityId();

                    // TODO: in win pwsh progress is weird.
                    try
                    {
                        while ((bytesRead = await responseStream.ReadAsync(buffer, 0, buffer.Length, cancellationToken)) > 0)
                        {
                            await fileStream.WriteAsync(buffer, 0, bytesRead, cancellationToken);
                            totalBytes += bytesRead;
                            ProgressRecord record = new (activityId, url, Resources.DownloadingMessage)
                            {
                                RecordType = ProgressRecordType.Processing,
                            };
                            record.StatusDescription = $"{totalBytes / 1000000.0f:0.0} MB / {contentLength / 1000000.0f:0.0} MB";
                            record.PercentComplete = (int)((double)totalBytes / (double)contentLength * 100);
                            pwshCmdlet.Write(StreamType.Progress, record);
                        }
                    }
                    finally
                    {
                        pwshCmdlet.CompleteProgress(activityId, url, Resources.DownloadingMessage);
                    }
                }
                else
                {
                    pwshCmdlet.Write(StreamType.Verbose, $"Content-Length not found in response");
                    await responseStream.CopyToAsync(fileStream);
                }
            }
            catch (Exception)
            {
                if (File.Exists(fileName))
                {
                    File.Delete(fileName);
                }
            }
        }
    }
}
