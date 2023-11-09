// make-errors.js - Create C error enums, codes and strings for use in pure.h.

var Node = {
  fs: require('fs')
};

var errors = {
  PURE_E_OK: "file is pure",
  PURE_E_SIZE_MAX: "exceeded 64 GB limit",
  PURE_E_MALLOC: "insufficient memory",
  PURE_E_STRING_NOT_FOUND: "string not found",
  PURE_E_UINT64_OVERFLOW: "uint64_t overflow",
  PURE_E_ZIP_BOMB_ARCHIVES: "zip bomb: too many archives",
  PURE_E_ZIP_BOMB_DEPTH: "zip bomb: too much recursion",
  PURE_E_ZIP_BOMB_FIFIELD: "zip bomb: local file header overlap (see research by David Fifield)",
  PURE_E_ZIP_BOMB_FILES: "zip bomb: too many files",
  PURE_E_ZIP_BOMB_RATIO: "zip bomb: dangerous compression ratio and uncompressed size",
  PURE_E_ZIP_BOMB_INFLATE_COMPRESSED_OVERFLOW: "zip bomb: compressed data is larger than compressed size (overflow)",
  PURE_E_ZIP_BOMB_INFLATE_UNCOMPRESSED_OVERFLOW: "zip bomb: uncompressed data is larger than uncompressed size (overflow)",
  PURE_E_ZIP_TOO_SMALL: "zip file too small (minimum size is 22 bytes)",
  PURE_E_ZIP_SIZE_4GB: "unsupported: zip file exceeds 4 GB limit (ZIP64)",
  PURE_E_ZIP_RAR: "not a zip file (malicious rar)",
  PURE_E_ZIP_TAR: "not a zip file (malicious tar)",
  PURE_E_ZIP_XAR: "not a zip file (malicious xar)",
  PURE_E_ZIP_SIGNATURE: "not a zip file (bad signature)",
  PURE_E_ZIP_EOCDR_NOT_FOUND: "end of central directory record: not found",
  PURE_E_ZIP_EOCDR_OVERFLOW: "end of central directory record: overflow",
  PURE_E_ZIP_EOCDR_COMMENT_OVERFLOW: "end of central directory record: comment overflow",
  PURE_E_ZIP_EOCDR_SIGNATURE: "end of central directory record: bad signature",
  PURE_E_ZIP_EOCDR_RECORDS: "end of central directory record: cd_disk_records != cd_records",
  PURE_E_ZIP_EOCDR_SIZE_OVERFLOW: "end of central directory record: cd_size too small for number of cd_records",
  PURE_E_ZIP_EOCDR_SIZE_UNDERFLOW: "end of central directory record: cd_size > 0 but cd_records == 0",
  PURE_E_ZIP_MULTIPLE_DISKS: "unsupported: multiple disks",
  PURE_E_ZIP_APPENDED_DATA_ZEROED: "zip file has appended data (zeroed)",
  PURE_E_ZIP_APPENDED_DATA_BUFFER_BLEED: "zip file has appended data (buffer bleed)",
  PURE_E_ZIP_PREPENDED_DATA: "zip file has prepended data",
  PURE_E_ZIP_PREPENDED_DATA_ZEROED: "zip file has prepended data (zeroed)",
  PURE_E_ZIP_PREPENDED_DATA_BUFFER_BLEED: "zip file has prepended data (buffer bleed)",
  PURE_E_ZIP_CDH_OVERFLOW: "central directory header: overflow",
  PURE_E_ZIP_CDH_SIGNATURE: "central directory header: bad signature",
  PURE_E_ZIP_CDH_RELATIVE_OFFSET_OVERFLOW: "central directory header: relative offset overflow",
  PURE_E_ZIP_CDH_RELATIVE_OFFSET_OVERLAP: "central directory header: relative offset overlaps central directory",
  PURE_E_ZIP_CDH_FILE_NAME_OVERFLOW: "central directory header: file name overflow",
  PURE_E_ZIP_CDH_EXTRA_FIELD_OVERFLOW: "central directory header: extra field overflow",
  PURE_E_ZIP_CDH_FILE_COMMENT_OVERFLOW: "central directory header: file comment overflow",
  PURE_E_ZIP_LFH_OVERFLOW: "local file header: overflow",
  PURE_E_ZIP_LFH_SIGNATURE: "local file header: bad signature",
  PURE_E_ZIP_LFH_FILE_NAME_OVERFLOW: "local file header: file name overflow",
  PURE_E_ZIP_LFH_EXTRA_FIELD_OVERFLOW: "local file header: extra field overflow",
  PURE_E_ZIP_LFH_UNDERFLOW_ZEROED: "local file header: gap (zeroed)",
  PURE_E_ZIP_LFH_UNDERFLOW_BUFFER_BLEED: "local file header: gap (buffer bleed)",
  PURE_E_ZIP_LFH_DATA_OVERFLOW: "local file header: data overflow",
  PURE_E_ZIP_DDR_OVERFLOW: "data descriptor record: overflow",
  PURE_E_ZIP_LF_OVERFLOW: "zip file has overlap between last local file and central directory",
  PURE_E_ZIP_LF_UNDERFLOW_ZEROED: "zip file has gap between last local file and central directory (zeroed)",
  PURE_E_ZIP_LF_UNDERFLOW_BUFFER_BLEED: "zip file has gap between last local file and central directory (buffer bleed)",
  PURE_E_ZIP_CD_OVERFLOW: "central directory: overflow",
  PURE_E_ZIP_CD_UNDERFLOW_ZEROED: "central directory: underflow (zeroed)",
  PURE_E_ZIP_CD_UNDERFLOW_BUFFER_BLEED: "central directory: underflow (buffer bleed)",
  PURE_E_ZIP_CD_EOCDR_OVERFLOW: "central directory overlaps end of central directory record",
  PURE_E_ZIP_CD_EOCDR_UNDERFLOW_ZEROED: "zip file has gap between central directory and end of central directory record (zeroed)",
  PURE_E_ZIP_CD_EOCDR_UNDERFLOW_BUFFER_BLEED: "zip file has gap between central directory and end of central directory record (buffer bleed)",
  PURE_E_ZIP_DIFF_LFH_GENERAL_PURPOSE_BIT_FLAG: "local file header diverges from central directory header: general purpose bit flag",
  PURE_E_ZIP_DIFF_LFH_COMPRESSION_METHOD: "local file header diverges from central directory header: compression method",
  PURE_E_ZIP_DIFF_LFH_LAST_MOD_FILE_TIME: "local file header diverges from central directory header: last mod file time",
  PURE_E_ZIP_DIFF_LFH_LAST_MOD_FILE_DATE: "local file header diverges from central directory header: last mod file date",
  PURE_E_ZIP_DIFF_LFH_CRC32: "local file header diverges from central directory header: crc32",
  PURE_E_ZIP_DIFF_LFH_COMPRESSED_SIZE: "local file header diverges from central directory header: compressed size",
  PURE_E_ZIP_DIFF_LFH_UNCOMPRESSED_SIZE: "local file header diverges from central directory header: uncompressed size",
  PURE_E_ZIP_DIFF_LFH_FILE_NAME_LENGTH: "local file header diverges from central directory header: file name length",
  PURE_E_ZIP_DIFF_LFH_FILE_NAME: "local file header diverges from central directory header: file name",
  PURE_E_ZIP_DIFF_LFH_DDR_CRC32: "local file header diverges from data descriptor record: crc32",
  PURE_E_ZIP_DIFF_LFH_DDR_COMPRESSED_SIZE: "local file header diverges from data descriptor record: compressed size",
  PURE_E_ZIP_DIFF_LFH_DDR_UNCOMPRESSED_SIZE: "local file header diverges from data descriptor record: uncompressed size",
  PURE_E_ZIP_DIFF_DDR_CRC32: "data descriptor record diverges from central directory header: crc32",
  PURE_E_ZIP_DIFF_DDR_COMPRESSED_SIZE: "data descriptor record diverges from central directory header: compressed size",
  PURE_E_ZIP_DIFF_DDR_UNCOMPRESSED_SIZE: "data descriptor record diverges from central directory header: uncompressed size",
  PURE_E_ZIP_FLAG_OVERFLOW: "general purpose bit flag: 16-bit overflow",
  PURE_E_ZIP_FLAG_TRADITIONAL_ENCRYPTION: "unsupported: traditional encryption",
  PURE_E_ZIP_FLAG_ENHANCED_DEFLATE: "unsupported: enhanced deflate",
  PURE_E_ZIP_FLAG_COMPRESSED_PATCHED_DATA: "unsupported: compressed patched data",
  PURE_E_ZIP_FLAG_STRONG_ENCRYPTION: "unsupported: strong encryption",
  PURE_E_ZIP_FLAG_UNUSED_BIT_7: "unsupported: unused flag (bit 7)",
  PURE_E_ZIP_FLAG_UNUSED_BIT_8: "unsupported: unused flag (bit 8)",
  PURE_E_ZIP_FLAG_UNUSED_BIT_9: "unsupported: unused flag (bit 9)",
  PURE_E_ZIP_FLAG_UNUSED_BIT_10: "unsupported: unused flag (bit 10)",
  PURE_E_ZIP_FLAG_ENHANCED_COMPRESSION: "unsupported: enhanced compression",
  PURE_E_ZIP_FLAG_MASKED_LOCAL_HEADERS: "unsupported: masked local headers",
  PURE_E_ZIP_FLAG_RESERVED_BIT_14: "unsupported: reserved flag (bit 14)",
  PURE_E_ZIP_FLAG_RESERVED_BIT_15: "unsupported: reserved flag (bit 15)",
  PURE_E_ZIP_COMPRESSION_METHOD_DANGEROUS: "compression method: exceeds 999 (CVE-2016-9844)",
  PURE_E_ZIP_COMPRESSION_METHOD_ENCRYPTED: "compression method: encrypted",
  PURE_E_ZIP_COMPRESSION_METHOD_UNSUPPORTED: "compression method: must be 0 or 8",
  PURE_E_ZIP_STORED_COMPRESSION_SIZE_MISMATCH: "file stored with no compression has mismatching compressed and uncompressed sizes",
  PURE_E_ZIP_DANGEROUS_NEGATIVE_COMPRESSION_RATIO: "dangerous negative compression ratio (CVE-2018-18384)",
  PURE_E_ZIP_TIME_OVERFLOW: "time: 16-bit overflow",
  PURE_E_ZIP_TIME_HOUR_OVERFLOW: "time: ms-dos hour: overflow",
  PURE_E_ZIP_TIME_MINUTE_OVERFLOW: "time: ms-dos minute: overflow",
  PURE_E_ZIP_TIME_SECOND_OVERFLOW: "time: ms-dos second: overflow",
  PURE_E_ZIP_DATE_OVERFLOW: "date: 16-bit overflow",
  PURE_E_ZIP_DATE_YEAR_OVERFLOW: "date: ms-dos year: overflow",
  PURE_E_ZIP_DATE_MONTH_OVERFLOW: "date: ms-dos month: overflow",
  PURE_E_ZIP_DATE_DAY_OVERFLOW: "date: ms-dos day: overflow",
  PURE_E_ZIP_FILE_NAME_LENGTH: "file name exceeds 4096 bytes (CVE-2018-1000035)",
  PURE_E_ZIP_FILE_NAME_CONTROL_CHARACTERS: "file name contains control characters (CVE-2003-0282)",
  PURE_E_ZIP_FILE_NAME_TRAVERSAL_DRIVE_PATH: "directory traversal (via file name drive path)",
  PURE_E_ZIP_FILE_NAME_TRAVERSAL_RELATIVE_PATH: "directory traversal (via file name relative path)",
  PURE_E_ZIP_FILE_NAME_TRAVERSAL_DOUBLE_DOTS: "directory traversal (via file name double dots)",
  PURE_E_ZIP_FILE_NAME_COMPONENT_OVERFLOW: "file name path component exceeds 255 bytes",
  PURE_E_ZIP_FILE_NAME_BACKSLASH: "file name contains backslash",
  PURE_E_ZIP_EXTRA_FIELD_MAX: "extra field length exceeds maximum",
  PURE_E_ZIP_EXTRA_FIELD_MIN: "extra field length must be 0 or at least 4 bytes",
  PURE_E_ZIP_EXTRA_FIELD_ATTRIBUTE_OVERFLOW: "extra field: attribute overflow",
  PURE_E_ZIP_EXTRA_FIELD_OVERFLOW: "extra field: overflow",
  PURE_E_ZIP_EXTRA_FIELD_UNDERFLOW_ZEROED: "extra field: underflow (zeroed)",
  PURE_E_ZIP_EXTRA_FIELD_UNDERFLOW_BUFFER_BLEED: "extra field: underflow (buffer bleed)",
  PURE_E_ZIP_EXTRA_FIELD_UNICODE_PATH_OVERFLOW: "extra field: unicode path overflow",
  PURE_E_ZIP_EXTRA_FIELD_UNICODE_PATH_VERSION: "extra field: unicode path has an invalid version",
  PURE_E_ZIP_EXTRA_FIELD_UNICODE_PATH_DIFF: "extra field: unicode path diverges from file name",
  PURE_E_ZIP_UNIX_MODE_OVERFLOW: "unix mode: overflow",
  PURE_E_ZIP_UNIX_MODE_BLOCK_DEVICE: "unix mode: dangerous type (block device)",
  PURE_E_ZIP_UNIX_MODE_CHARACTER_DEVICE: "unix mode: dangerous type (character device)",
  PURE_E_ZIP_UNIX_MODE_FIFO: "unix mode: dangerous type (fifo)",
  PURE_E_ZIP_UNIX_MODE_SOCKET: "unix mode: dangerous type (socket)",
  PURE_E_ZIP_UNIX_MODE_PERMISSIONS_STICKY: "unix mode: dangerous permissions (sticky)",
  PURE_E_ZIP_UNIX_MODE_PERMISSIONS_SETGID: "unix mode: dangerous permissions (setgid)",
  PURE_E_ZIP_UNIX_MODE_PERMISSIONS_SETUID: "unix mode: dangerous permissions (setuid)",
  PURE_E_ZIP_DIRECTORY_COMPRESSED: "directory: non-zero compressed size",
  PURE_E_ZIP_DIRECTORY_UNCOMPRESSED: "directory: non-zero uncompressed size",
  PURE_E_ZIP_SYMLINK_COMPRESSED: "unsupported: compressed symlink",
  PURE_E_ZIP_SYMLINK_LENGTH: "symlink exceeds 4096 bytes (CVE-2018-1000035)",
  PURE_E_ZIP_SYMLINK_CONTROL_CHARACTERS: "symlink contains control characters (CVE-2003-0282)",
  PURE_E_ZIP_SYMLINK_TRAVERSAL_DRIVE_PATH: "directory traversal (via symlink drive path)",
  PURE_E_ZIP_SYMLINK_TRAVERSAL_RELATIVE_PATH: "directory traversal (via symlink relative path)",
  PURE_E_ZIP_SYMLINK_TRAVERSAL_DOUBLE_DOTS: "directory traversal (via symlink double dots)",
  PURE_E_ZIP_SYMLINK_COMPONENT_OVERFLOW: "symlink path component exceeds 255 bytes",
  PURE_E_ZIP_STRING_MAX: "string exceeds reasonable limit (PURE_ZIP_STRING_MAX)",
  PURE_E_ZIP_STRING_NULL_BYTE: "string contains a dangerous null byte",
  PURE_E_ZIP_INFLATE: "zip file could not be uncompressed",
  PURE_E_ZIP_INFLATE_DICTIONARY: "zip file could not be uncompressed (dictionary error)",
  PURE_E_ZIP_INFLATE_STREAM: "zip file could not be uncompressed (stream error)",
  PURE_E_ZIP_INFLATE_DATA: "zip file could not be uncompressed (data error)",
  PURE_E_ZIP_INFLATE_MEMORY: "zip file could not be uncompressed (memory error)",
  PURE_E_ZIP_INFLATE_COMPRESSED_UNDERFLOW: "compressed data is smaller than compressed size",
  PURE_E_ZIP_INFLATE_UNCOMPRESSED_UNDERFLOW: "uncompressed data is smaller than uncompressed size",
  PURE_E_ZIP_AD_NIHILO: "file has a zero uncompressed size but a non-zero compressed size or invalid compressed data (ad nihilo)",
  PURE_E_ZIP_EX_NIHILO: "file has a zero compressed size but a non-zero uncompressed size (ex nihilo)",
  PURE_E_ZIP_CRC32: "file is corrupt or has an invalid crc32 checksum",
  PURE_E_ZIP_EOCDL_64_OVERFLOW: "zip64 end of central directory locator: overflow",
  PURE_E_ZIP_EOCDL_64_SIGNATURE: "zip64 end of central directory locator: bad signature",
  PURE_E_ZIP_EOCDL_64_NEGATIVE_OFFSET: "zip64 end of central directory locator: negative offset",
  PURE_E_ZIP_EOCDL_64_DISK: "zip64 end of central directory locator: disk != 0",
  PURE_E_ZIP_EOCDL_64_DISKS: "zip64 end of central directory locator: disks > 1",
  PURE_E_ZIP_EOCDR_64_OVERFLOW: "zip64 end of central directory record: overflow",
  PURE_E_ZIP_EOCDR_64_SIGNATURE: "zip64 end of central directory record: bad signature",
  PURE_E_ZIP_EOCDR_EOCDL_64_OVERFLOW: "zip64 eocdr overlaps zip64 eocdl",
  PURE_E_ZIP_EOCDR_EOCDL_64_UNDERFLOW_ZEROED: "gap between zip64 eocdr and zip64 eocdl (zeroed)",
  PURE_E_ZIP_EOCDR_EOCDL_64_UNDERFLOW_BUFFER_BLEED: "gap between zip64 eocdr and zip64 eocdl (buffer bleed)",
  PURE_E_ZIP_DIFF_EOCDR_DISK: "eocdr diverges from zip64 eocdr: disk",
  PURE_E_ZIP_DIFF_EOCDR_CD_DISK: "eocdr diverges from zip64 eocdr: cd_disk",
  PURE_E_ZIP_DIFF_EOCDR_CD_DISK_RECORDS: "eocdr diverges from zip64 eocdr: cd_disk_records",
  PURE_E_ZIP_DIFF_EOCDR_CD_RECORDS: "eocdr diverges from zip64 eocdr: cd_records",
  PURE_E_ZIP_DIFF_EOCDR_CD_SIZE: "eocdr diverges from zip64 eocdr: cd_size",
  PURE_E_ZIP_DIFF_EOCDR_CD_OFFSET: "eocdr diverges from zip64 eocdr: cd_offset",
  PURE_E_ZIP_EIEF_64_COMPRESSED_SIZE: "zip64 extended information extra field: missing compressed_size",
  PURE_E_ZIP_EIEF_64_DISK: "zip64 extended information extra field: missing disk",
  PURE_E_ZIP_EIEF_64_RELATIVE_OFFSET: "zip64 extended information extra field: missing relative_offset",
  PURE_E_ZIP_EIEF_64_UNCOMPRESSED_SIZE: "zip64 extended information extra field: missing uncompressed_size",
  PURE_E_ZIP_EIEF_64_UNDERFLOW_ZEROED: "zip64 extended information extra field: appended data (zeroed)",
  PURE_E_ZIP_EIEF_64_UNDERFLOW_BUFFER_BLEED: "zip64 extended information extra field: appended data (buffer bleed)",
  PURE_E_ZIP_EIEF_64_LFH: "zip64 extended information extra field: local file header must include both uncompressed_size and compressed_size",
  PURE_E_ZIP_DIRECTORY_HAS_NO_LFH: "a directory has no local file header"
};

var lines = [];

function line(string) {
  lines.push(string);
}

function enums() {
  line('enum PURE_E {');
  for (var code in errors) line('  ' + code + ',');
  line('  // Capture enum length for bounds checking by pure_error_string():');
  line('  PURE_E_ENUM_LENGTH');
  line('};');
}

function codes() {
  line('');
  line('const char* const PURE_E_CODES[] = {');
  var count = 0;
  var length = Object.keys(errors).length;
  for (var code in errors) {
    line('  "' + code + '"' + (++count < length ? ',' : ''));
  }
  line('};');
}

function strings() {
  line('');
  line('const char* const PURE_E_STRINGS[] = {');
  var count = 0;
  var length = Object.keys(errors).length;
  for (var code in errors) {
    line('  "' + errors[code] + '"' + (++count < length ? ',' : ''));
  }
  line('};');
}

enums();
codes();
strings();

Node.fs.writeFileSync('pure_errors.h', lines.join('\n'), 'utf-8');
