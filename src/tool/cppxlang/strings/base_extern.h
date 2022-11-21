
#ifdef _WIN32

extern "C"
{
    int32_t  XLANG_CALL XLANG_IIDFromString(wchar_t const* string, xlang::guid* iid) noexcept;
    void     XLANG_CALL XLANG_GetCurrentThreadStackLimits(uintptr_t* low_limit, uintptr_t* high_limit) noexcept;
    uint32_t XLANG_CALL XLANG_GetLastError() noexcept;
    int32_t  XLANG_CALL XLANG_CoGetObjectContext(xlang::guid const& iid, void** object) noexcept;

    uint32_t XLANG_CALL XLANG_WaitForSingleObject(void* handle, uint32_t milliseconds) noexcept;
    int32_t  XLANG_CALL XLANG_TrySubmitThreadpoolCallback(void(XLANG_CALL *callback)(void*, void* context), void* context, void*) noexcept;
    xlang::impl::ptp_timer XLANG_CALL XLANG_CreateThreadpoolTimer(void(XLANG_CALL *callback)(void*, void* context, void*), void* context, void*) noexcept;
    void     XLANG_CALL XLANG_SetThreadpoolTimer(xlang::impl::ptp_timer timer, void* time, uint32_t period, uint32_t window) noexcept;
    void     XLANG_CALL XLANG_CloseThreadpoolTimer(xlang::impl::ptp_timer timer) noexcept;
    xlang::impl::ptp_wait XLANG_CALL XLANG_CreateThreadpoolWait(void(XLANG_CALL *callback)(void*, void* context, void*, uint32_t result), void* context, void*) noexcept;
    void     XLANG_CALL XLANG_SetThreadpoolWait(xlang::impl::ptp_wait wait, void* handle, void* timeout) noexcept;
    void     XLANG_CALL XLANG_CloseThreadpoolWait(xlang::impl::ptp_wait wait) noexcept;
}

XLANG_LINK(IIDFromString, 8)
XLANG_LINK(GetCurrentThreadStackLimits, 8)
XLANG_LINK(GetLastError, 0)
XLANG_LINK(CoGetObjectContext, 8)

XLANG_LINK(WaitForSingleObject, 8)
XLANG_LINK(TrySubmitThreadpoolCallback, 12)
XLANG_LINK(CreateThreadpoolTimer, 12)
XLANG_LINK(SetThreadpoolTimer, 16)
XLANG_LINK(CloseThreadpoolTimer, 4)
XLANG_LINK(CreateThreadpoolWait, 12)
XLANG_LINK(SetThreadpoolWait, 12)
XLANG_LINK(CloseThreadpoolWait, 4)

#endif

extern "C"
{
    int32_t XLANG_CALL XLANG_GetActivationFactory(void* classId, void** factory) noexcept;
}
