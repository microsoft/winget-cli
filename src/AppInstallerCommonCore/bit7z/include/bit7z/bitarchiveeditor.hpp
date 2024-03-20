/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITARCHIVEEDITOR_HPP
#define BITARCHIVEEDITOR_HPP

#include <unordered_map>

#include "bitarchivewriter.hpp"

namespace bit7z {

using std::vector;

using EditedItems = std::unordered_map< uint32_t, BitItemsVector::value_type >;

/**
 * @brief The BitArchiveEditor class allows creating new file archives or updating old ones.
 *        Update operations supported are the addition of new items,
 *        as well as renaming/updating/deleting old items;
 *
 * @note  Changes are applied to the archive only after calling the applyChanges() method.
 */
class BIT7Z_MAYBE_UNUSED BitArchiveEditor final : public BitArchiveWriter {
    public:
        /**
         * @brief Constructs a BitArchiveEditor object, reading the given archive file path.
         *
         * @param lib      the 7z library to use.
         * @param inFile   the path to an input archive file.
         * @param format   the input/output archive format.
         * @param password (optional) the password needed to read the input archive.
         */
        BitArchiveEditor( const Bit7zLibrary& lib,
                          const tstring& inFile,
                          const BitInOutFormat& format,
                          const tstring& password = {} );

        BitArchiveEditor( const BitArchiveEditor& ) = delete;

        BitArchiveEditor( BitArchiveEditor&& ) = delete;

        auto operator=( const BitArchiveEditor& ) -> BitArchiveEditor& = delete;

        auto operator=( BitArchiveEditor&& ) -> BitArchiveEditor& = delete;

        ~BitArchiveEditor() override;

        /**
         * @brief Sets how the editor performs the update of the items in the archive.
         *
         * @note BitArchiveEditor doesn't support UpdateMode::None.
         *
         * @param mode the desired update mode (either UpdateMode::Append or UpdateMode::Overwrite).
         */
        void setUpdateMode( UpdateMode mode ) override;

        /**
         * @brief Requests to change the path of the item at the specified index with the given one.
         *
         * @param index    the index of the item to be renamed.
         * @param newPath the new path (in the archive) desired for the item.
         */
        void renameItem( uint32_t index, const tstring& newPath );

        /**
         * @brief Requests to change the path of the item from oldPath to the newPath.
         *
         * @param oldPath the old path (in the archive) of the item to be renamed.
         * @param newPath the new path (in the archive) desired for the item.
         */
        void renameItem( const tstring& oldPath, const tstring& newPath );

        /**
         * @brief Requests to update the content of the item at the specified index
         *        with the data from the given file.
         *
         * @param index     the index of the item to be updated.
         * @param inFile    the path to the file containing the new data for the item.
         */
        void updateItem( uint32_t index, const tstring& inFile );

        /**
         * @brief Requests to update the content of the item at the specified index
         *        with the data from the given buffer.
         *
         * @param index     the index of the item to be updated.
         * @param inBuffer  the buffer containing the new data for the item.
         */
        void updateItem( uint32_t index, const std::vector< byte_t >& inBuffer );

        /**
         * @brief Requests to update the content of the item at the specified index
         *        with the data from the given stream.
         *
         * @param index     the index of the item to be updated.
         * @param inStream  the stream of new data for the item.
         */
        void updateItem( uint32_t index, std::istream& inStream );

        /**
         * @brief Requests to update the content of the item at the specified path
         *        with the data from the given file.
         *
         * @param itemPath  the path (in the archive) of the item to be updated.
         * @param inFile    the path to the file containing the new data for the item.
         */
        void updateItem( const tstring& itemPath, const tstring& inFile );

        /**
         * @brief Requests to update the content of the item at the specified path
         *        with the data from the given buffer.
         *
         * @param itemPath  the path (in the archive) of the item to be updated.
         * @param inBuffer  the buffer containing the new data for the item.
         */
        void updateItem( const tstring& itemPath, const std::vector< byte_t >& inBuffer );

        /**
         * @brief Requests to update the content of the item at the specified path
         *        with the data from the given stream.
         *
         * @param itemPath  the path (in the archive) of the item to be updated.
         * @param inStream  the stream of new data for the item.
         */
        void updateItem( const tstring& itemPath, istream& inStream );

        /**
         * @brief Marks the item at the given index as deleted.
         *
         * @param index the index of the item to be deleted.
         */
        void deleteItem( uint32_t index );

        /**
         * @brief Marks the item at the given path (in the archive) as deleted.
         *
         * @param itemPath the path (in the archive) of the item to be deleted.
         */
        void deleteItem( const tstring& itemPath );

        /**
         * @brief Applies the requested changes (i.e., rename/update/delete operations) to the input archive.
         */
        void applyChanges();

    private:
        EditedItems mEditedItems;

        auto findItem( const tstring& itemPath ) -> uint32_t;

        void checkIndex( uint32_t index );

        auto itemProperty( InputIndex index, BitProperty property ) const -> BitPropVariant override;

        auto itemStream( InputIndex index, ISequentialInStream** inStream ) const -> HRESULT override;

        auto hasNewData( uint32_t index ) const noexcept -> bool override;

        auto hasNewProperties( uint32_t index ) const noexcept -> bool override;
};

}  // namespace bit7z

#endif //BITARCHIVEEDITOR_HPP
