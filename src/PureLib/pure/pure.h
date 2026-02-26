// pure.h - C99 interface:

#ifndef PURE_H
#define PURE_H

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pure_constants.h"
#include "pure_errors.h"
#include "pure_signatures.h"
#include "pure_routines.h"

#include "zlib.h"

typedef struct pure_ctx {
  uint64_t flags;
  uint64_t depth;
  uint64_t files;
  uint64_t archives;
  uint64_t size;
  uint64_t compressed_size;
  uint64_t uncompressed_size;
} pure_ctx;

typedef struct pure_zip_lfh {
  uint64_t offset;
  uint64_t length;
  uint64_t version_minimum;
  uint64_t general_purpose_bit_flag;
  uint64_t compression_method;
  uint64_t last_mod_file_time;
  uint64_t last_mod_file_date;
  uint64_t crc32;
  uint64_t compressed_size;
  uint64_t uncompressed_size;
  uint8_t* file_name;
  uint64_t file_name_length;
  uint8_t* extra_field;
  uint64_t extra_field_length;
  uint8_t zip64;
} pure_zip_lfh;

typedef struct pure_zip_ddr {
  uint64_t offset;
  uint64_t length;
  uint64_t crc32;
  uint64_t compressed_size;
  uint64_t uncompressed_size;
  uint8_t zip64;
} pure_zip_ddr;

typedef struct pure_zip_cdh {
  uint64_t offset;
  uint64_t length;
  uint64_t version_made;
  uint64_t version_minimum;
  uint64_t general_purpose_bit_flag;
  uint64_t compression_method;
  uint64_t last_mod_file_time;
  uint64_t last_mod_file_date;
  uint64_t crc32;
  uint64_t compressed_size;
  uint64_t uncompressed_size;
  uint8_t* file_name;
  uint64_t file_name_length;
  uint8_t* extra_field;
  uint64_t extra_field_length;
  uint8_t* file_comment;
  uint64_t file_comment_length;
  uint64_t disk;
  uint64_t internal_file_attributes;
  uint64_t external_file_attributes;
  uint64_t relative_offset;
  uint64_t unix_mode;
  uint8_t directory;
  uint8_t zip64;
} pure_zip_cdh;

typedef struct pure_zip_eocdr_64 {
  uint64_t offset;
  uint64_t length;
  uint64_t version_made;
  uint64_t version_minimum;
  uint64_t disk;
  uint64_t cd_disk;
  uint64_t cd_disk_records;
  uint64_t cd_records;
  uint64_t cd_size;
  uint64_t cd_offset;
  uint8_t* extensible_data_sector;
  uint64_t extensible_data_sector_length;
} pure_zip_eocdr_64;

typedef struct pure_zip_eocdl_64 {
  uint64_t offset;
  uint64_t length;
  uint64_t disk;
  uint64_t eocdr_64_offset;
  uint64_t disks;
} pure_zip_eocdl_64;

typedef struct pure_zip_eocdr {
  uint64_t offset;
  uint64_t length;
  uint64_t disk;
  uint64_t cd_disk;
  uint64_t cd_disk_records;
  uint64_t cd_records;
  uint64_t cd_size;
  uint64_t cd_offset;
  uint8_t* comment;
  uint64_t comment_length;
  uint8_t zip64;
} pure_zip_eocdr;

int pure_zip_crc32(
  const uint8_t* buffer,
  const uint64_t size,
  uint64_t* result
) {
  assert(SIZE_MAX >= UINT32_MAX);
  assert(size <= SIZE_MAX);
  assert(*result == 0);
  unsigned long checksum = crc32_z(0UL, Z_NULL, 0);
  checksum = crc32_z(checksum, buffer, (size_t) size);
  *result = (uint64_t) checksum;
  return 0;
}

void pure_zip_debug_lfh(const pure_zip_lfh* header) {
  printf("LOCAL FILE HEADER:\n");
  printf(
    "offset=%llu length=%llu\n",
    header->offset,
    header->length
  );
  printf(
    "general_purpose_bit_flag=%llu version_minimum=%llu\n",
    header->general_purpose_bit_flag,
    header->version_minimum
  );
  printf(
    "compression_method=%llu last_mod_file_time=%llu last_mod_file_date=%llu\n",
    header->compression_method,
    header->last_mod_file_time,
    header->last_mod_file_date
  );
  printf(
    "crc32=%llu compressed_size=%llu uncompressed_size=%llu\n",
    header->crc32,
    header->compressed_size,
    header->uncompressed_size
  );
  printf("file_name=\"");
  fwrite(header->file_name, 1, header->file_name_length, stdout);
  printf("\"\n");
  printf(
    "file_name_length=%llu extra_field_length=%llu\n",
    header->file_name_length,
    header->extra_field_length
  );
  printf("\n");
}

void pure_zip_debug_ddr(const pure_zip_ddr* header) {
  printf("DATA DESCRIPTOR RECORD:\n");
  printf(
    "offset=%llu length=%llu\n",
    header->offset,
    header->length
  );
  printf(
    "crc32=%llu compressed_size=%llu uncompressed_size=%llu\n",
    header->crc32,
    header->compressed_size,
    header->uncompressed_size
  );
  printf("\n");
}

void pure_zip_debug_cdh(const pure_zip_cdh* header) {
  printf("CENTRAL DIRECTORY HEADER:\n");
  printf(
    "offset=%llu length=%llu\n",
    header->offset,
    header->length
  );
  printf(
    "general_purpose_bit_flag=%llu version_minimum=%llu version_made=%llu\n",
    header->general_purpose_bit_flag,
    header->version_minimum,
    header->version_made
  );
  printf(
    "compression_method=%llu last_mod_file_time=%llu last_mod_file_date=%llu\n",
    header->compression_method,
    header->last_mod_file_time,
    header->last_mod_file_date
  );
  printf(
    "crc32=%llu compressed_size=%llu uncompressed_size=%llu\n",
    header->crc32,
    header->compressed_size,
    header->uncompressed_size
  );
  printf("file_name=\"");
  fwrite(header->file_name, 1, header->file_name_length, stdout);
  printf("\"\n");
  printf(
    "file_name_length=%llu extra_field_length=%llu file_comment_length=%llu\n",
    header->file_name_length,
    header->extra_field_length,
    header->file_comment_length
  );
  printf("file_comment=\"");
  fwrite(header->file_comment, 1, header->file_comment_length, stdout);
  printf("\"\n");
  printf(
    "internal_file_attributes=%llu external_file_attributes=%llu\n",
    header->internal_file_attributes,
    header->external_file_attributes
  );
  printf(
    "disk=%llu relative_offset=%llu\n",
    header->disk,
    header->relative_offset
  );
  printf(
    "unix_mode=%llu directory=%u zip64=%u\n",
    header->unix_mode,
    header->directory,
    header->zip64
  );
  printf("\n");
}

void pure_zip_debug_eocdl_64(const pure_zip_eocdl_64* header) {
  printf("ZIP64 END OF CENTRAL DIRECTORY LOCATOR:\n");
  printf("offset=%llu\n", header->offset);
  printf("length=%llu\n", header->length);
  printf("disk=%llu\n", header->disk);
  printf("eocdr_64_offset=%llu\n", header->eocdr_64_offset);
  printf("disks=%llu\n", header->disks);
  printf("\n");
}

void pure_zip_debug_eocdr_64(const pure_zip_eocdr_64* header) {
  printf("ZIP64 END OF CENTRAL DIRECTORY RECORD:\n");
  printf("offset=%llu\n", header->offset);
  printf("length=%llu\n", header->length);
  printf("version_made=%llu\n", header->version_made);
  printf("version_minimum=%llu\n", header->version_minimum);
  printf("disk=%llu\n", header->disk);
  printf("cd_disk=%llu\n", header->cd_disk);
  printf("cd_disk_records=%llu\n", header->cd_disk_records);
  printf("cd_records=%llu\n", header->cd_records);
  printf("cd_size=%llu\n", header->cd_size);
  printf("cd_offset=%llu\n", header->cd_offset);
  printf(
    "extensible_data_sector_length=%llu\n",
    header->extensible_data_sector_length
  );
  printf("\n");
}

void pure_zip_debug_eocdr(const pure_zip_eocdr* header) {
  printf("END OF CENTRAL DIRECTORY RECORD:\n");
  printf("offset=%llu\n", header->offset);
  printf("length=%llu\n", header->length);
  printf("disk=%llu\n", header->disk);
  printf("cd_disk=%llu\n", header->cd_disk);
  printf("cd_disk_records=%llu\n", header->cd_disk_records);
  printf("cd_records=%llu\n", header->cd_records);
  printf("cd_size=%llu\n", header->cd_size);
  printf("cd_offset=%llu\n", header->cd_offset);
  printf("comment=\"");
  fwrite(header->comment, 1, header->comment_length, stdout);
  printf("\"\n");
  printf("comment_length=%llu\n", header->comment_length);
  printf("\n");
}

int pure_zip_verify_compression_ratio(
  const uint64_t compressed_size,
  const uint64_t uncompressed_size
) {
  // Here, compressed size and uncompressed size could relate to the respective
  // sizes of an individual file or the sum of the sizes across an archive.
  //
  // We do not use only a ratio limit to detect dangerous compression ratios,
  // since an extreme ratio can be mitigated by a negligible uncompressed size.
  //
  // Instead, we calculate a score that combines the ratio and uncompressed size
  // and then compare this score against a limit. The higher the ratio and the
  // larger the uncompressed size, the more dangerous the file or archive.
  //
  // Conversely, a file or archive with high ratio but negligible uncompressed
  // size is less likely to be dangerous, since while it may have high power,
  // this power is multiplied by less weight.
  if (compressed_size == 0) return 0; // Prevent divide-by-zero.
  if (uncompressed_size < PURE_ZIP_COMPRESSION_RATIO_SIZE_MIN) return 0;
  const uint64_t ratio = uncompressed_size / compressed_size;
  const uint64_t score = ratio * uncompressed_size;
  if (score > PURE_ZIP_COMPRESSION_RATIO_SCORE_MAX) {
    return PURE_E_ZIP_BOMB_RATIO;
  }
  return 0;
}

int pure_zip_verify_compression_method(const uint64_t value) {
  if (value == PURE_ZIP_COMPRESSION_METHOD_NONE) return 0;
  if (value == PURE_ZIP_COMPRESSION_METHOD_DEFLATE) return 0;
  // CVE-2016-9844:
  // Defend vulnerable implementations against overflow when the two-byte
  // compression method in the central directory file header exceeds 999.
  // https://bugs.launchpad.net/ubuntu/+source/unzip/+bug/1643750
  if (value > 999) return PURE_E_ZIP_COMPRESSION_METHOD_DANGEROUS;
  if (value == 99) return PURE_E_ZIP_COMPRESSION_METHOD_ENCRYPTED;
  return PURE_E_ZIP_COMPRESSION_METHOD_UNSUPPORTED;
}

int pure_zip_verify_compression_method_sizes(
  const uint64_t compression_method,
  const uint64_t compressed_size,
  const uint64_t uncompressed_size
) {
  {
    int error = pure_zip_verify_compression_method(compression_method);
    if (error) return error;
  }
  if (compression_method == 0 && compressed_size != uncompressed_size) {
    return PURE_E_ZIP_STORED_COMPRESSION_SIZE_MISMATCH;
  }
  // CVE-2018-18384:
  // Defend vulnerable implementations against buffer overflow.
  // https://bugzilla.suse.com/show_bug.cgi?id=1110194
  if (
    uncompressed_size > 0 &&
    compressed_size > uncompressed_size &&
    compressed_size / uncompressed_size >= 2
  ) {
    return PURE_E_ZIP_DANGEROUS_NEGATIVE_COMPRESSION_RATIO;
  }
  return 0;
}

int pure_zip_verify_date(const uint64_t value) {
  /*
  An MS-DOS date is a packed 16-bit (2 bytes) value in which bits in the value
  represent the day, month, and year:
  
     Bits:  0-4   5-8    9-15
     Unit:  Day   Month  Year
    Range:  1-31  1-12   Relative to 1980
  */
  if (value > PURE_16_BIT_MAX) return PURE_E_ZIP_DATE_OVERFLOW;
  uint64_t y = (value >> 9) + 1980;
  uint64_t m = (value >> 5) & 15;
  uint64_t d = value & 31;
  // 7 bits supports up to 127 years, or 2107 (1980 + 127).
  // However, years after 2099 are not correctly handled by some software:
  if (y > 2099) return PURE_E_ZIP_DATE_YEAR_OVERFLOW;
  if (m > 12) return PURE_E_ZIP_DATE_MONTH_OVERFLOW;
  if (d > 31) return PURE_E_ZIP_DATE_DAY_OVERFLOW;
  return 0;
}

int pure_zip_verify_directory(const pure_zip_cdh* header) {
  assert(header->directory == 0 || header->directory == 1);
  if (header->directory) {
    if (header->compressed_size > 0) return PURE_E_ZIP_DIRECTORY_COMPRESSED;
    if (header->uncompressed_size > 0) return PURE_E_ZIP_DIRECTORY_UNCOMPRESSED;
  }
  return 0;
}

int pure_zip_verify_disk(const uint64_t value) {
  if (value != 0) return PURE_E_ZIP_MULTIPLE_DISKS;
  return 0;
}

int pure_zip_verify_extra_field(
  const uint8_t* extra_field,
  const uint64_t extra_field_length,
  const uint8_t* file_name,
  const uint64_t file_name_length
) {
  if (extra_field_length > PURE_ZIP_EXTRA_FIELD_MAX) {
    return PURE_E_ZIP_EXTRA_FIELD_MAX;
  }
  // The extra field contains a variety of optional data such as OS-specific
  // attributes. It is divided into chunks, each with a 16-bit ID code and a
  // 16-bit length.
  if (extra_field_length != 0 && extra_field_length < 4) {
    return PURE_E_ZIP_EXTRA_FIELD_MIN;
  }
  // Check extra field attribute sizes, and specifically the unicode path:
  uint64_t offset = 0;
  while (offset + 2 + 2 <= extra_field_length) {
    const uint16_t id = pure_u16(extra_field + offset + 0);
    const uint16_t size = pure_u16(extra_field + offset + 2);
    if (offset + 2 + 2 + size > extra_field_length) {
      return PURE_E_ZIP_EXTRA_FIELD_ATTRIBUTE_OVERFLOW;
    }
    if (id == 0x7075) {
      // We expect at least a 1-byte version followed by a 4-byte crc32:
      if (size < 1 + 4) return PURE_E_ZIP_EXTRA_FIELD_UNICODE_PATH_OVERFLOW;
      assert(offset + 2 + 2 + 1 <= extra_field_length);
      uint8_t version = extra_field[offset + 2 + 2];
      if (version != 1) return PURE_E_ZIP_EXTRA_FIELD_UNICODE_PATH_VERSION;
      // We require the unicode path to match the central directory file even if
      // the crc32 of the non-unicode path is different. Otherwise, an attacker
      // could present an alternative extension to bypass content inspection.
      const uint8_t* unicode_path = extra_field + offset + 2 + 2 + 1 + 4;
      const uint64_t unicode_path_length = size - 1 - 4;
      if (
        unicode_path_length != file_name_length ||
        memcmp(unicode_path, file_name, (size_t) file_name_length)
      ) {
        return PURE_E_ZIP_EXTRA_FIELD_UNICODE_PATH_DIFF;
      }
    }
    offset += 4;
    offset += size;
  }
  if (offset > extra_field_length) return PURE_E_ZIP_EXTRA_FIELD_OVERFLOW;
  if (offset < extra_field_length) {
    if (pure_zeroes(extra_field, offset, extra_field_length)) {
      return PURE_E_ZIP_EXTRA_FIELD_UNDERFLOW_ZEROED;
    } else {
      return PURE_E_ZIP_EXTRA_FIELD_UNDERFLOW_BUFFER_BLEED;
    }
  }
  return 0;
}

int pure_zip_verify_file_name(
  const uint8_t* file_name,
  const uint64_t file_name_length
) {
  // A "file name" in this context is a path name with multiple components.
  // The file name length may be 0 if the archiver input came from stdin.

  // CVE-2018-1000035:
  // Heap-based buffer overflow in password protected ZIP archives.
  // https://sec-consult.com/en/blog/advisories/
  //   multiple-vulnerabilities-in-infozip-unzip/index.html
  // We want to defend vulnerable implementations against PATH_MAX allocation
  // bugs, e.g. malloc(N * PATH_MAX) where the assumption is that user data
  // cannot exceed PATH_MAX.
  if (file_name_length > PURE_PATH_MAX) return PURE_E_ZIP_FILE_NAME_LENGTH;

  // CVE-2003-0282 (aka "JELMER"):
  // Some zip implementations filter control characters amongst others.
  // This behavior can be exploited to mask ".." in a directory traversal.
  // https://www.securityfocus.com/archive/1/321090
  if (pure_path_control_characters(file_name, file_name_length)) {
    return PURE_E_ZIP_FILE_NAME_CONTROL_CHARACTERS;
  }
  if (pure_path_drive(file_name, file_name_length)) {
    return PURE_E_ZIP_FILE_NAME_TRAVERSAL_DRIVE_PATH;
  }
  if (pure_path_relative(file_name, file_name_length)) {
    return PURE_E_ZIP_FILE_NAME_TRAVERSAL_RELATIVE_PATH;
  }
  if (pure_path_double_dots(file_name, file_name_length)) {
    return PURE_E_ZIP_FILE_NAME_TRAVERSAL_DOUBLE_DOTS;
  }
  if (pure_path_component_overflow(file_name, file_name_length)) {
    return PURE_E_ZIP_FILE_NAME_COMPONENT_OVERFLOW;
  }
  // All slashes must be forward according to the APPNOTE.TXT specification:
  for (uint64_t index = 0; index < file_name_length; index++) {
    if (file_name[index] == PURE_BACKSLASH) {
      return PURE_E_ZIP_FILE_NAME_BACKSLASH;
    }
  }
  return 0;
}

int pure_zip_verify_flags(const uint64_t value) {
  if (value > PURE_16_BIT_MAX) return PURE_E_ZIP_FLAG_OVERFLOW;
  if (value & (1 << 0)) return PURE_E_ZIP_FLAG_TRADITIONAL_ENCRYPTION;
  if (value & (1 << 4)) return PURE_E_ZIP_FLAG_ENHANCED_DEFLATE;
  if (value & (1 << 5)) return PURE_E_ZIP_FLAG_COMPRESSED_PATCHED_DATA;
  if (value & (1 << 6)) return PURE_E_ZIP_FLAG_STRONG_ENCRYPTION;
  if (value & (1 << 7)) return PURE_E_ZIP_FLAG_UNUSED_BIT_7;
  if (value & (1 << 8)) return PURE_E_ZIP_FLAG_UNUSED_BIT_8;
  if (value & (1 << 9)) return PURE_E_ZIP_FLAG_UNUSED_BIT_9;
  if (value & (1 << 10)) return PURE_E_ZIP_FLAG_UNUSED_BIT_10;
  if (value & (1 << 12)) return PURE_E_ZIP_FLAG_ENHANCED_COMPRESSION;
  if (value & (1 << 13)) return PURE_E_ZIP_FLAG_MASKED_LOCAL_HEADERS;
  if (value & (1 << 14)) return PURE_E_ZIP_FLAG_RESERVED_BIT_14;
  if (value & (1 << 15)) return PURE_E_ZIP_FLAG_RESERVED_BIT_15;
  return 0;
}

int pure_zip_verify_string(
  const uint8_t* string,
  const uint64_t string_length,
  const uint64_t utf8
) {
  if (string_length > PURE_ZIP_STRING_MAX) return PURE_E_ZIP_STRING_MAX;
  for (uint64_t index = 0; index < string_length; index++) {
    if (string[index] == 0) return PURE_E_ZIP_STRING_NULL_BYTE;
  }
  if (utf8) {
    // TO DO: Verify that UTF-8 encoding is valid:
    // Some systems such as macOS never bother to set bit 11 to indicate UTF-8.
    // We therefore always attempt UTF-8 and fallback to CP437 only on error.
    // If the string must be UTF-8 then reject the string as invalid.
  }
  return 0;
}

int pure_zip_verify_symlink(
  const pure_zip_cdh* cdh,
  const pure_zip_lfh* lfh,
  const uint8_t* buffer
) {
  if ((cdh->unix_mode & PURE_UNIX_MODE_MASK) != PURE_UNIX_MODE_LNK) return 0;
  if (cdh->compression_method != 0) return PURE_E_ZIP_SYMLINK_COMPRESSED;
  assert(cdh->relative_offset == lfh->offset);
  assert(!pure_overflow(cdh->relative_offset, lfh->length, cdh->offset));
  const uint8_t* symlink = buffer + cdh->relative_offset + lfh->length;
  const uint64_t symlink_length = cdh->compressed_size;
  // Check for PATH_MAX overflow:
  if (symlink_length > PURE_PATH_MAX) return PURE_E_ZIP_SYMLINK_LENGTH;
  // Check for control characters used to mask a directory traversal:
  if (pure_path_control_characters(symlink, symlink_length)) {
    return PURE_E_ZIP_SYMLINK_CONTROL_CHARACTERS;
  }
  // Check for a directory traversal:
  if (pure_path_drive(symlink, symlink_length)) {
    return PURE_E_ZIP_SYMLINK_TRAVERSAL_DRIVE_PATH;
  }
  if (pure_path_relative(symlink, symlink_length)) {
    return PURE_E_ZIP_SYMLINK_TRAVERSAL_RELATIVE_PATH;
  }
  if (pure_path_double_dots(symlink, symlink_length)) {
    return PURE_E_ZIP_SYMLINK_TRAVERSAL_DOUBLE_DOTS;
  }
  // Check for path component overflow:
  if (pure_path_component_overflow(symlink, symlink_length)) {
    return PURE_E_ZIP_SYMLINK_COMPONENT_OVERFLOW;
  }
  return 0;
}

int pure_zip_verify_unix_mode(const uint64_t value) {
  if (value > PURE_16_BIT_MAX) return PURE_E_ZIP_UNIX_MODE_OVERFLOW;
  // Detect dangerous file types:
  if ((value & PURE_UNIX_MODE_MASK) == PURE_UNIX_MODE_BLK) {
    return PURE_E_ZIP_UNIX_MODE_BLOCK_DEVICE;
  }
  if ((value & PURE_UNIX_MODE_MASK) == PURE_UNIX_MODE_CHR) {
    return PURE_E_ZIP_UNIX_MODE_CHARACTER_DEVICE;
  }
  if ((value & PURE_UNIX_MODE_MASK) == PURE_UNIX_MODE_FIFO) {
    return PURE_E_ZIP_UNIX_MODE_FIFO;
  }
  if ((value & PURE_UNIX_MODE_MASK) == PURE_UNIX_MODE_SOCK) {
    return PURE_E_ZIP_UNIX_MODE_SOCKET;
  }
  // Detect dangerous permissions:
  // CVE-2005-0602
  // https://marc.info/?l=bugtraq&m=110960796331943&w=2
  // 01000:
  if (value & 512) return PURE_E_ZIP_UNIX_MODE_PERMISSIONS_STICKY;
  // 02000:
  if (value & 1024) return PURE_E_ZIP_UNIX_MODE_PERMISSIONS_SETGID;
  // 04000:
  if (value & 2048) return PURE_E_ZIP_UNIX_MODE_PERMISSIONS_SETUID;
  return 0;
}

int pure_zip_verify_time(const uint64_t value) {
  /*
  An MS-DOS time is a packed 16-bit (2 bytes) value in which bits in the value
  represent the hour, minute, and second:
  
     Bits:  0-4     5-10    11-15
     Unit:  Second  Minute  Hour
    Range:  0-59    0-59    0-23
  */
  if (value > PURE_16_BIT_MAX) return PURE_E_ZIP_TIME_OVERFLOW;
  uint64_t h = (value >> 11);
  uint64_t m = (value >> 5) & 63;
  uint64_t s = (value & 31) * 2;
  if (h > 23) return PURE_E_ZIP_TIME_HOUR_OVERFLOW;
  if (m > 59) return PURE_E_ZIP_TIME_MINUTE_OVERFLOW;
  if (s > 59) return PURE_E_ZIP_TIME_SECOND_OVERFLOW;
  return 0;
}

int pure_zip_verify_lfh(const pure_zip_lfh* header) {
  assert(
    header->length == (
      PURE_ZIP_LFH_MIN +
      header->file_name_length +
      header->extra_field_length
    )
  );
  {
    int error = pure_zip_verify_flags(header->general_purpose_bit_flag);
    if (error) return error;
  }
  {
    int error = pure_zip_verify_compression_method_sizes(
      header->compression_method,
      header->compressed_size,
      header->uncompressed_size
    );
    if (error) return error;
  }
  {
    int error = pure_zip_verify_date(header->last_mod_file_date);
    if (error) return error;
  }
  {
    int error = pure_zip_verify_time(header->last_mod_file_time);
    if (error) return error;
  }
  {
    int error = pure_zip_verify_file_name(
      header->file_name,
      header->file_name_length
    );
    if (error) return error;
  }
  {
    int error = pure_zip_verify_string(
      header->file_name,
      header->file_name_length,
      header->general_purpose_bit_flag & PURE_ZIP_FLAG_UTF8
    );
    if (error) return error;
  }
  {
    int error = pure_zip_verify_extra_field(
      header->extra_field,
      header->extra_field_length,
      header->file_name,
      header->file_name_length
    );
    if (error) return error;
  }
  assert(header->zip64 == 0 || header->zip64 == 1);
  return 0;
}

int pure_zip_verify_cdh(const pure_zip_cdh* header) {
  assert(
    header->length == (
      PURE_ZIP_CDH_MIN +
      header->file_name_length +
      header->extra_field_length +
      header->file_comment_length
    )
  );
  {
    int error = pure_zip_verify_flags(header->general_purpose_bit_flag);
    if (error) return error;
  }
  {
    int error = pure_zip_verify_compression_method_sizes(
      header->compression_method,
      header->compressed_size,
      header->uncompressed_size
    );
    if (error) return error;
  }
  // An LFH may have a zero compressed size with a non-zero uncompressed size
  // because the actual sizes may be in the DDR, but a CDH cannot be ex nihilo.
  // We cross-check the DDR against the CDH, so this check applies to the DDR:
  if (header->compressed_size == 0 && header->uncompressed_size != 0) {
    return PURE_E_ZIP_EX_NIHILO;
  }
  {
    int error = pure_zip_verify_date(header->last_mod_file_date);
    if (error) return error;
  }
  {
    int error = pure_zip_verify_time(header->last_mod_file_time);
    if (error) return error;
  }
  {
    int error = pure_zip_verify_file_name(
      header->file_name,
      header->file_name_length
    );
    if (error) return error;
  }
  {
    int error = pure_zip_verify_string(
      header->file_name,
      header->file_name_length,
      header->general_purpose_bit_flag & PURE_ZIP_FLAG_UTF8
    );
    if (error) return error;
  }
  {
    int error = pure_zip_verify_extra_field(
      header->extra_field,
      header->extra_field_length,
      header->file_name,
      header->file_name_length
    );
    if (error) return error;
  }
  {
    int error = pure_zip_verify_string(
      header->file_comment,
      header->file_comment_length,
      header->general_purpose_bit_flag & PURE_ZIP_FLAG_UTF8
    );
    if (error) return error;
  }
  {
    int error = pure_zip_verify_disk(header->disk);
    if (error) return error;
  }
  {
    int error = pure_zip_verify_unix_mode(header->unix_mode);
    if (error) return error;
  }
  {
    int error = pure_zip_verify_directory(header);
    if (error) return error;
  }
  assert(header->zip64 == 0 || header->zip64 == 1);
  return 0;
}

int pure_zip_decode_eief_64(
  const uint8_t* extra_field,
  const uint64_t length,
  uint64_t* compressed_size,
  uint64_t* uncompressed_size,
  uint64_t* relative_offset,
  uint64_t* disk,
  uint8_t* zip64,
  uint8_t lfh
) {
  assert(
    *compressed_size   == PURE_32_BIT_MAX ||
    *uncompressed_size == PURE_32_BIT_MAX ||
    *relative_offset   == PURE_32_BIT_MAX ||
    *disk              == PURE_16_BIT_MAX
  );
  assert(*zip64 == 0);
  assert(lfh == 0 || lfh == 1);
  if (length > PURE_ZIP_EXTRA_FIELD_MAX) return PURE_E_ZIP_EXTRA_FIELD_MAX;
  if (length != 0 && length < 4) return PURE_E_ZIP_EXTRA_FIELD_MIN;
  uint64_t offset = 0;
  while (offset + 4 <= length) {
    const uint16_t id = pure_u16(extra_field + offset + 0);
    const uint16_t size = pure_u16(extra_field + offset + 2);
    offset += 4;
    if (offset + size > length) {
      return PURE_E_ZIP_EXTRA_FIELD_ATTRIBUTE_OVERFLOW;
    }
    if (id == 0x0001) {
      *zip64 = 1;
      uint64_t index = 0;
      if (*uncompressed_size == PURE_32_BIT_MAX) {
        if ((index += 8) > size) return PURE_E_ZIP_EIEF_64_UNCOMPRESSED_SIZE;
        *uncompressed_size = pure_u64(extra_field + offset + index - 8);
      }
      if (*compressed_size == PURE_32_BIT_MAX) {
        if ((index += 8) > size) return PURE_E_ZIP_EIEF_64_COMPRESSED_SIZE;
        *compressed_size = pure_u64(extra_field + offset + index - 8);
      }
      if (*relative_offset == PURE_32_BIT_MAX) {
        if ((index += 8) > size) return PURE_E_ZIP_EIEF_64_RELATIVE_OFFSET;
        *relative_offset = pure_u64(extra_field + offset + index - 8);
      }
      if (*disk == PURE_16_BIT_MAX) {
        if ((index += 4) > size) return PURE_E_ZIP_EIEF_64_DISK;
        *disk = pure_u32(extra_field + offset + index - 4);
      }
      assert(offset + size <= length);
      if (index < size) {
        if (pure_zeroes(extra_field, offset + index, offset + size)) {
          return PURE_E_ZIP_EIEF_64_UNDERFLOW_ZEROED;
        } else {
          return PURE_E_ZIP_EIEF_64_UNDERFLOW_BUFFER_BLEED;
        }
      }
      // The EIEF in an LFH must include both uncompressed and compressed size:
      if (lfh && index != 16) return PURE_E_ZIP_EIEF_64_LFH;
      assert(index == size);
      return 0;
    }
    offset += size;
  }
  assert(offset <= length);
  return 0;
}

int pure_zip_decode_lfh(
  const uint8_t* buffer,
  const uint64_t size,
  const uint64_t offset,
  pure_zip_lfh* header
) {
  assert(offset < size);
  if (pure_overflow(offset, PURE_ZIP_LFH_MIN, size)) {
    return PURE_E_ZIP_LFH_OVERFLOW;
  }
  if (!pure_eq(buffer, size, offset, PURE_S_ZIP_LFH, PURE_L_ZIP_LFH)) {
    return PURE_E_ZIP_LFH_SIGNATURE;
  }
  header->offset = offset;
  assert(PURE_L_ZIP_LFH == 4);
  header->version_minimum = pure_u16(buffer + offset + 4);
  header->general_purpose_bit_flag = pure_u16(buffer + offset + 6);
  header->compression_method = pure_u16(buffer + offset + 8);
  header->last_mod_file_time = pure_u16(buffer + offset + 10);
  header->last_mod_file_date = pure_u16(buffer + offset + 12);
  header->crc32 = pure_u32(buffer + offset + 14);
  header->compressed_size = pure_u32(buffer + offset + 18);
  header->uncompressed_size = pure_u32(buffer + offset + 22);
  header->file_name_length = pure_u16(buffer + offset + 26);
  header->extra_field_length = pure_u16(buffer + offset + 28);
  assert(PURE_ZIP_LFH_MIN == 30);
  header->length = PURE_ZIP_LFH_MIN;
  header->file_name = (uint8_t*) (buffer + header->offset + header->length);
  header->length += header->file_name_length;
  if (pure_overflow(header->offset, header->length, size)) {
    return PURE_E_ZIP_LFH_FILE_NAME_OVERFLOW;
  }
  header->extra_field = (uint8_t*) (buffer + header->offset + header->length);
  header->length += header->extra_field_length;
  if (pure_overflow(header->offset, header->length, size)) {
    return PURE_E_ZIP_LFH_EXTRA_FIELD_OVERFLOW;
  }
  // ZIP64:
  header->zip64 = 0;
  if (
    header->compressed_size   == PURE_32_BIT_MAX ||
    header->uncompressed_size == PURE_32_BIT_MAX
  ) {
    uint64_t relative_offset = 0;
    uint64_t disk = 0;
    int error = pure_zip_decode_eief_64(
      header->extra_field,
      header->extra_field_length,
      &header->compressed_size,
      &header->uncompressed_size,
      &relative_offset,
      &disk,
      &header->zip64,
      1
    );
    if (error) return error;
  }
  {
    int error = pure_zip_verify_lfh(header);
    if (error) return error;
  }
  return 0;
}

int pure_zip_decode_ddr(
  const uint8_t* buffer,
  const uint64_t size,
  uint64_t offset,
  pure_zip_ddr* header
) {
  assert(offset < size);
  assert(header->zip64 == 0 || header->zip64 == 1);
  const uint64_t min = header->zip64 ? PURE_ZIP_DDR_64_MIN : PURE_ZIP_DDR_MIN;
  // The DDR signature is optional but we expect at least 4 bytes regardless:
  if (pure_overflow(offset, PURE_L_ZIP_DDR, size)) {
    return PURE_E_ZIP_DDR_OVERFLOW;
  }
  if (pure_eq(buffer, size, offset, PURE_S_ZIP_DDR, PURE_L_ZIP_DDR)) {
    header->offset = offset;
    header->length = PURE_L_ZIP_DDR + min;
    offset += PURE_L_ZIP_DDR;
  } else {
    header->offset = offset;
    header->length = min;
  }
  if (pure_overflow(offset, min, size)) return PURE_E_ZIP_DDR_OVERFLOW;
  if (header->zip64) {
    assert(min == 4 + 8 + 8);
    header->crc32 = pure_u32(buffer + offset + 0);
    header->compressed_size = pure_u64(buffer + offset + 4);
    header->uncompressed_size = pure_u64(buffer + offset + 12);
  } else {
    assert(min == 4 + 4 + 4);
    header->crc32 = pure_u32(buffer + offset + 0);
    header->compressed_size = pure_u32(buffer + offset + 4);
    header->uncompressed_size = pure_u32(buffer + offset + 8);
  }
  return 0;
}

int pure_zip_decode_cdh(
  const uint8_t* buffer,
  const uint64_t size,
  const uint64_t offset,
  pure_zip_cdh* header
) {
  assert(offset < size);
  if (pure_overflow(offset, PURE_ZIP_CDH_MIN, size)) {
    return PURE_E_ZIP_CDH_OVERFLOW;
  }
  if (!pure_eq(buffer, size, offset, PURE_S_ZIP_CDH, PURE_L_ZIP_CDH)) {
    return PURE_E_ZIP_CDH_SIGNATURE;
  }
  header->offset = offset;
  assert(PURE_L_ZIP_CDH == 4);
  header->version_made = pure_u16(buffer + offset + 4);
  header->version_minimum = pure_u16(buffer + offset + 6);
  header->general_purpose_bit_flag = pure_u16(buffer + offset + 8);
  header->compression_method = pure_u16(buffer + offset + 10);
  header->last_mod_file_time = pure_u16(buffer + offset + 12);
  header->last_mod_file_date = pure_u16(buffer + offset + 14);
  header->crc32 = pure_u32(buffer + offset + 16);
  header->compressed_size = pure_u32(buffer + offset + 20);
  header->uncompressed_size = pure_u32(buffer + offset + 24);
  header->file_name_length = pure_u16(buffer + offset + 28);
  header->extra_field_length = pure_u16(buffer + offset + 30);
  header->file_comment_length = pure_u16(buffer + offset + 32);
  header->disk = pure_u16(buffer + offset + 34);
  header->internal_file_attributes = pure_u16(buffer + offset + 36);
  header->external_file_attributes = pure_u32(buffer + offset + 38);
  header->relative_offset = pure_u32(buffer + offset + 42);
  assert(PURE_ZIP_CDH_MIN == 46);
  header->length = PURE_ZIP_CDH_MIN;
  header->file_name = (uint8_t*) (buffer + header->offset + header->length);
  header->length += header->file_name_length;
  if (pure_overflow(header->offset, header->length, size)) {
    return PURE_E_ZIP_CDH_FILE_NAME_OVERFLOW;
  }
  header->extra_field = (uint8_t*) (buffer + header->offset + header->length);
  header->length += header->extra_field_length;
  if (pure_overflow(header->offset, header->length, size)) {
    return PURE_E_ZIP_CDH_EXTRA_FIELD_OVERFLOW;
  }
  header->file_comment = (uint8_t*) (buffer + header->offset + header->length);
  header->length += header->file_comment_length;
  if (pure_overflow(header->offset, header->length, size)) {
    return PURE_E_ZIP_CDH_FILE_COMMENT_OVERFLOW;
  }
  header->unix_mode = 0;
  if ((header->version_made >> 8) == PURE_ZIP_VERSION_MADE_UNIX) {
    header->unix_mode = header->external_file_attributes >> 16;
  }
  header->directory = (
    (header->unix_mode & PURE_UNIX_MODE_MASK) == PURE_UNIX_MODE_DIR ||
    (header->external_file_attributes & 0x0010) ||
    (
      header->file_name_length > 0 &&
      header->file_name[header->file_name_length - 1] == PURE_FORWARD_SLASH
    )
  );
  // ZIP64:
  header->zip64 = 0;
  if (
    header->compressed_size   == PURE_32_BIT_MAX ||
    header->uncompressed_size == PURE_32_BIT_MAX ||
    header->relative_offset   == PURE_32_BIT_MAX ||
    header->disk              == PURE_16_BIT_MAX
  ) {
    int error = pure_zip_decode_eief_64(
      header->extra_field,
      header->extra_field_length,
      &header->compressed_size,
      &header->uncompressed_size,
      &header->relative_offset,
      &header->disk,
      &header->zip64,
      0
    );
    if (error) return error;
  }
  // These are critical checks and must not be moved to pure_zip_verify_cdh():
  if (header->relative_offset > size) {
    return PURE_E_ZIP_CDH_RELATIVE_OFFSET_OVERFLOW;
  }
  if (header->relative_offset > offset) {
    return PURE_E_ZIP_CDH_RELATIVE_OFFSET_OVERLAP;
  }
  {
    int error = pure_zip_verify_cdh(header);
    if (error) return error;
  }
  return 0;
}

int pure_zip_decode_eocdl_64(
  const uint8_t* buffer,
  const uint64_t size,
  const uint64_t offset,
  pure_zip_eocdl_64* header
) {
  assert(offset < size);
  if (pure_overflow(offset, PURE_ZIP_EOCDL_64, size)) {
    return PURE_E_ZIP_EOCDL_64_OVERFLOW;
  }
  if (
    !pure_eq(buffer, size, offset, PURE_S_ZIP_EOCDL_64, PURE_L_ZIP_EOCDL_64)
  ) {
    return PURE_E_ZIP_EOCDL_64_SIGNATURE;
  }
  header->offset = offset;
  assert(PURE_L_ZIP_EOCDL_64 == 4);
  header->disk = pure_u32(buffer + offset + 4);
  header->eocdr_64_offset = pure_u64(buffer + offset + 8);
  header->disks = pure_u32(buffer + offset + 16);
  assert(PURE_ZIP_EOCDL_64 == 20);
  header->length = PURE_ZIP_EOCDL_64;
  if (header->disk != 0) return PURE_E_ZIP_EOCDL_64_DISK;
  if (
    pure_overflow(
      header->eocdr_64_offset,
      PURE_ZIP_EOCDR_64_MIN,
      header->offset
    )
  ) {
    return PURE_E_ZIP_EOCDR_EOCDL_64_OVERFLOW;
  }
  if (header->disks != 0 && header->disks != 1) {
    return PURE_E_ZIP_EOCDL_64_DISKS;
  }
  return 0;
}

int pure_zip_decode_eocdr_64(
  const uint8_t* buffer,
  const uint64_t size,
  const uint64_t offset,
  pure_zip_eocdr_64* header
) {
  assert(offset < size);
  if (pure_overflow(offset, PURE_ZIP_EOCDR_64_MIN, size)) {
    return PURE_E_ZIP_EOCDR_64_OVERFLOW;
  }
  if (
    !pure_eq(buffer, size, offset, PURE_S_ZIP_EOCDR_64, PURE_L_ZIP_EOCDR_64)
  ) {
    return PURE_E_ZIP_EOCDR_64_SIGNATURE;
  }
  header->offset = offset;
  assert(PURE_L_ZIP_EOCDR_64 == 4);
  // The value stored in "size of zip64 end of central directory record" is the
  // size of the remaining record and excludes the leading 12 bytes:
  // size_remaining = PURE_ZIP_EOCDR_64_MIN + extensible_data_sector_length - 12
  // extensible_data_sector_length = size_remaining - PURE_ZIP_EOCDR_64_MIN + 12
  header->extensible_data_sector_length = (
    pure_u64(buffer + offset + 4) - (PURE_ZIP_EOCDR_64_MIN - 12)
  );
  header->version_made = pure_u16(buffer + offset + 12);
  header->version_minimum = pure_u16(buffer + offset + 14);
  header->disk = pure_u32(buffer + offset + 16);
  header->cd_disk = pure_u32(buffer + offset + 20);
  header->cd_disk_records = pure_u64(buffer + offset + 24);
  header->cd_records = pure_u64(buffer + offset + 32);
  header->cd_size = pure_u64(buffer + offset + 40);
  header->cd_offset = pure_u64(buffer + offset + 48);
  assert(PURE_ZIP_EOCDR_64_MIN == 56);
  header->length = PURE_ZIP_EOCDR_64_MIN;
  header->extensible_data_sector = (
    (uint8_t*) (buffer + header->offset + header->length)
  );
  header->length += header->extensible_data_sector_length;
  return 0;
}

int pure_zip_decode_eocdr_64_inherit(
  pure_zip_eocdr* a,
  const pure_zip_eocdr_64* b
) {
  // Inherit only those values that are maxed out in the EOCDR:
  if (a->disk == PURE_16_BIT_MAX) a->disk = b->disk;
  if (a->cd_disk == PURE_16_BIT_MAX) a->cd_disk = b->cd_disk;
  if (a->cd_disk_records == PURE_16_BIT_MAX) {
    a->cd_disk_records = b->cd_disk_records;
  }
  if (a->cd_records == PURE_16_BIT_MAX) a->cd_records = b->cd_records;
  if (a->cd_size == PURE_32_BIT_MAX) a->cd_size = b->cd_size;
  if (a->cd_offset == PURE_32_BIT_MAX) a->cd_offset = b->cd_offset;
  // Verify that all values are now in agreement between the EOCDR and EOCDR_64:
  if (a->disk != b->disk) return PURE_E_ZIP_DIFF_EOCDR_DISK;
  if (a->cd_disk != b->cd_disk) return PURE_E_ZIP_DIFF_EOCDR_CD_DISK;
  if (a->cd_disk_records != b->cd_disk_records) {
    return PURE_E_ZIP_DIFF_EOCDR_CD_DISK_RECORDS;
  }
  if (a->cd_records != b->cd_records) return PURE_E_ZIP_DIFF_EOCDR_CD_RECORDS;
  if (a->cd_size != b->cd_size) return PURE_E_ZIP_DIFF_EOCDR_CD_SIZE;
  if (a->cd_offset != b->cd_offset) return PURE_E_ZIP_DIFF_EOCDR_CD_OFFSET;
  return 0;
}

int pure_zip_decode_eocdr_64_upgrade(
  const uint8_t* buffer,
  const uint64_t size,
  pure_zip_eocdr* header
) {
  header->zip64 = 0;
  if (
    header->disk            != PURE_16_BIT_MAX &&
    header->cd_disk         != PURE_16_BIT_MAX &&
    header->cd_disk_records != PURE_16_BIT_MAX &&
    header->cd_records      != PURE_16_BIT_MAX &&
    header->cd_size         != PURE_32_BIT_MAX &&
    header->cd_offset       != PURE_32_BIT_MAX
  ) {
    return 0;
  }
  pure_zip_eocdl_64 eocdl_64;
  pure_zip_eocdr_64 eocdr_64;
  if (header->offset < PURE_ZIP_EOCDL_64) {
    return PURE_E_ZIP_EOCDL_64_NEGATIVE_OFFSET;
  }
  assert(header->offset >= PURE_ZIP_EOCDL_64);
  {
    int error = pure_zip_decode_eocdl_64(
      buffer,
      size,
      header->offset - PURE_ZIP_EOCDL_64,
      &eocdl_64
    );
    // A header field may actually be FFFF or FFFFFFFF without any Zip64 format:
    if (error == PURE_E_ZIP_EOCDL_64_SIGNATURE) return 0;
    if (error) return error;
  }
  assert(!pure_overflow(eocdl_64.offset, eocdl_64.length, header->offset));
  assert(eocdl_64.offset + eocdl_64.length == header->offset);
  assert(
    !pure_overflow(
      eocdl_64.eocdr_64_offset,
      PURE_ZIP_EOCDR_64_MIN,
      eocdl_64.offset
    )
  );
  {
    int error = pure_zip_decode_eocdr_64(
      buffer,
      size,
      eocdl_64.eocdr_64_offset,
      &eocdr_64
    );
    if (error) return error;
  }
  const uint64_t eocdl_offset = eocdr_64.offset + eocdr_64.length;
  assert(eocdl_offset > eocdr_64.offset);
  if (eocdl_offset > eocdl_64.offset) return PURE_E_ZIP_EOCDR_EOCDL_64_OVERFLOW;
  if (eocdl_offset < eocdl_64.offset) {
    assert(eocdl_64.offset <= size);
    if (pure_zeroes(buffer, eocdl_offset, eocdl_64.offset)) {
      return PURE_E_ZIP_EOCDR_EOCDL_64_UNDERFLOW_ZEROED;
    } else {
      return PURE_E_ZIP_EOCDR_EOCDL_64_UNDERFLOW_BUFFER_BLEED;
    }
  }
  assert(!pure_overflow(eocdr_64.offset, eocdr_64.length, eocdl_64.offset));
  assert(eocdr_64.offset + eocdr_64.length == eocdl_64.offset);
  {
    int error = pure_zip_decode_eocdr_64_inherit(header, &eocdr_64);
    if (error) return error;
  }
  header->zip64 = 1;
  header->offset = eocdr_64.offset;
  header->length = eocdr_64.length + eocdl_64.length + header->length;
  return 0;
}

int pure_zip_decode_eocdr(
  const uint8_t* buffer,
  const uint64_t size,
  const uint64_t offset,
  pure_zip_eocdr* header
) {
  assert(offset < size);
  if (pure_overflow(offset, PURE_ZIP_EOCDR_MIN, size)) {
    return PURE_E_ZIP_EOCDR_OVERFLOW;
  }
  if (!pure_eq(buffer, size, offset, PURE_S_ZIP_EOCDR, PURE_L_ZIP_EOCDR)) {
    return PURE_E_ZIP_EOCDR_SIGNATURE;
  }
  // We consider header->offset to start at EOCDR_64 or EOCDR.
  // We consider header->length to include EOCDR_64, EOCDL_64 and EOCDR.
  header->offset = offset;
  assert(PURE_L_ZIP_EOCDR == 4);
  header->disk = pure_u16(buffer + offset + 4);
  header->cd_disk = pure_u16(buffer + offset + 6);
  header->cd_disk_records = pure_u16(buffer + offset + 8);
  header->cd_records = pure_u16(buffer + offset + 10);
  header->cd_size = pure_u32(buffer + offset + 12);
  header->cd_offset = pure_u32(buffer + offset + 16);
  header->comment_length = pure_u16(buffer + offset + 20);
  assert(PURE_ZIP_EOCDR_MIN == 22);
  header->length = PURE_ZIP_EOCDR_MIN;
  header->comment = (uint8_t*) (buffer + header->offset + header->length);
  header->length += header->comment_length;
  if (pure_overflow(header->offset, header->length, size)) {
    return PURE_E_ZIP_EOCDR_COMMENT_OVERFLOW;
  }
  {
    // If we find an EOCDR_64 and EOCDL_64, we modify header->(offset, length):
    const uint64_t length = PURE_ZIP_EOCDR_MIN + header->comment_length;
    assert(header->length == length);
    int error = pure_zip_decode_eocdr_64_upgrade(buffer, size, header);
    if (error) return error;
    assert(header->zip64 == 0 || header->zip64 == 1);
    if (header->zip64) {
      const uint64_t length_64 = PURE_ZIP_EOCDR_64_MIN + PURE_ZIP_EOCDL_64;
      assert(header->offset <= offset - length_64);
      assert(header->length >= length + length_64);
    } else {
      assert(header->offset == offset);
      assert(header->length == length);
    }
  }
  if (header->cd_size < header->cd_records * PURE_ZIP_CDH_MIN) {
    return PURE_E_ZIP_EOCDR_SIZE_OVERFLOW;
  }
  if (header->cd_size > 0 && header->cd_records == 0) {
    return PURE_E_ZIP_EOCDR_SIZE_UNDERFLOW;
  }
  if (pure_overflow(header->cd_offset, header->cd_size, header->offset)) {
    return PURE_E_ZIP_CD_EOCDR_OVERFLOW;
  }
  if (header->disk != 0 || header->cd_disk != 0) {
    return PURE_E_ZIP_MULTIPLE_DISKS;
  }
  if (header->cd_disk_records != header->cd_records) {
    return PURE_E_ZIP_EOCDR_RECORDS;
  }
  {
    int error = pure_zip_verify_string(
      header->comment,
      header->comment_length,
      // The EOCDR has no General Purpose Bit Flag with which to indicate UTF-8.
      // Therefore, the comment encoding must always be CP437:
      0
    );
    if (error) return error;
  }
  uint64_t suffix_offset = header->offset + header->length;
  if (suffix_offset < size) {
    if (pure_zeroes(buffer, suffix_offset, size)) {
      return PURE_E_ZIP_APPENDED_DATA_ZEROED;
    } else {
      return PURE_E_ZIP_APPENDED_DATA_BUFFER_BLEED;
    }
  }
  return 0;
}

// Compare LFH against CDH taking Data Descriptor Record into account:
int pure_zip_diff_cld(
  const uint64_t cdh_value,
  const uint64_t lfh_value,
  const pure_zip_lfh* lfh
) {
  // LFH matches CDH:
  if (lfh_value == cdh_value) return 0;
  // LFH delegates value to Data Descriptor:
  if ((lfh->general_purpose_bit_flag & (1 << 3)) && lfh_value == 0) return 0;
  // LFH diverges from CDH:
  return 1;
}

// Compare DDR against CDH:
int pure_zip_diff_cdh_ddr(const pure_zip_cdh* cdh, const pure_zip_ddr* ddr) {
  if (ddr->crc32 != cdh->crc32) {
    return PURE_E_ZIP_DIFF_DDR_CRC32;
  }
  if (ddr->compressed_size != cdh->compressed_size) {
    return PURE_E_ZIP_DIFF_DDR_COMPRESSED_SIZE;
  }
  if (ddr->uncompressed_size != cdh->uncompressed_size) {
    return PURE_E_ZIP_DIFF_DDR_UNCOMPRESSED_SIZE;
  }
  return 0;
}

// Compare LFH against CDH:
int pure_zip_diff_cdh_lfh(const pure_zip_cdh* cdh, const pure_zip_lfh* lfh) {
  if (lfh->general_purpose_bit_flag != cdh->general_purpose_bit_flag) {
    return PURE_E_ZIP_DIFF_LFH_GENERAL_PURPOSE_BIT_FLAG;
  }
  if (lfh->compression_method != cdh->compression_method) {
    return PURE_E_ZIP_DIFF_LFH_COMPRESSION_METHOD;
  }
  if (lfh->last_mod_file_time != cdh->last_mod_file_time) {
    return PURE_E_ZIP_DIFF_LFH_LAST_MOD_FILE_TIME;
  }
  if (lfh->last_mod_file_date != cdh->last_mod_file_date) {
    return PURE_E_ZIP_DIFF_LFH_LAST_MOD_FILE_DATE;
  }
  if (pure_zip_diff_cld(cdh->crc32, lfh->crc32, lfh)) {
    return PURE_E_ZIP_DIFF_LFH_CRC32;
  }
  if (pure_zip_diff_cld(cdh->compressed_size, lfh->compressed_size, lfh)) {
    return PURE_E_ZIP_DIFF_LFH_COMPRESSED_SIZE;
  }
  if (pure_zip_diff_cld(cdh->uncompressed_size, lfh->uncompressed_size, lfh)) {
    return PURE_E_ZIP_DIFF_LFH_UNCOMPRESSED_SIZE;
  }
  if (lfh->file_name_length != cdh->file_name_length) {
    return PURE_E_ZIP_DIFF_LFH_FILE_NAME_LENGTH;
  }
  // We assume decode_lfh() and decode_cdh() have already checked for overflow:
  if (memcmp(lfh->file_name, cdh->file_name, (size_t) cdh->file_name_length)) {
    return PURE_E_ZIP_DIFF_LFH_FILE_NAME;
  }
  return 0;
}

// Compare LFH against DDR:
int pure_zip_diff_ddr_lfh(const pure_zip_ddr* ddr, const pure_zip_lfh* lfh) {
  if (lfh->crc32 != 0 && lfh->crc32 != ddr->crc32) {
    return PURE_E_ZIP_DIFF_LFH_DDR_CRC32;
  }
  if (
    lfh->compressed_size != 0 &&
    lfh->compressed_size != ddr->compressed_size
  ) {
    return PURE_E_ZIP_DIFF_LFH_DDR_COMPRESSED_SIZE;
  }
  if (
    lfh->uncompressed_size != 0 &&
    lfh->uncompressed_size != ddr->uncompressed_size
  ) {
    return PURE_E_ZIP_DIFF_LFH_DDR_UNCOMPRESSED_SIZE;
  }
  return 0;
}

int pure_zip_inflate_raw(
  const uint8_t* compressed,
  const uint64_t compressed_size,
  const uint8_t* uncompressed,
  const uint64_t uncompressed_size
) {
  if (uncompressed_size == 0) return 0;
  z_stream z;
  z.zalloc = Z_NULL;
  z.zfree = Z_NULL;
  z.opaque = Z_NULL;
  z.avail_in = 0;
  z.avail_out = 0;
  z.next_in = Z_NULL;
  z.next_out = Z_NULL;
  // We indicate raw inflate to zlib by setting window_bits to negative:
  // We further allow for the greatest possible compression window size (15).
  int window_bits = -15;
  {
    int error = inflateInit2(&z, window_bits);
    if (error != Z_OK) return PURE_E_ZIP_INFLATE;
  }
  // TO DO: Switch to streaming zlib inflate to support larger file sizes.
  assert(sizeof(z.avail_in) >= 4);
  assert(sizeof(z.avail_out) >= 4);
  assert(compressed_size <= UINT32_MAX);
  assert(uncompressed_size <= UINT32_MAX);
  z.avail_in = (unsigned int) compressed_size;
  z.avail_out = (unsigned int) uncompressed_size;
  z.next_in = (uint8_t*) compressed;
  z.next_out = (uint8_t*) uncompressed;
  int error = inflate(&z, Z_FINISH);
  (void) inflateEnd(&z);
  if (error == Z_STREAM_END) {
    if (z.avail_in > 0) return PURE_E_ZIP_INFLATE_COMPRESSED_UNDERFLOW;
    if (z.avail_out > 0) return PURE_E_ZIP_INFLATE_UNCOMPRESSED_UNDERFLOW;
    assert(z.total_in == compressed_size);
    assert(z.total_out == uncompressed_size);
    return 0;
  } else {
    if (error == Z_NEED_DICT) return PURE_E_ZIP_INFLATE_DICTIONARY;
    if (error == Z_STREAM_ERROR) return PURE_E_ZIP_INFLATE_STREAM;
    if (error == Z_DATA_ERROR) return PURE_E_ZIP_INFLATE_DATA;
    if (error == Z_MEM_ERROR) return PURE_E_ZIP_INFLATE_MEMORY;
    if (error == Z_BUF_ERROR) {
      // If avail_out and avail_in are both zero then it is likely an output
      // overflow since the input stream has more chance of terminating first.
      // We therefore check avail_out first since we cannot further distinguish.
      if (!z.avail_out) return PURE_E_ZIP_BOMB_INFLATE_UNCOMPRESSED_OVERFLOW;
      if (!z.avail_in) return PURE_E_ZIP_BOMB_INFLATE_COMPRESSED_OVERFLOW;
    }
    return PURE_E_ZIP_INFLATE;
  }
}

int pure_zip_locate_eocdr(
  const uint8_t* buffer,
  const uint64_t size,
  uint64_t* offset
) {
  assert(*offset == 0);
  if (size < PURE_ZIP_EOCDR_MIN) return PURE_E_ZIP_TOO_SMALL;
  int64_t index = size - PURE_ZIP_EOCDR_MIN;
  // The EOCDR can be at most EOCDR_MIN plus a variable length comment.
  // The variable length of the comment can be at most a 16-bit integer.
  // Assuming no garbage after the EOCDR, our maximum search distance is:
  int64_t floor = size - PURE_ZIP_EOCDR_MIN - PURE_ZIP_EOCDR_COMMENT_MAX;
  if (index < 0) index = 0;
  if (floor < 0) floor = 0;
  assert(!pure_overflow((uint64_t) index, PURE_L_ZIP_EOCDR, size));
  assert(PURE_L_ZIP_EOCDR == 4);
  assert(sizeof(PURE_S_ZIP_EOCDR) == 4);
  while (index >= floor) {
    if (
      buffer[index + 0] == PURE_S_ZIP_EOCDR[0] &&
      buffer[index + 1] == PURE_S_ZIP_EOCDR[1] &&
      buffer[index + 2] == PURE_S_ZIP_EOCDR[2] &&
      buffer[index + 3] == PURE_S_ZIP_EOCDR[3]
    ) {
      assert(size >= PURE_ZIP_EOCDR_MIN);
      assert(!pure_overflow((uint64_t) index, 4, size));
      *offset = (uint64_t) index;
      return 0;
    }
    index--;
  }
  return PURE_E_ZIP_EOCDR_NOT_FOUND;
}

int pure_zip_locate_first_lfh(
  const uint8_t* buffer,
  const uint64_t size,
  const pure_zip_eocdr* eocdr,
  uint64_t* offset
) {
  assert(size >= 8);
  assert(*offset == 0);
  // We expect a Local File Header or End Of Central Directory Record signature:
  // TO DO: Test empty file OK.
  // TO DO: Test non-empty file OK.
  const uint8_t* string = eocdr->cd_records ? PURE_S_ZIP_LFH : PURE_S_ZIP_EOCDR;
  const uint64_t string_size = PURE_L_ZIP_LFH;
  assert(PURE_L_ZIP_EOCDR == PURE_L_ZIP_LFH);
  if (pure_eq(buffer, size, 0, string, string_size)) {
    *offset = 0;
    return 0;
  }
  // A spanned/split archive may be preceded by a special spanning signature:
  // See APPNOTE 8.5.3 and 8.5.4.
  // TO DO: Test spanned file OK (empty, non-empty).
  // TO DO: Test split file OK (empty, non-empty).
  if (
    pure_eq(buffer, size, 0, PURE_S_ZIP_SPAN, PURE_L_ZIP_SPAN) ||
    pure_eq(buffer, size, 0, PURE_S_ZIP_TEMP, PURE_L_ZIP_TEMP)
  ) {
    assert(PURE_L_ZIP_TEMP == PURE_L_ZIP_SPAN);
    if (pure_eq(buffer, size, PURE_L_ZIP_SPAN, string, string_size)) {
      *offset = PURE_L_ZIP_SPAN;
      return 0;
    }
  }
  uint64_t prepended_data_size = 0;
  int error = pure_search(
    buffer,                             // buffer
    size,                               // buffer_size
    0,                                  // search_offset
    PURE_ZIP_PREPENDED_DATA_SEARCH_MAX, // search_size
    string,                             // string
    string_size,                        // string_size
    &prepended_data_size                // offset
  );
  // We could not find any end to the prepended data:
  if (error) return PURE_E_ZIP_PREPENDED_DATA;
  assert(prepended_data_size != 0);
  assert(prepended_data_size <= size);
  if (pure_zeroes(buffer, 0, prepended_data_size)) {
    return PURE_E_ZIP_PREPENDED_DATA_ZEROED;
  } else {
    return PURE_E_ZIP_PREPENDED_DATA_BUFFER_BLEED;
  }
}

// Forward declaration required by pure_zip_data():
int pure_zip_meta_data(
  pure_ctx* ctx,
  const uint8_t* buffer,
  const uint64_t size
);

int pure_zip_data(
  pure_ctx* ctx,
  const uint8_t* buffer,
  const pure_zip_cdh* cdh,
  const pure_zip_lfh* lfh,
  uint8_t** data,
  uint64_t* data_size
) {
  if (cdh->directory) {
    assert(cdh->compressed_size == 0);
    assert(cdh->uncompressed_size == 0);
    return 0;
  }
  if (cdh->uncompressed_size == 0) {
    if (cdh->compressed_size == 0) return 0;
    if (
      cdh->compressed_size == 2 &&
      cdh->compression_method == PURE_ZIP_COMPRESSION_METHOD_DEFLATE &&
      buffer[cdh->relative_offset + lfh->length + 0] == 3 &&
      buffer[cdh->relative_offset + lfh->length + 1] == 0
    ) {
      return 0;
    }
    return PURE_E_ZIP_AD_NIHILO;
  }
  assert(cdh->compressed_size > 0);
  assert(cdh->compressed_size <= SIZE_MAX);
  assert(cdh->uncompressed_size > 0);
  assert(cdh->uncompressed_size <= SIZE_MAX);
  // We verify the compression ratio here, after first checking for LFH overlap:
  // We do this to return PURE_E_ZIP_BOMB_FIFIELD before PURE_E_ZIP_BOMB_RATIO.
  if (
    pure_overflow(ctx->compressed_size, cdh->compressed_size, UINT64_MAX) ||
    pure_overflow(ctx->uncompressed_size, cdh->uncompressed_size, UINT64_MAX)
  ) {
    return PURE_E_UINT64_OVERFLOW;
  }
  ctx->compressed_size += cdh->compressed_size;
  ctx->uncompressed_size += cdh->uncompressed_size;
  {
    int error = pure_zip_verify_compression_ratio(
      ctx->compressed_size,
      ctx->uncompressed_size
    );
    if (error) return error;
  }
  uint8_t* raw = NULL;
  if (cdh->compression_method == PURE_ZIP_COMPRESSION_METHOD_DEFLATE) {
    {
      int error = pure_realloc(data, data_size, cdh->uncompressed_size);
      if (error) return error;
    }
    assert(*data != NULL);
    assert(*data_size >= cdh->uncompressed_size);
    {
      int error = pure_zip_inflate_raw(
        buffer + cdh->relative_offset + lfh->length, // compressed
        cdh->compressed_size,                        // compressed_size
        *data,                                       // uncompressed
        cdh->uncompressed_size                       // uncompressed_size
      );
      if (error) return error;
    }
    raw = *data;
  } else {
    assert(cdh->compression_method == PURE_ZIP_COMPRESSION_METHOD_NONE);
    raw = (uint8_t*) (buffer + cdh->relative_offset + lfh->length);
  }
  assert(raw != NULL);
  {
    uint64_t checksum = 0;
    int error = pure_zip_crc32(raw, cdh->uncompressed_size, &checksum);
    if (error) return error;
    if (checksum != cdh->crc32) return PURE_E_ZIP_CRC32;
  }
  // TO DO: Check for common ZIP extensions in addition to PK signature.
  if (pure_eq(raw, cdh->uncompressed_size, 0, PURE_S_ZIP_PK, PURE_L_ZIP_PK)) {
    int error = pure_zip_meta_data(ctx, raw, cdh->uncompressed_size);
    if (error) return error;
  } else {
    if ((++ctx->files) > PURE_FILES_MAX) return PURE_E_ZIP_BOMB_FILES;
  }
  return 0;
}

int pure_zip_meta(
  pure_ctx* ctx,
  const uint8_t* buffer,
  const uint64_t size,
  uint8_t** data,
  uint64_t* data_size
) {
  // Update and check context against limits:
  if ((++ctx->depth) > PURE_DEPTH_MAX) return PURE_E_ZIP_BOMB_DEPTH;
  if ((++ctx->files) > PURE_FILES_MAX) return PURE_E_ZIP_BOMB_FILES;
  if ((++ctx->archives) > PURE_ARCHIVES_MAX) return PURE_E_ZIP_BOMB_ARCHIVES;
  if (pure_overflow(ctx->size, size, UINT64_MAX)) return PURE_E_UINT64_OVERFLOW;
  if ((ctx->size += size) > PURE_SIZE_MAX) return PURE_E_SIZE_MAX;

  // A zip file must contain at least an end of central directory record:
  if (size < PURE_ZIP_EOCDR_MIN) return PURE_E_ZIP_TOO_SMALL;

  // ZIP64:
  if (size > 4294967295) return PURE_E_ZIP_SIZE_4GB;

  // Malicious archive signatures (almost certainly when masquerading as a ZIP):
  if (pure_eq(buffer, size, 0, PURE_S_RAR, PURE_L_RAR)) return PURE_E_ZIP_RAR;
  if (pure_eq(buffer, size, 0, PURE_S_TAR, PURE_L_TAR)) return PURE_E_ZIP_TAR;
  if (pure_eq(buffer, size, 0, PURE_S_XAR, PURE_L_XAR)) return PURE_E_ZIP_XAR;
  
  // Locate and decode end of central directory record:
  uint64_t eocdr_offset = 0;
  pure_zip_eocdr eocdr;
  {
    int error = pure_zip_locate_eocdr(buffer, size, &eocdr_offset);
    if (error) return error;
  }
  {
    int error = pure_zip_decode_eocdr(buffer, size, eocdr_offset, &eocdr);
    if (error) return error;
  }

  // Locate the offset of the first local file header:
  uint64_t lfh_offset = 0;
  {
    int error = pure_zip_locate_first_lfh(buffer, size, &eocdr, &lfh_offset);
    if (error) return error;
    assert(lfh_offset == 0 || lfh_offset == PURE_L_ZIP_SPAN);
  }

  // Compare central directory headers with local file headers:
  pure_zip_cdh cdh;
  pure_zip_lfh lfh;
  pure_zip_ddr ddr;
  pure_zip_cdh cdh_p;
  pure_zip_lfh lfh_p;
  uint64_t cdh_offset = eocdr.cd_offset;
  uint64_t cdh_record = 0;
  while (cdh_record < eocdr.cd_records) {
    // Central Directory Header:
    {
      int error = pure_zip_decode_cdh(buffer, size, cdh_offset, &cdh);
      if (error) return error;
    }
    if (lfh_offset > cdh.relative_offset) {
      if (
        cdh.directory && cdh.relative_offset == 0 &&
        cdh.crc32 == 0 && cdh.compressed_size == 0 && cdh.uncompressed_size == 0
      ) {
        return PURE_E_ZIP_DIRECTORY_HAS_NO_LFH;
      }
      return PURE_E_ZIP_BOMB_FIFIELD;
    }
    if (lfh_offset < cdh.relative_offset) {
      assert(cdh.relative_offset <= size);
      if (pure_zeroes(buffer, lfh_offset, cdh.relative_offset)) {
        return PURE_E_ZIP_LFH_UNDERFLOW_ZEROED;
      } else {
        return PURE_E_ZIP_LFH_UNDERFLOW_BUFFER_BLEED;
      }
    }
    // Local File Header:
    {
      assert(cdh.relative_offset == lfh_offset);
      int error = pure_zip_decode_lfh(buffer, size, cdh.relative_offset, &lfh);
      if (error) return error;
    }
    {
      int error = pure_zip_diff_cdh_lfh(&cdh, &lfh);
      if (error) return error;
    }
    {
      int error = pure_zip_verify_symlink(&cdh, &lfh, buffer);
      if (error) return error;
    }
    assert(lfh.length >= PURE_ZIP_LFH_MIN);
    lfh_offset += lfh.length;
    // File Data (compressed or uncompressed):
    if (pure_overflow(lfh_offset, cdh.compressed_size, UINT64_MAX)) {
      return PURE_E_UINT64_OVERFLOW;
    }
    lfh_offset += cdh.compressed_size;
    if (lfh_offset > size) return PURE_E_ZIP_LFH_DATA_OVERFLOW;
    // Data Descriptor Record (optional):
    if (lfh.general_purpose_bit_flag & (1 << 3)) {
      {
        ddr.zip64 = lfh.zip64;
        int error = pure_zip_decode_ddr(buffer, size, lfh_offset, &ddr);
        if (error) return error;
      }
      {
        int error = pure_zip_diff_cdh_ddr(&cdh, &ddr);
        if (error) return error;
      }
      {
        int error = pure_zip_diff_ddr_lfh(&ddr, &lfh);
        if (error) return error;
      }
      lfh_offset += ddr.length;
    }
    if (lfh_offset > eocdr.cd_offset) return PURE_E_ZIP_LF_OVERFLOW;
    // We descend into the data only after checking for LFH overlap above:
    // We can therefore descend only after decoding at least two entries.
    if (cdh_record > 0) {
      int error = pure_zip_data(ctx, buffer, &cdh_p, &lfh_p, data, data_size);
      if (error) return error;
    }
    // Shallow copy the CDH and LFH to descend next time around the loop:
    cdh_p = cdh;
    lfh_p = lfh;
    assert(cdh.length >= PURE_ZIP_CDH_MIN);
    cdh_offset += cdh.length;
    cdh_record++;
  }
  // Descend into the previous CDH and LFH:
  if (cdh_record > 0) {
    int error = pure_zip_data(ctx, buffer, &cdh_p, &lfh_p, data, data_size);
    if (error) return error;
  }
  if (lfh_offset > eocdr.cd_offset) return PURE_E_ZIP_LF_OVERFLOW;
  if (lfh_offset < eocdr.cd_offset) {
    assert(eocdr.cd_offset <= size);
    if (pure_zeroes(buffer, lfh_offset, eocdr.cd_offset)) {
      return PURE_E_ZIP_LF_UNDERFLOW_ZEROED;
    } else {
      return PURE_E_ZIP_LF_UNDERFLOW_BUFFER_BLEED;
    }
  }
  uint64_t cdh_offset_expected = eocdr.cd_offset + eocdr.cd_size;
  if (cdh_offset > cdh_offset_expected) return PURE_E_ZIP_CD_OVERFLOW;
  if (cdh_offset < cdh_offset_expected) {
    assert(cdh_offset_expected <= size);
    if (pure_zeroes(buffer, cdh_offset, cdh_offset_expected)) {
      return PURE_E_ZIP_CD_UNDERFLOW_ZEROED;
    } else {
      return PURE_E_ZIP_CD_UNDERFLOW_BUFFER_BLEED;
    }
  }
  if (cdh_offset < eocdr.offset) {
    assert(eocdr.offset <= size);
    if (pure_zeroes(buffer, cdh_offset, eocdr.offset)) {
      return PURE_E_ZIP_CD_EOCDR_UNDERFLOW_ZEROED;
    } else {
      return PURE_E_ZIP_CD_EOCDR_UNDERFLOW_BUFFER_BLEED;
    }
  }
  assert(cdh_offset == eocdr.offset);
  assert(cdh_offset + eocdr.length == size);
  assert(ctx->depth > 0);
  ctx->depth--;
  return PURE_E_OK;
}

int pure_zip_meta_data(
  pure_ctx* ctx,
  const uint8_t* buffer,
  const uint64_t size
) {
  uint8_t* data = NULL;
  uint64_t data_size = 0;
  int error = pure_zip_meta(ctx, buffer, size, &data, &data_size);
  pure_free(&data, &data_size);
  assert(data == NULL);
  assert(data_size == 0);
  return error;
}

int pure_zip(
  const uint8_t* buffer, // Zip file buffer
  const uint64_t size,   // Size of zip file buffer in bytes
  const uint64_t flags   // Bit flags (optional)
) {
  // SPEC: https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT

  // Further, as per ISO/IEC 21320-1:2015, we disable support for:
  // * multiple disks
  // * encryption and archive headers
  // * encryption mechanisms
  // * compression methods other than 0 or 8
  // * ZIP64 version 2 (and we also disable ZIP64 version 1)
  // * unused and reserved flags
  
  // Contrary to ISO/IEC 21320-1:2015, we do not require:
  // * `version needed to extract` to be at most 45 (too many false positives)
  // * bit 11 (UTF-8) when a string byte exceeds 0x7F (this could also be CP437)
  pure_ctx ctx;
  ctx.flags = flags;
  ctx.depth = 0;
  ctx.files = 0;
  ctx.archives = 0;
  ctx.size = 0;
  ctx.compressed_size = 0;
  ctx.uncompressed_size = 0;
  return pure_zip_meta_data(&ctx, buffer, size);
}

int pure_zip_bomb(const int error) {
  if (error == PURE_E_ZIP_BOMB_ARCHIVES) return 1;
  if (error == PURE_E_ZIP_BOMB_DEPTH) return 1;
  if (error == PURE_E_ZIP_BOMB_FILES) return 1;
  if (error == PURE_E_ZIP_BOMB_FIFIELD) return 1;
  if (error == PURE_E_ZIP_BOMB_RATIO) return 1;
  if (error == PURE_E_ZIP_BOMB_INFLATE_COMPRESSED_OVERFLOW) return 1;
  if (error == PURE_E_ZIP_BOMB_INFLATE_UNCOMPRESSED_OVERFLOW) return 1;
  return 0;
}

const char* pure_error_code(const int error) {
  assert(error >= 0);
  assert(error < PURE_E_ENUM_LENGTH);
  if (error < 0 || error >= PURE_E_ENUM_LENGTH) return "";
  return PURE_E_CODES[error];
}

const char* pure_error_string(const int error) {
  assert(error >= 0);
  assert(error < PURE_E_ENUM_LENGTH);
  if (error < 0 || error >= PURE_E_ENUM_LENGTH) return "";
  return PURE_E_STRINGS[error];
}

#endif /* PURE_H */
