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
#include "../picojson.h"

int main(void)
{
  picojson::value v;
  
  // read json value from stream
  std::cin >> v;
  if (std::cin.fail()) {
    std::cerr << picojson::get_last_error() << std::endl;
    return 1;
  }
  
  // dump json object
  std::cout << "---- dump input ----" << std::endl;
  std::cout << v << std::endl;

  // accessors
  std::cout << "---- analyzing input ----" << std::endl;
  if (v.is<picojson::null>()) {
    std::cout << "input is null" << std::endl;
  } else if (v.is<bool>()) {
    std::cout << "input is " << (v.get<bool>() ? "true" : "false") << std::endl;
  } else if (v.is<double>()) {
    std::cout << "input is " << v.get<double>() << std::endl;
  } else if (v.is<std::string>()) {
    std::cout << "input is " << v.get<std::string>() << std::endl;
  } else if (v.is<picojson::array>()) {
    std::cout << "input is an array" << std::endl;
    const picojson::array& a = v.get<picojson::array>();
    for (picojson::array::const_iterator i = a.begin(); i != a.end(); ++i) {
      std::cout << "  " << *i << std::endl;
    }
  } else if (v.is<picojson::object>()) {
    std::cout << "input is an object" << std::endl;
    const picojson::object& o = v.get<picojson::object>();
    for (picojson::object::const_iterator i = o.begin(); i != o.end(); ++i) {
      std::cout << i->first << "  " << i->second << std::endl;
    }
  }
  
  return 0;
}
