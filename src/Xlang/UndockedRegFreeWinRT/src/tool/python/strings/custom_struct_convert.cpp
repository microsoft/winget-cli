

static int64_t custom_get(winrt::Windows::Foundation::DateTime const& instance) 
{
    return instance.time_since_epoch().count();
}

static void custom_set(winrt::Windows::Foundation::DateTime& instance, int64_t value) 
{
    instance = winrt::Windows::Foundation::DateTime{ winrt::Windows::Foundation::TimeSpan{ value } };
}

static int64_t custom_get(winrt::Windows::Foundation::TimeSpan const& instance) 
{
    return instance.count();
}

static void custom_set(winrt::Windows::Foundation::TimeSpan& instance, int64_t value) 
{
    instance = winrt::Windows::Foundation::TimeSpan{ value };
}

static int64_t custom_get(winrt::event_token const& instance) 
{
    return instance.value;
}

static void custom_set(winrt::event_token& instance, int64_t value) 
{
    instance.value = value;
}

static int32_t custom_get(winrt::hresult const& instance) 
{
    return instance;
}

static void custom_set(winrt::hresult& instance, int32_t value) 
{
    instance = value;
}