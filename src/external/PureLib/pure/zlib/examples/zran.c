/* zran.c -- example of zlib/gzip stream indexing and random access
 * Copyright (C) 2005, 2012, 2018 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 * Version 1.2  14 Oct 2018  Mark Adler */

/* Version History:
 1.0  29 May 2005  First version
 1.1  29 Sep 2012  Fix memory reallocation error
 1.2  14 Oct 2018  Handle gzip streams with multiple members
                   Add a header file to facilitate usage in applications
 */

/* Illustrate the use of Z_BLOCK, inflatePrime(), and inflateSetDictionary()
   for random access of a compressed file.  A file containing a zlib or gzip
   stream is provided on the command line.  The compressed stream is decoded in
   its entirety, and an index built with access points about every SPAN bytes
   in the uncompressed output.  The compressed file is left open, and can then
   be read randomly, having to decompress on the average SPAN/2 uncompressed
   bytes before getting to the desired block of data.

   An access point can be created at the start of any deflate block, by saving
   the starting file offset and bit of that block, and the 32K bytes of
   uncompressed data that precede that block.  Also the uncompressed offset of
   that block is saved to provide a reference for locating a desired starting
   point in the uncompressed stream.  deflate_index_build() works by
   decompressing the input zlib or gzip stream a block at a time, and at the
   end of each block deciding if enough uncompressed data has gone by to
   justify the creation of a new access point.  If so, that point is saved in a
   data structure that grows as needed to accommodate the points.

   To use the index, an offset in the uncompressed data is provided, for which
   the latest access point at or preceding that offset is located in the index.
   The input file is positioned to the specified location in the index, and if
   necessary the first few bits of the compressed data is read from the file.
   inflate is initialized with those bits and the 32K of uncompressed data, and
   the decompression then proceeds until the desired offset in the file is
   reached.  Then the decompression continues to read the desired uncompressed
   data from the file.

   Another approach would be to generate the index on demand.  In that case,
   requests for random access reads from the compressed data would try to use
   the index, but if a read far enough past the end of the index is required,
   then further index entries would be generated and added.

   There is some fair bit of overhead to starting inflation for the random
   access, mainly copying the 32K byte dictionary.  So if small pieces of the
   file are being accessed, it would make sense to implement a cache to hold
   some lookahead and avoid many calls to deflate_index_extract() for small
   lengths.

   Another way to build an index would be to use inflateCopy().  That would
   not be constrained to have access points at block boundaries, but requires
   more memory per access point, and also cannot be saved to file due to the
   use of pointers in the state.  The approach here allows for storage of the
   index in a file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "zlib.h"
#include "zran.h"

#define WINSIZE 32768U      /* sliding window size */
#define CHUNK 16384         /* file input buffer size */

/* Access point entry. */
struct point {
    off_t out;          /* corresponding offset in uncompressed data */
    off_t in;           /* offset in input file of first full byte */
    int bits;           /* number of bits (1-7) from byte at in-1, or 0 */
    unsigned char window[WINSIZE];  /* preceding 32K of uncompressed data */
};

/* See comments in zran.h. */
void deflate_index_free(struct deflate_index *index)
{
    if (index != NULL) {
        free(index->list);
        free(index);
    }
}

/* Add an entry to the access point list. If out of memory, deallocate the
   existing list and return NULL. index->gzip is the allocated size of the
   index in point entries, until it is time for deflate_index_build() to
   return, at which point gzip is set to indicate a gzip file or not.
 */
static struct deflate_index *addpoint(struct deflate_index *index, int bits,
                                      off_t in, off_t out, unsigned left,
                                      unsigned char *window)
{
    struct point *next;

    /* if list is empty, create it (start with eight points) */
    if (index == NULL) {
        index = malloc(sizeof(struct deflate_index));
        if (index == NULL) return NULL;
        index->list = malloc(sizeof(struct point) << 3);
        if (index->list == NULL) {
            free(index);
            return NULL;
        }
        index->gzip = 8;
        index->have = 0;
    }

    /* if list is full, make it bigger */
    else if (index->have == index->gzip) {
        index->gzip <<= 1;
        next = realloc(index->list, sizeof(struct point) * index->gzip);
        if (next == NULL) {
            deflate_index_free(index);
            return NULL;
        }
        index->list = next;
    }

    /* fill in entry and increment how many we have */
    next = (struct point *)(index->list) + index->have;
    next->bits = bits;
    next->in = in;
    next->out = out;
    if (left)
        memcpy(next->window, window + WINSIZE - left, left);
    if (left < WINSIZE)
        memcpy(next->window + left, window, WINSIZE - left);
    index->have++;

    /* return list, possibly reallocated */
    return index;
}

/* See comments in zran.h. */
int deflate_index_build(FILE *in, off_t span, struct deflate_index **built)
{
    int ret;
    int gzip = 0;               /* true if reading a gzip file */
    off_t totin, totout;        /* our own total counters to avoid 4GB limit */
    off_t last;                 /* totout value of last access point */
    struct deflate_index *index;    /* access points being generated */
    z_stream strm;
    unsigned char input[CHUNK];
    unsigned char window[WINSIZE];

    /* initialize inflate */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, 47);      /* automatic zlib or gzip decoding */
    if (ret != Z_OK)
        return ret;

    /* inflate the input, maintain a sliding window, and build an index -- this
       also validates the integrity of the compressed data using the check
       information in the gzip or zlib stream */
    totin = totout = last = 0;
    index = NULL;               /* will be allocated by first addpoint() */
    strm.avail_out = 0;
    do {
        /* get some compressed data from input file */
        strm.avail_in = fread(input, 1, CHUNK, in);
        if (ferror(in)) {
            ret = Z_ERRNO;
            goto deflate_index_build_error;
        }
        if (strm.avail_in == 0) {
            ret = Z_DATA_ERROR;
            goto deflate_index_build_error;
        }
        strm.next_in = input;

        /* check for a gzip stream */
        if (totin == 0 && strm.avail_in >= 3 &&
            input[0] == 31 && input[1] == 139 && input[2] == 8)
            gzip = 1;

        /* process all of that, or until end of stream */
        do {
            /* reset sliding window if necessary */
            if (strm.avail_out == 0) {
                strm.avail_out = WINSIZE;
                strm.next_out = window;
            }

            /* inflate until out of input, output, or at end of block --
               update the total input and output counters */
            totin += strm.avail_in;
            totout += strm.avail_out;
            ret = inflate(&strm, Z_BLOCK);      /* return at end of block */
            totin -= strm.avail_in;
            totout -= strm.avail_out;
            if (ret == Z_NEED_DICT)
                ret = Z_DATA_ERROR;
            if (ret == Z_MEM_ERROR || ret == Z_DATA_ERROR)
                goto deflate_index_build_error;
            if (ret == Z_STREAM_END) {
                if (gzip &&
                    (strm.avail_in || ungetc(getc(in), in) != EOF)) {
                    ret = inflateReset(&strm);
                    if (ret != Z_OK)
                        goto deflate_index_build_error;
                    continue;
                }
                break;
            }

            /* if at end of block, consider adding an index entry (note that if
               data_type indicates an end-of-block, then all of the
               uncompressed data from that block has been delivered, and none
               of the compressed data after that block has been consumed,
               except for up to seven bits) -- the totout == 0 provides an
               entry point after the zlib or gzip header, and assures that the
               index always has at least one access point; we avoid creating an
               access point after the last block by checking bit 6 of data_type
             */
            if ((strm.data_type & 128) && !(strm.data_type & 64) &&
                (totout == 0 || totout - last > span)) {
                index = addpoint(index, strm.data_type & 7, totin,
                                 totout, strm.avail_out, window);
                if (index == NULL) {
                    ret = Z_MEM_ERROR;
                    goto deflate_index_build_error;
                }
                last = totout;
            }
        } while (strm.avail_in != 0);
    } while (ret != Z_STREAM_END);

    /* clean up and return index (release unused entries in list) */
    (void)inflateEnd(&strm);
    index->list = realloc(index->list, sizeof(struct point) * index->have);
    index->gzip = gzip;
    index->length = totout;
    *built = index;
    return index->have;

    /* return error */
  deflate_index_build_error:
    (void)inflateEnd(&strm);
    deflate_index_free(index);
    return ret;
}

/* See comments in zran.h. */
int deflate_index_extract(FILE *in, struct deflate_index *index, off_t offset,
                          unsigned char *buf, int len)
{
    int ret, skip;
    z_stream strm;
    struct point *here;
    unsigned char input[CHUNK];
    unsigned char discard[WINSIZE];

    /* proceed only if something reasonable to do */
    if (len < 0)
        return 0;

    /* find where in stream to start */
    here = index->list;
    ret = index->have;
    while (--ret && here[1].out <= offset)
        here++;

    /* initialize file and inflate state to start there */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, -15);         /* raw inflate */
    if (ret != Z_OK)
        return ret;
    ret = fseeko(in, here->in - (here->bits ? 1 : 0), SEEK_SET);
    if (ret == -1)
        goto deflate_index_extract_ret;
    if (here->bits) {
        ret = getc(in);
        if (ret == -1) {
            ret = ferror(in) ? Z_ERRNO : Z_DATA_ERROR;
            goto deflate_index_extract_ret;
        }
        (void)inflatePrime(&strm, here->bits, ret >> (8 - here->bits));
    }
    (void)inflateSetDictionary(&strm, here->window, WINSIZE);

    /* skip uncompressed bytes until offset reached, then satisfy request */
    offset -= here->out;
    strm.avail_in = 0;
    skip = 1;                               /* while skipping to offset */
    do {
        /* define where to put uncompressed data, and how much */
        if (offset > WINSIZE) {             /* skip WINSIZE bytes */
            strm.avail_out = WINSIZE;
            strm.next_out = discard;
            offset -= WINSIZE;
        }
        else if (offset > 0) {              /* last skip */
            strm.avail_out = (unsigned)offset;
            strm.next_out = discard;
            offset = 0;
        }
        else if (skip) {                    /* at offset now */
            strm.avail_out = len;
            strm.next_out = buf;
            skip = 0;                       /* only do this once */
        }

        /* uncompress until avail_out filled, or end of stream */
        do {
            if (strm.avail_in == 0) {
                strm.avail_in = fread(input, 1, CHUNK, in);
                if (ferror(in)) {
                    ret = Z_ERRNO;
                    goto deflate_index_extract_ret;
                }
                if (strm.avail_in == 0) {
                    ret = Z_DATA_ERROR;
                    goto deflate_index_extract_ret;
                }
                strm.next_in = input;
            }
            ret = inflate(&strm, Z_NO_FLUSH);       /* normal inflate */
            if (ret == Z_NEED_DICT)
                ret = Z_DATA_ERROR;
            if (ret == Z_MEM_ERROR || ret == Z_DATA_ERROR)
                goto deflate_index_extract_ret;
            if (ret == Z_STREAM_END) {
                /* the raw deflate stream has ended */
                if (index->gzip == 0)
                    /* this is a zlib stream that has ended -- done */
                    break;

                /* near the end of a gzip member, which might be followed by
                   another gzip member -- skip the gzip trailer and see if
                   there is more input after it */
                if (strm.avail_in < 8) {
                    fseeko(in, 8 - strm.avail_in, SEEK_CUR);
                    strm.avail_in = 0;
                }
                else {
                    strm.avail_in -= 8;
                    strm.next_in += 8;
                }
                if (strm.avail_in == 0 && ungetc(getc(in), in) == EOF)
                    /* the input ended after the gzip trailer -- done */
                    break;

                /* there is more input, so another gzip member should follow --
                   validate and skip the gzip header */
                ret = inflateReset2(&strm, 31);
                if (ret != Z_OK)
                    goto deflate_index_extract_ret;
                do {
                    if (strm.avail_in == 0) {
                        strm.avail_in = fread(input, 1, CHUNK, in);
                        if (ferror(in)) {
                            ret = Z_ERRNO;
                            goto deflate_index_extract_ret;
                        }
                        if (strm.avail_in == 0) {
                            ret = Z_DATA_ERROR;
                            goto deflate_index_extract_ret;
                        }
                        strm.next_in = input;
                    }
                    ret = inflate(&strm, Z_BLOCK);
                    if (ret == Z_MEM_ERROR || ret == Z_DATA_ERROR)
                        goto deflate_index_extract_ret;
                } while ((strm.data_type & 128) == 0);

                /* set up to continue decompression of the raw deflate stream
                   that follows the gzip header */
                ret = inflateReset2(&strm, -15);
                if (ret != Z_OK)
                    goto deflate_index_extract_ret;
            }

            /* continue to process the available input before reading more */
        } while (strm.avail_out != 0);

        if (ret == Z_STREAM_END)
            /* reached the end of the compressed data -- return the data that
               was available, possibly less than requested */
            break;

        /* do until offset reached and requested data read */
    } while (skip);

    /* compute the number of uncompressed bytes read after the offset */
    ret = skip ? 0 : len - strm.avail_out;

    /* clean up and return the bytes read, or the negative error */
  deflate_index_extract_ret:
    (void)inflateEnd(&strm);
    return ret;
}

#ifdef TEST

#define SPAN 1048576L       /* desired distance between access points */
#define LEN 16384           /* number of bytes to extract */

/* Demonstrate the use of deflate_index_build() and deflate_index_extract() by
   processing the file provided on the command line, and extracting LEN bytes
   from 2/3rds of the way through the uncompressed output, writing that to
   stdout. An offset can be provided as the second argument, in which case the
   data is extracted from there instead. */
int main(int argc, char **argv)
{
    int len;
    off_t offset = -1;
    FILE *in;
    struct deflate_index *index = NULL;
    unsigned char buf[LEN];

    /* open input file */
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: zran file.gz [offset]\n");
        return 1;
    }
    in = fopen(argv[1], "rb");
    if (in == NULL) {
        fprintf(stderr, "zran: could not open %s for reading\n", argv[1]);
        return 1;
    }

    /* get optional offset */
    if (argc == 3) {
        char *end;
        offset = strtoll(argv[2], &end, 10);
        if (*end || offset < 0) {
            fprintf(stderr, "zran: %s is not a valid offset\n", argv[2]);
            return 1;
        }
    }

    /* build index */
    len = deflate_index_build(in, SPAN, &index);
    if (len < 0) {
        fclose(in);
        switch (len) {
        case Z_MEM_ERROR:
            fprintf(stderr, "zran: out of memory\n");
            break;
        case Z_DATA_ERROR:
            fprintf(stderr, "zran: compressed data error in %s\n", argv[1]);
            break;
        case Z_ERRNO:
            fprintf(stderr, "zran: read error on %s\n", argv[1]);
            break;
        default:
            fprintf(stderr, "zran: error %d while building index\n", len);
        }
        return 1;
    }
    fprintf(stderr, "zran: built index with %d access points\n", len);

    /* use index by reading some bytes from an arbitrary offset */
    if (offset == -1)
        offset = (index->length << 1) / 3;
    len = deflate_index_extract(in, index, offset, buf, LEN);
    if (len < 0)
        fprintf(stderr, "zran: extraction failed: %s error\n",
                len == Z_MEM_ERROR ? "out of memory" : "input corrupted");
    else {
        fwrite(buf, 1, len, stdout);
        fprintf(stderr, "zran: extracted %d bytes at %llu\n", len, offset);
    }

    /* clean up and exit */
    deflate_index_free(index);
    fclose(in);
    return 0;
}

#endif
