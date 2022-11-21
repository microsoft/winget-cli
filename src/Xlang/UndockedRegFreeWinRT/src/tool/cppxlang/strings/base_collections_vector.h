
namespace xlang
{
    template <typename T, typename Allocator = std::allocator<T>>
    Foundation::Collections::IVector<T> single_threaded_vector(std::vector<T, Allocator>&& values = {})
    {
        return make<impl::input_vector<T, std::vector<T, Allocator>>>(std::move(values));
    }
}
