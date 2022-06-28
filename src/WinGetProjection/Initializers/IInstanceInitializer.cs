namespace WinGetProjection
{
    public interface IInstanceInitializer
    {
        public T CreateInstance<T>() where T : new();
    }
}
