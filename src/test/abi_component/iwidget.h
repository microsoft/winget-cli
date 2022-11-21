#pragma once

#include <pal.h>

inline constexpr xlang_guid iwidget_guid{ 0xbd6f821f, 0x66, 0x440c, { 0xb1, 0x15, 0xeb, 0x4, 0x5a, 0xfe, 0xfa, 0x4e } };

struct XLANG_NOVTABLE iwidget : xlang_unknown
{
    virtual xlang_error_info* get_answer(int32_t* answer) noexcept = 0;
};

inline constexpr xlang_guid iwidget_factory_guid{0x4419ea1c, 0xf721, 0x4aa2, { 0x82, 0xdd, 0xb2, 0xf5, 0xd2, 0xcf, 0xc6, 0x3a } };

struct XLANG_NOVTABLE iwidget_factory : xlang_unknown
{
    virtual xlang_error_info* activate_widget(iwidget** widget) noexcept = 0;
};
