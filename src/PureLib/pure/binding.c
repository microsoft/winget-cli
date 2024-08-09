// binding.c - Node.js binding:

#include <assert.h>
#include <limits.h>
#include <math.h>
#include <node_api.h>
#include <stdint.h>
#include "pure.h"

#define OK(call)                                                               \
  assert((call) == napi_ok);

#define THROW(env, message)                                                    \
  do {                                                                         \
    napi_throw_error((env), NULL, (message));                                  \
    return NULL;                                                               \
  } while (0)

static int arg_buf(
  napi_env env,
  napi_value value,
  uint8_t** buffer,
  uint64_t* buffer_length
) {
  assert(value != NULL);
  assert(*buffer == NULL);
  assert(*buffer_length == 0);
  bool is_buffer = 0;
  OK(napi_is_buffer(env, value, &is_buffer));
  if (!is_buffer) return 0;
  size_t length = 0;
  OK(napi_get_buffer_info(env, value, (void**) buffer, &length));
  assert(*buffer != NULL);
  assert(length <= UINT64_MAX);
  *buffer_length = length;
  return 1;
}

static int arg_int(napi_env env, napi_value value, uint64_t* integer) {
  assert(*integer == 0);
  double temp = 0;
  if (
    // We get the value as a double so we can check for NaN, Infinity and float:
    // https://github.com/nodejs/node/issues/26323
    napi_get_value_double(env, value, &temp) != napi_ok ||
    temp < 0 ||
    isnan(temp) ||
    // Infinity, also prevent UB for double->int cast below:
    // https://groups.google.com/forum/#!topic/comp.lang.c/rhPzd4bgKJk
    temp > UINT64_MAX ||
    // Float:
    (double) ((uint64_t) temp) != temp
  ) {
    return 0;
  }
  *integer = (uint64_t) temp;
  return 1;
}

static napi_value call_pure_zip(napi_env env, napi_callback_info info) {
  size_t argc = 2;
  napi_value argv[2];
  OK(napi_get_cb_info(env, info, &argc, argv, NULL, NULL));
  uint8_t* buffer = NULL;
  uint64_t size = 0;
  uint64_t flags = 0;
  if (
    argc != 2 ||
    !arg_buf(env, argv[0], &buffer, &size) ||
    !arg_int(env, argv[1], &flags)
  ) {
    THROW(env, "bad arguments, expected: (buffer, flags)");
  }
  assert(buffer != NULL);
  int error = pure_zip(buffer, size, flags);
  const char* code = pure_error_code(error);
  const char* string = pure_error_string(error);
  napi_value result;
  napi_value result_code;
  napi_value result_string;
  OK(napi_create_string_utf8(env, code, NAPI_AUTO_LENGTH, &result_code));
  OK(napi_create_string_utf8(env, string, NAPI_AUTO_LENGTH, &result_string));
  OK(napi_create_error(env, result_code, result_string, &result));
  return result;
}

void set_method(
  napi_env env,
  napi_value object,
  const char* name,
  void* method
) {
  napi_value value;
  OK(napi_create_function(env, NULL, 0, method, NULL, &value));
  OK(napi_set_named_property(env, object, name, value));
}

static napi_value Init(napi_env env, napi_value exports) {
  // We require assert() for safety (our asserts are not side-effect free):
  #ifdef NDEBUG
    fprintf(stderr, "NDEBUG compile flag not supported\n");
    abort();
  #endif
  set_method(env, exports, "zip", call_pure_zip);
  return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
