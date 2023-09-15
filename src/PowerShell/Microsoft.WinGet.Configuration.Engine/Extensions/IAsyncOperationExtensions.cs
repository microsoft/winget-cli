// -----------------------------------------------------------------------------
// <copyright file="IAsyncOperationExtensions.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Configuration.Engine.Extensions
{
    using System.Threading;
    using System.Threading.Tasks;
    using Windows.Foundation;

    /// <summary>
    /// Extension methods for IAsyncOperation objects.
    /// </summary>
    internal static class IAsyncOperationExtensions
    {
        /// <summary>
        /// Wrap IAsyncOperationWithProgress into a task with cancellation support.
        /// </summary>
        /// <typeparam name="TOperationResult">The result of the operation.</typeparam>
        /// <typeparam name="TProgressData">The progress data of the operation.</typeparam>
        /// <param name="asyncOperation">The async operation.</param>
        /// <param name="cancellationToken">Optional cancellation token.</param>
        /// <returns>A task.</returns>
        public static Task<TOperationResult> AsTask<TOperationResult, TProgressData>(this IAsyncOperationWithProgress<TOperationResult, TProgressData> asyncOperation, CancellationToken cancellationToken = default)
        {
            var tcs = new TaskCompletionSource<TOperationResult>();
            if (cancellationToken != default)
            {
                cancellationToken.Register(asyncOperation.Cancel);
            }

            asyncOperation.Completed = (asyncInfo, asyncStatus) =>
            {
                switch (asyncStatus)
                {
                    case AsyncStatus.Canceled:
                        tcs.SetCanceled();
                        break;
                    case AsyncStatus.Completed:
                        tcs.SetResult(asyncInfo.GetResults());
                        break;
                    case AsyncStatus.Error:
                        tcs.SetException(asyncInfo.ErrorCode);
                        break;
                    case AsyncStatus.Started:
                        break;
                    default:
                        break;
                }
            };

            // Make sure to throw operation cancelled exception if needed.
            return tcs.Task.ContinueWith(
                t =>
                {
                    if (t.IsCanceled)
                    {
                        cancellationToken.ThrowIfCancellationRequested();
                    }

                    if (!t.IsFaulted)
                    {
                        return t.Result;
                    }

                    // If IsFaulted is true, the task's Status is equal to Faulted,
                    // and its Exception property will be non-null.
                    throw t.Exception!;
                });
        }
    }
}
