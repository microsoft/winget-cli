/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITFILECOMPRESSOR_HPP
#define BITFILECOMPRESSOR_HPP

#include <map>
#include <ostream>
#include <vector>

#include "bitcompressor.hpp"

namespace bit7z {

using std::vector;
using std::map;
using std::ostream;

using namespace filesystem;

/**
 * @brief The BitFileCompressor class allows compressing files and directories.
 * The compressed archives can be saved to the filesystem, standard streams, or memory buffers.
 *
 * It let decide various properties of the produced archive, such as the password
 * protection and the compression level desired.
 */
class BitFileCompressor final : public BitCompressor< const tstring& > {
    public:
        /**
         * @brief Constructs a BitFileCompressor object.
         *
         * The Bit7zLibrary parameter is needed to have access to the functionalities
         * of the 7z DLLs. On the contrary, the BitInOutFormat is required to know the
         * format of the output archive.
         *
         * @param lib       the 7z library used.
         * @param format    the output archive format.
         */
        BitFileCompressor( const Bit7zLibrary& lib, const BitInOutFormat& format );

        /* Compression from the file system to the file system. */

        /**
         * @brief Compresses the given files or directories.
         *
         * The items in the first argument must be the relative or absolute paths to files or
         * directories existing on the filesystem.
         *
         * @param inPaths  a vector of paths.
         * @param outFile  the path (relative or absolute) to the output archive file.
         */
        void compress( const std::vector< tstring >& inPaths, const tstring& outFile ) const;

        /**
         * @brief Compresses the given files or directories using the specified aliases.
         *
         * The items in the first argument must be the relative or absolute paths to files or
         * directories existing on the filesystem.
         * Each pair in the map must follow the following format:
         *  {"path to file in the filesystem", "alias path in the archive"}.
         *
         * @param inPaths  a map of paths and corresponding aliases.
         * @param outFile  the path (relative or absolute) to the output archive file.
         */
        void compress( const std::map< tstring, tstring >& inPaths, const tstring& outFile ) const;

        /**
         * @brief Compresses a group of files.
         *
         * @note Any path to a directory or to a not-existing file will be ignored!
         *
         * @param inFiles  the path (relative or absolute) to the input files.
         * @param outFile  the path (relative or absolute) to the output archive file.
         */
        void compressFiles( const std::vector< tstring >& inFiles, const tstring& outFile ) const;

        /**
         * @brief Compresses the files contained in a directory.
         *
         * @param inDir        the path (relative or absolute) to the input directory.
         * @param outFile      the path (relative or absolute) to the output archive file.
         * @param recursive    (optional) if true, it searches files inside the sub-folders of inDir.
         * @param filter       (optional) the filter to use when searching files inside inDir.
         */
        void compressFiles( const tstring& inDir,
                            const tstring& outFile,
                            bool recursive = true,
                            const tstring& filter = BIT7Z_STRING( "*" ) ) const;

        /**
         * @brief Compresses an entire directory.
         *
         * @note This method is equivalent to compressFiles with filter set to L"".
         *
         * @param inDir    the path (relative or absolute) to the input directory.
         * @param outFile  the path (relative or absolute) to the output archive file.
         */
        void compressDirectory( const tstring& inDir, const tstring& outFile ) const;

        /* Compression from the file system to standard streams. */

        /**
         * @brief Compresses the given files or directories.
         *
         * The items in the first argument must be the relative or absolute paths to files or
         * directories existing on the filesystem.
         *
         * @param inPaths      a vector of paths.
         * @param outStream    the standard ostream where the archive will be output.
         */
        void compress( const std::vector< tstring >& inPaths, std::ostream& outStream ) const;

        /**
         * @brief Compresses the given files or directories using the specified aliases.
         *
         * The items in the first argument must be the relative or absolute paths to files or
         * directories existing on the filesystem.
         * Each pair in the map must follow the following format:
         *  {"path to file in the filesystem", "alias path in the archive"}.
         *
         * @param inPaths      a map of paths and corresponding aliases.
         * @param outStream    the standard ostream where to output the archive file.
         */
        void compress( const std::map< tstring, tstring >& inPaths, std::ostream& outStream ) const;
};

}  // namespace bit7z
#endif // BITFILECOMPRESSOR_HPP
