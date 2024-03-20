/*
 * bit7z - A C++ static library to interface with the 7-zip shared libraries.
 * Copyright (c) 2014-2023 Riccardo Ostani - All Rights Reserved.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at https://mozilla.org/MPL/2.0/.
 */

#ifndef BITEXTRACTOR_HPP
#define BITEXTRACTOR_HPP

#include <algorithm>

#include "bitabstractarchiveopener.hpp"
#include "biterror.hpp"
#include "bitexception.hpp"
#include "bitinputarchive.hpp"

namespace bit7z {

namespace filesystem { // NOLINT(modernize-concat-nested-namespaces)
namespace fsutil {
auto wildcard_match( const tstring& pattern, const tstring& str ) -> bool;
} // namespace fsutil
} // namespace filesystem

/**
 * @brief The BitExtractor template class allows extracting the content of archives from supported input types.
 *
 * @tparam Input    the type of input archives that the generated extractor class supports.
 */
template< typename Input >
class BitExtractor final : public BitAbstractArchiveOpener {
    public:
        /**
         * @brief Constructs a BitExtractor object.
         *
         * The Bit7zLibrary parameter is needed to have access to the functionalities
         * of the 7z DLLs. On the contrary, the BitInFormat is required to know the
         * format of the in_file archives.
         *
         * @note When bit7z is compiled using the BIT7Z_AUTO_FORMAT macro define, the format
         * argument has the default value BitFormat::Auto (automatic format detection of the in_file archive).
         * Otherwise, when BIT7Z_AUTO_FORMAT is not defined (i.e., no auto format detection available),
         * the format argument must be specified.
         *
         * @param lib       the 7z library to use.
         * @param format    the in_file archive format.
         */
        explicit BitExtractor( const Bit7zLibrary& lib, const BitInFormat& format BIT7Z_DEFAULT_FORMAT )
            : BitAbstractArchiveOpener( lib, format ) {}

        /**
         * @brief Extracts the given archive to the chosen directory.
         *
         * @param inArchive    the input archive to be extracted.
         * @param outDir       the output directory where extracted files will be put.
         */
        void extract( Input inArchive, const tstring& outDir = {} ) const {
            BitInputArchive inputArchive( *this, inArchive );
            inputArchive.extractTo( outDir );
        }

        /**
         * @brief Extracts a file from the given archive to the output buffer.
         *
         * @param inArchive   the input archive to extract from.
         * @param outBuffer   the output buffer where the content of the extracted file will be put.
         * @param index        the index of the file to be extracted from the archive.
         */
        void extract( Input inArchive, vector< byte_t >& outBuffer, uint32_t index = 0 ) const {
            BitInputArchive inputArchive( *this, inArchive );
            inputArchive.extractTo( outBuffer, index );
        }

        /**
         * @brief Extracts a file from the given archive to the output stream.
         *
         * @param inArchive   the input archive to extract from.
         * @param outStream   the (binary) stream where the content of the extracted file will be put.
         * @param index       the index of the file to be extracted from the archive.
         */
        void extract( Input inArchive, std::ostream& outStream, uint32_t index = 0 ) const {
            BitInputArchive inputArchive( *this, inArchive );
            inputArchive.extractTo( outStream, index );
        }

        /**
         * @brief Extracts the content of the given archive into a map of memory buffers, where the keys are
         * the paths of the files (inside the archive), and the values are their decompressed contents.
         *
         * @param inArchive    the input archive to be extracted.
         * @param outMap       the output map.
         */
        void extract( Input inArchive, std::map< tstring, vector< byte_t > >& outMap ) const {
            BitInputArchive inputArchive( *this, inArchive );
            inputArchive.extractTo( outMap );
        }

        /**
         * @brief Extracts the files in the archive that match the given wildcard pattern to the chosen directory.
         *
         * @param inArchive    the input archive to extract from.
         * @param itemFilter   the wildcard pattern used for matching the paths of files inside the archive.
         * @param outDir       the output directory where extracted files will be put.
         * @param policy       the filtering policy to be applied to the matched items.
         */
        void extractMatching( Input inArchive,
                              const tstring& itemFilter,
                              const tstring& outDir = {},
                              FilterPolicy policy = FilterPolicy::Include ) const {
            using namespace filesystem;

            if ( itemFilter.empty() ) {
                throw BitException( "Cannot extract items", make_error_code( BitError::FilterNotSpecified ) );
            }

            extractMatchingFilter( inArchive, outDir, policy, [ &itemFilter ]( const tstring& itemPath ) -> bool {
                return fsutil::wildcard_match( itemFilter, itemPath );
            } );
        }

        /**
         * @brief Extracts to the output buffer the first file in the archive matching the given wildcard pattern.
         *
         * @param inArchive    the input archive to extract from.
         * @param itemFilter   the wildcard pattern used for matching the paths of files inside the archive.
         * @param outBuffer    the output buffer where to extract the file.
         * @param policy       the filtering policy to be applied to the matched items.
         */
        void extractMatching( Input inArchive,
                              const tstring& itemFilter,
                              vector< byte_t >& outBuffer,
                              FilterPolicy policy = FilterPolicy::Include ) const {
            using namespace filesystem;

            if ( itemFilter.empty() ) {
                throw BitException( "Cannot extract items", make_error_code( BitError::FilterNotSpecified ) );
            }

            extractMatchingFilter( inArchive, outBuffer, policy,
                                   [ &itemFilter ]( const tstring& itemPath ) -> bool {
                                       return fsutil::wildcard_match( itemFilter, itemPath );
                                   } );
        }

        /**
         * @brief Extracts the specified items from the given archive to the chosen directory.
         *
         * @param inArchive    the input archive to extract from.
         * @param indices      the indices of the files in the archive that should be extracted.
         * @param outDir       the output directory where the extracted files will be placed.
         */
        void extractItems( Input inArchive,
                           const std::vector< uint32_t >& indices,
                           const tstring& outDir = {} ) const {
            if ( indices.empty() ) {
                throw BitException( "Cannot extract items", make_error_code( BitError::IndicesNotSpecified ) );
            }

            BitInputArchive inputArchive( *this, inArchive );
            inputArchive.extractTo( outDir, indices );
        }

#ifdef BIT7Z_REGEX_MATCHING

        /**
         * @brief Extracts the files in the archive that match the given regex pattern to the chosen directory.
         *
         * @note Available only when compiling bit7z using the BIT7Z_REGEX_MATCHING preprocessor define.
         *
         * @param inArchive    the input archive to extract from.
         * @param regex        the regex used for matching the paths of files inside the archive.
         * @param outDir       the output directory where extracted files will be put.
         * @param policy       the filtering policy to be applied to the matched items.
         */
        void extractMatchingRegex( Input inArchive,
                                   const tstring& regex,
                                   const tstring& outDir = {},
                                   FilterPolicy policy = FilterPolicy::Include ) const {
            if ( regex.empty() ) {
                throw BitException( "Cannot extract items", make_error_code( BitError::FilterNotSpecified ) );
            }

            const tregex regexFilter( regex, tregex::ECMAScript | tregex::optimize );
            extractMatchingFilter( inArchive, outDir, policy, [ &regexFilter ]( const tstring& itemPath ) -> bool {
                return std::regex_match( itemPath, regexFilter );
            } );
        }

        /**
         * @brief Extracts the first file in the archive that matches the given regex pattern to the output buffer.
         *
         * @note Available only when compiling bit7z using the BIT7Z_REGEX_MATCHING preprocessor define.
         *
         * @param inArchive    the input archive to extract from.
         * @param regex        the regex used for matching the paths of files inside the archive.
         * @param outBuffer    the output buffer where the extracted file will be put.
         * @param policy       the filtering policy to be applied to the matched items.
         */
        void extractMatchingRegex( Input inArchive,
                                   const tstring& regex,
                                   vector< byte_t >& outBuffer,
                                   FilterPolicy policy = FilterPolicy::Include ) const {
            if ( regex.empty() ) {
                throw BitException( "Cannot extract items", make_error_code( BitError::FilterNotSpecified ) );
            }

            const tregex regexFilter( regex, tregex::ECMAScript | tregex::optimize );
            return extractMatchingFilter( inArchive, outBuffer, policy,
                                          [ &regexFilter ]( const tstring& itemPath ) -> bool {
                                              return std::regex_match( itemPath, regexFilter );
                                          } );
        }

#endif

        /**
         * @brief Tests the given archive without extracting its content.
         *
         * If the archive is not valid, a BitException is thrown!
         *
         * @param inArchive   the input archive to be tested.
         */
        void test( Input inArchive ) const {
            BitInputArchive inputArchive( *this, inArchive );
            inputArchive.test();
        }

    private:
        void extractMatchingFilter( Input inArchive,
                                    const tstring& outDir,
                                    FilterPolicy policy,
                                    const std::function< bool( const tstring& ) >& filter ) const {
            BitInputArchive inputArchive( *this, inArchive );

            vector< uint32_t > matchedIndices;
            const bool shouldExtractMatchedItems = policy == FilterPolicy::Include;
            // Searching for files inside the archive that match the given filter
            for ( const auto& item : inputArchive ) {
                const bool itemMatches = filter( item.path() );
                if ( itemMatches == shouldExtractMatchedItems ) {
                    /* The if-condition is equivalent to an exclusive XNOR (negated XOR) between
                     * itemMatches and shouldExtractMatchedItems.
                     * In other words, it is true only if the current item either:
                     *  - matches the filter, and we must include any matching item; or
                     *  - doesn't match the filter, and we must exclude those that match. */
                    matchedIndices.push_back( item.index() );
                }
            }

            if ( matchedIndices.empty() ) {
                throw BitException( "Cannot extract items", make_error_code( BitError::NoMatchingItems ) );
            }

            inputArchive.extractTo( outDir, matchedIndices );
        }

        void extractMatchingFilter( Input inArchive,
                                    vector< byte_t >& outBuffer,
                                    FilterPolicy policy,
                                    const std::function< bool( const tstring& ) >& filter ) const {
            BitInputArchive inputArchive( *this, inArchive );

            const bool shouldExtractMatchedItem = policy == FilterPolicy::Include;
            // Searching for files inside the archive that match the given filter
            for ( const auto& item : inputArchive ) {
                const bool itemMatches = filter( item.path() );
                if ( itemMatches == shouldExtractMatchedItem ) {
                    /* The if-condition is equivalent to an exclusive NOR (negated XOR) between
                     *  itemMatches and shouldExtractMatchedItem. */
                    inputArchive.extractTo( outBuffer, item.index() );
                    return;
                }
            }

            throw BitException( "Failed to extract items", make_error_code( BitError::NoMatchingItems ) );
        }
};

}  // namespace bit7z

#endif //BITEXTRACTOR_HPP
