#define _OksBuildDll_

#include <oks/defs.h>
#include "oks_utils.h"

#include <stdlib.h>
#include <time.h>

#include <iomanip>
#include <sstream>

    //
    // The method converts string 's' to time
    // It is expected that the 's' is in the
    // following format: dd/mm/[yy]yy hh:mm[:ss]
    //

void
oks::Date::set(const char * s)
{
  p_tm.tm_sec = p_tm.tm_min = p_tm.tm_hour = p_tm.tm_mday = p_tm.tm_mon = p_tm.tm_year = 0;

  if(s == 0) return;

    // skip leading spaces

  while(*s == ' ') ++s;


  const char * s2 = s;	// original time used in error reports
  char * nds = 0;	// non-digit symbol returned by strtol


    // find the day
  
  p_tm.tm_mday = strtol(s, &nds, 10);
  
  if(*nds != '/') {
    Oks::error_msg("oks::Time::Time()") << "failed to find the day in \'" << s2 << "\'\n";
    return;
  }
  else
    s = nds + 1;
  
  if(p_tm.tm_mday > 31 || p_tm.tm_mday == 0) {
    Oks::error_msg("oks::Time::Time()") << "bad day " << p_tm.tm_mday << " in \'" << s2 << "\'\n";
    return;
  }


    // find the month

  p_tm.tm_mon = strtol(s, &nds, 10);

  if(*nds != '/') {
    Oks::error_msg("oks::Time::Time()") << "failed to find the month in \'" << s2 << "\'\n";
    return;
  }
  else
    s = nds + 1;

  if(p_tm.tm_mon > 12 || p_tm.tm_mon == 0) {
    Oks::error_msg("oks::Time::Time()") << "bad month " << p_tm.tm_mon << " in \'" << s2 << "\'\n";
    return;
  }

  p_tm.tm_mon--;


    // find the year

  p_tm.tm_year = strtol(s, &nds, 10);

  if(p_tm.tm_year >= 0) {
    if(p_tm.tm_year < 70) p_tm.tm_year += 2000;
    else if(p_tm.tm_year < 100) p_tm.tm_year += 1900;
  }

    // check if there is no data anymore

  if(*nds == 0) return;

  s = nds;


    // skip spaces

  while(*s == ' ') ++s;
  if(*s == 0) return;


    // find the hour

  p_tm.tm_hour = strtol(s, &nds, 10);

  if(*nds != ':') {
    Oks::error_msg("oks::Time::Time()") << "failed to find the hour in \'" << s2 << "\'\n";
    return;
  }
  else
    s = nds + 1;

  if(p_tm.tm_hour > 23) {
    Oks::error_msg("oks::Time::Time()") << "bad hour " << p_tm.tm_hour << " in \'" << s2 << "\'\n";
    return;
  }


    // find the minute

  p_tm.tm_min = strtol(s, &nds, 10);

  if(p_tm.tm_min > 59) {
    Oks::error_msg("oks::Time::Time()") << "bad minute " << p_tm.tm_min << " in \'" << s2 << "\'\n";
    return;
  }

  if(*nds != ':') return;


    // find the second

  p_tm.tm_sec = strtol(nds + 1, &nds, 10);

  if(p_tm.tm_sec > 59) {
    Oks::error_msg("oks::Time::Time()") << "bad second " << p_tm.tm_sec << " in \'" << s2 << "\'\n";
    return;
  }
}


std::string
oks::Date::str() const
{
  std::ostringstream s;

  s << p_tm.tm_mday << '/' << (p_tm.tm_mon + 1) << '/' << std::setfill('0') << std::setw(2)
    << ((p_tm.tm_year < 1970 || p_tm.tm_year >= 2070) ? p_tm.tm_year : (p_tm.tm_year < 2000 ? (p_tm.tm_year - 1900) : (p_tm.tm_year - 2000)));

  return s.str();
}

std::string
oks::Time::str() const
{
  std::string ds = oks::Date::str();

  std::ostringstream s;

  s << ds << std::setfill('0')
    << ' ' << std::setw(2) << p_tm.tm_hour
    << ':' << std::setw(2) << p_tm.tm_min
    << ':' << std::setw(2) << p_tm.tm_sec;

  return s.str();
}


std::ostream& oks::operator<<(std::ostream& s, const oks::Date& d)
{
  std::string str = d.str();
  s << str;
  return s;
}


std::ostream& oks::operator<<(std::ostream& s, const oks::Time& t)
{
  std::string str = t.str();
  s << str;
  return s;
}

////////////////////////////////////////////////////////////////////////////////

std::string Oks::Tokenizer::s_empty;

Oks::Tokenizer::Tokenizer(const std::string& s, const char * d) :
  p_string(s),
  p_delimeters(d),
  p_idx(p_string.find_first_not_of(p_delimeters))
{
}

const std::string
Oks::Tokenizer::next()
{
  if(p_idx == std::string::npos) return s_empty;
  std::string::size_type end_idx = p_string.find_first_of(p_delimeters, p_idx);
  if(end_idx == std::string::npos) end_idx=p_string.length();
  std::string token = p_string.substr(p_idx, end_idx - p_idx);
  p_idx = p_string.find_first_not_of(p_delimeters, end_idx);
  return token;
}

bool
Oks::Tokenizer::next(std::string& token)
{
  if(p_idx == std::string::npos) {
    token.clear();
    return false;
  }

  std::string::size_type end_idx = p_string.find_first_of(p_delimeters, p_idx);
  if(end_idx == std::string::npos) end_idx=p_string.length();
  token.assign(p_string, p_idx, end_idx - p_idx);
  p_idx = p_string.find_first_not_of(p_delimeters, end_idx);
  return true;
}
