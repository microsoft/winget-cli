/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITABSTRACTARCHIVECREATOR_HPP
#define BITABSTRACTARCHIVECREATOR_HPP

#include <map>
#include <memory>

#include "bitabstractarchivehandler.hpp"
#include "bitcompressionlevel.hpp"
#include "bitcompressionmethod.hpp"
#include "bitformat.hpp"
#include "bitinputarchive.hpp"

struct IOutStream;
struct ISequentialOutStream;

namespace bit7z {

using std::ostream;

class ArchiveProperties;

/**
 * @brief Enumeration representing how an archive creator should deal when the output archive already exists.
 */
enum struct UpdateMode {
    None, ///< The creator will throw an exception (unless the OverwriteMode is not None).
    Append, ///< The creator will append the new items to the existing archive.
    Update, ///< New items whose path already exists in the archive will overwrite the old ones, other will be appended.
    BIT7Z_DEPRECATED_ENUMERATOR( Overwrite, Update, "Since v4.0; please use the UpdateMode::Update enumerator." ) ///< @deprecated since v4.0; please use the UpdateMode::Update enumerator.
};

/**
 * @brief Abstract class representing a generic archive creator.
 */
class BitAbstractArchiveCreator : public BitAbstractArchiveHandler {
    public:
        BitAbstractArchiveCreator( const BitAbstractArchiveCreator& ) = delete;

        BitAbstractArchiveCreator( BitAbstractArchiveCreator&& ) = delete;

        auto operator=( const BitAbstractArchiveCreator& ) -> BitAbstractArchiveCreator& = delete;

        auto operator=( BitAbstractArchiveCreator&& ) -> BitAbstractArchiveCreator& = delete;

        ~BitAbstractArchiveCreator() override = default;

        /**
         * @return the format used for creating/updating an archive.
         */
        BIT7Z_NODISCARD auto format() const noexcept -> const BitInFormat& override;

        /**
         * @return the format used for creating/updating an archive.
         */
        BIT7Z_NODISCARD auto compressionFormat() const noexcept -> const BitInOutFormat&;

        /**
         * @return whether the creator crypts also the headers of archives or not.
         */
        BIT7Z_NODISCARD auto cryptHeaders() const noexcept -> bool;

        /**
         * @return the compression level used for creating/updating an archive.
         */
        BIT7Z_NODISCARD auto compressionLevel() const noexcept -> BitCompressionLevel;

        /**
         * @return the compression method used for creating/updating an archive.
         */
        BIT7Z_NODISCARD auto compressionMethod() const noexcept -> BitCompressionMethod;

        /**
         * @return the dictionary size used for creating/updating an archive.
         */
        BIT7Z_NODISCARD auto dictionarySize() const noexcept -> uint32_t;

        /**
         * @return the word size used for creating/updating an archive.
         */
        BIT7Z_NODISCARD auto wordSize() const noexcept -> uint32_t;

        /**
         * @return whether the archive creator uses solid compression or not.
         */
        BIT7Z_NODISCARD auto solidMode() const noexcept -> bool;

        /**
         * @return the update mode used when updating existing archives.
         */
        BIT7Z_NODISCARD auto updateMode() const noexcept -> UpdateMode;

        /**
         * @return the volume size (in bytes) used when creating multi-volume archives
         *         (a 0 value means that all files are going in a single archive).
         */
        BIT7Z_NODISCARD auto volumeSize() const noexcept -> uint64_t;

        /**
         * @return the number of threads used when creating/updating an archive
         *         (a 0 value means that it will use the 7-zip default value).
         */
        BIT7Z_NODISCARD auto threadsCount() const noexcept -> uint32_t;

        /**
         * @return whether the archive creator stores symbolic links as links in the output archive.
         */
        BIT7Z_NODISCARD auto storeSymbolicLinks() const noexcept -> bool;

        /**
         * @brief Sets up a password for the output archives.
         *
         * When setting a password, the produced archives will be encrypted using the default
         * cryptographic method of the output format. The option "crypt headers" remains unchanged,
         * in contrast with what happens when calling the setPassword(tstring, bool) method.
         *
         * @note Calling setPassword when the output format doesn't support archive encryption
         * (e.g., GZip, BZip2, etc...) does not have any effects (in other words, it doesn't
         * throw exceptions, and it has no effects on compression operations).
         *
         * @note After a password has been set, it will be used for every subsequent operation.
         * To disable the use of the password, you need to call the clearPassword method
         * (inherited from BitAbstractArchiveHandler), which is equivalent to setPassword(L"").
         *
         * @param password the password to be used when creating/updating archives.
         */
        void setPassword( const tstring& password ) override;

        /**
         * @brief Sets up a password for the output archive.
         *
         * When setting a password, the produced archive will be encrypted using the default
         * cryptographic method of the output format. If the format is 7z, and the option
         * "cryptHeaders" is set to true, the headers of the archive will be encrypted,
         * resulting in a password request every time the output file will be opened.
         *
         * @note Calling setPassword when the output format doesn't support archive encryption
         * (e.g., GZip, BZip2, etc...) does not have any effects (in other words, it doesn't
         * throw exceptions, and it has no effects on compression operations).
         *
         * @note Calling setPassword with "cryptHeaders" set to true does not have effects on
         * formats different from 7z.
         *
         * @note After a password has been set, it will be used for every subsequent operation.
         * To disable the use of the password, you need to call the clearPassword method
         * (inherited from BitAbstractArchiveHandler), which is equivalent to setPassword(L"").
         *
         * @param password          the password to be used when creating/updating archives.
         * @param cryptHeaders     if true, the headers of the output archives will be encrypted
         *                          (valid only when using the 7z format).
         */
        void setPassword( const tstring& password, bool cryptHeaders );

        /**
         * @brief Sets the compression level to be used when creating/updating an archive.
         *
         * @param level the compression level desired.
         */
        void setCompressionLevel( BitCompressionLevel level ) noexcept;

        /**
         * @brief Sets the compression method to be used when creating/updating an archive.
         *
         * @param method the compression method desired.
         */
        void setCompressionMethod( BitCompressionMethod method );

        /**
         * @brief Sets the dictionary size to be used when creating/updating an archive.
         *
         * @param dictionarySize the dictionary size desired.
         */
        void setDictionarySize( uint32_t dictionarySize );

        /**
         * @brief Sets the word size to be used when creating/updating an archive.
         *
         * @param wordSize the word size desired.
         */
        void setWordSize( uint32_t wordSize );

        /**
         * @brief Sets whether to use solid compression or not.
         *
         * @note Setting the solid compression mode to true has effect only when using the 7z format with multiple
         * input files.
         *
         * @param solidMode    if true, it will be used the "solid compression" method.
         */
        void setSolidMode( bool solidMode ) noexcept;

        /**
         * @brief Sets whether and how the creator can update existing archives or not.
         *
         * @note If set to UpdateMode::None, a subsequent compression operation may throw an exception
         *       if it targets an existing archive.
         *
         * @param mode the desired update mode.
         */
        virtual void setUpdateMode( UpdateMode mode );

        /**
         * @brief Sets whether the creator can update existing archives or not.
         *
         * @deprecated since v4.0; it is provided just for an easier transition from the old v3 API.
         *
         * @note If set to false, a subsequent compression operation may throw an exception
         *       if it targets an existing archive.
         *
         * @param canUpdate if true, compressing operations will update existing archives.
         */
        BIT7Z_DEPRECATED_MSG( "Since v4.0; please use the overloaded function that takes an UpdateMode enumerator." )
        void setUpdateMode( bool canUpdate );

        /**
         * @brief Sets the volumeSize (in bytes) of the output archive volumes.
         *
         * @note This setting has effects only when the destination archive is on the filesystem.
         *
         * @param volumeSize    The dimension of a volume.
         */
        void setVolumeSize( uint64_t volumeSize ) noexcept;

        /**
         * @brief Sets the number of threads to be used when creating/updating an archive.
         *
         * @param threadsCount the number of threads desired.
         */
        void setThreadsCount( uint32_t threadsCount ) noexcept;

        /**
         * @brief Sets whether the creator will store symbolic links as links in the output archive.
         *
         * @param storeSymlinks    if true, symbolic links will be stored as links.
         */
        void setStoreSymbolicLinks( bool storeSymlinks ) noexcept;

        /**
         * @brief Sets a property for the output archive format as described by the 7-zip documentation
         * (e.g., https://sevenzip.osdn.jp/chm/cmdline/switches/method.htm).
         *
         * @tparam T    An integral type (i.e., a bool or an integer type).
         *
         * @param name  The string name of the property to be set.
         * @param value The value to be used for the property.
         */
        template< std::size_t N, typename T, typename = typename std::enable_if< std::is_integral< T >::value >::type >
        void setFormatProperty( const wchar_t (&name)[N], T value ) noexcept { // NOLINT(*-avoid-c-arrays)
            mExtraProperties[ name ] = value;
        }

        /**
         * @brief Sets a property for the output archive format as described by the 7-zip documentation
         * (e.g., https://sevenzip.osdn.jp/chm/cmdline/switches/method.htm).
         *
         * For example, passing the string L"tm" with a false value while creating a .7z archive
         * will disable storing the last modified timestamps of the compressed files.
         *
         * @tparam T    A non-integral type (i.e., a string).
         *
         * @param name  The string name of the property to be set.
         * @param value The value to be used for the property.
         */
        template< std::size_t N, typename T, typename = typename std::enable_if< !std::is_integral< T >::value >::type >
        void setFormatProperty( const wchar_t (&name)[N], const T& value ) noexcept { // NOLINT(*-avoid-c-arrays)
            mExtraProperties[ name ] = value;
        }

    protected:
        BitAbstractArchiveCreator( const Bit7zLibrary& lib,
                                   const BitInOutFormat& format,
                                   tstring password = {},
                                   UpdateMode updateMode = UpdateMode::None );

        BIT7Z_NODISCARD auto archiveProperties() const -> ArchiveProperties;

        friend class BitOutputArchive;

    private:
        const BitInOutFormat& mFormat;

        UpdateMode mUpdateMode;
        BitCompressionLevel mCompressionLevel;
        BitCompressionMethod mCompressionMethod;
        uint32_t mDictionarySize;
        uint32_t mWordSize;
        bool mCryptHeaders;
        bool mSolidMode;
        uint64_t mVolumeSize;
        uint32_t mThreadsCount;
        bool mStoreSymbolicLinks;
        std::map< std::wstring, BitPropVariant > mExtraProperties;
};

}  // namespace bit7z

#endif // BITABSTRACTARCHIVECREATOR_HPP
