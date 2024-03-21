/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */
#ifndef BITINPUTARCHIVE_HPP
#define BITINPUTARCHIVE_HPP

#include <array>
#include <map>

#include "bitabstractarchivehandler.hpp"
#include "bitarchiveitemoffset.hpp"
#include "bitformat.hpp"
#include "bitfs.hpp"

struct IInStream;
struct IInArchive;
struct IOutArchive;

namespace bit7z {

using std::vector;

/**
 * @brief The BitInputArchive class, given a handler object, allows reading/extracting the content of archives.
 */
class BitInputArchive {
    public:
        /**
         * @brief Constructs a BitInputArchive object, opening the input file archive.
         *
         * @param handler  the reference to the BitAbstractArchiveHandler object containing all the settings to
         *                 be used for reading the input archive
         * @param inFile   the path to the input archive file
         */
        BitInputArchive( const BitAbstractArchiveHandler& handler, const tstring& inFile );

        /**
         * @brief Constructs a BitInputArchive object, opening the input file archive.
         *
         * @param handler  the reference to the BitAbstractArchiveHandler object containing all the settings to
         *                 be used for reading the input archive
         * @param arcPath  the path to the input archive file
         */
        BitInputArchive( const BitAbstractArchiveHandler& handler, const fs::path& arcPath );

        /**
         * @brief Constructs a BitInputArchive object, opening the archive given in the input buffer.
         *
         * @param handler  the reference to the BitAbstractArchiveHandler object containing all the settings to
         *                 be used for reading the input archive
         * @param inBuffer the buffer containing the input archive
         */
        BitInputArchive( const BitAbstractArchiveHandler& handler, const std::vector< byte_t >& inBuffer );

        /**
         * @brief Constructs a BitInputArchive object, opening the archive by reading the given input stream.
         *
         * @param handler  the reference to the BitAbstractArchiveHandler object containing all the settings to
         *                 be used for reading the input archive
         * @param inStream the standard input stream of the input archive
         */
        BitInputArchive( const BitAbstractArchiveHandler& handler, std::istream& inStream );

        BitInputArchive( const BitInputArchive& ) = delete;

        BitInputArchive( BitInputArchive&& ) = delete;

        auto operator=( const BitInputArchive& ) -> BitInputArchive& = delete;

        auto operator=( BitInputArchive&& ) -> BitInputArchive& = delete;

        virtual ~BitInputArchive();

        /**
         * @return the detected format of the file.
         */
        BIT7Z_NODISCARD auto detectedFormat() const noexcept -> const BitInFormat&;

        /**
         * @brief Gets the specified archive property.
         *
         * @param property  the property to be retrieved.
         *
         * @return the current value of the archive property or an empty BitPropVariant if no value is specified.
         */
        BIT7Z_NODISCARD auto archiveProperty( BitProperty property ) const -> BitPropVariant;

        /**
         * @brief Gets the specified property of an item in the archive.
         *
         * @param index     the index (in the archive) of the item.
         * @param property  the property to be retrieved.
         *
         * @return the current value of the item property or an empty BitPropVariant if the item has no value for
         * the property.
         */
        BIT7Z_NODISCARD auto itemProperty( uint32_t index, BitProperty property ) const -> BitPropVariant;

        /**
         * @return the number of items contained in the archive.
         */
        BIT7Z_NODISCARD auto itemsCount() const -> uint32_t;

        /**
         * @param index the index of an item in the archive.
         *
         * @return true if and only if the item at the given index is a folder.
         */
        BIT7Z_NODISCARD auto isItemFolder( uint32_t index ) const -> bool;

        /**
         * @param index the index of an item in the archive.
         *
         * @return true if and only if the item at the given index is encrypted.
         */
        BIT7Z_NODISCARD auto isItemEncrypted( uint32_t index ) const -> bool;

        /**
         * @return the path to the archive (the empty string for buffer/stream archives).
         */
        BIT7Z_NODISCARD auto archivePath() const noexcept -> const tstring&;

        /**
         * @return the BitAbstractArchiveHandler object containing the settings for reading the archive.
         */
        BIT7Z_NODISCARD auto handler() const noexcept -> const BitAbstractArchiveHandler&;

        BIT7Z_DEPRECATED_MSG("Since v4.0; please, use the extractTo method.")
        inline void extract( const tstring& outDir, const std::vector< uint32_t >& indices = {} ) const {
            extractTo( outDir, indices );
        }

        /**
         * @brief Extracts the archive to the chosen directory.
         *
         * @param outDir   the output directory where the extracted files will be put.
         */
        void extractTo( const tstring& outDir ) const;

        /**
         * @brief Extracts the specified items to the chosen directory.
         *
         * @param outDir   the output directory where the extracted files will be put.
         * @param indices  the array of indices of the files in the archive that must be extracted.
         */
        void extractTo( const tstring& outDir, const std::vector< uint32_t >& indices ) const;

        BIT7Z_DEPRECATED_MSG("Since v4.0; please, use the extractTo method.")
        inline void extract( std::vector< byte_t >& outBuffer, uint32_t index = 0 ) const {
            extractTo( outBuffer, index );
        }

        /**
         * @brief Extracts a file to the output buffer.
         *
         * @param outBuffer   the output buffer where the content of the archive will be put.
         * @param index       the index of the file to be extracted.
         */
        void extractTo( std::vector< byte_t >& outBuffer, uint32_t index = 0 ) const;

        template< std::size_t N >
        BIT7Z_DEPRECATED_MSG("Since v4.0; please, use the extractTo method.")
        void extract( std::array< byte_t, N >& buffer, uint32_t index = 0 ) const {
            extractTo( buffer.data(), buffer.size(), index );
        }

        /**
         * @brief Extracts a file to the pre-allocated output buffer.
         *
         * @tparam N     the size of the output buffer (it must be equal to the unpacked size
         *               of the item to be extracted).
         * @param buffer the pre-allocated output buffer.
         * @param index  the index of the file to be extracted.
         */
        template< std::size_t N >
        void extractTo( std::array< byte_t, N >& buffer, uint32_t index = 0 ) const {
            extractTo( buffer.data(), buffer.size(), index );
        }

        template< std::size_t N >
        BIT7Z_DEPRECATED_MSG("Since v4.0; please, use the extractTo method.")
        void extract( byte_t (& buffer)[N], uint32_t index = 0 ) const { // NOLINT(*-avoid-c-arrays)
            extractTo( buffer, N, index );
        }

        /**
         * @brief Extracts a file to the pre-allocated output buffer.
         *
         * @tparam N     the size of the output buffer (it must be equal to the unpacked size
         *               of the item to be extracted).
         * @param buffer the pre-allocated output buffer.
         * @param index  the index of the file to be extracted.
         */
        template< std::size_t N >
        void extractTo( byte_t (& buffer)[N], uint32_t index = 0 ) const { // NOLINT(*-avoid-c-arrays)
            extractTo( buffer, N, index );
        }

        BIT7Z_DEPRECATED_MSG("Since v4.0; please, use the extractTo method.")
        inline void extract( byte_t* buffer, std::size_t size, uint32_t index = 0 ) const {
            extractTo( buffer, size, index );
        }

        /**
         * @brief Extracts a file to the pre-allocated output buffer.
         *
         * @param buffer the pre-allocated output buffer.
         * @param size   the size of the output buffer (it must be equal to the unpacked size
         *               of the item to be extracted).
         * @param index  the index of the file to be extracted.
         */
        void extractTo( byte_t* buffer, std::size_t size, uint32_t index = 0 ) const;

        BIT7Z_DEPRECATED_MSG("Since v4.0; please, use the extractTo method.")
        inline void extract( std::ostream& outStream, uint32_t index = 0 ) const {
            extractTo( outStream, index );
        }

        /**
         * @brief Extracts a file to the output stream.
         *
         * @param outStream   the (binary) stream where the content of the archive will be put.
         * @param index       the index of the file to be extracted.
         */
        void extractTo( std::ostream& outStream, uint32_t index = 0 ) const;

        BIT7Z_DEPRECATED_MSG("Since v4.0; please, use the extractTo method.")
        inline void extract( std::map< tstring, std::vector< byte_t > >& outMap ) const {
            extractTo( outMap );
        }

        /**
         * @brief Extracts the content of the archive to a map of memory buffers, where the keys are the paths
         * of the files (inside the archive), and the values are their decompressed contents.
         *
         * @param outMap   the output map.
         */
        void extractTo( std::map< tstring, std::vector< byte_t > >& outMap ) const;

        /**
         * @brief Tests the archive without extracting its content.
         *
         * If the archive is not valid, a BitException is thrown!
         */
        void test() const;

        /**
         * @brief Tests the item at the given index inside the archive without extracting it.
         *
         * If the archive is not valid, or there's no item at the given index, a BitException is thrown!
         *
         * @param index  the index of the file to be tested.
         */
        void testItem( uint32_t index ) const;

    protected:
        auto initUpdatableArchive( IOutArchive** newArc ) const -> HRESULT;

        BIT7Z_NODISCARD auto close() const noexcept -> HRESULT;

        friend class BitAbstractArchiveOpener;

        friend class BitAbstractArchiveCreator;

        friend class BitOutputArchive;

    private:
        IInArchive* mInArchive;
        const BitInFormat* mDetectedFormat;
        const BitAbstractArchiveHandler& mArchiveHandler;
        tstring mArchivePath;

        auto openArchiveStream( const fs::path& name, IInStream* inStream ) -> IInArchive*;

    public:
        /**
         * @brief An iterator for the elements contained in an archive.
         */
        class ConstIterator {
            public:
                // iterator traits
                using iterator_category BIT7Z_MAYBE_UNUSED = std::input_iterator_tag;
                using value_type BIT7Z_MAYBE_UNUSED = BitArchiveItemOffset;
                using reference = const BitArchiveItemOffset&;
                using pointer = const BitArchiveItemOffset*;
                using difference_type BIT7Z_MAYBE_UNUSED = uint32_t; //so that count_if returns an uint32_t

                /**
                 * @brief Advances the iterator to the next element in the archive.
                 *
                 * @return the iterator pointing to the next element in the archive.
                 */
                auto operator++() noexcept -> ConstIterator&;

                /**
                 * @brief Advances the iterator to the next element in the archive.
                 *
                 * @return the iterator before the advancement.
                 */
                auto operator++( int ) noexcept -> ConstIterator; // NOLINT(cert-dcl21-cpp)

                /**
                 * @brief Compares the iterator with another iterator.
                 *
                 * @param other Another iterator.
                 *
                 * @return whether the two iterators point to the same element in the archive or not.
                 */
                auto operator==( const ConstIterator& other ) const noexcept -> bool;

                /**
                 * @brief Compares the iterator with another iterator.
                 *
                 * @param other Another iterator.
                 *
                 * @return whether the two iterators point to the different elements in the archive or not.
                 */
                auto operator!=( const ConstIterator& other ) const noexcept -> bool;

                /**
                 * @brief Accesses the pointed-to element in the archive.
                 *
                 * @return a reference to the pointed-to element in the archive.
                 */
                auto operator*() const noexcept -> reference;

                /**
                 * @brief Accesses the pointed-to element in the archive.
                 *
                 * @return a pointer to the pointed-to element in the archive.
                 */
                auto operator->() const noexcept -> pointer;

            private:
                BitArchiveItemOffset mItemOffset;

                ConstIterator( uint32_t itemIndex, const BitInputArchive& itemArchive ) noexcept;

                friend class BitInputArchive;
        };

        BIT7Z_DEPRECATED_TYPEDEF( const_iterator, ConstIterator, "Use ConstIterator" );

        /**
         * @return an iterator to the first element of the archive; if the archive is empty,
         *         the returned iterator will be equal to the end() iterator.
         */
        BIT7Z_NODISCARD auto begin() const noexcept -> BitInputArchive::ConstIterator;

        /**
         * @return an iterator to the element following the last element of the archive;
         *         this element acts as a placeholder: attempting to access it results in undefined behavior.
         */
        BIT7Z_NODISCARD auto end() const noexcept -> BitInputArchive::ConstIterator;

        /**
         * @return an iterator to the first element of the archive; if the archive is empty,
         *         the returned iterator will be equal to the end() iterator.
         */
        BIT7Z_NODISCARD auto cbegin() const noexcept -> BitInputArchive::ConstIterator;

        /**
         * @return an iterator to the element following the last element of the archive;
         *         this element acts as a placeholder: attempting to access it results in undefined behavior.
         */
        BIT7Z_NODISCARD auto cend() const noexcept -> BitInputArchive::ConstIterator;

        /**
         * @brief Find an item in the archive that has the given path.
         *
         * @param path the path to be searched in the archive.
         *
         * @return an iterator to the item with the given path, or an iterator equal to the end() iterator
         * if no item is found.
         */
        BIT7Z_NODISCARD auto find( const tstring& path ) const noexcept -> BitInputArchive::ConstIterator;

        /**
         * @brief Find if there is an item in the archive that has the given path.
         *
         * @param path the path to be searched in the archive.
         *
         * @return true if and only if an item with the given path exists in the archive.
         */
        BIT7Z_NODISCARD auto contains( const tstring& path ) const noexcept -> bool;

        /**
         * @brief Retrieve the item at the given index.
         *
         * @param index the index of the item to be retrieved.
         *
         * @return the item at the given index within the archive.
         */
        BIT7Z_NODISCARD auto itemAt( uint32_t index ) const -> BitArchiveItemOffset;
};

}  // namespace bit7z

#endif //BITINPUTARCHIVE_HPP
