#ifndef OKS_XML_H
#define OKS_XML_H

#include <string.h>

#include <memory>
#include <stack>
#include <fstream>
#include <stdexcept>

#include <oks/defs.h>
#include <oks/exceptions.h>
#include <oks/cstring.h>

struct OksData;

namespace oks {

  namespace xml {

      /** Converts boolean to string literal **/
    inline const char * bool2str(bool b) noexcept { return (b ? "yes" : "no"); }

      /** Converts string literal to boolean **/
    inline bool str2bool(const char * s) noexcept { return oks::cmp_str3(s, "yes"); }

  }

    /** Bad file exception. **/

  class BadFileData : public exception {

    public:

      BadFileData(const std::string& what, unsigned long line_no, unsigned long line_pos, int level_arg = 0) noexcept : exception (fill(what, line_no, line_pos), level_arg) {}

      virtual ~BadFileData() noexcept { }


    private:
    
      static std::string fill(const std::string& what, unsigned long line_no, unsigned long line_pos) noexcept;
  };


    /** Failed to read a value. **/

  class FailedRead : public exception {

    public:

        /** Get reason from nested oks exception. **/
      FailedRead(const std::string& what, const exception& reason) noexcept   : exception (fill(what, reason.what()), reason.level() + 1) { }

        /** Get reason from nested non-oks exception. **/
      FailedRead(const std::string& what, const std::string& reason) noexcept : exception (fill(what, reason), 0) { }

      virtual ~FailedRead() noexcept { }


    private:

      static std::string fill(const std::string& what, const std::string& reason) noexcept;

  };


    /** Failed to save a value. **/

  class FailedSave : public exception {

    public:

        /** Get reason from nested oks exception. **/
      FailedSave(const std::string& what, const std::string& name, const exception& reason) noexcept   : exception (fill(what, name, reason.what()), reason.level() + 1) { }

        /** Get reason from nested non-oks exception. **/
      FailedSave(const std::string& what, const std::string& name, const std::string& reason) noexcept : exception (fill(what, name, reason), 0) { }

      virtual ~FailedSave() noexcept { }


    private:

      static std::string fill(const std::string& what, const std::string& name, const std::string& reason) noexcept;

  };
  
  
  class EndOfXmlStream : public exception {

    public:

      EndOfXmlStream(const std::string& tag) noexcept : exception (fill(tag), 0) { }

      virtual ~EndOfXmlStream() noexcept { }


    private:

      static std::string fill(const std::string& tag);

  };

  struct ReadFileParams;
}


class OksXmlOutputStream
{

public:

  OksXmlOutputStream(std::ostream &p) : f(p)
  {
    ;
  }

  std::ostream&
  get_stream() const
  {
    return f;
  }

  /**
   *  Write char to xml output stream. Convert special symbols.
   *  \throw std::exception if failed
   */
  void
  put(char c);

  /**
   *  Write string to xml output stream. Convert special symbols.
   *  \throw std::exception if failed
   */
  void
  put(const char * s);

  /**
   *  Write char to xml output stream without check for special symbols.
   *  \throw std::exception if failed
   */
  void
  put_raw(char c)
  {
    if (__builtin_expect((f.rdbuf()->sputc(c) != c), 0))
      __throw_write_failed();
  }

  void
  put_raw(const char * s, long len)
  {
    if (__builtin_expect((f.rdbuf()->sputn(s, len) != len), 0))
      __throw_write_failed();
  }

  template<class T>
    void
    put_value(T value)
    {
      f << value;
    }

  void
  put_quoted(const char *);

  void
  put_start_tag(const char *, size_t len);   // '<tag-name'

  void
  put_end_tag();                             // '/>\n'

  void
  put_last_tag(const char *, size_t len);    // '</tag-name>\n'

  void
  put_attribute(const char *, size_t len, const char *);

  void
  put_attribute(const char *, size_t len, uint32_t);

  void
  put_attribute(const char *, size_t len, const OksData&);

  void
  put_eol();                                 // '>\n'

private:

  std::ostream& f;

  static void
  __throw_write_failed();

};


struct OksXmlToken {

  size_t m_len;
  size_t m_len2; // (m_len - 2)
  char * m_buf;

  OksXmlToken(size_t len = 2048) : m_len(len), m_len2(len-2), m_buf(new char [len]) { ; }
  ~OksXmlToken() {delete [] m_buf;}

  void realloc(unsigned long pos) {
    if( __builtin_expect((pos >= m_len2), 0) ) {
      m_len += 2048;
      m_len2 = m_len-2;
      char *ptr = new char [m_len];
      memcpy(ptr, m_buf, pos);
      delete [] m_buf;
      m_buf = ptr;
    }
  }

};


class OksXmlInputStream;
class OksClass;
class OksString;

struct OksXmlValue
{
  OksXmlToken * m_token;
  unsigned int m_len;
  unsigned int m_line_no;
  unsigned int m_line_pos;

  OksXmlValue(OksXmlToken * token = nullptr, unsigned int len = 0, unsigned int line_no = 0, unsigned int line_pos = 0) :
    m_token(token), m_len(len), m_line_no(line_no), m_line_pos(line_pos)
  {
    ;
  }

  bool
  is_empty()
  {
    return (m_token == nullptr);
  }

  OksXmlToken *
  get_token() const
  {
    return m_token;
  }

  char * buf() const
  {
    return m_token->m_buf;
  }

  unsigned int len() const
  {
    return m_len;
  }

  unsigned int line_no() const
  {
    return m_line_no;
  }

  unsigned int line_pos() const
  {
    return m_line_pos;
  }
};

struct OksXmlRelValue
{
  OksXmlRelValue(const oks::ReadFileParams& file_params) : m_file_params(file_params), m_class(nullptr), m_class_id(nullptr)
  {
    ;
  }

  bool is_empty()
  {
    return m_value.is_empty() || (m_class == nullptr && m_class_id == nullptr);
  }

  const oks::ReadFileParams& m_file_params;
  OksXmlValue m_value;
  OksClass * m_class;
  mutable OksString * m_class_id;
};

  /**
   *   The struct is used to read pair of strings (attribute name and value) from oks xml input stream.
   */

struct OksXmlAttribute {

  OksXmlToken& p_name;
  OksXmlToken& p_value;
  size_t p_value_len;

  OksXmlAttribute(OksXmlInputStream& s);

  char * name() const {return p_name.m_buf;}
  char * value() const {return p_value.m_buf;}
  size_t value_len() const {return p_value_len;}
};

struct OksXmlTokenPool {

  friend class OksXmlInputStream;

  public:

    ~OksXmlTokenPool() {
      while(!m_tokens.empty()) {
        OksXmlToken * t = m_tokens.top();
	m_tokens.pop();
	delete t;
      }
    }


  private:

      /**
       *  Get free OksXmlToken from pool or create new one.
       *  Lock mutex before usage!
       */

    OksXmlToken * get() {
      if(!m_tokens.empty()) {
        OksXmlToken * t = m_tokens.top();
        m_tokens.pop();
        return t;
      }
      else {
        return new OksXmlToken();
      }
    }


      /**
       *  Add OksXmlToken to pool.
       *  Lock mutex before usage!
       */

    void release(OksXmlToken * token) {
      m_tokens.push(token);
    }


  private:

    std::mutex m_mutex;
    std::stack<OksXmlToken *> m_tokens;

};


class OksXmlInputStream {

friend struct OksXmlAttribute;
friend struct OksData;
friend class  OksObject;

public:

  OksXmlInputStream(std::shared_ptr<std::istream> p) :
    f(p), m_pbuf(p->rdbuf()), line_no(1), line_pos(0)
  {
    init();
  };

  ~OksXmlInputStream() {
    std::lock_guard lock(s_tokens_pool.m_mutex);
    s_tokens_pool.release(m_v3);
    s_tokens_pool.release(m_v2);
    s_tokens_pool.release(m_v1);
    s_tokens_pool.release(m_cvt_char);
  }


    // get token separated by whitespace or "last" symbol
    // put result on m_cvt_char token

  void get_num_token(char last);


    // throw exception

  static void __throw_strto(const char * f, const char * where, const char * what, unsigned long line_no, unsigned long line_pos);

  size_t get_quoted();

  const char * get_tag();
  const char * get_tag_start();

  bool good() const { return f->good(); }
  bool eof() const { return f->eof(); }

  void store_position() { pos = f->tellg(); m_line_no_sav = line_no; m_line_pos_sav = line_pos; }
  void restore_position() { f->seekg(pos); line_no = m_line_no_sav; line_pos = m_line_pos_sav; }
  long get_position() const {
    return f->tellg();
  }

  void inline seek_position(std::streamoff off) { f->seekg(off, std::ios_base::cur); }

  std::ostream&	error_msg(const char *);

  unsigned long	get_line_no() const  { return line_no; }
  unsigned long	get_line_pos() const { return line_pos; }

  OksXmlToken&  get_xml_token() {return *m_v1;}

  OksXmlValue
  get_value(unsigned int len)
  {
    OksXmlValue value(m_v2, len, line_no, line_pos);
    m_v2 = m_v3;
    m_v3 = value.get_token();
    return value;
  }

  void throw_unexpected_tag(const char * what, const char * expected = 0);
  void throw_unexpected_attribute(const char * what);

private:

  std::shared_ptr<std::istream> f;
  std::streambuf * m_pbuf;
  unsigned long	line_no;
  unsigned long	line_pos;
  
  char last_read_c;

  std::istream::pos_type pos;

  unsigned long	m_line_no_sav;
  unsigned long	m_line_pos_sav;
  
  static OksXmlTokenPool s_tokens_pool;

  OksXmlToken *  m_cvt_char; // the token to be used by cvt_char()
  OksXmlToken *  m_v1;
  OksXmlToken *  m_v2;
  OksXmlToken *  m_v3; // used for swap with m_v2 to keep value of "val" attribute


  void init() {
    f->setf(std::ios::showbase, std::ios::basefield);
    f->exceptions ( std::istream::eofbit | std::istream::failbit | std::istream::badbit );

    std::lock_guard lock(s_tokens_pool.m_mutex);
    m_cvt_char = s_tokens_pool.get();
    m_v1 = s_tokens_pool.get();
    m_v2 = s_tokens_pool.get();
    m_v3 = s_tokens_pool.get();
  }

  inline size_t get_token(const char s1, OksXmlToken& token);                                                                /*!< Read token (one separator)   */
  inline size_t get_token2(char, const char s1, const char s2, OksXmlToken& token);                                          /*!< Read token (two separators)  */
  inline void get_token5(const char s1, const char s2, const char s3, const char s4, const char s5, OksXmlToken& token);     /*!< Read token (five separators) */
  char cvt_char();                                                                                                           /*!< Read encoded symbol (&xxx;)  */

  inline char get_first_non_empty();
  inline char get();

  inline unsigned long get_value_pre();
  inline void get_value_post(unsigned long);

  void __throw_eof() const;


    // protect usage of copy constructor and assignment operator

  OksXmlInputStream(const OksXmlInputStream&);
  OksXmlInputStream& operator=(const OksXmlInputStream &);

};


    //
    // Read new symbol from stream and change LINE and POS numbers.
    // Throw exception in case of I/O error or EOF
    //

inline char
OksXmlInputStream::get()
{
  char c(m_pbuf->sbumpc());

  line_pos++;
  if( __builtin_expect((c == EOF), 0) ) __throw_eof();
  else if( __builtin_expect((c == '\n'), 0) ) { line_no++; line_pos = 0; }

  return c;
}


    //
    // Read first non-null xml symbol
    // Return 0 if error
    //

inline char
OksXmlInputStream::get_first_non_empty()
{
  while(true) {
    char c(get());
    if(c == ' ' || c == '\n' || c == '\r' || c == '\t') continue;
    else return c;
  }
}

#endif
