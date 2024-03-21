/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITARCHIVEITEM_HPP
#define BITARCHIVEITEM_HPP

#include "bitgenericitem.hpp"

namespace bit7z {

/**
 * The BitArchiveItem class represents a generic item inside an archive.
 */
class BitArchiveItem : public BitGenericItem {
    public:
        /**
         * @return the index of the item in the archive.
         */
        BIT7Z_NODISCARD auto index() const noexcept -> uint32_t;

        /**
         * @return true if and only if the item is a directory (i.e., it has the property BitProperty::IsDir).
         */
        BIT7Z_NODISCARD auto isDir() const -> bool override;

        /**
         * @return true if and only if the item is a symbolic link (either has a non-empty BitProperty::SymLink,
         *         or it has POSIX/Win32 symbolic link file attributes).
         */
        BIT7Z_NODISCARD auto isSymLink() const -> bool override;

        /**
         * @return the item's name; if not available, it tries to get it from the element's path or,
         *         if not possible, it returns an empty string.
         */
        BIT7Z_NODISCARD auto name() const -> tstring override;

        /**
         * @return the extension of the item, if available or if it can be inferred from the name;
         *         otherwise it returns an empty string (e.g., when the item is a folder).
         */
        BIT7Z_NODISCARD auto extension() const -> tstring;

        /**
         * @return the path of the item in the archive, if available or inferable from the name, or an empty string
         *         otherwise.
         */
        BIT7Z_NODISCARD auto path() const -> tstring override;

        /**
         * @return the uncompressed size of the item.
         */
        BIT7Z_NODISCARD auto size() const -> uint64_t override;

        /**
         * @return the item creation time.
         */
        BIT7Z_NODISCARD auto creationTime() const -> time_type;

        /**
         * @return the item last access time.
         */
        BIT7Z_NODISCARD auto lastAccessTime() const -> time_type;

        /**
         * @return the item last write time.
         */
        BIT7Z_NODISCARD auto lastWriteTime() const -> time_type;

        /**
         * @return the item attributes.
         */
        BIT7Z_NODISCARD auto attributes() const -> uint32_t override;

        /**
         * @return the compressed size of the item.
         */
        BIT7Z_NODISCARD auto packSize() const -> uint64_t;

        /**
         * @return the CRC value of the item.
         */
        BIT7Z_NODISCARD auto crc() const -> uint32_t;

        /**
         * @return true if and only if the item is encrypted.
         */
        BIT7Z_NODISCARD auto isEncrypted() const -> bool;

    protected:
        uint32_t mItemIndex; //Note: it is not const since the subclass BitArchiveItemOffset can increment it!

        explicit BitArchiveItem( uint32_t itemIndex ) noexcept;
};

}  // namespace bit7z

#endif // BITARCHIVEITEM_HPP
