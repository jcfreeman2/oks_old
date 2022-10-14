#define _OksBuildDll_

#include "oks/file.hpp"
#include "oks/kernel.hpp"
#include "oks/xml.hpp"
#include "oks/cstring.hpp"

#include "oks_utils.h"

#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>

#include <fstream>
#include <sstream>
#include <stdexcept>

#include <boost/date_time/posix_time/posix_time_types.hpp>
#include <boost/date_time/posix_time/time_formatters.hpp>
#include <boost/date_time/posix_time/time_parsers.hpp>
#include <boost/interprocess/sync/file_lock.hpp>

#include "ers/ers.hpp"
#include "logging/Logging.hpp"

    //
    // Define XML formats used to store OKS schema and data files
    //

const char OksFile::xml_info_tag[]     = "info";
const char OksFile::xml_include_tag[]  = "include";
const char OksFile::xml_file_tag[]     = "file";
const char OksFile::xml_comments_tag[] = "comments";
const char OksFile::xml_comment_tag[]  = "comment";

const char OksFile::xml_file_header[]  = "<?xml version=\"1.0\" encoding=\"ASCII\"?>";

const char OksFile::xml_schema_file_dtd[] =
  "<!DOCTYPE oks-schema [\n"

  "  <!ELEMENT oks-schema (info, (include)?, (comments)?, (class)+)>\n"

  "  <!ELEMENT info EMPTY>\n"
  "  <!ATTLIST info\n"
  "      name CDATA #IMPLIED\n"
  "      type CDATA #IMPLIED\n"
  "      num-of-items CDATA #REQUIRED\n"
  "      oks-format CDATA #FIXED \"schema\"\n"
  "      oks-version CDATA #REQUIRED\n"
  "      created-by CDATA #IMPLIED\n"
  "      created-on CDATA #IMPLIED\n"
  "      creation-time CDATA #IMPLIED\n"
  "      last-modified-by CDATA #IMPLIED\n"
  "      last-modified-on CDATA #IMPLIED\n"
  "      last-modification-time CDATA #IMPLIED\n"
  "  >\n"

  "  <!ELEMENT include (file)+>\n"
  "  <!ELEMENT file EMPTY>\n"
  "  <!ATTLIST file\n"
  "      path CDATA #REQUIRED\n"
  "  >\n"

  "  <!ELEMENT comments (comment)+>\n"
  "  <!ELEMENT comment EMPTY>\n"
  "  <!ATTLIST comment\n"
  "      creation-time CDATA #REQUIRED\n"
  "      created-by CDATA #REQUIRED\n"
  "      created-on CDATA #REQUIRED\n"
  "      author CDATA #REQUIRED\n"
  "      text CDATA #REQUIRED\n"
  "  >\n"

  "  <!ELEMENT class (superclass | attribute | relationship | method)*>\n"
  "  <!ATTLIST class\n"
  "      name CDATA #REQUIRED\n"
  "      description CDATA \"\"\n"
  "      is-abstract (yes|no) \"no\"\n"
  "  >\n"

  "  <!ELEMENT superclass EMPTY>\n"
  "  <!ATTLIST superclass name CDATA #REQUIRED>\n"

  "  <!ELEMENT attribute EMPTY>\n"
  "  <!ATTLIST attribute\n"
  "      name CDATA #REQUIRED\n"
  "      description CDATA \"\"\n"
  "      type (bool|s8|u8|s16|u16|s32|u32|s64|u64|float|double|date|time|string|uid|enum|class) #REQUIRED\n"
  "      range CDATA \"\"\n"
  "      format (dec|hex|oct) \"dec\"\n"
  "      is-multi-value (yes|no) \"no\"\n"
  "      init-value CDATA \"\"\n"
  "      is-not-null (yes|no) \"no\"\n"
  "      ordered (yes|no) \"no\"\n"
  "  >\n"

  "  <!ELEMENT relationship EMPTY>\n"
  "  <!ATTLIST relationship\n"
  "      name CDATA #REQUIRED\n"
  "      description CDATA \"\"\n"
  "      class-type CDATA #REQUIRED\n"
  "      low-cc (zero|one) #REQUIRED\n"
  "      high-cc (one|many) #REQUIRED\n"
  "      is-composite (yes|no) #REQUIRED\n"
  "      is-exclusive (yes|no) #REQUIRED\n"
  "      is-dependent (yes|no) #REQUIRED\n"
  "      ordered (yes|no) \"no\"\n"
  "  >\n"

  "  <!ELEMENT method (method-implementation*)>\n"
  "  <!ATTLIST method\n"
  "      name CDATA #REQUIRED\n"
  "      description CDATA \"\"\n"
  "  >\n"

  "  <!ELEMENT method-implementation EMPTY>\n"
  "  <!ATTLIST method-implementation\n"
  "      language CDATA #REQUIRED\n"
  "      prototype CDATA #REQUIRED\n"
  "      body CDATA \"\"\n"
  "  >\n"

  "]>";

const char OksFile::xml_data_file_dtd[] =
  "<!DOCTYPE oks-data [\n"

  "  <!ELEMENT oks-data (info, (include)?, (comments)?, (obj)+)>\n"

  "  <!ELEMENT info EMPTY>\n"
  "  <!ATTLIST info\n"
  "      name CDATA #IMPLIED\n"
  "      type CDATA #IMPLIED\n"
  "      num-of-items CDATA #REQUIRED\n"
  "      oks-format CDATA #FIXED \"data\"\n"
  "      oks-version CDATA #REQUIRED\n"
  "      created-by CDATA #IMPLIED\n"
  "      created-on CDATA #IMPLIED\n"
  "      creation-time CDATA #IMPLIED\n"
  "      last-modified-by CDATA #IMPLIED\n"
  "      last-modified-on CDATA #IMPLIED\n"
  "      last-modification-time CDATA #IMPLIED\n"
  "  >\n"

  "  <!ELEMENT include (file)*>\n"
  "  <!ELEMENT file EMPTY>\n"
  "  <!ATTLIST file\n"
  "      path CDATA #REQUIRED\n"
  "  >\n"

  "  <!ELEMENT comments (comment)*>\n"
  "  <!ELEMENT comment EMPTY>\n"
  "  <!ATTLIST comment\n"
  "      creation-time CDATA #REQUIRED\n"
  "      created-by CDATA #REQUIRED\n"
  "      created-on CDATA #REQUIRED\n"
  "      author CDATA #REQUIRED\n"
  "      text CDATA #REQUIRED\n"
  "  >\n"

  "  <!ELEMENT obj (attr | rel)*>\n"
  "  <!ATTLIST obj\n"
  "      class CDATA #REQUIRED\n"
  "      id CDATA #REQUIRED\n"
  "  >\n"
  "  <!ELEMENT attr (data)*>\n"
  "  <!ATTLIST attr\n"
  "      name CDATA #REQUIRED\n"
  "      type (bool|s8|u8|s16|u16|s32|u32|s64|u64|float|double|date|time|string|uid|enum|class|-) \"-\"\n"
  "      val CDATA \"\"\n"
  "  >\n"
  "  <!ELEMENT data EMPTY>\n"
  "  <!ATTLIST data\n"
  "      val CDATA #REQUIRED\n"
  "  >\n"
  "  <!ELEMENT rel (ref)*>\n"
  "  <!ATTLIST rel\n"
  "      name CDATA #REQUIRED\n"
  "      class CDATA \"\"\n"
  "      id CDATA \"\"\n"
  "  >\n"
  "  <!ELEMENT ref EMPTY>\n"
  "  <!ATTLIST ref\n"
  "      class CDATA #REQUIRED\n"
  "      id CDATA #REQUIRED\n"
  "  >\n"
  "]>";


namespace oks {

  std::string
  FailedAddInclude::fill(const OksFile& file, const std::string& include_name, const std::string& reason) noexcept
  {
    return ( std::string("Failed to add include \'") + include_name + "\' to file \'" + file.get_full_file_name() + "\' because:\n" + reason);
  }

  std::string
  FailedRemoveInclude::fill(const OksFile& file, const std::string& include_name, const std::string& reason) noexcept
  {
    return ( std::string("Failed to remove include \'") + include_name + "\' from file \'" + file.get_full_file_name() + "\' because:\n" + reason);
  }

  std::string
  FailedRenameInclude::fill(const OksFile& file, const std::string& from, const std::string& to, const std::string& reason) noexcept
  {
    return ( std::string("Failed to rename include of \'") + file.get_full_file_name() + "\' from \'" + from + "\' to \'" + to + "\' because:\n" + reason);
  }

  std::string
  FailedAddComment::fill(const OksFile& file, const std::string& reason) noexcept
  {
    return ( std::string("Failed to add new comment to file \'") + file.get_full_file_name() + "\' because:\n" + reason);
  }

  std::string
  FailedRemoveComment::fill(const OksFile& file, const std::string& creation_time, const std::string& reason) noexcept
  {
    return ( std::string("Failed to remove comment created at \'") + creation_time + "\' from file \'" + file.get_full_file_name() + "\' because:\n" + reason);
  }

  std::string
  FailedChangeComment::fill(const OksFile& file, const std::string& creation_time, const std::string& reason) noexcept
  {
    return ( std::string("Failed to change comment created at \'") + creation_time + "\' from file \'" + file.get_full_file_name() + "\' because:\n" + reason);
  }

  std::string
  FileLockError::fill(const OksFile& file, bool lock_action, const std::string& reason) noexcept
  {
    return ( std::string("Failed to ") + (lock_action ? "" : "un") + "lock file \'" + file.get_full_file_name() + "\' because:\n" + reason);
  }

  std::string
  FileChangeError::fill(const OksFile& file, const std::string& action, const std::string& reason) noexcept
  {
    return ( std::string("Failed to ") + action + " of file \'" + file.get_full_file_name() + "\' because:\n" + reason);
  }

  std::string
  FileCompareError::fill(const std::string& src, const std::string& dest, const std::string& reason) noexcept
  {
    return ( std::string("Failed to compare file \'") + src + "\' with \'" + dest + "\' because:\n" + reason);
  }
}


OksFile::OksFile(const std::string& s, const std::string& ln, const std::string& ft, const std::string& ff, OksKernel * k) :
 p_short_name               (s),
 p_full_name                (s),
 p_logical_name             (ln),
 p_type                     (ft),
 p_oks_format               (ff),
 p_number_of_items          (0),
 p_size                     (0),
 p_created_by               (OksKernel::get_user_name()),
 p_creation_time            (boost::posix_time::second_clock::universal_time()),
 p_created_on               (OksKernel::get_host_name()),
 p_last_modification_time   (p_creation_time),
 p_lock                     (),
 p_is_updated               (false),
 p_is_read_only             (true),
 p_last_modified            (0),
 p_repository_last_modified (0),
 p_is_on_disk               (false),
 p_included_by              (0),
 p_kernel                   (k)
{
  p_last_modified_on = p_created_on;
  p_last_modified_by = p_created_by;
  create_lock_name();
  check_repository();
}

void
OksFile::clear_comments() noexcept
{
  for(std::map<std::string, oks::Comment *>::iterator i = p_comments.begin(); i != p_comments.end(); ++i) {
    delete i->second;
  }

  p_comments.clear();
}


OksFile::~OksFile() noexcept
{
  clear_comments();
}

OksFile&
OksFile::operator=(const OksFile & f)
{
  if (&f != this)
    {
      p_list_of_include_files.clear();
      clear_comments();

      p_short_name = f.p_short_name;
      p_full_name = f.p_full_name;
//      p_repository = f.p_repository;
      p_repository_name = f.p_repository_name;
      p_logical_name = f.p_logical_name;
      p_type = f.p_type;
      p_oks_format = f.p_oks_format;
      p_number_of_items = f.p_number_of_items;
      p_size = f.p_size;
      p_created_by = f.p_created_by;
      p_creation_time = f.p_creation_time;
      p_created_on = f.p_created_on;
      p_last_modified_by = f.p_last_modified_by;
      p_last_modification_time = f.p_last_modification_time;
      p_last_modified_on = f.p_last_modified_on;
      p_open_mode = f.p_open_mode;
      p_is_updated = f.p_is_updated;
      p_lock = f.p_lock;
      p_is_read_only = f.p_is_read_only;
      p_lock_file_name = f.p_lock_file_name;
      p_list_of_include_files = f.p_list_of_include_files;
      p_last_modified = f.p_last_modified;
      p_repository_last_modified = f.p_repository_last_modified;
      p_is_on_disk = f.p_is_on_disk;
      p_included_by = f.p_included_by;
      p_kernel = f.p_kernel;

      for (const auto& i : f.p_comments)
        {
          p_comments[i.first] = new oks::Comment(*i.second);
        }
    }

  return *this;
}


  // read xml-file header from file

const char _empty_str[] = "empty";

OksFile::OksFile(std::shared_ptr<OksXmlInputStream> xmls, const std::string& sp, const std::string& fp, OksKernel * k) :
 p_short_name               (sp),
 p_full_name                (fp),
 p_oks_format               (_empty_str, sizeof(_empty_str)-1),
 p_creation_time            (boost::posix_time::not_a_date_time),
 p_last_modification_time   (boost::posix_time::not_a_date_time),
 p_lock                     (),
 p_is_updated               (false),
 p_is_read_only             (true),
 p_last_modified            (0),
 p_repository_last_modified (0),
 p_is_on_disk               (true),
 p_included_by              (0),
 p_kernel                   (k)
{
  const char * fname = "OksFile::OksFile()";

//  create_lock_name();
  check_repository();


    // check file header

  try {

      // skip an xml file header

    {
      const char * tag_start = xmls->get_tag_start();

      if(!oks::cmp_str4(tag_start, "?xml")) {
        xmls->throw_unexpected_tag(tag_start, "?xml");
      }
    }

      // skip possible tag attributes

    while(true) {
      OksXmlAttribute attr(*xmls);

        // check for close of tag

      if(
        oks::cmp_str1(attr.name(), ">") ||
        oks::cmp_str1(attr.name(), "/") ||
	oks::cmp_str1(attr.name(), "?")
      ) { break; }

        // check for start of info tag

      if(!oks::cmp_str7(attr.name(), "version") && !oks::cmp_str8(attr.name(), "encoding")) {
        if(!p_kernel->get_silence_mode()) {
          Oks::warning_msg(fname) << "  Unexpected attribute \'" << attr.name() << "\' in the xml file header. Either \'version\' or \'encoding\' is expected.\n";
        }
      }
    }

  }
  catch (oks::exception & e) {
    throw oks::FailedRead("an xml file header", e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead("an xml file header", e.what());
  }


    // skip document type declaration and comments

  {
    while(true) {
      try {
        const char * dtd = xmls->get_tag();
        const char dtd_start[] = "<!DOCTYPE ";
        if(!strncmp(dtd_start, dtd, sizeof(dtd_start) - 1)) {
          break;
        }
      }
      catch (oks::exception & e) {
        throw oks::FailedRead("DTD section", e);
      }
      catch (std::exception & e) {
        throw oks::FailedRead("DTD section", e.what());
      }
    }
  }


    // skip

  try {
    xmls->store_position();

    const char * a_tag = xmls->get_tag();

    const char xml_stylesheet_start[] = "<?xml-stylesheet";
    if(strncmp(xml_stylesheet_start, a_tag, sizeof(xml_stylesheet_start) - 1)) {
      xmls->restore_position();
    }
  }
  catch (oks::exception & e) {
    throw oks::FailedRead("an xml-stylesheet tag or an oks tag", e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead("an xml-stylesheet tag or an oks tag", e.what());
  }

    // read start-of-tag

  try {
    xmls->get_tag_start();
  }
  catch (oks::exception & e) {
    throw oks::FailedRead("tag", e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead("tag", e.what());
  }


    // read info tag

  try {
    const char * tag_start = xmls->get_tag_start();

    if(!oks::cmp_str4(tag_start, xml_info_tag)) {
      xmls->throw_unexpected_tag(tag_start, xml_info_tag);
    }
  }
  catch (oks::exception & e) {
    throw oks::FailedRead("<info> tag", e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead("<info> tag", e.what());
  }


  std::string oks_version;


    // read class attributes

  try {
    while(true) {
      OksXmlAttribute attr(*xmls);

        // check for close of tag

      if(oks::cmp_str1(attr.name(), ">") || oks::cmp_str1(attr.name(), "/")) { break; }

        // check for known info' attributes

      else if(oks::cmp_str4 (attr.name(), "name"))             p_logical_name.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str4 (attr.name(), "type"))             p_type.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str10(attr.name(), "oks-format"))       p_oks_format.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str10(attr.name(), "created-by"))       p_created_by.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str10(attr.name(), "created-on"))       p_created_on.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str11(attr.name(), "oks-version"))      oks_version.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str12(attr.name(), "num-of-items"))     p_number_of_items = atol(attr.value());
      else if(oks::cmp_str13(attr.name(), "creation-time"))    p_creation_time = oks::str2time(attr.value(), attr.value_len(), p_full_name.c_str());
      else if(oks::cmp_str15(attr.name(), "num-of-includes"))  continue; // ignore this value
      else if(oks::cmp_str16(attr.name(), "last-modified-by")) p_last_modified_by.assign(attr.value(), attr.value_len());
      else if(oks::cmp_str16(attr.name(), "last-modified-on")) p_last_modified_on.assign(attr.value(), attr.value_len());
      else if(!strcmp(attr.name(), "last-modification-time"))  p_last_modification_time = oks::str2time(attr.value(), attr.value_len(), p_full_name.c_str());
      else {
	p_oks_format.clear();
	xmls->throw_unexpected_tag(attr.name(), "info\'s tags");
      }
    }
  }
  catch(oks::exception & e) {
    throw oks::FailedRead("<info> tag attribute", e);
  }
  catch (std::exception & e) {
    throw oks::FailedRead("<info> tag attribute", e.what());
  }

    // read include and comments sections
  while(true) {
    xmls->store_position();

    bool read_include = false;

    try {
      const char * tag_start = xmls->get_tag_start();
      if(oks::cmp_str7(tag_start, xml_include_tag)) { read_include = true; }
      else if(oks::cmp_str8(tag_start, xml_comments_tag)) { read_include = false; }
      else { xmls->restore_position(); return; }
    }
    catch (oks::exception & e) {
      throw oks::FailedRead("a tag", e);
    }
    catch (std::exception & e) {
      throw oks::FailedRead("a tag", e.what());
    }

    if(read_include) {
      while(true) {
        try {
    
          {
	    const char * tag_start = xmls->get_tag_start();
            if(!oks::cmp_str4(tag_start, xml_file_tag)) {
	      if(!oks::cmp_str8(tag_start, "/include")) { xmls->throw_unexpected_tag(tag_start + 1, xml_file_tag); }
	      break;
            }
          }

          while(true) {
            OksXmlAttribute attr(*xmls);

              // check for close of tag
            if(oks::cmp_str1(attr.name(), ">") || oks::cmp_str1(attr.name(), "/")) { break; }

              // check for known info' attributes
            else if(oks::cmp_str4(attr.name(), "path")) p_list_of_include_files.push_back(std::string(attr.value(),attr.value_len()));
            else {
              p_oks_format.clear();
	      xmls->throw_unexpected_tag(attr.name(), "path");
            }
          }

        }
        catch (oks::exception & e) {
          throw oks::FailedRead("<file> tag", e);
        }
        catch (std::exception & e) {
          throw oks::FailedRead("<file> tag", e.what());
        }
      }
    }
    else {
      while(true) {
        try {

          {
	    const char * tag_start = xmls->get_tag_start();
            if(!oks::cmp_str7(tag_start, xml_comment_tag)) {
	      if(!oks::cmp_str9(tag_start, "/comments")) { xmls->throw_unexpected_tag(tag_start, xml_comment_tag); }
	      return;
            }
          }

	  std::unique_ptr<oks::Comment> comment( new oks::Comment() );
	  std::string creation_time;

          while(true) {
            OksXmlAttribute attr(*xmls);

              // check for close of tag
            if(oks::cmp_str1(attr.name(), ">") || oks::cmp_str1(attr.name(), "/")) { break; }

              // check for known info' attributes
            else if(oks::cmp_str4 (attr.name(), "text"))          comment->p_text.assign(attr.value(), attr.value_len());
            else if(oks::cmp_str6 (attr.name(), "author"))        comment->p_author.assign(attr.value(), attr.value_len());
            else if(oks::cmp_str10(attr.name(), "created-by"))    comment->p_created_by.assign(attr.value(), attr.value_len());
            else if(oks::cmp_str10(attr.name(), "created-on"))    comment->p_created_on.assign(attr.value(), attr.value_len());
            else if(oks::cmp_str13(attr.name(), "creation-time")) creation_time.assign(attr.value(), attr.value_len());
            else {
              p_oks_format.clear();
	      xmls->throw_unexpected_tag(attr.name(), "comment\'s tags");
            }
          }

          comment->validate(creation_time);

          if(p_comments.find(creation_time) != p_comments.end()) {
            std::ostringstream text;
            text << "The comment created at \'" << creation_time << "\' was already defined in this file";
            throw std::runtime_error(text.str().c_str());
          }

	  p_comments[creation_time] = comment.release();

        }
        catch (oks::exception & e) {
          throw oks::FailedRead("<comment>", e);
        }
        catch (std::exception & e) {
          throw oks::FailedRead("<comment>", e.what());
        }
      }
    }
  }
}


void
oks::Comment::validate(const std::string& creation_time)
{
    // YYYYMMDDThhmmss
  if( creation_time.size() != 15 || creation_time[8] != 'T' ) {
    std::ostringstream msg;
    msg << "the creation time \'" << creation_time << "\' is not an ISO-8601 date-time";
    throw std::runtime_error(msg.str().c_str());
  }

  validate();
}

void
oks::Comment::validate()
{
  if(p_text.empty()) {
    throw std::runtime_error("text is empty");
  }
}

oks::Comment::Comment(const std::string& text, const std::string& author) :
  p_created_by  (OksKernel::get_user_name()),
  p_author      (author),
  p_created_on  (OksKernel::get_host_name()),
  p_text        (text)
{
  validate();
}

oks::Comment::Comment(const oks::Comment& src) noexcept :
  p_created_by  (src.p_created_by),
  p_author      (src.p_author),
  p_created_on  (src.p_created_on),
  p_text        (src.p_text)
{
}

void
OksFile::add_comment(const std::string& text, const std::string& author)
{
  try {
    lock();
  }
  catch(oks::exception& ex) {
    throw oks::FailedAddComment(*this, ex);
  }

  boost::posix_time::ptime now = boost::posix_time::second_clock::universal_time();
  std::string creation_time = boost::posix_time::to_iso_string(now);

  try {
    std::unique_ptr<oks::Comment> comment( new oks::Comment(text, author) );
    comment->validate();

    oks::Comment *& x(p_comments[creation_time]);

    if(x == 0)
      {
        x = comment.release();
      }
    else
      {
        x->p_text += "\n-----\n";
        x->p_text.append(text);
      }
  }
  catch(std::exception & ex) {
    throw oks::FailedAddComment(*this, ex.what());
  }

  p_is_updated = true;
}

oks::Comment *
OksFile::get_comment(const std::string& creation_time) noexcept
{
  std::map<std::string, oks::Comment *>::const_iterator i = p_comments.find(creation_time);
  return (i == p_comments.end()) ? 0: i->second;
}

void
OksFile::modify_comment(const std::string& creation_time, const std::string& text, const std::string& author)
{
  if(oks::Comment * comment = get_comment(creation_time)) {
    try {
      lock();
    }
    catch(oks::exception& ex) {
      throw oks::FailedChangeComment(*this, creation_time, ex);
    }

    std::string saved_author(comment->p_author);
    std::string saved_text(comment->p_text);

    comment->p_author = author;
    comment->p_text = text;

    try {
      comment->validate();
    }
    catch(std::exception& ex) {
      comment->p_author = saved_author;
      comment->p_text = saved_text;
      throw oks::FailedChangeComment(*this, creation_time, ex.what());
    }

    p_is_updated = true;
  }
  else {
    throw oks::FailedChangeComment(*this, creation_time, "cannot find comment");
  }
}


void
OksFile::remove_comment(const std::string& creation_time)
{
  if(oks::Comment * comment = get_comment(creation_time)) {
    try {
      lock();
    }
    catch(oks::exception& ex) {
      throw oks::FailedRemoveComment(*this, creation_time, ex);
    }

    delete comment;
    p_comments.erase(creation_time);
    p_is_updated = true;
  }
  else {
    throw oks::FailedRemoveComment(*this, creation_time, "cannot find comment");
  }
}


void
OksFile::write(OksXmlOutputStream& xmls)
{
  const static std::string __data("data");
  const static std::string __schema("schema");

  try
    {
      const char __oks_data[] = "oks-data";
      const char __oks_schema[] = "oks-schema";

      const char * id(__oks_data);
      long id_len(sizeof(__oks_data) - 1);
      const char * dtd(xml_data_file_dtd);
      long dtd_len(sizeof(xml_data_file_dtd) - 1);

      if (p_oks_format != __data)
        {
          if (p_oks_format == __schema)
            {
              id = __oks_schema;
              id_len = sizeof(__oks_schema) - 1;
              dtd = xml_schema_file_dtd;
              dtd_len = sizeof(xml_schema_file_dtd) - 1;
            }
          else
            {
              throw std::runtime_error("bad oks format");
            }
        }

        {
          const char __hdr_start[] = "\n\n<!-- ";
          const char __hdr_end[] = " version 2.2 -->\n\n\n";

          std::string header(xml_file_header, sizeof(xml_file_header) - 1);
          header.append(__hdr_start, sizeof(__hdr_start) - 1);
          header.append(id, id_len);
          header.append(__hdr_end, sizeof(__hdr_end) - 1);
          header.append(dtd, dtd_len);

          xmls.put_raw(header.c_str(), header.size());
          xmls.put_raw('\n');
          xmls.put_raw('\n');
        }

      xmls.put_start_tag(id, id_len);

      xmls.put_raw('>');
      xmls.put_raw('\n');
      xmls.put_raw('\n');

      // "creation-time" may not be filled

      std::string created_c_str;
      if (!p_creation_time.is_not_a_date_time())
        {
          created_c_str = boost::posix_time::to_iso_string(p_creation_time);
        }

      // get last-modified information

      if (p_repository_name.empty())
        {
          p_last_modification_time = boost::posix_time::second_clock::universal_time();
          p_last_modified_by = OksKernel::get_user_name();
          p_last_modified_on = OksKernel::get_host_name();
        }

      std::string last_modified_c_str = boost::posix_time::to_iso_string(p_last_modification_time);

      xmls.put_start_tag(xml_info_tag, sizeof(xml_info_tag) - 1);
      xmls.put_attribute("name", sizeof("name") - 1, p_logical_name.c_str());
      xmls.put_attribute("type", sizeof("type") - 1, p_type.c_str());
      xmls.put_attribute("num-of-items", sizeof("num-of-items") - 1, p_number_of_items);
      xmls.put_attribute("oks-format", sizeof("oks-format") - 1, p_oks_format.c_str());
      xmls.put_attribute("oks-version", sizeof("oks-version") - 1, OksKernel::GetVersion());
      xmls.put_attribute("created-by", sizeof("created-by") - 1, p_created_by.c_str());
      xmls.put_attribute("created-on", sizeof("created-on") - 1, p_created_on.c_str());
      if (!created_c_str.empty())
        xmls.put_attribute("creation-time", sizeof("creation-time") - 1, created_c_str.c_str());
      if (p_repository_name.empty())
        {
          xmls.put_attribute("last-modified-by", sizeof("last-modified-by") - 1, p_last_modified_by.c_str());
          xmls.put_attribute("last-modified-on", sizeof("last-modified-on") - 1, p_last_modified_on.c_str());
          xmls.put_attribute("last-modification-time", sizeof("last-modification-time") - 1, last_modified_c_str.c_str());
        }
      xmls.put_end_tag();

      if (!p_list_of_include_files.empty())
        {
          xmls.put_raw('\n');
          xmls.put_start_tag(xml_include_tag, sizeof(xml_include_tag) - 1);

          xmls.put_raw('>');
          xmls.put_raw('\n');

          for (const auto& i : p_list_of_include_files)
            {
              xmls.put_raw(' ');
              xmls.put_start_tag(xml_file_tag, sizeof(xml_file_tag) - 1);
              xmls.put_attribute("path", sizeof("path") - 1, i.c_str());
              xmls.put_end_tag();
            }

          xmls.put_last_tag(xml_include_tag, sizeof(xml_include_tag) - 1);
          xmls.put_raw('\n');
        }

      if (!p_comments.empty())
        {
          xmls.put_start_tag(xml_comments_tag, sizeof(xml_comments_tag) - 1);

          xmls.put_raw('>');
          xmls.put_raw('\n');

          for (const auto& i : p_comments)
            {
              xmls.put_raw(' ');
              xmls.put_start_tag(xml_comment_tag, sizeof(xml_comment_tag) - 1);
              xmls.put_attribute("creation-time", sizeof("creation-time") - 1, i.first.c_str());
              xmls.put_attribute("created-by", sizeof("created-by") - 1, i.second->p_created_by.c_str());
              xmls.put_attribute("created-on", sizeof("created-on") - 1, i.second->p_created_on.c_str());
              xmls.put_attribute("author", sizeof("author") - 1, i.second->p_author.c_str());
              xmls.put_attribute("text", sizeof("text") - 1, i.second->p_text.c_str());
              xmls.put_end_tag();
            }

          xmls.put_last_tag(xml_comments_tag, sizeof(xml_comments_tag) - 1);
          xmls.put_raw('\n');
        }

      xmls.put_raw('\n');
    }
  catch (std::exception& ex)
    {
      throw oks::FailedSave("oks-file", p_full_name, ex.what());
    }

  p_is_on_disk = true;
}

bool
OksFile::is_repository_file() const
{
  return (!p_kernel->get_user_repository_root().empty());
}

void
OksFile::check_repository()
{
//  if(size_t len = OksKernel::get_repository_root().size()) {
//    if(p_full_name.size() > len && p_full_name.substr(0, len) == OksKernel::get_repository_root()) {
//      p_repository = GlobalRepository;
//      p_repository_name = p_full_name.substr(len + 1);
//      TLOG_DEBUG( 3 ) << "file \'" << p_full_name << "\' comes from user repository [\'" << p_repository_name << "\']";
//      return;
//    }
//  }

  const std::string& s(p_kernel->get_user_repository_root());
  if(size_t len = s.size()) {
    if(p_full_name.size() > len && p_full_name.substr(0, len) == s) {
//      p_repository = UserRepository;
      p_repository_name = p_full_name.substr(len + 1);

      TLOG_DEBUG( 3 ) << "file \'" << p_full_name << "\' comes from user repository [\'" << p_repository_name << "\']" ;
      return;

//      std::string repository_file = OksKernel::get_repository_root() + '/' + p_repository_name;
//
//      struct stat buf;
//      if(stat(repository_file.c_str(), &buf) == 0) {
//        TLOG_DEBUG( 3 ) << "file \'" << p_full_name << "\' comes from user repository [\'" << p_repository_name << "\']" ;
//        return;
//      }
//      else {
//        TLOG_DEBUG( 3 ) << "file \'" << p_full_name << "\' is found in user repository [\'" << p_repository_name << "\'] but does not exist in global one [\'" << OksKernel::get_repository_root() << "\']" ;
//      }
    }
  }

  TLOG_DEBUG( 3 ) << "file \'" << p_full_name << "\' is not in a repository" ;
//  p_repository = NoneRepository;
  p_repository_name.clear();
}

void
OksFile::create_lock_name()
{
  p_lock_file_name = p_full_name;

  std::string::size_type idx = p_lock_file_name.find_last_of('/');
  if(idx == std::string::npos) p_lock_file_name = "./";
  else p_lock_file_name.erase(idx+1);

  const char _prefix_str[] = ".oks-lock-";
  p_lock_file_name.append(_prefix_str, sizeof(_prefix_str)-1);

  idx = p_full_name.find_last_of('/');
  std::string::size_type len(p_full_name.size());
  if(idx == std::string::npos) {
    idx = 0;
  }
  else {
    idx++;
    len -= idx;
  }

  p_lock_file_name.append(p_full_name, idx, len);
  p_lock_file_name.append(".txt");
}


  // return true if lock exists and false if does not exist
  // the string is non-empty, if the lock file was read

bool
OksFile::get_lock_string(std::string& lock_file_contents) const
{
  init_lock_name();

  lock_file_contents.clear();

  const char * lock_name = p_lock_file_name.c_str();
  struct stat buf;

  if(stat(lock_name, &buf) == 0) {
    std::ifstream f(lock_name);

    if(f.good()) {
      std::getline(f, lock_file_contents);
      return true;
    }
    else {
      lock_file_contents = "unknown [cannot read lock file \'";
      lock_file_contents += p_lock_file_name;
      lock_file_contents += "\']";
      return true;
    }
  }

  return false;
}

void
OksFile::lock()
{
  init_lock_name();

  if (p_lock != nullptr)
    {
      return;
    }


    // check that the file is not read-only

    {
      struct stat buf;

      if (stat(p_full_name.c_str(), &buf) == 0)
        {
          if (OksKernel::check_read_only(this) == true)
            {
              throw oks::FileLockError(*this, true, "file is read-only");
            }
        }
    }


    // check lock and report problem if it exists and cannot be reset

    {
      std::string lock_file_contents;

      if (get_lock_string(lock_file_contents))
        {
          try
            {
              boost::interprocess::file_lock lock(p_lock_file_name.c_str());

              if (lock.try_lock() == false)
                {
                  std::ostringstream text;
                  text << "file is already locked by \"" << lock_file_contents << '\"';
                  throw oks::FileLockError(*this, true, text.str());
                }
              else
                {
                  try
                    {
                      lock.unlock();
                    }
                  catch (const boost::interprocess::interprocess_exception& ex)
                    {
                      std::ostringstream text;
                      text << "boost::interprocess::unlock() failed: \"" << ex.what() << '\"';
                      throw oks::FileLockError(*this, false, text.str());
                    }

                  if (!p_kernel->get_silence_mode())
                    {
                      Oks::warning_msg("OksFile::lock()") << "Remove obsolete lock of file \'" << p_full_name << "\' created by \n\"" << lock_file_contents << "\"\n";
                    }

                  if (unlink(p_lock_file_name.c_str()) != 0)
                    {
                      std::ostringstream text;
                      text << "failed to remove lock file \'" << p_lock_file_name << "\':\n" << oks::strerror(errno);
                      throw oks::FileLockError(*this, false, text.str());
                    }
                }
            }
          catch (const boost::interprocess::interprocess_exception& ex)
            {
              std::ostringstream text;
              text << "boost::interprocess::try_lock() failed: \"" << ex.what() << '\"';
              throw oks::FileLockError(*this, false, text.str());
            }
        }
    }


    // write lock file

    {
      std::ofstream f(p_lock_file_name.c_str());

      if (!f.good())
        {
          std::ostringstream text;
          text << "failed to create lock file \'" << p_lock_file_name << '\'';
          throw oks::FileLockError(*this, true, text.str());
        }

      try
        {
          boost::posix_time::ptime now(boost::posix_time::second_clock::universal_time());
          f << "process " << getpid() << " on " << OksKernel::get_host_name() << " started by " << OksKernel::get_user_name() << " at " << boost::posix_time::to_simple_string(now) << " (UTC)" << std::endl;
          f.flush();
          f.close();
        }
      catch (std::exception& ex)
        {
          std::ostringstream text;
          text << "failed to write lock file \'" << p_lock_file_name << "\': " << ex.what();
          throw oks::FileLockError(*this, true, text.str());
        }
    }


  try
    {
      p_lock.reset(new boost::interprocess::file_lock(p_lock_file_name.c_str()));

      if (p_lock->try_lock() == false)
        {
          throw std::runtime_error("file is locked by another process");
        }
    }
  catch (const std::exception& ex)
    {
      std::ostringstream text;
      text << "boost::interprocess::try_lock() failed: \"" << ex.what() << '\"';
      p_lock.reset();
      throw oks::FileLockError(*this, false, text.str());
    }
}

void
OksFile::unlock()
{
  if (p_lock == nullptr)
    return;

  try
    {
      p_lock->unlock();
    }
  catch (const boost::interprocess::interprocess_exception& ex)
    {
      std::ostringstream text;
      text << "boost::interprocess::unlock() failed: \"" << ex.what() << '\"';
      p_lock.reset();
      throw oks::FileLockError(*this, false, text.str());
    }

  p_lock.reset();

  if (unlink(p_lock_file_name.c_str()) != 0 && errno != ENOENT)
    {
      std::ostringstream text;
      text << "failed to remove lock file \'" << p_lock_file_name << "\':\n" << oks::strerror(errno);
      throw oks::FileLockError(*this, false, text.str());
    }
}


std::list<std::string>::iterator
OksFile::find_include_file(const std::string& s)
{
  std::list<std::string>::iterator i = p_list_of_include_files.begin();

  for(;i != p_list_of_include_files.end(); ++i) {if(*i == s) break;}

  return i;
}


OksFile *
OksFile::check_parent(const OksFile * parent_h)
{
  if(p_included_by == 0 && parent_h != 0) {
    p_included_by = parent_h;
  }

  return this;
}


void
OksFile::add_include_file(const std::string& s)
{
    // check if the file was already included
  if(find_include_file(s) != p_list_of_include_files.end()) { return; }

    // lock file
  try {
    lock();
  }
  catch(oks::exception& ex) {
    throw oks::FailedAddInclude(*this, s, ex);
  }

    // try to find path to include and load it
  try {
    std::string path = p_kernel->get_file_path(s, this);
    p_kernel->k_load_file(path, true, this, 0);
  }
  catch(std::exception& ex) {
    throw oks::FailedAddInclude(*this, s, ex.what());
  }

    // add include and mark file as updated
  p_list_of_include_files.push_back(s);
  p_is_updated = true;
}


void
OksFile::remove_include_file(const std::string& s)
{
    // search included file
  std::list<std::string>::iterator i = find_include_file(s);

    // check if the file was included
  if(i == p_list_of_include_files.end()) {
    throw oks::FailedRemoveInclude(*this, s, "there is no such include file");
  }

    // lock file
  try {
    lock();
  }
  catch(oks::exception& ex) {
    throw oks::FailedRemoveInclude(*this, s, ex);
  }

    // remove include and mark file as updated
  p_list_of_include_files.erase(i);
  p_is_updated = true;

    // close file, if it is not referenced by others
  p_kernel->k_close_dangling_includes();
}


void
OksFile::rename_include_file(const std::string& old_s, const std::string& new_s)
{
  if(old_s == new_s) return;

  std::list<std::string>::iterator i1 = find_include_file(old_s);
  std::list<std::string>::iterator i2 = find_include_file(new_s);

  if(i1 == p_list_of_include_files.end()) {
    throw oks::FailedRenameInclude(*this, old_s, new_s, "there is no such include file");
  }

  if(i2 != p_list_of_include_files.end()) {
    throw oks::FailedRenameInclude(*this, old_s, new_s, "file with new name is already included");
  }

  try {
    lock();
  }
  catch(oks::exception& ex) {
    throw oks::FailedRenameInclude(*this, old_s, new_s, ex);
  }

  (*i1) = new_s;
  p_is_updated = true;
}


void
OksFile::set_logical_name(const std::string& s)
{
  if(s == p_logical_name) return;

  try {
    lock();
  }
  catch(oks::exception& ex) {
    throw oks::FileChangeError(*this, "set logical name", ex);
  }

  p_logical_name = s;
  p_is_updated = true;
}


void
OksFile::set_type(const std::string& s)
{
  if(s == p_type) return;

  try {
    lock();
  }
  catch(oks::exception& ex) {
    throw oks::FileChangeError(*this, "set type", ex);
  }

  p_type = s;
  p_is_updated = true;
}

std::ostream& operator<<(std::ostream& s, const oks::Comment& c)
{
  s << "user \"" << c.get_created_by() << "\" (" << c.get_author() << ")"
    << " on \"" << c.get_created_on() << "\" wrote: \"" << c.get_text() << "\"";

  return s;
}


std::ostream&
operator<<(std::ostream& s, const OksFile& f)
{
  s << "OKS file\n"
       " short file name:        \'" << f.get_short_file_name() << "\'\n"
       " full file name:         \'" << f.get_full_file_name() << "\'\n"
       " lock file name:         \'" << f.get_lock_file() << "\'\n"
       " logical name:           \'" << f.get_logical_name() << "\'\n"
       " type:                   \'" << f.get_type() << "\'\n"
       " oks format:             \'" << f.get_oks_format() << "\'\n"
       " number of items:        " << f.get_number_of_items() << "\n"
       " created by:             " << f.get_created_by() << "\n"
       " creation time:          " << boost::posix_time::to_simple_string(f.get_creation_time()) << "\n"
       " created on:             " << f.get_created_on() << "\n"
       " last modified by:       \'" << f.get_last_modified_by() << "\'\n"
       " last modification time: " << boost::posix_time::to_simple_string(f.get_last_modification_time()) << "\'\n"
       " last modified on:       " << f.get_last_modified_on() << "\'\n"
       " is " << (f.is_read_only() ? "read-only" : "read-write") << "\n"
       " is " << (f.is_locked() ? "" : "not ") << "locked\n"
       " is " << (f.is_updated() ? "" : "not ") << "updated\n";

    {
      size_t num_of_includes = f.get_include_files().size();

      if (num_of_includes)
        {
          s << " contains " << num_of_includes << " include file(s)";
          for (const auto& i : f.get_include_files())
            s << "  - \'" << i << "\'\n";
        }
      else
        {
          s << " contains no include files\n";
        }
    }

    {
      size_t num_of_comments = f.get_comments().size();

      if (num_of_comments)
        {
          s << " contains " << num_of_comments << " comment(s)";
          for (const auto& i : f.get_comments())
            s << "  - " << boost::posix_time::to_simple_string(boost::posix_time::from_iso_string(i.first)) << ": \'" << *i.second << "\'\n";
        }
      else
        {
          s << " contains no comments\n";
        }
    }

  s << "END of oks file\n";

  return s;
}


std::string
OksFile::make_repository_name() const
{
  std::string name;

  if(!OksKernel::get_repository_root().empty()) {
    name = OksKernel::get_repository_root() + '/' + get_repository_name();
  }

  return name;
}


void
OksFile::update_status_of_file(bool update_local, bool update_repository)
{
  struct stat buf;

  if(update_local == true && p_is_on_disk == true) {

    if(stat(p_full_name.c_str(), &buf) == 0) {
      p_last_modified = buf.st_mtime;
    }
  }

  if(update_repository == true /*&& get_repository() == UserRepository*/) {
    std::string full_repository_name = make_repository_name();

    if(!full_repository_name.empty() && stat(full_repository_name.c_str(), &buf) == 0) {
      p_repository_last_modified = buf.st_mtime;
    }
  }
}


OksFile::FileStatus
OksFile::get_status_of_file() const
{
  if(p_is_on_disk == false) {
    return FileWasNotSaved;
  }
  else {
    struct stat buf;

    if(stat(p_full_name.c_str(), &buf) != 0) {
      return FileRemoved;
    }

    if(buf.st_mtime != p_last_modified) {
      return FileModified;
    }

//    if(get_repository() == UserRepository) {
//      std::string full_repository_name = make_repository_name();
//
//      if(!full_repository_name.empty()) {
//        if(stat(full_repository_name.c_str(), &buf) != 0) {
//          return FileRepositoryRemoved;
//        }
//
//        if(buf.st_mtime != p_repository_last_modified) {
//          return FileRepositoryModified;
//        }
//      }
//    }
  }

  return FileNotModified;
}


void
OksFile::rename(const std::string& short_name, const std::string& full_name)
{
  p_short_name = short_name;
  p_full_name = full_name;
  create_lock_name();
  check_repository();
}

void
OksFile::rename(const std::string& full_name)
{
  p_full_name = full_name;
  create_lock_name();
  update_status_of_file();
}

void
OksFile::get_all_include_files(const OksKernel * kernel, std::set<OksFile *>& out)
{
  const OksFile::Map & schema_files = kernel->schema_files();
  const OksFile::Map & data_files = kernel->data_files();

  for(std::list<std::string>::iterator i = p_list_of_include_files.begin(); i != p_list_of_include_files.end(); ++i) {
    std::string s = kernel->get_file_path(*i, this);
    if(s.size()) {
      OksFile::Map::const_iterator j = data_files.find(&s);
      if(j != data_files.end()) {
        if(out.find(j->second) != out.end()) { continue; }
        out.insert(j->second);
      }
      else {
        j = schema_files.find(&s);
        if(j != schema_files.end()) {
          if(out.find(j->second) != out.end()) { continue; }
          out.insert(j->second);
        }
        else {
          continue;
        }
      }

      j->second->get_all_include_files(kernel, out);
    }
  }
}

// return true if the string is not info line:
// <info ... last-modification-time="nnnnnnnnTnnnnnn"/>

static bool
is_not_info(const std::string& line)
{
  static const char start_tag[] = "<info ";
  static const char last_tag[] = "last-modification-time=\"yyyymmddThhmmss\"/>"; // 24 - is a position of first double quote

  const std::string::size_type len = line.length();

  return(
    ( len < (sizeof(start_tag) + sizeof(last_tag)) ) ||
    ( line.compare(0, sizeof(start_tag)-1, start_tag, sizeof(start_tag)-1) != 0 ) ||
    ( line.compare(len - sizeof(last_tag) + 1, 24, last_tag, 24) != 0 )
  );
}

bool
OksFile::compare(const char * file1_name, const char * file2_name)
{
  std::ifstream f1(file1_name);
  std::ifstream f2(file2_name);

  if (!f1)
    {
      throw oks::FileCompareError(file1_name, file2_name, "cannot open file");
    }

  if (!f2)
    {
      throw oks::FileCompareError(file2_name, file1_name, "cannot open file");
    }

  std::string line1;
  std::string line2;

  while (true)
    {
      if (f1.eof() && f2.eof())
        {
          return true;
        }

      std::getline(f1, line1);
      std::getline(f2, line2);

      if ((line1 != line2) && (is_not_info(line1) || is_not_info(line2)))
        {
          return false;
        }
    }

  return true;
}
