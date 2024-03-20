/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITARCHIVEWRITER_HPP
#define BITARCHIVEWRITER_HPP

#include "bitoutputarchive.hpp"

namespace bit7z {

/**
 * @brief The BitArchiveWriter class allows creating new archives or updating old ones with new items.
 */
class BitArchiveWriter : public BitAbstractArchiveCreator, public BitOutputArchive {
    public:
        /**
         * @brief Constructs an empty BitArchiveWriter object that can write archives of the specified format.
         *
         * @param lib    the 7z library to use.
         * @param format the output archive format.
         */
        BitArchiveWriter( const Bit7zLibrary& lib, const BitInOutFormat& format );

        /**
         * @brief Constructs a BitArchiveWriter object, reading the given archive file path.
         *
         * @param lib           the 7z library to use.
         * @param inArchive     the path to an input archive file.
         * @param format        the input/output archive format.
         * @param password      (optional) the password needed to read the input archive.
         */
        BitArchiveWriter( const Bit7zLibrary& lib,
                          const tstring& inArchive,
                          const BitInOutFormat& format,
                          const tstring& password = {} );

        /**
         * @brief Constructs a BitArchiveWriter object, reading the archive in the given buffer.
         *
         * @param lib           the 7z library to use.
         * @param inArchive     the buffer containing the input archive.
         * @param format        the input/output archive format.
         * @param password      (optional) the password needed to read the input archive.
         */
        BitArchiveWriter( const Bit7zLibrary& lib,
                          const std::vector< byte_t >& inArchive,
                          const BitInOutFormat& format,
                          const tstring& password = {} );

        /**
         * @brief Constructs a BitArchiveWriter object, reading the archive from the given standard input stream.
         *
         * @param lib           the 7z library to use.
         * @param inArchive     the standard stream of the input archive.
         * @param format        the input/output archive format.
         * @param password      (optional) the password needed to read the input archive.
         */
        BitArchiveWriter( const Bit7zLibrary& lib,
                          std::istream& inArchive,
                          const BitInOutFormat& format,
                          const tstring& password = {} );
};

}  // namespace bit7z

#endif //BITARCHIVEWRITER_HPP
