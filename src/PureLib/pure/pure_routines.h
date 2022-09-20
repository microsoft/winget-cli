int pure_eq(
  const uint8_t* buffer,
  const uint64_t buffer_size,   // The absolute length of the entire buffer.
  const uint64_t buffer_offset, // The offset from which to start comparing.
  const uint8_t* string,
  const uint64_t string_size
) {
  assert(buffer_offset <= buffer_size);
  assert(string_size > 0);
  if (buffer_offset + string_size > buffer_size) return 0;
  return memcmp(buffer + buffer_offset, string, (size_t) string_size) == 0;
}

int pure_free(uint8_t** data, uint64_t* data_size) {
  if (*data_size == 0) {
    assert(*data == NULL);
    // We never call malloc for 0.
    // We always assert that `required` is greater than 0.
    // We may therefore use `data_size == 0` to indicate the lack of allocation.
  } else {
    assert(*data != NULL);
    free(*data);
    *data = NULL;
    *data_size = 0;
  }
  return 0;
}

int pure_overflow(
  const uint64_t offset,
  const uint64_t length,
  const uint64_t available
) {
  if (available < length) return 1;
  if (offset > available - length) return 1;
  return 0;
}

int pure_realloc(
  uint8_t** data,
  uint64_t* data_size,
  const uint64_t required
) {
  assert(required > 0); // N.B. See comment in pure_free().
  assert(required <= SIZE_MAX);
  if (*data_size > 0 && *data_size < required) {
    pure_free(data, data_size);
    assert(*data == NULL);
    assert(*data_size == 0);
  }
  if (*data_size == 0) {
    assert(*data == NULL);
    uint64_t size = required;
    if (size < PURE_MALLOC_MIN) size = PURE_MALLOC_MIN;
    *data = (uint8_t*) malloc(size);
    if (*data == NULL) return PURE_E_MALLOC;
    *data_size = size;
  }
  assert(*data != NULL);
  assert(*data_size >= required);
  return 0;
}

int pure_search(
  const uint8_t* buffer,
  const uint64_t buffer_size,
  const uint64_t search_offset,
        uint64_t search_size,
  const uint8_t* string,
  const uint64_t string_size,
  uint64_t* offset
) {
  assert(*offset == 0);
  assert(string_size > 0);
  if (search_offset >= buffer_size) return PURE_E_STRING_NOT_FOUND;
  if (search_offset + search_size > buffer_size) {
    search_size = buffer_size - search_offset;
  }
  assert(search_offset + search_size <= buffer_size);
  if (search_size < string_size) return PURE_E_STRING_NOT_FOUND;
  uint64_t index = search_offset;
  uint64_t length = search_offset + search_size - string_size;
  assert(length + string_size <= buffer_size);
  while (index < length) {
    if (
      // Avoid a function call most of the time:
      buffer[index] == string[0] &&
      // Check the string in full:
      pure_eq(
        buffer,
        buffer_size,
        index,
        string,
        string_size
      )
    ) {
      *offset = index;
      return 0;
    }
    index++;
  }
  return PURE_E_STRING_NOT_FOUND;
}

uint16_t pure_u16(const uint8_t* buffer) {
  const uint8_t a = buffer[0];
  const uint8_t b = buffer[1];
  return (a << 0) | (b << 8);
}

uint32_t pure_u32(const uint8_t* buffer) {
  const uint8_t a = buffer[0];
  const uint8_t b = buffer[1];
  const uint8_t c = buffer[2];
  const uint8_t d = buffer[3];
  return (a << 0) | (b << 8) | (c << 16) | (d << 24);
}

uint64_t pure_u64(const uint8_t* buffer) {
  const uint32_t a = pure_u32(buffer + 0);
  const uint32_t b = pure_u32(buffer + 4);
  return ((uint64_t) b << 32) + a;
}

int pure_zeroes(
  const uint8_t* buffer, 
        uint64_t offset,
  const uint64_t length
) {
  assert(offset <= length);
  // TO DO: Optimize: Perform 64-bit comparisons:
  while (offset < length) {
    if (buffer[offset++] != 0) return 0;
  }
  return 1;
}

// Find end of component delimited by slash or EOF.
uint64_t pure_path_component_index(
  const uint8_t* path,
  uint64_t index,
  const uint64_t length
) {
  assert(index <= length);
  while (index < length) {
    if (path[index] == PURE_BACKSLASH || path[index] == PURE_FORWARD_SLASH) {
      return index;
    } else {
      index++;
    }
  }
  return index;
}

int pure_path_component_overflow(const uint8_t* path, const uint64_t length) {
  if (length < PURE_PATH_COMPONENT_MAX) return 0;
  uint64_t start = 0;
  while (start < length) {
    uint64_t end = pure_path_component_index(path, start, length);
    if (end - start > PURE_PATH_COMPONENT_MAX) return 1;
    start = end + 1;
  }
  return 0;
}

int pure_path_control_characters_iconr(
  const uint8_t* path,
  const uint64_t length
) {
  if (length < PURE_L_ICONR) return 0;
  uint64_t offset = length - PURE_L_ICONR;
  if (!pure_eq(path, length, offset, PURE_S_ICONR, PURE_L_ICONR)) return 0;
  return (
    offset == 0 ||
    // Do not fall for partial path component matches:
    path[offset - 1] == PURE_BACKSLASH ||
    path[offset - 1] == PURE_FORWARD_SLASH
  );
}

int pure_path_control_characters(const uint8_t* path, const uint64_t length) {
  // We want to check for control characters, except the "\r" in "Icon\r" files:
  uint64_t excluding_iconr_length = (uint64_t) length;
  if (pure_path_control_characters_iconr(path, length)) {
    assert(excluding_iconr_length >= PURE_L_ICONR);
    excluding_iconr_length -= PURE_L_ICONR;
  }
  for (uint64_t index = 0; index < excluding_iconr_length; index++) {
    if (PURE_CONTROL_CHARACTER[path[index]]) return 1;
  }
  return 0;
}

int pure_path_double_dots(const uint8_t* path, const uint64_t length) {
  uint64_t start = 0;
  while (start < length) {
    uint64_t end = pure_path_component_index(path, start, length);
    // Check two-character components for double dots (".."):
    if (end - start == 2 && path[start + 0] == 46 && path[start + 1] == 46) {
      return 1;
    }
    start = end + 1;
  }
  return 0;
}

int pure_path_drive(const uint8_t* path, const uint64_t length) {
  return (
    length >= 2 &&
    path[1] == 58 && // ":"
    path[0] >= 65 && // "A"
    path[0] <= 122   // "z"
  );
}

int pure_path_relative(const uint8_t* path, const uint64_t length) {
  if (length == 0) return 0;
  return path[0] == PURE_BACKSLASH || path[0] == PURE_FORWARD_SLASH;
}
