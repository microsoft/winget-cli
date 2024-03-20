/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITMEMEXTRACTOR_HPP
#define BITMEMEXTRACTOR_HPP

#include "bitextractor.hpp"

namespace bit7z {

/**
 * @brief The BitMemExtractor alias allows extracting the content of in-memory archives.
 */
using BitMemExtractor BIT7Z_MAYBE_UNUSED = BitExtractor< const std::vector< byte_t >& >;

} // namespace bit7z

#endif // BITMEMEXTRACTOR_HPP
