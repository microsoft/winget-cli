/* zran.h -- example of zlib/gzip stream indexing and random access
 * Copyright (C) 2005, 2012, 2018 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 * Version 1.2  14 Oct 2018  Mark Adler */

#include <stdio.h>
#include "zlib.h"

/* Access point list. */
struct deflate_index {
    int have;           /* number of list entries */
    int gzip;           /* 1 if the index is of a gzip file, 0 if it is of a
                           zlib stream */
    off_t length;       /* total length of uncompressed data */
    void *list;         /* allocated list of entries */
};

/* Make one entire pass through a zlib or gzip compressed stream and build an
   index, with access points about every span bytes of uncompressed output.
   gzip files with multiple members are indexed in their entirety. span should
   be chosen to balance the speed of random access against the memory
   requirements of the list, about 32K bytes per access point. The return value
   is the number of access points on success (>= 1), Z_MEM_ERROR for out of
   memory, Z_DATA_ERROR for an error in the input file, or Z_ERRNO for a file
   read error. On success, *built points to the resulting index. */
int deflate_index_build(FILE *in, off_t span, struct deflate_index **built);

/* Deallocate an index built by deflate_index_build() */
void deflate_index_free(struct deflate_index *index);

/* Use the index to read len bytes from offset into buf. Return bytes read or
   negative for error (Z_DATA_ERROR or Z_MEM_ERROR). If data is requested past
   the end of the uncompressed data, then deflate_index_extract() will return a
   value less than len, indicating how much was actually read into buf. This
   function should not return a data error unless the file was modified since
   the index was generated, since deflate_index_build() validated all of the
   input. deflate_index_extract() will return Z_ERRNO if there is an error on
   reading or seeking the input file. */
int deflate_index_extract(FILE *in, struct deflate_index *index, off_t offset,
                          unsigned char *buf, int len);
