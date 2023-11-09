//
// get1.cpp
// ~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <urdl/istream.hpp>
#include <boost/progress.hpp>
#include <iostream>
#include <fstream>

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: get1 <url> <outputfile>\n";
      return 1;
    }

    urdl::istream is(argv[1]);
    if (!is)
    {
      std::cout << is.error().message() << std::endl;
      return 1;
    }

    std::ofstream os(argv[2], std::ios_base::out | std::ios_base::binary);
    if (is.content_length() != std::numeric_limits<std::size_t>::max())
    {
      boost::progress_display progress(is.content_length());
      while (is && os)
      {
        char buffer[1024] = "";
        is.read(buffer, sizeof(buffer));
        os.write(buffer, is.gcount());
        progress += is.gcount();
      }
      std::cout << std::endl;
    }
    else
    {
      os << is.rdbuf();
    }

    std::cout << is.error().message() << std::endl;
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}
