/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITCOMPRESSIONLEVEL_HPP
#define BITCOMPRESSIONLEVEL_HPP

namespace bit7z {

/**
 * @brief The BitCompressionLevel enum represents the compression level used by 7z when creating archives.
 * @note It uses the same values used by [7-zip](https://sevenzip.osdn.jp/chm/cmdline/switches/method.htm#ZipX).
 */
enum struct BitCompressionLevel {
    None = 0,    ///< Copy mode (no compression)
    Fastest = 1, ///< Fastest compressing
    Fast = 3,    ///< Fast compressing
    Normal = 5,  ///< Normal compressing
    Max = 7,     ///< Maximum compressing
    Ultra = 9    ///< Ultra compressing
};

}  // namespace bit7z

#endif // BITCOMPRESSIONLEVEL_HPP
