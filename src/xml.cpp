#include "oks/xml.hpp"
#include "oks/defs.hpp"
#include "oks/exceptions.hpp"
#include "oks/kernel.hpp"
#include "oks/object.hpp"
#include "oks/cstring.hpp"

#include <iostream>
#include <sstream>

#include <boost/spirit/include/karma.hpp>

#include <errno.h>
#include <stdlib.h>

#include "ers/ers.hpp"
#include "logging/Logging.hpp"

OksXmlTokenPool OksXmlInputStream::s_tokens_pool;

namespace oks {

  namespace xml {

    const char left_angle_bracket[]     = "&lt;";
    const char right_angle_bracket[]    = "&gt;";
    const char ampersand[]              = "&amp;";
    const char carriage_return[]        = "&#xD;";
    const char new_line[]               = "&#xA;";
    const char tabulation[]             = "&#x9;";
    const char single_quote[]           = "&apos;";
    const char double_quote[]           = "&quot;";

  }
  
  exception::exception(const std::string& what_arg, int level_arg) noexcept : p_level (level_arg)
  {
    std::ostringstream s;
    s << "oks[" << p_level << "] ***: " << what_arg << std::ends;
    p_what = s.str();
  }

  void throw_validate_not_empty(const char * name)
  {
    std::string text(name);
    text += " is not set";
    throw std::runtime_error(text.c_str());
  }

  std::string
  BadFileData::fill(const std::string& item, unsigned long line_no, unsigned long line_pos) noexcept
  {
    std::ostringstream s;
    s << item << " (line " << line_no << ", char " << line_pos << ')';
    return s.str();
  }

  std::string
  FailedRead::fill(const std::string& item, const std::string& reason) noexcept
  {
    return (std::string("Failed to read \'") + item + "\'\n" + reason);
  }

  std::string
  FailedSave::fill(const std::string& item, const std::string& name, const std::string& reason) noexcept
  {
    return (std::string("Failed to save ") + item + " \'" + name + "\'\n" + reason);
  }

  std::string
  EndOfXmlStream::fill(const std::string& tag)
  {
    return (std::string("Read end-of-stream tag \'") + tag + '\'');
  }

}

static std::string
report_unexpected(const char * what, const char expected, const char read)
{
  std::string s;

  if(what) {
    s += "Failed to read ";
    s += what;
    s += ": ";
  }

  s += "symbol \'";
  s += expected;
  s += "\' is expected instead of \'";
  s += read;
  s += '\'';

  return s;
}


static void
__throw_runtime_error_unexpected_symbol(const char expected, const char read)
{
  throw std::runtime_error(report_unexpected(0, expected, read));
}

static void
__throw_bad_file_data_unexpected_symbol(const char * what, const char expected, const char read, unsigned long l, unsigned long p)
{
  throw oks::BadFileData(report_unexpected(what, expected, read), l, p);
}

void
OksXmlOutputStream::__throw_write_failed()
{
  throw std::runtime_error("write to file failed");
}


  //
  // Save char to xml output stream
  // Throw std::exception if failed
  //

void
OksXmlOutputStream::put(char c)
{
  if(c == '<')       put_raw(oks::xml::left_angle_bracket, sizeof(oks::xml::left_angle_bracket)-1);
  else if(c == '>')  put_raw(oks::xml::right_angle_bracket, sizeof(oks::xml::right_angle_bracket)-1);
  else if(c == '&')  put_raw(oks::xml::ampersand, sizeof(oks::xml::ampersand)-1);
  else if(c == '\'') put_raw(oks::xml::single_quote, sizeof(oks::xml::single_quote)-1);
  else if(c == '\"') put_raw(oks::xml::double_quote, sizeof(oks::xml::double_quote)-1);
  else if(c == '\r') put_raw(oks::xml::carriage_return, sizeof(oks::xml::carriage_return)-1);
  else if(c == '\n') put_raw(oks::xml::new_line, sizeof(oks::xml::new_line)-1);
  else if(c == '\t') put_raw(oks::xml::tabulation, sizeof(oks::xml::tabulation)-1);
  else               put_raw(c);

}


    //
    // Save string to xml output stream
    // Throw std::exception if failed
    //

void
OksXmlOutputStream::put(const char * str)
{
  while(*str) put(*str++);
}


    //
    // Save quoted string to xml output stream
    // Throw std::exception if failed
    //

void
OksXmlOutputStream::put_quoted(const char *str)
{
  put_raw('\"');
  put(str);
  put_raw('\"');
}


    //
    // Save xml start tag and element name, i.e. '<element-name'
    // Throw std::exception if failed
    //

void
OksXmlOutputStream::put_start_tag(const char *name, size_t len)
{
  put_raw('<');
  put_raw(name, len);
}



    //
    // Save xml end tag, i.e. '/>\n'
    // Throw std::exception if failed
    //

void
OksXmlOutputStream::put_end_tag()
{
  static const char __end_tag[] = "/>\n";
  put_raw(__end_tag, sizeof(__end_tag)-1);
}

void
OksXmlOutputStream::put_eol()
{
  static const char __eol_tag[] = ">\n";
  put_raw(__eol_tag, sizeof(__eol_tag)-1);
}

    //
    // Save xml last tag, i.e. '</tag-name>\n'
    // Throw std::exception if failed
    //

void
OksXmlOutputStream::put_last_tag(const char * name, size_t len)
{
  put_raw('<');
  put_raw('/');
  put_raw(name, len);
  put_raw('>');
  put_raw('\n');
}


    //
    // Save xml element attribute with value, i.e. ' name="value"'
    // Throw std::exception if failed
    //

void
OksXmlOutputStream::put_attribute(const char * name, size_t len, const char *value)
{
  put_raw(' ');
  put_raw(name, len);
  put_raw('=');
  put_raw('\"');
  put(value);
  put_raw('\"');
}

void
OksXmlOutputStream::put_attribute(const char * name, size_t len, uint32_t value)
{
  put_raw(' ');
  put_raw(name, len);
  put_raw('=');
  put_raw('\"');
  if(value == 0) {
    put_raw('0');
  }
  else {
    char buf[12];
    char * ptr = buf;
    boost::spirit::karma::generate(ptr, boost::spirit::uint_, (unsigned int)value);
    put_raw(buf, ptr - buf);
  }
  put_raw('\"');
}

void
OksXmlOutputStream::put_attribute(const char * name, size_t len, const OksData& value)
{
  put_raw(' ');
  put_raw(name, len);
  put_raw('=');
  put_raw('\"');
  value.WriteTo(*this);
  put_raw('\"');
}

void
OksXmlInputStream::throw_unexpected_tag(const char * what, const char * expected)
{
  std::ostringstream s;

  if(!*what) {
    if(expected) {
      s << "expected tag \'" << expected << '\'';
    }
    else {
      s << "empty tag";
    }
  }
  else {
    s << "unexpected tag \'" << what << '\'' ;
    if(expected) {
      s << " instead of \'" << expected << '\'' ;
    }
  }

  throw oks::BadFileData(s.str(), line_no, line_pos);
}

void
OksXmlInputStream::throw_unexpected_attribute(const char * what)
{
  throw oks::BadFileData(std::string("Value \'") + what + "\' is not a valid attribute name", line_no, line_pos);
}

size_t
OksXmlInputStream::get_quoted()
{
  size_t pos(0);

  try {
    char c = get_first_non_empty();

    if( __builtin_expect((c != '\"'), 0) ) {
      __throw_runtime_error_unexpected_symbol('\"', c);
    }

    while(true) {
      c = get();

      if( __builtin_expect((c == '\"'), 0) ) {
        m_v1->m_buf[pos] = 0;
        return pos;
      }
      else if( __builtin_expect((c == '&'), 0) ) {
        c = cvt_char();
      }

      m_v1->realloc(pos);
      m_v1->m_buf[pos++] = c;
    }

    throw std::runtime_error("cannot find closing quote character");
  }
  catch (std::exception & ex) {
    m_v1->m_buf[pos] = 0;
    throw oks::BadFileData(std::string("Failed to read quoted string: ") + ex.what(), line_no, line_pos);
  }
}


  // check for start of comment

inline bool __is_comment(const char * s) {
  return oks::cmp_str4n(s, "<!--");
}


  //
  // Read xml tag while number of closing angles (i.e. '>') will not be equal
  // number of opening ones (i.e. '<')
  //

const char *
OksXmlInputStream::get_tag()
{
  m_v1->m_buf[0] = 0;

  unsigned long start_line_no = line_no;
  unsigned long start_line_pos = line_pos;

  size_t pos(0);

  try {
    while(true) {
      char c(get_first_non_empty());

      if( __builtin_expect((c != '<'), 0) ) {
        __throw_bad_file_data_unexpected_symbol("start-of-tag", '<', c, line_no, line_pos);
      }

      start_line_no = line_no;
      start_line_pos = line_pos;

      m_v1->m_buf[0] = c;
      pos = 1;

      int count(1);

      while(true) {
        char c2 = get();

        if(c2 == '<') count++;
        else if(c2 == '>') count--;

        m_v1->realloc(pos);
        m_v1->m_buf[pos++] = c2;

        if(pos > 3) {
          if(__is_comment(m_v1->m_buf + pos - 4)) {
            char buf[3] = {0, 0, 0};
            unsigned long start_comment_line_no = line_no;
            try {
              while(true) {
                buf[0] = buf[1];
                buf[1] = buf[2];
                buf[2] = get();

                if(oks::cmp_str3n(buf, "-->")) {
                  break; // end of comment = '-->'
                }
              }
            }
            catch(std::exception& failure) {
              std::ostringstream what;
              what << "Cannot find end of comment started on line " << start_comment_line_no << ": " << failure.what();
              throw oks::BadFileData(what.str(), line_no, line_pos);
            }

	    pos -= 4;
	    count--;
          }
        }

        if(count == 0) {
          m_v1->m_buf[pos] = '\0';
          break;
        }
      }

      if(pos > 0) {
        if(count != 0) { throw std::runtime_error("unbalanced tags"); }
        else { return m_v1->m_buf; }
      }
    }
  }
  catch(std::exception& ex) {
    m_v1->m_buf[pos] = '\0';
    throw oks::BadFileData(std::string(ex.what()) + " when read xml tag started at", start_line_no, start_line_pos); 
  }
}


    //
    // Reads token from stream until any symbol from 'separator' will be found.
    // If 'first_symbol' is not 0, if will be first symbol of token.
    //

inline size_t
OksXmlInputStream::get_token(const char __s1, OksXmlToken& token)
{
  size_t pos(0);

  while(true) {
    last_read_c = get();

    if(last_read_c == __s1) { token.m_buf[pos] = '\0'; return pos; }
    else if( __builtin_expect((last_read_c == '&'), 0) ) last_read_c = cvt_char();

    token.realloc(pos);
    token.m_buf[pos++] = last_read_c;
  }

  token.m_buf[pos] = 0;

  throw std::runtime_error("cannot find closing separator while read token");
}

inline size_t
OksXmlInputStream::get_token2(char __fs, const char __s1, const char __s2, OksXmlToken& token)
{
  size_t pos(1);
  token.m_buf[0] = __fs;
  if(__fs == __s1 || __fs == __s2) { token.m_buf[1] = '\0'; return 1; }

  while(true) {
    last_read_c = get();

    if(last_read_c == __s1 || last_read_c == __s2) { token.m_buf[pos] = '\0'; return pos; }
    else if( __builtin_expect((last_read_c == '&'), 0) ) last_read_c = cvt_char();

    token.realloc(pos);
    token.m_buf[pos++] = last_read_c;
  }

  token.m_buf[pos] = 0;

  throw std::runtime_error("cannot find closing separator while read token");
}

inline void
OksXmlInputStream::get_token5(const char __s1, const char __s2, const char __s3, const char __s4, const char __s5, OksXmlToken& token)
{
  size_t pos(0);

  while(true) {
    last_read_c = get();

    if(last_read_c == __s1 || last_read_c == __s2 || last_read_c == __s3 || last_read_c == __s4 || last_read_c == __s5) { token.m_buf[pos] = '\0'; return; }
    else if( __builtin_expect((last_read_c == '&'), 0) ) last_read_c = cvt_char();

    token.realloc(pos);
    token.m_buf[pos++] = last_read_c;
  }

  token.m_buf[pos] = 0;

  throw std::runtime_error("cannot find closing separator while read token");
}

const char *
OksXmlInputStream::get_tag_start()
{
  try {
    while(true) {
      char c = get_first_non_empty();

      if( __builtin_expect((c != '<'), 0) ) {
        __throw_runtime_error_unexpected_symbol('<', c);
      }

      get_token5(' ', '>', '\t', '\n', '\r', *m_v1);
      TLOG_DEBUG(8) << "read tag \'" << m_v1->m_buf << '\'';

      if( __builtin_expect((oks::cmp_str3n(m_v1->m_buf, "!--")), 0) ) {
        char buf[3] = {0, 0, 0};
        unsigned long start_comment_line_no = line_no;
        auto tag_len = strlen(m_v1->m_buf);
        if(tag_len < 5 || !oks::cmp_str2n(m_v1->m_buf +  tag_len- 2, "--")) // special case for comments without empty symbols like <!--foo-->
          try {
            while(true) {
              buf[0] = buf[1];
              buf[1] = buf[2];
              c = buf[2] = get();
              if(oks::cmp_str3n(buf, "-->")) {
                break; // end of comment = '-->'
              }
            }
          }
          catch(std::exception& failure) {
            std::ostringstream what;
            what << "Cannot find end of comment started on line " << start_comment_line_no << ": " << failure.what();
            throw oks::BadFileData(what.str(), line_no, line_pos);
          }
      }

      if( __builtin_expect((c != '>'), 1) ) return m_v1->m_buf;
    }
  }
  catch(std::exception& ex) {
    throw oks::BadFileData(std::string("Failed to read start-of-tag: ") + ex.what(), line_no, line_pos);
  }

  return 0;  // never reach this line
}


OksXmlAttribute::OksXmlAttribute(OksXmlInputStream& s) : p_name(*s.m_v1), p_value(*s.m_v2)
{
  unsigned long start_line_no = s.line_no;
  unsigned long start_line_pos = s.line_pos;

  try {
    char c = s.get_first_non_empty();

    start_line_no = s.line_no;
    start_line_pos = s.line_pos;

    size_t pos = s.get_token2(c, '=', '>', p_name);

    char * m_buf_ptr(p_name.m_buf);

      // skip empty symbols

    for(char * str(m_buf_ptr + pos - 1); str >= m_buf_ptr; --str) {
      if(*str == ' ' || *str == '\n' || *str == '\r' || *str == '\t') {
        *str = 0;
      }
      else {
        break;
      }
    }

    if(c == '>' || c == '/' || c == '?') return;  // end of attribute

    c = s.get_first_non_empty();

    if( __builtin_expect((c != '\"'), 0) ) {
      __throw_bad_file_data_unexpected_symbol("start-of-attribute-value", '\"', c, s.line_no, s.line_pos);
    }

    p_value_len = s.get_token('\"', p_value);

    TLOG_DEBUG(8) << "read attribute \'" << m_buf_ptr << "\' and value \'" << p_value.m_buf << '\'';
  }
  catch(std::exception& ex) {
    throw oks::BadFileData(std::string(ex.what()) + " when read xml attribute started at", start_line_no, start_line_pos);
  }
}


    //
    // This method is used to convert special xml symbols:
    //  "&lt;"   -> <
    //  "&gt;"   -> >
    //  "&amp;"  -> &
    //  "&apos;" -> '
    //  "&quot;" -> "
    //

char
OksXmlInputStream::cvt_char()
{
  get_token(';', *m_cvt_char);

  if(oks::cmp_str2n(m_cvt_char->m_buf, oks::xml::left_angle_bracket + 1)) return '<';
  else if(oks::cmp_str2n(m_cvt_char->m_buf, oks::xml::right_angle_bracket + 1)) return '>';
  else if(oks::cmp_str3n(m_cvt_char->m_buf, oks::xml::ampersand + 1)) return '&';
  else if(oks::cmp_str3n(m_cvt_char->m_buf, oks::xml::carriage_return + 1)) return '\r';
  else if(oks::cmp_str3n(m_cvt_char->m_buf, oks::xml::new_line + 1)) return '\n';
  else if(oks::cmp_str3n(m_cvt_char->m_buf, oks::xml::tabulation + 1)) return '\t';
  else if(oks::cmp_str4n(m_cvt_char->m_buf, oks::xml::single_quote + 1)) return '\'';
  else if(oks::cmp_str4n(m_cvt_char->m_buf, oks::xml::double_quote + 1)) return '\"';
  else {
    std::string text("bad symbol \'&");
    text += m_cvt_char->m_buf;
    text += '\'';
    throw std::runtime_error(text.c_str());
  }
}


std::ostream&
OksXmlInputStream::error_msg(const char *msg)
{
  return Oks::error_msg(msg)
    << "(line " << line_no << ", char " << line_pos << ")\n";
}

void
OksXmlInputStream::__throw_eof() const
{
  throw std::runtime_error("unexpected end of file");
}

void
OksXmlInputStream::__throw_strto(const char * f, const char * where, const char * what, unsigned long line_no, unsigned long line_pos)
{
  std::ostringstream text;
  text << "function " << f << "(\'" << what << "\') has failed";
  if(*where) text << " on unrecognized characters \'" << where << "\'";
  if(errno) text << " with code " << errno << ", reason = \'" << oks::strerror(errno) << '\'';
  throw oks::BadFileData(text.str(), line_no, line_pos);
}
