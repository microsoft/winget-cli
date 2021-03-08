/***
 * Copyright (C) Microsoft. All rights reserved.
 * Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
 *
 * =+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *
 * Defines an RAII container for setting global locale.
 *
 * =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 ****/

#include <locale>

namespace tests
{
namespace common
{
namespace utilities
{
class locale_guard
{
public:
    locale_guard(std::locale const& loc) { m_prev = std::locale::global(loc); }
    ~locale_guard() { std::locale::global(m_prev); }

private:
    std::locale m_prev;
    locale_guard(locale_guard const&);
    locale_guard& operator=(locale_guard const&);
};

} // namespace utilities
} // namespace common
} // namespace tests
