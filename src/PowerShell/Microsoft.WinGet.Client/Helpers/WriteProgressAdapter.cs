// -----------------------------------------------------------------------------
// <copyright file="WriteProgressAdapter.cs" company="Microsoft Corporation">
//     Copyright (c) Microsoft Corporation. Licensed under the MIT License.
// </copyright>
// -----------------------------------------------------------------------------

namespace Microsoft.WinGet.Client.Helpers
{
    using System.Collections.Generic;
    using System.Management.Automation;
    using System.Threading;

    /// <summary>
    /// Marshals calls to <see cref="Cmdlet.WriteProgress(ProgressRecord)" /> back to the main thread.
    /// </summary>
    public class WriteProgressAdapter
    {
        private readonly AutoResetEvent resetEvent = new (false);
        private readonly Queue<ProgressRecord> records = new ();
        private readonly Cmdlet cmdlet;
        private volatile bool completed = false;

        /// <summary>
        /// Initializes a new instance of the <see cref="WriteProgressAdapter" /> class.
        /// </summary>
        /// <param name="cmdlet">A <see cref="Cmdlet" /> instance.</param>
        public WriteProgressAdapter(Cmdlet cmdlet)
        {
            this.cmdlet = cmdlet;
        }

        /// <summary>
        /// Sets a value indicating whether the asynchronous operation is finished and the main thread can continue.
        /// </summary>
        public bool Completed
        {
            set
            {
                this.completed = value;
                if (value)
                {
                    this.resetEvent.Set();
                }
            }
        }

        /// <summary>
        /// This should be called on the main thread to wait for the asynchronous operation to complete.
        /// </summary>
        public void Wait()
        {
            while (!this.completed)
            {
                lock (this.records)
                {
                    this.Flush();
                }

                this.resetEvent.WaitOne();
            }

            this.Flush();
        }

        /// <summary>
        /// This is an analogue of the <see cref="Cmdlet.WriteProgress(ProgressRecord)" /> function.
        /// </summary>
        /// <param name="record">A <see cref="ProgressRecord" /> instance.</param>
        public void WriteProgress(ProgressRecord record)
        {
            if (record != null)
            {
                lock (this.records)
                {
                    this.records.Enqueue(record);
                }

                this.resetEvent.Set();
            }
        }

        private void Flush()
        {
            while (this.records.Count > 0)
            {
                this.cmdlet.WriteProgress(this.records.Dequeue());
            }
        }
    }
}
