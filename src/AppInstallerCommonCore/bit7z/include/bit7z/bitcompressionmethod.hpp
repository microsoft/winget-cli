/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITCOMPRESSIONMETHOD_HPP
#define BITCOMPRESSIONMETHOD_HPP

namespace bit7z {

/**
 * @brief The BitCompressionMethod enum represents the compression methods used by 7z when creating archives.
 */
enum struct BitCompressionMethod {
    Copy,
    Deflate,
    Deflate64,
    BZip2,
    Lzma,
    Lzma2,
    Ppmd
};

}  // namespace bit7z

#endif // BITCOMPRESSIONMETHOD_HPP
