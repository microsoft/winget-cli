/*
 * Copyright 2009-2010 Cybozu Labs, Inc.
 * Copyright 2011-2014 Kazuho Oku
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include <iostream>
#include <iterator>
#include "../picojson.h"

// this example reads a array of hashes (each item representing a 2D point),
// and prints the x and y values to stdout

namespace {
  
  class root_context : public picojson::deny_parse_context {
  public:
    bool parse_array_start() {
      return true; // only allow array as root
    }
    template <typename Iter> bool parse_array_item(picojson::input<Iter>& in, size_t) {
      picojson::value item;
      // parse the array item
      picojson::default_parse_context ctx(&item);
      if (! picojson::_parse(ctx, in)) {
	return false;
      }
      // assert that the array item is a hash
      if (! item.is<picojson::object>()) {
	return false;
      }
      // print x and y
      std::cout << item.get("x") << ',' << item.get("y").to_str()
		<< std::endl;
      return true;
    }
  };
  
}

int main(void)
{
  root_context ctx;
  std::string err;
  
  picojson::_parse(ctx, std::istream_iterator<char>(std::cin),
		   std::istream_iterator<char>(), &err);
  
  if (! err.empty()) {
    std::cerr << err << std::endl;
    return 1;
  }
  
  return 0;
}
