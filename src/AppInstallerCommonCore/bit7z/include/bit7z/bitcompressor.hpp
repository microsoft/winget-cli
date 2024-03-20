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

#ifndef BITCOMPRESSOR_HPP
#define BITCOMPRESSOR_HPP

#include <vector>

#include "bitoutputarchive.hpp"

namespace bit7z {

using std::vector;

namespace filesystem { // NOLINT(modernize-concat-nested-namespaces)
namespace fsutil {
auto stem( const tstring& path ) -> tstring;
} // namespace fsutil
} // namespace filesystem

using namespace filesystem;

#ifdef __cpp_if_constexpr
#define BIT7Z_IF_CONSTEXPR if constexpr
#else
#define BIT7Z_IF_CONSTEXPR if
#endif

/**
 * @brief The BitCompressor template class allows compressing files into archives.
 *
 * It let decide various properties of the produced archive file, such as the password
 * protection and the compression level desired.
 */
template< typename Input >
class BitCompressor : public BitAbstractArchiveCreator {
    public:
        /**
         * @brief Constructs a BitCompressor object.
         *
         * The Bit7zLibrary parameter is needed to have access to the functionalities
         * of the 7z DLLs. On the contrary, the BitInOutFormat is required to know the
         * format of the output archive.
         *
         * @param lib       the 7z library to use.
         * @param format    the output archive format.
         */
        BitCompressor( Bit7zLibrary const& lib, BitInOutFormat const& format )
            : BitAbstractArchiveCreator( lib, format ) {}

        /**
         * @brief Compresses a single file.
         *
         * @param inFile       the file to be compressed.
         * @param outFile      the path (relative or absolute) to the output archive file.
         * @param inputName    (optional) the name to give to the compressed file inside the output archive.
         */
        void compressFile( Input inFile,
                           const tstring& outFile,
                           const tstring& inputName = {} ) const {
            /* Note: if inFile is a filesystem path (i.e., its type is const tstring&), we can deduce the archived
             * item filename using the original filename. Otherwise, if the user didn't specify the input file name,
             * we use the filename (without extension) of the output file path. */
            tstring name;
            BIT7Z_IF_CONSTEXPR( !std::is_same< Input, const tstring& >::value ) {
                name = inputName.empty() ? fsutil::stem( outFile ) : inputName;
            }

            BitOutputArchive outputArchive{ *this, outFile };
            outputArchive.addFile( inFile, name );
            outputArchive.compressTo( outFile );
        }

        /**
         * @brief Compresses the input file to the output buffer.
         *
         * @param inFile     the file to be compressed.
         * @param outBuffer  the buffer going to contain the output archive.
         * @param inputName  (optional) the name to give to the compressed file inside the output archive.
         */
        void compressFile( Input inFile,
                           vector< byte_t >& outBuffer,
                           const tstring& inputName = {} ) const {
            BitOutputArchive outputArchive{ *this, outBuffer };
            outputArchive.addFile( inFile, inputName );
            outputArchive.compressTo( outBuffer );
        }

        /**
         * @brief Compresses the input file to the output stream.
         *
         * @param inFile     the file to be compressed.
         * @param outStream  the output stream.
         * @param inputName  (optional) the name to give to the compressed file inside the output archive.
         */
        void compressFile( Input inFile,
                           ostream& outStream,
                           const tstring& inputName = {} ) const {
            BitOutputArchive outputArchive{ *this };
            outputArchive.addFile( inFile, inputName );
            outputArchive.compressTo( outStream );
        }
};

}  // namespace bit7z

#endif //BITCOMPRESSOR_HPP
