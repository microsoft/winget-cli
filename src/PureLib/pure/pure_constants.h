const uint64_t PURE_16_BIT_MAX = 0xFFFF;
const uint64_t PURE_32_BIT_MAX = 0xFFFFFFFF;
const uint64_t PURE_MALLOC_MIN = 65536;
const uint64_t PURE_PATH_COMPONENT_MAX = 255;
const uint64_t PURE_PATH_MAX = 4096;
const uint64_t PURE_UNIX_MODE_MASK = 61440;
const uint64_t PURE_UNIX_MODE_BLK = 24576;
const uint64_t PURE_UNIX_MODE_CHR = 8192;
const uint64_t PURE_UNIX_MODE_DIR = 16384;
const uint64_t PURE_UNIX_MODE_FIFO = 4096;
const uint64_t PURE_UNIX_MODE_LNK = 40960;
const uint64_t PURE_UNIX_MODE_SOCK = 49152;

const uint64_t PURE_ARCHIVES_MAX = 1000;
const uint64_t PURE_DEPTH_MAX = 4;
const uint64_t PURE_FILES_MAX = 10000;
const uint64_t PURE_SIZE_MAX = 34359738368ULL;

const uint64_t PURE_BACKSLASH = 92;
const uint64_t PURE_FORWARD_SLASH = 47;

const uint64_t PURE_ZIP_CDH_MIN = 46;
const uint64_t PURE_ZIP_COMPRESSION_METHOD_DEFLATE = 8;
const uint64_t PURE_ZIP_COMPRESSION_METHOD_NONE = 0;
const uint64_t PURE_ZIP_COMPRESSION_RATIO_SCORE_MAX = 17179869184ULL;
const uint64_t PURE_ZIP_COMPRESSION_RATIO_SIZE_MIN = 128 * 1024 * 1024;
const uint64_t PURE_ZIP_DDR_64_MIN = 20;
const uint64_t PURE_ZIP_DDR_MIN = 12;
const uint64_t PURE_ZIP_EOCDL_64 = 20;
const uint64_t PURE_ZIP_EOCDR_64_MIN = 56;
const uint64_t PURE_ZIP_EOCDR_MIN = 22;
const uint64_t PURE_ZIP_EOCDR_COMMENT_MAX = 65535;
const uint64_t PURE_ZIP_EXTRA_FIELD_MAX = 4096;
const uint64_t PURE_ZIP_FLAG_UTF8 = 1 << 11;
const uint64_t PURE_ZIP_LFH_MIN = 30;
const uint64_t PURE_ZIP_PREPENDED_DATA_SEARCH_MAX = 1024;
const uint64_t PURE_ZIP_STRING_MAX = 16384;
const uint64_t PURE_ZIP_VERSION_MADE_UNIX = 3;

const uint8_t PURE_CONTROL_CHARACTER[256] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

