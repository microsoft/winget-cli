# @ronomon/pure

Pure is a **static analysis file format checker** that checks ZIP files for dangerous compression ratios, spec deviations, malicious archive signatures, mismatching local and central directory headers, ambiguous UTF-8 filenames, directory and symlink traversals, invalid MS-DOS dates, overlapping headers, overflow, underflow, sparseness, accidental buffer bleeds etc.

Pure's goal is to **narrow the semantic gap available to attackers** attempting to exploit vulnerable software, and to **reduce the probability of zero-days**, for example David Fifield's [A better zip bomb](https://www.usenix.org/system/files/woot19-paper_fifield_0.pdf), which was [detected by an early version of Pure as a zero-day](https://news.ycombinator.com/item?id=20352439).

## Acknowledgements

Pure was commissioned and sponsored by the **Product Release And Security Team** at **Microsoft**.

A special thanks to **Maxim Vainstein** and his team at Microsoft for their vision and encouragement.

Looking to the future, we want to add support for more archive containers (GZ, TAR, RAR etc.) and other file formats (MS-CFBF or OOXML Office files, RTF, PDF, image formats etc.). Please contact [Joran Dirk Greef](mailto:joran@ronomon.com) if you want to support new file formats in Pure.

## Files

* **pure.h**: C interface to library source code.

* **pure**: CLI script for development testing. Usage: `./pure <file>`.

* **test.js**: Node.js test runner to run Pure against all test files in `./tests`.

* **binding.c**: Node.js binding used by CLI and test runner.

* **make-errors.js**: Script to recreate C error enums, error codes and error strings dynamically.

* **make-signatures.js**: Script to recreate C signature strings dynamically.

* **make-tests.js**: Script to recreate test files dynamically.

## Installation

```shell
npm install @ronomon/pure
```

## Usage

At the command line, you can run Pure on any zip file:

```shell
$ ./pure <file.zip>
```

For example:

```shell
$ ./pure samples/zbsm.zip
PURE_E_ZIP_BOMB_FIFIELD: zip bomb: local file header overlap (see research by David Fifield)
```

As an embedded C library, **pure.h** provides:

```c
int pure_zip(
  const uint8_t* buffer, // Zip file buffer
  const uint64_t size,   // Size of zip file buffer in bytes
  const uint64_t flags   // Bit flags (optional)
)

int pure_zip_bomb(const int error)

const char* pure_error_code(const int error)

const char* pure_error_string(const int error)
```

* **pure_zip()** returns a non-zero error return code, or a zero return code if the zip file is clean and has no file format anomalies. A buffer instead of a path is passed to pure_zip() for portability, for reduced surface area for bugs, and to avoid forcing unnecessary IO if file contents are already in memory.

* **pure_zip_bomb()** returns 1 if the error return code indicates a zip bomb, otherwise 0.

* **pure_error_code()** returns the constant name of the error return code.

* **pure_error_string()** returns the error message string corresponding the error return code.

## File Format Anomalies

Pure detects **more than 150 zip file format anomalies**.

Pure provides defenses against directory and symlink traversal exploits, dangerous unix mode permissions, parser ambiguity, and many other defenses against known (and unknown) exploits, including zip bombs and buffer overflows.

For example, Pure detects **all variants of known (and unknown) zip bombs**:

1. Overlapping local file headers (cf. [David Fifield](https://www.usenix.org/system/files/woot19-paper_fifield_0.pdf)).
2. Quines (cf. [Russ Cox](https://research.swtch.com/zip)).
3. Dangerous compression ratios for individual files and across an archive.
4. "Lying" zip metadata vs actual compressed data.
5. Excessive archive recursion.
6. Excessive number of files.

As a static analysis file format checker, Pure reduces the surface area for zero-day exploits by orders of magnitude. **If a file can get past Pure, it's "pure"... or at least 99% pure**.

Here is the exhaustive list of what Pure can already detect:

* PURE_E_SIZE_MAX
* PURE_E_MALLOC
* PURE_E_STRING_NOT_FOUND
* PURE_E_UINT64_OVERFLOW
* PURE_E_ZIP_BOMB_ARCHIVES
* PURE_E_ZIP_BOMB_DEPTH
* PURE_E_ZIP_BOMB_FIFIELD
* PURE_E_ZIP_BOMB_FILES
* PURE_E_ZIP_BOMB_RATIO
* PURE_E_ZIP_BOMB_INFLATE_COMPRESSED_OVERFLOW
* PURE_E_ZIP_BOMB_INFLATE_UNCOMPRESSED_OVERFLOW
* PURE_E_ZIP_TOO_SMALL
* PURE_E_ZIP_SIZE_4GB
* PURE_E_ZIP_RAR
* PURE_E_ZIP_TAR
* PURE_E_ZIP_XAR
* PURE_E_ZIP_SIGNATURE
* PURE_E_ZIP_EOCDR_NOT_FOUND
* PURE_E_ZIP_EOCDR_OVERFLOW
* PURE_E_ZIP_EOCDR_COMMENT_OVERFLOW
* PURE_E_ZIP_EOCDR_SIGNATURE
* PURE_E_ZIP_EOCDR_RECORDS
* PURE_E_ZIP_EOCDR_SIZE_OVERFLOW
* PURE_E_ZIP_EOCDR_SIZE_UNDERFLOW
* PURE_E_ZIP_MULTIPLE_DISKS
* PURE_E_ZIP_APPENDED_DATA_ZEROED
* PURE_E_ZIP_APPENDED_DATA_BUFFER_BLEED
* PURE_E_ZIP_PREPENDED_DATA
* PURE_E_ZIP_PREPENDED_DATA_ZEROED
* PURE_E_ZIP_PREPENDED_DATA_BUFFER_BLEED
* PURE_E_ZIP_CDH_OVERFLOW
* PURE_E_ZIP_CDH_SIGNATURE
* PURE_E_ZIP_CDH_RELATIVE_OFFSET_OVERFLOW
* PURE_E_ZIP_CDH_RELATIVE_OFFSET_OVERLAP
* PURE_E_ZIP_CDH_FILE_NAME_OVERFLOW
* PURE_E_ZIP_CDH_EXTRA_FIELD_OVERFLOW
* PURE_E_ZIP_CDH_FILE_COMMENT_OVERFLOW
* PURE_E_ZIP_LFH_OVERFLOW
* PURE_E_ZIP_LFH_SIGNATURE
* PURE_E_ZIP_LFH_FILE_NAME_OVERFLOW
* PURE_E_ZIP_LFH_EXTRA_FIELD_OVERFLOW
* PURE_E_ZIP_LFH_UNDERFLOW_ZEROED
* PURE_E_ZIP_LFH_UNDERFLOW_BUFFER_BLEED
* PURE_E_ZIP_LFH_DATA_OVERFLOW
* PURE_E_ZIP_DDR_OVERFLOW
* PURE_E_ZIP_LF_OVERFLOW
* PURE_E_ZIP_LF_UNDERFLOW_ZEROED
* PURE_E_ZIP_LF_UNDERFLOW_BUFFER_BLEED
* PURE_E_ZIP_CD_OVERFLOW
* PURE_E_ZIP_CD_UNDERFLOW_ZEROED
* PURE_E_ZIP_CD_UNDERFLOW_BUFFER_BLEED
* PURE_E_ZIP_CD_EOCDR_OVERFLOW
* PURE_E_ZIP_CD_EOCDR_UNDERFLOW_ZEROED
* PURE_E_ZIP_CD_EOCDR_UNDERFLOW_BUFFER_BLEED
* PURE_E_ZIP_DIFF_LFH_GENERAL_PURPOSE_BIT_FLAG
* PURE_E_ZIP_DIFF_LFH_COMPRESSION_METHOD
* PURE_E_ZIP_DIFF_LFH_LAST_MOD_FILE_TIME
* PURE_E_ZIP_DIFF_LFH_LAST_MOD_FILE_DATE
* PURE_E_ZIP_DIFF_LFH_CRC32
* PURE_E_ZIP_DIFF_LFH_COMPRESSED_SIZE
* PURE_E_ZIP_DIFF_LFH_UNCOMPRESSED_SIZE
* PURE_E_ZIP_DIFF_LFH_FILE_NAME_LENGTH
* PURE_E_ZIP_DIFF_LFH_FILE_NAME
* PURE_E_ZIP_DIFF_LFH_DDR_CRC32
* PURE_E_ZIP_DIFF_LFH_DDR_COMPRESSED_SIZE
* PURE_E_ZIP_DIFF_LFH_DDR_UNCOMPRESSED_SIZE
* PURE_E_ZIP_DIFF_DDR_CRC32
* PURE_E_ZIP_DIFF_DDR_COMPRESSED_SIZE
* PURE_E_ZIP_DIFF_DDR_UNCOMPRESSED_SIZE
* PURE_E_ZIP_FLAG_OVERFLOW
* PURE_E_ZIP_FLAG_TRADITIONAL_ENCRYPTION
* PURE_E_ZIP_FLAG_ENHANCED_DEFLATE
* PURE_E_ZIP_FLAG_COMPRESSED_PATCHED_DATA
* PURE_E_ZIP_FLAG_STRONG_ENCRYPTION
* PURE_E_ZIP_FLAG_UNUSED_BIT_7
* PURE_E_ZIP_FLAG_UNUSED_BIT_8
* PURE_E_ZIP_FLAG_UNUSED_BIT_9
* PURE_E_ZIP_FLAG_UNUSED_BIT_10
* PURE_E_ZIP_FLAG_ENHANCED_COMPRESSION
* PURE_E_ZIP_FLAG_MASKED_LOCAL_HEADERS
* PURE_E_ZIP_FLAG_RESERVED_BIT_14
* PURE_E_ZIP_FLAG_RESERVED_BIT_15
* PURE_E_ZIP_COMPRESSION_METHOD_DANGEROUS
* PURE_E_ZIP_COMPRESSION_METHOD_ENCRYPTED
* PURE_E_ZIP_COMPRESSION_METHOD_UNSUPPORTED
* PURE_E_ZIP_STORED_COMPRESSION_SIZE_MISMATCH
* PURE_E_ZIP_DANGEROUS_NEGATIVE_COMPRESSION_RATIO
* PURE_E_ZIP_TIME_OVERFLOW
* PURE_E_ZIP_TIME_HOUR_OVERFLOW
* PURE_E_ZIP_TIME_MINUTE_OVERFLOW
* PURE_E_ZIP_TIME_SECOND_OVERFLOW
* PURE_E_ZIP_DATE_OVERFLOW
* PURE_E_ZIP_DATE_YEAR_OVERFLOW
* PURE_E_ZIP_DATE_MONTH_OVERFLOW
* PURE_E_ZIP_DATE_DAY_OVERFLOW
* PURE_E_ZIP_FILE_NAME_LENGTH
* PURE_E_ZIP_FILE_NAME_CONTROL_CHARACTERS
* PURE_E_ZIP_FILE_NAME_TRAVERSAL_DRIVE_PATH
* PURE_E_ZIP_FILE_NAME_TRAVERSAL_RELATIVE_PATH
* PURE_E_ZIP_FILE_NAME_TRAVERSAL_DOUBLE_DOTS
* PURE_E_ZIP_FILE_NAME_COMPONENT_OVERFLOW
* PURE_E_ZIP_FILE_NAME_BACKSLASH
* PURE_E_ZIP_EXTRA_FIELD_MAX
* PURE_E_ZIP_EXTRA_FIELD_MIN
* PURE_E_ZIP_EXTRA_FIELD_ATTRIBUTE_OVERFLOW
* PURE_E_ZIP_EXTRA_FIELD_OVERFLOW
* PURE_E_ZIP_EXTRA_FIELD_UNDERFLOW_ZEROED
* PURE_E_ZIP_EXTRA_FIELD_UNDERFLOW_BUFFER_BLEED
* PURE_E_ZIP_EXTRA_FIELD_UNICODE_PATH_OVERFLOW
* PURE_E_ZIP_EXTRA_FIELD_UNICODE_PATH_VERSION
* PURE_E_ZIP_EXTRA_FIELD_UNICODE_PATH_DIFF
* PURE_E_ZIP_UNIX_MODE_OVERFLOW
* PURE_E_ZIP_UNIX_MODE_BLOCK_DEVICE
* PURE_E_ZIP_UNIX_MODE_CHARACTER_DEVICE
* PURE_E_ZIP_UNIX_MODE_FIFO
* PURE_E_ZIP_UNIX_MODE_SOCKET
* PURE_E_ZIP_UNIX_MODE_PERMISSIONS_STICKY
* PURE_E_ZIP_UNIX_MODE_PERMISSIONS_SETGID
* PURE_E_ZIP_UNIX_MODE_PERMISSIONS_SETUID
* PURE_E_ZIP_DIRECTORY_COMPRESSED
* PURE_E_ZIP_DIRECTORY_UNCOMPRESSED
* PURE_E_ZIP_SYMLINK_COMPRESSED
* PURE_E_ZIP_SYMLINK_LENGTH
* PURE_E_ZIP_SYMLINK_CONTROL_CHARACTERS
* PURE_E_ZIP_SYMLINK_TRAVERSAL_DRIVE_PATH
* PURE_E_ZIP_SYMLINK_TRAVERSAL_RELATIVE_PATH
* PURE_E_ZIP_SYMLINK_TRAVERSAL_DOUBLE_DOTS
* PURE_E_ZIP_SYMLINK_COMPONENT_OVERFLOW
* PURE_E_ZIP_STRING_MAX
* PURE_E_ZIP_STRING_NULL_BYTE
* PURE_E_ZIP_INFLATE
* PURE_E_ZIP_INFLATE_DICTIONARY
* PURE_E_ZIP_INFLATE_STREAM
* PURE_E_ZIP_INFLATE_DATA
* PURE_E_ZIP_INFLATE_MEMORY
* PURE_E_ZIP_INFLATE_COMPRESSED_UNDERFLOW
* PURE_E_ZIP_INFLATE_UNCOMPRESSED_UNDERFLOW
* PURE_E_ZIP_AD_NIHILO
* PURE_E_ZIP_EX_NIHILO
* PURE_E_ZIP_CRC32
* PURE_E_ZIP_EOCDL_64_OVERFLOW
* PURE_E_ZIP_EOCDL_64_SIGNATURE
* PURE_E_ZIP_EOCDL_64_NEGATIVE_OFFSET
* PURE_E_ZIP_EOCDL_64_DISK
* PURE_E_ZIP_EOCDL_64_DISKS
* PURE_E_ZIP_EOCDR_64_OVERFLOW
* PURE_E_ZIP_EOCDR_64_SIGNATURE
* PURE_E_ZIP_EOCDR_EOCDL_64_OVERFLOW
* PURE_E_ZIP_EOCDR_EOCDL_64_UNDERFLOW_ZEROED
* PURE_E_ZIP_EOCDR_EOCDL_64_UNDERFLOW_BUFFER_BLEED
* PURE_E_ZIP_DIFF_EOCDR_DISK
* PURE_E_ZIP_DIFF_EOCDR_CD_DISK
* PURE_E_ZIP_DIFF_EOCDR_CD_DISK_RECORDS
* PURE_E_ZIP_DIFF_EOCDR_CD_RECORDS
* PURE_E_ZIP_DIFF_EOCDR_CD_SIZE
* PURE_E_ZIP_DIFF_EOCDR_CD_OFFSET
* PURE_E_ZIP_EIEF_64_COMPRESSED_SIZE
* PURE_E_ZIP_EIEF_64_DISK
* PURE_E_ZIP_EIEF_64_RELATIVE_OFFSET
* PURE_E_ZIP_EIEF_64_UNCOMPRESSED_SIZE
* PURE_E_ZIP_EIEF_64_UNDERFLOW_ZEROED
* PURE_E_ZIP_EIEF_64_UNDERFLOW_BUFFER_BLEED
* PURE_E_ZIP_EIEF_64_LFH
* PURE_E_ZIP_DIRECTORY_HAS_NO_LFH

## ...and this is only for zip files, we want to add static analysis for more file formats, please consider asking your company to [sponsor open-source work on Pure](mailto:joran@ronomon.com).
