//
// multiget2.cpp
// ~~~~~~~~~~~~~
//
// Copyright (c) 2009-2013 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <urdl/read_stream.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/bind.hpp>
#include <boost/ref.hpp>
#include <iostream>
#include <fstream>
#include <string>

void download(boost::asio::io_service& io_service,
    const urdl::url& url, const std::string& file,
    boost::asio::yield_context yield)
{
  try
  {
    urdl::read_stream read_stream(io_service);
    read_stream.async_open(url, yield);

    std::ofstream ofstream(file.c_str(),
        std::ios_base::out | std::ios_base::binary);

    char buffer[1024];
    std::size_t length;
    boost::system::error_code ec;

    do
    {
      length = read_stream.async_read_some(
          boost::asio::buffer(buffer), yield[ec]);
      ofstream.write(buffer, length);
    } while (length > 0);
  }
  catch (std::exception& e)
  {
    std::cerr << "Download exception: " << e.what() << std::endl;
  }
}

int main(int argc, char* argv[])
{
  try
  {
    if (argc < 3 || argc % 2 == 0)
    {
      std::cerr << "Usage: multiget2 <url> <outputfile> ";
      std::cerr << "[<url> <outputfile> ...]\n";
      return 1;
    }

    boost::asio::io_service io_service;

    for (int i = 1; i < argc; i += 2)
    {
      boost::asio::spawn(io_service,
          boost::bind(download, boost::ref(io_service),
            urdl::url(argv[i]), std::string(argv[i + 1]), _1));
    }

    io_service.run();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << std::endl;
  }
}
