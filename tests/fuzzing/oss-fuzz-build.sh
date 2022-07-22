#!/bin/bash -eu

# This line causes an abort which breaks fuzzing:
sed -i '27d' include/valijson/utils/rapidjson_utils.hpp

mkdir build
cd build
cmake \
  -Dvalijson_BUILD_TESTS=TRUE \
  -Dvalijson_BUILD_EXAMPLES=FALSE \
	-Dvalijson_EXCLUDE_BOOST=TRUE \
	..

make -j"$(nproc)"

cd ../tests/fuzzing

find ../.. -name "*.o" -exec ar rcs fuzz_lib.a {} \;

# CXXFLAGS may contain spaces
# shellcheck disable=SC2086
"$CXX" $CXXFLAGS -DVALIJSON_USE_EXCEPTIONS=1 \
	-I/src/valijson/thirdparty/rapidjson-48fbd8c/include \
	-I/src/valijson/thirdparty/rapidjson-48fbd8c/include/rapidjson \
	-I/src/valijson/include \
	-I/src/valijson/include/valijson \
	-I/src/valijson/include/valijson/adapters \
	-c fuzzer.cpp -o fuzzer.o

# shellcheck disable=SC2086
"$CXX" $CXXFLAGS "$LIB_FUZZING_ENGINE" \
	-DVALIJSON_USE_EXCEPTIONS=1 \
	-rdynamic fuzzer.o \
	-o "${OUT}/fuzzer" fuzz_lib.a

zip "${OUT}/fuzzer_seed_corpus.zip" \
	"${SRC}/valijson/doc/schema/draft-03.json"
