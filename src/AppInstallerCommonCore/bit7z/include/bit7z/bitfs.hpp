// This is an open source non-commercial project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITFS_HPP
#define BITFS_HPP

/* Header for forward declaring fs namespace. */

#include "bitdefines.hpp" /* For BIT7Z_USE_STANDARD_FILESYSTEM */

#ifdef BIT7Z_USE_STANDARD_FILESYSTEM
#include <filesystem>
#else
/* Notes: we use this forward declaration to avoid including private headers (e.g. fs.hpp).
 *        Since some public API headers include bitgenericitem.hpp (e.g. "bitoutputarchive.hpp"),
 *        including private headers here would result in the "leaking" out of these latter in the public API.*/
namespace ghc {
namespace filesystem {
class path;
} // namespace filesystem
} // namespace ghc
#endif

namespace bit7z {
namespace fs {
#ifdef BIT7Z_USE_STANDARD_FILESYSTEM
using namespace std::filesystem;
#else
using namespace ghc::filesystem;
#endif
} // namespace fs
} // namespace bit7z

#endif //BITFS_HPP
