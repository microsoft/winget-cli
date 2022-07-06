﻿
namespace Microsoft.Management.Deployment.Projection
{
    using System;
    using System.Collections.Generic;

    public enum ClsidContext
    {
        // In process
        InProc,

        // Out of process production
        OutOfProc,

        // Out of process development
        OutOfProcDev
    }

    public class ProjectionDefinition
    {
        /// <summary>
        /// Interface type corresponding to the object's IID
        /// </summary>
        public Type Interface { init;  get; }

        /// <summary>
        /// Projected class type by CsWinRT
        /// </summary>
        public Type ProjectedClassType { init;  get; }

        /// <summary>
        /// Clsids for each context (e.g. InProc, OutOfProc, OutOfProcDev)
        /// </summary>
        public Dictionary<ClsidContext, Guid> Clsids { init; get; }

        /// <summary>
        /// Get CLSID based on the provided context
        /// </summary>
        /// <param name="context">Context</param>
        /// <returns>CLSID for the provided context, or throw an exception if not found.</returns>
        /// <exception cref="InvalidOperationException"></exception>
        public Guid GetClsid(ClsidContext context)
        {
            if (!Clsids.ContainsKey(context))
            {
                throw new InvalidOperationException($"{ProjectedClassType.FullName} is not implemented in context {context}");
            }

            return Clsids[context];
        }

        /// <summary>
        /// Get IID corresponding to the COM object
        /// </summary>
        /// <returns>IID.</returns>
        public Guid GetIid()
        {
            return Interface.GUID;
        }
    }
}
