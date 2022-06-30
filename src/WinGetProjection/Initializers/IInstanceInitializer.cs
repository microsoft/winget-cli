namespace WinGetProjection
{
    public interface IInstanceInitializer
    {
        /// <summary>
        /// Create an in or out-of process COM instance
        /// </summary>
        /// <typeparam name="T">Projected class typ</typeparam>
        /// <returns>Projected class intance</returns>
        public T CreateInstance<T>() where T : new();
    }
}
