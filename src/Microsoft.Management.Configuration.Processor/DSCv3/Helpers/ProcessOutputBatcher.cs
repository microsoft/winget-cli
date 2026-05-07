// -----------------------------------------------------------------------------
// <copyright file="ProcessOutputBatcher.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.Management.Configuration.Processor.DSCv3.Helpers
{
    using System;
    using System.Text;
    using System.Threading;

    /// <summary>
    /// Subscribes to a <see cref="ProcessExecution"/>'s output and error events,
    /// accumulates lines in a thread-safe buffer, and forwards them to an
    /// <see cref="IDiagnosticsSink"/> as a single batched message on a fixed interval.
    /// This avoids one cross-process IPC call per output line while still delivering
    /// output promptly even if the process never exits.
    /// </summary>
    internal sealed class ProcessOutputBatcher : IDisposable
    {
        private readonly IDiagnosticsSink? sink;
        private readonly Timer flushTimer;
        private readonly object bufferLock = new object();
        private StringBuilder buffer = new StringBuilder();
        private string batchHeader = "--- Process Output ---";
        private string outputPrefix = "[out] ";
        private string errorPrefix = "[err] ";
        private bool disposed = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProcessOutputBatcher"/> class.
        /// </summary>
        /// <param name="sink">The diagnostics sink to forward batched output to. May be null.</param>
        /// <param name="flushInterval">How often to flush accumulated output to the sink.</param>
        public ProcessOutputBatcher(IDiagnosticsSink? sink, TimeSpan flushInterval)
        {
            this.sink = sink;
            this.flushTimer = new Timer(this.OnTimerTick, null, flushInterval, flushInterval);
        }

        /// <summary>
        /// Subscribes to the given <see cref="ProcessExecution"/>'s output and error events.
        /// Must be called before <see cref="ProcessExecution.Start"/>.
        /// </summary>
        /// <param name="processExecution">The process execution to monitor.</param>
        public void Subscribe(ProcessExecution processExecution)
        {
            int number = processExecution.ExecutionNumber;
            this.batchHeader = $"--- [{number}] Process Output ---";
            this.outputPrefix = $"[{number}:out] ";
            this.errorPrefix = $"[{number}:err] ";
            processExecution.OutputLineReceived += this.OnOutputLineReceived;
            processExecution.ErrorLineReceived += this.OnErrorLineReceived;
        }

        /// <summary>
        /// Stops the timer, flushes any remaining buffered lines to the sink, and disposes resources.
        /// Call this after <see cref="ProcessExecution.WaitForExit"/> returns to ensure all output is delivered.
        /// </summary>
        public void Flush()
        {
            this.flushTimer.Change(Timeout.Infinite, Timeout.Infinite);
            this.EmitBuffer();
        }

        /// <inheritdoc/>
        public void Dispose()
        {
            if (!this.disposed)
            {
                this.disposed = true;
                this.flushTimer.Dispose();
            }
        }

        private void OnOutputLineReceived(object? sender, string line)
        {
            lock (this.bufferLock)
            {
                this.buffer.Append('\n').Append(this.outputPrefix).Append(line);
            }
        }

        private void OnErrorLineReceived(object? sender, string line)
        {
            lock (this.bufferLock)
            {
                this.buffer.Append('\n').Append(this.errorPrefix).Append(line);
            }
        }

        private void OnTimerTick(object? state)
        {
            this.EmitBuffer();
        }

        private void EmitBuffer()
        {
            if (this.sink == null)
            {
                return;
            }

            StringBuilder toEmit;
            lock (this.bufferLock)
            {
                if (this.buffer.Length == 0)
                {
                    return;
                }

                toEmit = this.buffer;
                this.buffer = new StringBuilder();
            }

            this.sink.OnDiagnostics(DiagnosticLevel.Verbose, this.batchHeader + toEmit);
        }
    }
}
