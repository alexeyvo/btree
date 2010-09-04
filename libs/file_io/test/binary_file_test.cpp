//  Boost binary_file_test.cpp  --------------------------------------------------------//

//  Copyright Beman Dawes 2006, 2010

//  Distributed under the Boost Software License, Version 1.0.
//  See http://www.boost.org/LICENSE_1_0.txt

//  See library home page http://www.boost.org/libs/filesystem

//--------------------------------------------------------------------------------------//

#include <boost/config/warning_disable.hpp>

#define BOOST_FILESYSTEM_VERSION 3

#include <boost/file_io/binary_file.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/detail/lightweight_test.hpp>

namespace fs = boost::filesystem;

#include <iostream>
#include <iomanip>
#include <string>
#include <cstring>

#ifdef BOOST_WINDOWS_API
# include <winerror.h>
# define BOOST_BAD_SEEK ERROR_NEGATIVE_SEEK
#else
# include <errnoh>
# define BOOST_BAD_SEEK EINVAL
#endif

namespace
{
  void open_flag_tests()
  {
    std::cout << "open flag tests..." << std::endl;

    fs::path p("test.txt");
    fs::remove(p);
    BOOST_TEST(!fs::exists(p));

    boost::system::error_code ec;

    {
      fs::binary_file f(p, fs::oflag::in, ec);
      BOOST_TEST(ec);
    }
    {
      fs::binary_file f(p, fs::oflag::out, ec);
      BOOST_TEST(!ec);
      BOOST_TEST(fs::exists(p));
      BOOST_TEST_EQ(fs::file_size(p), 0);
      f.write("foo", 3, ec);
      BOOST_TEST(!ec);
      BOOST_TEST_EQ(fs::file_size(p), 3);
    }
    {
      fs::binary_file f(p, fs::oflag::in, ec);
      BOOST_TEST(!ec);
      BOOST_TEST_EQ(fs::file_size(p), 3);
    }
    {
      fs::binary_file f(p, fs::oflag::in | fs::oflag::out, ec);
      BOOST_TEST(!ec);
      BOOST_TEST_EQ(fs::file_size(p), 3);
    }
    {
      fs::binary_file f(p, fs::oflag::in | fs::oflag::out | fs::oflag::truncate, ec);
      BOOST_TEST(!ec);
      BOOST_TEST_EQ(fs::file_size(p), 0);
    }

    fs::remove(p);
    std::cout << "  completed open flag tests" << std::endl;
  }
}

//  main  ------------------------------------------------------------------------------//

int main(int argc, char * argv[])
{
  fs::binary_file::offset_type gap(32);

  if (argc > 1) gap =
    static_cast<fs::binary_file::offset_type>(std::atol(argv[1])) * 1024;

  /// TODO:
  ///   * Test open modes

  char buf[128] = "0123456789abcdef";

  std::string filename("file_with_gap");
  fs::binary_file f(filename, fs::oflag::in |fs::oflag::out | fs::oflag::truncate);
  BOOST_TEST(f.file_path() == filename);
  BOOST_TEST(fs::exists(filename));


  boost::system::error_code ec;
  f.seek(-1, fs::seekdir::begin, ec);
  BOOST_TEST(ec);
  BOOST_TEST_EQ(ec.value(), BOOST_BAD_SEEK);

  bool error_thrown(false);
  try
  {
    f.seek(-1, fs::seekdir::begin);
  }
  catch (const fs::filesystem_error & ex)
  {
    error_thrown = true;
    BOOST_TEST_EQ(ec.value(), BOOST_BAD_SEEK);
    BOOST_TEST_EQ(ex.path1().string(), filename);
  }
  BOOST_TEST(error_thrown);

  char beginning[] = "beginning";
  f.write(beginning, 10);
  BOOST_TEST(fs::file_size(filename) == 10);
  BOOST_TEST(f.seek(0, fs::seekdir::end) == 10);
  BOOST_TEST(f.seek(gap, fs::seekdir::current) == gap + 10);

  char ending[] = "ending";
  f.write(ending, 7);
  BOOST_TEST(f.seek(0, fs::seekdir::current) == gap + 17);
  BOOST_TEST(f.seek(0, fs::seekdir::end) == gap + 17);

  BOOST_TEST(f.seek(0, fs::seekdir::begin) == 0);
  
  BOOST_TEST(f.read(buf, 10));
  BOOST_TEST(std::strcmp(buf, beginning) == 0);
  BOOST_TEST(std::memcmp(&buf[10], "abcdef", 6) == 0);

  BOOST_TEST(f.seek(gap + 10, fs::seekdir::begin) == gap + 10);
  BOOST_TEST(f.read(buf, 7));
  BOOST_TEST(std::strcmp(buf, ending) == 0);

  BOOST_TEST(!f.read(buf, 1));

  BOOST_TEST(f.is_open());
  f.close();
  BOOST_TEST(!f.is_open());

  BOOST_TEST(fs::file_size(filename) == gap + 17);

  open_flag_tests();

  return boost::report_errors();
}
