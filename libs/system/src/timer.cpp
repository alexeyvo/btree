//  boost timer.cpp  ---------------------------------------------------------//

//  Copyright Beman Dawes 1994-2006

//  Distributed under the Boost Software License, Version 1.0. (See accompanying
//  file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

//  See http://www.boost.org/libs/system for documentation.

//----------------------------------------------------------------------------//

// define BOOST_SYSTEM_SOURCE so that <boost/system/config.hpp> knows
// the library is being built (possibly exporting rather than importing code)
#define BOOST_SYSTEM_SOURCE 

#include <boost/system/timer.hpp>
#include <boost/system/system_error.hpp>
#include <boost/io/ios_state.hpp>
#include <boost/throw_exception.hpp>
#include <boost/cerrno.hpp>
#include <cstring>
#include <cassert>

# if defined(BOOST_WINDOWS_API)
#   include <windows.h>
# elif defined(BOOST_POSIX_API)
#   include <sys/times.h>
# else
# error unknown API
# endif

namespace boost
{
  namespace system
  {

    BOOST_SYSTEM_DECL
    void times( times_t & current )
    {
      error_code ec;
      if ( times( current, ec ) )
        boost::throw_exception( system_error( ec, "boost::system::times" ) );
    }

    BOOST_SYSTEM_DECL
    error_code & times( times_t & current, error_code & ec )
    {
      ec = error_code();
#   if defined(BOOST_WINDOWS_API)
      ::GetSystemTimeAsFileTime( (LPFILETIME)&current.wall );
      FILETIME creation, exit;
      if ( ::GetProcessTimes( ::GetCurrentProcess(), &creation, &exit,
             (LPFILETIME)&current.system, (LPFILETIME)&current.user ) )
      {
        current.wall   /= 10;  // Windows uses 100 nanosecond ticks
        current.user   /= 10;
        current.system /= 10;
      }
      else
      {
        ec = error_code( ::GetLastError(), system_category() );
        current.wall = current.system = current.user = microsecond_t(-1);
      }
#   else
      tms tm;
      clock_t c = ::times( &tm );
      if ( c == -1 ) // error
      {
        ec = error_code( errno, native_ecat );
        current.wall = current.system = current.user = microsecond_t(-1);
      }
      else
      {
        current.wall = microsecond_t(c);
        current.system = microsecond_t(tm.tms_stime + tm.tms_cstime);
        current.user = microsecond_t(tm.tms_utime + tm.tms_cutime);
        if ( tick_factor() != -1 )
        {
          current.wall *= tick_factor();
          current.user *= tick_factor();
          current.system *= tick_factor();
        }
        else
        {
          ec = error_code( errno, native_ecat );
          current.wall = current.user = current.system = microsecond_t(-1);
        }
      }
#   endif
      return ec;
    }

#define  BOOST_TIMES(C)            \
      if ( m_flags & m_nothrow )   \
      {                            \
        error_code ec;             \
        times( C, ec );            \
      }                            \
      else                         \
        times( C );

    //  timer  ---------------------------------------------------------------//

    void timer::start()
    {
      m_flags = static_cast<m_flags_t>(m_flags & ~m_stopped);
      BOOST_TIMES( m_times );
    }

    const times_t & timer::stop()
    {
      if ( stopped() ) return m_times;
      m_flags = static_cast<m_flags_t>(m_flags | m_stopped);
      
      times_t current;
      BOOST_TIMES( current );
      m_times.wall = (current.wall - m_times.wall);
      m_times.user = (current.user - m_times.user);
      m_times.system = (current.system - m_times.system);
      return m_times;
    }

    void timer::elapsed( times_t & current )
    {
      if ( stopped() )
      {
        current.wall = m_times.wall;
        current.user = m_times.user;
        current.system = m_times.system;
      }
      else
      {
        BOOST_TIMES( current );
        current.wall -= m_times.wall;
        current.user -= m_times.user;
        current.system -= m_times.system;
      }
    }

  } // namespace system
} // namespace boost
