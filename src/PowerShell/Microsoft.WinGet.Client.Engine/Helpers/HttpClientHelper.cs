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
    using Microsoft.WinGet.Client.Engine.Common;
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

                    byte[] buffer = new byte[Constants.OneMB];
                    int bytesRead, totalBytes = 0;

                    var activityId = pwshCmdlet.GetNewProgressActivityId();
                    double lengthInMB = (double)contentLength.Value / Constants.OneMB;
                    try
                    {
                        int maxPercentComplete = 0;
                        while ((bytesRead = await responseStream.ReadAsync(buffer, 0, buffer.Length, cancellationToken)) > 0)
                        {
                            await fileStream.WriteAsync(buffer, 0, bytesRead, cancellationToken);
                            totalBytes += bytesRead;

                            int percentComplete = (int)((double)totalBytes / contentLength * 100);
                            if (percentComplete > maxPercentComplete)
                            {
                                maxPercentComplete = percentComplete;
                                ProgressRecord record = new (activityId, url, Resources.DownloadingMessage)
                                {
                                    RecordType = ProgressRecordType.Processing,
                                };

                                double progress = (double)totalBytes / Constants.OneMB;
                                record.StatusDescription = $"{progress:0.0} MB / {lengthInMB:0.0} MB";
                                record.PercentComplete = percentComplete;
                                pwshCmdlet.Write(StreamType.Progress, record);
                            }
                        }
                    }
                    finally
                    {
                        pwshCmdlet.CompleteProgress(activityId, url, Resources.DownloadingMessage, true);
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

                throw;
            }
        }
    }
}
