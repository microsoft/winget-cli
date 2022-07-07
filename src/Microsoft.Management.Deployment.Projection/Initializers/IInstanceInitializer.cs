namespace Microsoft.Management.Deployment.Projection
{
    public interface IInstanceInitializer
    {
        /// <summary>
        /// CLSID context.
        /// </summary>
        public ClsidContext Context { get; }

        /// <summary>
        /// Create an in-process or out-of process instance.
        /// </summary>
        /// <typeparam name="T">Projected class typ</typeparam>
        /// <returns>Projected class intance</returns>
        public T CreateInstance<T>() where T : new();
    }
}
