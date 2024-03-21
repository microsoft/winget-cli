/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITARCHIVEREADER_HPP
#define BITARCHIVEREADER_HPP

#include "bitabstractarchiveopener.hpp"
#include "bitarchiveiteminfo.hpp"
#include "bitexception.hpp"
#include "bitinputarchive.hpp"

struct IInArchive;
struct IOutArchive;
struct IArchiveExtractCallback;

namespace bit7z {

/**
 * @brief The BitArchiveReader class allows reading metadata of archives, as well as extracting them.
 */
class BitArchiveReader final : public BitAbstractArchiveOpener, public BitInputArchive {
    public:
        /**
         * @brief Constructs a BitArchiveReader object, opening the input file archive.
         *
         * @note When bit7z is compiled using the `BIT7Z_AUTO_FORMAT` option, the format
         * argument has the default value BitFormat::Auto (automatic format detection of the input archive).
         * On the contrary, when `BIT7Z_AUTO_FORMAT` is not defined (i.e., no auto format detection available),
         * the format argument must be specified.
         *
         * @param lib           the 7z library used.
         * @param inArchive     the path to the archive to be read.
         * @param format        the format of the input archive.
         * @param password      the password needed for opening the input archive.
         */
        BitArchiveReader( const Bit7zLibrary& lib,
                          const tstring& inArchive,
                          const BitInFormat& format BIT7Z_DEFAULT_FORMAT,
                          const tstring& password = {} );

        /**
         * @brief Constructs a BitArchiveReader object, opening the archive in the input buffer.
         *
         * @note When bit7z is compiled using the `BIT7Z_AUTO_FORMAT` option, the format
         * argument has the default value BitFormat::Auto (automatic format detection of the input archive).
         * On the contrary, when `BIT7Z_AUTO_FORMAT` is not defined (i.e., no auto format detection available),
         * the format argument must be specified.
         *
         * @param lib           the 7z library used.
         * @param inArchive     the input buffer containing the archive to be read.
         * @param format        the format of the input archive.
         * @param password      the password needed for opening the input archive.
         */
        BitArchiveReader( const Bit7zLibrary& lib,
                          const std::vector< byte_t >& inArchive,
                          const BitInFormat& format BIT7Z_DEFAULT_FORMAT,
                          const tstring& password = {} );

        /**
         * @brief Constructs a BitArchiveReader object, opening the archive from the standard input stream.
         *
         * @note When bit7z is compiled using the `BIT7Z_AUTO_FORMAT` option, the format
         * argument has the default value BitFormat::Auto (automatic format detection of the input archive).
         * On the contrary, when `BIT7Z_AUTO_FORMAT` is not defined (i.e., no auto format detection available),
         * the format argument must be specified.
         *
         * @param lib           the 7z library used.
         * @param inArchive     the standard input stream of the archive to be read.
         * @param format        the format of the input archive.
         * @param password      the password needed for opening the input archive.
         */
        BitArchiveReader( const Bit7zLibrary& lib,
                          std::istream& inArchive,
                          const BitInFormat& format BIT7Z_DEFAULT_FORMAT,
                          const tstring& password = {} );

        BitArchiveReader( const BitArchiveReader& ) = delete;

        BitArchiveReader( BitArchiveReader&& ) = delete;

        auto operator=( const BitArchiveReader& ) -> BitArchiveReader& = delete;

        auto operator=( BitArchiveReader&& ) -> BitArchiveReader& = delete;

        /**
         * @brief BitArchiveReader destructor.
         *
         * @note It releases the input archive file.
         */
        ~BitArchiveReader() override = default;

        /**
         * @return a map of all the available (i.e., non-empty) archive properties and their respective values.
         */
        BIT7Z_NODISCARD auto archiveProperties() const -> map< BitProperty, BitPropVariant >;

        /**
         * @return a vector of all the archive items as BitArchiveItem objects.
         */
        BIT7Z_NODISCARD auto items() const -> vector< BitArchiveItemInfo >;

        /**
         * @return the number of folders contained in the archive.
         */
        BIT7Z_NODISCARD auto foldersCount() const -> uint32_t;

        /**
         * @return the number of files contained in the archive.
         */
        BIT7Z_NODISCARD auto filesCount() const -> uint32_t;

        /**
         * @return the total uncompressed size of the archive content.
         */
        BIT7Z_NODISCARD auto size() const -> uint64_t;

        /**
         * @return the total compressed size of the archive content.
         */
        BIT7Z_NODISCARD auto packSize() const -> uint64_t;

        /**
         * @return true if and only if the archive has at least one encrypted item.
         */
        BIT7Z_NODISCARD auto hasEncryptedItems() const -> bool;

        /**
         * @return true if and only if the archive has only encrypted items.
         */
        BIT7Z_NODISCARD auto isEncrypted() const -> bool;

        /**
         * @return the number of volumes composing the archive.
         */
        BIT7Z_NODISCARD auto volumesCount() const -> uint32_t;

        /**
         * @return true if and only if the archive is composed by multiple volumes.
         */
        BIT7Z_NODISCARD auto isMultiVolume() const -> bool;

        /**
         * @return true if and only if the archive was created using solid compression.
         */
        BIT7Z_NODISCARD auto isSolid() const -> bool;

        /**
         * Checks if the given archive is header-encrypted or not.
         *
         * @tparam T The input type of the archive (i.e., file path, buffer, or standard stream).
         *
         * @param lib           the 7z library used.
         * @param inArchive     the archive to be read.
         * @param format        the format of the input archive.
         *
         * @return true if and only if the archive has at least one encrypted item.
         */
        template< typename T >
        BIT7Z_NODISCARD
        static auto isHeaderEncrypted( const Bit7zLibrary& lib,
                                       T&& inArchive,
                                       const BitInFormat& format BIT7Z_DEFAULT_FORMAT ) -> bool {
            try {
                const BitArchiveReader reader{ lib, std::forward< T >( inArchive ), format };
                return false;
            } catch ( const BitException& ex ) {
                return isOpenEncryptedError( ex.code() );
            }
        }

        /**
         * Checks if the given archive contains only encrypted items.
         *
         * @note A header-encrypted archive is also encrypted, but the contrary is not generally true.
         *
         * @note An archive might contain both plain and encrypted files; in this case, this function will
         * return false.
         *
         * @tparam T The input type of the archive (i.e., file path, buffer, or standard stream).
         *
         * @param lib           the 7z library used.
         * @param inArchive     the archive to be read.
         * @param format        the format of the input archive.
         *
         * @return true if and only if the archive has only encrypted items.
         */
        template< typename T >
        BIT7Z_NODISCARD
        static auto isEncrypted( const Bit7zLibrary& lib,
                                 T&& inArchive,
                                 const BitInFormat& format BIT7Z_DEFAULT_FORMAT ) -> bool {
            try {
                const BitArchiveReader reader{ lib, std::forward< T >( inArchive ), format };
                return reader.isEncrypted();
            } catch ( const BitException& ex ) {
                return isOpenEncryptedError( ex.code() );
            }
        }

    private:
        static auto isOpenEncryptedError( std::error_code error ) -> bool;
};

BIT7Z_DEPRECATED_TYPEDEF( BitArchiveInfo, BitArchiveReader, "Since v4.0; please use BitArchiveReader." );

}  // namespace bit7z

#endif // BITARCHIVEREADER_HPP
