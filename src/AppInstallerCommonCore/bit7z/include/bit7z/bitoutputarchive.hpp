/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITOUTPUTARCHIVE_HPP
#define BITOUTPUTARCHIVE_HPP

#include <istream>
#include <set>

#include "bitabstractarchivecreator.hpp"
#include "bititemsvector.hpp"
#include "bitexception.hpp" //for FailedFiles
#include "bitpropvariant.hpp"

//! @cond IGNORE_BLOCK_IN_DOXYGEN
struct ISequentialInStream;

template< typename T >
class CMyComPtr;
//! @endcond

namespace bit7z {

using std::istream;

using DeletedItems = std::set< uint32_t >;

/* General note: I tried my best to explain how indices work here, but it is a bit complex. */

/* We introduce a strong index type to differentiate between indices in the output
 * archive (uint32_t, as used by the UpdateCallback), and the corresponding indexes
 * in the input archive (InputIndex). In this way, we avoid implicit conversions
 * between the two kinds of indices.
 *
 * UpdateCallback uses indices in the range [0, BitOutputArchive::itemsCount() - 1]
 *
 * Now, if the user doesn't delete any item in the input archive, itemsCount()
 * is just equal to <n° of items in the input archive> + <n° of newly added items>.
 * In this case, an InputIndex value is just equal to the index used by UpdateCallback.
 *
 * On the contrary, if the user wants to delete an item in the input archive, the value
 * of an InputIndex may differ from the corresponding UpdateCallback's index.
 *
 * Note: given an InputIndex i:
 *         if i < mInputArchiveItemsCount, the item is old (old item in the input archive);
 *         if i >= mInputArchiveItemsCount, the item is new (added by the user); */
enum class InputIndex : std::uint32_t {};

class UpdateCallback;

/**
 * @brief The BitOutputArchive class, given a creator object, allows creating new archives.
 */
class BitOutputArchive {
    public:
        /**
         * @brief Constructs a BitOutputArchive object for a completely new archive.
         *
          * @param creator  the reference to the BitAbstractArchiveCreator object containing all the settings to
         *                  be used for creating the new archive.
         */
        explicit BitOutputArchive( const BitAbstractArchiveCreator& creator );

        /**
         * @brief Constructs a BitOutputArchive object, opening an (optional) input file archive.
         *
         * If a non-empty input file path is passed, the corresponding archive will be opened and
         * used as a base for the creation of the new archive. Otherwise, the class will behave
         * as if it is creating a completely new archive.
         *
         * @param creator the reference to the BitAbstractArchiveCreator object containing all the settings to
         *                be used for creating the new archive and reading the (optional) input archive.
         * @param inFile  (optional) the path to an input archive file.
         */
        explicit BitOutputArchive( const BitAbstractArchiveCreator& creator, const tstring& inFile );

        /**
         * @brief Constructs a BitOutputArchive object, opening an input file archive from the given buffer.
         *
         * If a non-empty input buffer is passed, the archive file it contains will be opened and
         * used as a base for the creation of the new archive. Otherwise, the class will behave
         * as if it is creating a completely new archive.
         *
         * @param creator   the reference to the BitAbstractArchiveCreator object containing all the settings to
         *                  be used for creating the new archive and reading the (optional) input archive.
         * @param inBuffer  the buffer containing an input archive file.
         */
        BitOutputArchive( const BitAbstractArchiveCreator& creator, const std::vector< byte_t >& inBuffer );

        /**
         * @brief Constructs a BitOutputArchive object, reading an input file archive from the given std::istream.
         *
         * @param creator   the reference to the BitAbstractArchiveCreator object containing all the settings to
         *                  be used for creating the new archive and reading the (optional) input archive.
         * @param inStream  the standard input stream of the input archive file.
         */
        BitOutputArchive( const BitAbstractArchiveCreator& creator, std::istream& inStream );

        BitOutputArchive( const BitOutputArchive& ) = delete;

        BitOutputArchive( BitOutputArchive&& ) = delete;

        auto operator=( const BitOutputArchive& ) -> BitOutputArchive& = delete;

        auto operator=( BitOutputArchive&& ) -> BitOutputArchive& = delete;

        /**
         * @brief Adds all the items that can be found by indexing the given vector of filesystem paths.
         *
         * @param inPaths the vector of filesystem paths.
         */
        void addItems( const std::vector< tstring >& inPaths );

        /**
         * @brief Adds all the items that can be found by indexing the keys of the given map of filesystem paths;
         *       the corresponding mapped values are the user-defined paths wanted inside the output archive.
         *
         * @param inPaths map of filesystem paths with the corresponding user-defined path desired inside the
         *                 output archive.
         */
        void addItems( const std::map< tstring, tstring >& inPaths );

        /**
         * @brief Adds the given file path, with an optional user-defined path to be used in the output archive.
         *
         * @note If a directory path is given, a BitException is thrown.
         *
         * @param inFile the path to the filesystem file to be added to the output archive.
         * @param name    (optional) user-defined path to be used inside the output archive.
         */
        void addFile( const tstring& inFile, const tstring& name = {} );

        /**
         * @brief Adds the given buffer file, using the given name as a path when compressed in the output archive.
         *
         * @param inBuffer  the buffer containing the file to be added to the output archive.
         * @param name      user-defined path to be used inside the output archive.
         */
        void addFile( const std::vector< byte_t >& inBuffer, const tstring& name );

        /**
         * @brief Adds the given standard input stream, using the given name as a path when compressed
         *        in the output archive.
         *
         * @param inStream  the input stream to be added.
         * @param name      the name of the file inside the output archive.
         */
        void addFile( std::istream& inStream, const tstring& name );

        /**
         * @brief Adds all the files in the given vector of filesystem paths.
         *
         * @note Paths to directories are ignored.
         *
         * @param inFiles the vector of paths to files.
         */
        void addFiles( const std::vector< tstring >& inFiles );

        /**
         * @brief Adds all the files inside the given directory path that match the given wildcard filter.
         *
         * @param inDir     the directory where to search for files to be added to the output archive.
         * @param filter    (optional) the wildcard filter to be used for searching the files.
         * @param recursive (optional) recursively search the files in the given directory
         *                  and all of its subdirectories.
         */
        void addFiles( const tstring& inDir,
                       const tstring& filter = BIT7Z_STRING( "*" ),
                       bool recursive = true );

        /**
         * @brief Adds all the files inside the given directory path that match the given wildcard filter.
         *
         * @param inDir     the directory where to search for files to be added to the output archive.
         * @param filter    (optional) the wildcard filter to be used for searching the files.
         * @param recursive (optional) recursively search the files in the given directory
         *                  and all of its subdirectories.
         * @param policy    (optional) the filtering policy to be applied to the matched items.
         */
        void addFiles( const tstring& inDir,
                       const tstring& filter = BIT7Z_STRING( "*" ),
                       FilterPolicy policy = FilterPolicy::Include,
                       bool recursive = true );

        /**
         * @brief Adds all the items inside the given directory path.
         *
         * @param inDir the directory where to search for items to be added to the output archive.
         */
        void addDirectory( const tstring& inDir );

        /**
         * @brief Compresses all the items added to this object to the specified archive file path.
         *
         * @note If this object was created by passing an input archive file path, and this latter is the same as
         * the outFile path parameter, the file will be updated.
         *
         * @param outFile the output archive file path.
         */
        void compressTo( const tstring& outFile );

        /**
         * @brief Compresses all the items added to this object to the specified buffer.
         *
         * @param outBuffer the output buffer.
         */
        void compressTo( std::vector< byte_t >& outBuffer );

        /**
         * @brief Compresses all the items added to this object to the specified buffer.
         *
         * @param outStream the output standard stream.
         */
        void compressTo( std::ostream& outStream );

        /**
         * @return the total number of items added to the output archive object.
         */
        auto itemsCount() const -> uint32_t;

        /**
         * @return a constant reference to the BitAbstractArchiveHandler object containing the
         *         settings for writing the output archive.
         */
        auto handler() const noexcept -> const BitAbstractArchiveHandler&;

        /**
         * @return a constant reference to the BitAbstractArchiveHandler object containing the
         *         settings for writing the output archive.
         */
        auto creator() const noexcept -> const BitAbstractArchiveCreator&;

        /**
         * @brief Default destructor.
         */
        virtual ~BitOutputArchive() = default;

    protected:
        virtual auto itemProperty( InputIndex index, BitProperty property ) const -> BitPropVariant;

        virtual auto itemStream( InputIndex index, ISequentialInStream** inStream ) const -> HRESULT;

        virtual auto hasNewData( uint32_t index ) const noexcept -> bool;

        virtual auto hasNewProperties( uint32_t index ) const noexcept -> bool;

        auto itemInputIndex( uint32_t newIndex ) const noexcept -> InputIndex;

        auto outputItemProperty( uint32_t index, BitProperty property ) const -> BitPropVariant;

        auto outputItemStream( uint32_t index, ISequentialInStream** inStream ) const -> HRESULT;

        auto indexInArchive( uint32_t index ) const noexcept -> uint32_t;

        inline auto inputArchive() const -> BitInputArchive* {
            return mInputArchive.get();
        }

        inline void setInputArchive( std::unique_ptr< BitInputArchive >&& inputArchive ) {
            mInputArchive = std::move( inputArchive );
        }

        inline auto inputArchiveItemsCount() const -> uint32_t {
            return mInputArchiveItemsCount;
        }

        inline void setDeletedIndex( uint32_t index ) {
            mDeletedItems.insert( index );
        }

        inline auto isDeletedIndex( uint32_t index ) const -> bool {
            return mDeletedItems.find( index ) != mDeletedItems.cend();
        }

        inline auto hasDeletedIndexes() const -> bool {
            return !mDeletedItems.empty();
        }

        inline auto hasNewItems() const -> bool {
            return mNewItemsVector.size() > 0;
        }

        friend class UpdateCallback;

    private:
        const BitAbstractArchiveCreator& mArchiveCreator;

        unique_ptr< BitInputArchive > mInputArchive;
        uint32_t mInputArchiveItemsCount;

        BitItemsVector mNewItemsVector;
        DeletedItems mDeletedItems;

        mutable FailedFiles mFailedFiles;

        /* mInputIndices:
         *  - Position i = index in range [0, itemsCount() - 1] used by UpdateCallback.
         *  - Value at position i = corresponding index in the input archive (type InputIndex).
         *
         * If there are some deleted items, then i != mInputIndices[i]
         * (at least for values of i greater than the index of the first deleted item).
         *
         * Otherwise, if there are no deleted items, the vector is empty, and itemInputIndex(i)
         * will return InputIndex with value i.
         *
         * This vector is either empty, or it has size equal to itemsCount() (thanks to updateInputIndices()). */
        std::vector< InputIndex > mInputIndices;

        auto initOutArchive() const -> CMyComPtr< IOutArchive >;

        auto initOutFileStream( const fs::path& outArchive, bool updatingArchive ) const -> CMyComPtr< IOutStream >;

        BitOutputArchive( const BitAbstractArchiveCreator& creator, const fs::path& inArc );

        void compressToFile( const fs::path& outFile, UpdateCallback* updateCallback );

        void compressOut( IOutArchive* outArc, IOutStream* outStream, UpdateCallback* updateCallback );

        void setArchiveProperties( IOutArchive* outArchive ) const;

        void updateInputIndices();
};

}  // namespace bit7z

#endif //BITOUTPUTARCHIVE_HPP
