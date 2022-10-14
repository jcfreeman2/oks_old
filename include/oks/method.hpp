/**	
 *	\file oks/method.h
 *	
 *	This file is part of the OKS package.
 *      Author: Igor SOLOVIEV "https://phonebook.cern.ch/phonebook/#personDetails/?id=432778"
 *	
 *	This file contains the declarations for the OKS method.
 */

#ifndef OKS_METHOD_H
#define OKS_METHOD_H

#include <string>
#include <list>

#include "oks/defs.hpp"

class   OksXmlOutputStream;
class   OksXmlInputStream;
class   OksClass;
class   OksMethod;


  ///	OKS method implementation class.
  /**
   *  	An OKS method may have implementations on different languages (e.g. "c++", "java", etc.).
   *  	Implementations linked with a method shall have different languages.
   *  	The method implementations are used by genconfig package generating DAL.
   */

class OksMethodImplementation
{
  friend class OksKernel;
  friend class OksMethod;


  public:

      /**
       *  \brief OKS method implementation constructor.
       *
       *  Create new OKS method implementation providing language, method prototype and body.
       *
       *  The parameters are:
       *  \param language     programming language used for given method implementation (max 16 bytes, see #s_max_language_length variable)
       *  \param prototype    prototype of method implementation for selected programming language (max 1024 bytes, see #s_max_prototype_length variable)
       *  \param body         optional body of method implementation (max 2048 bytes, see #s_max_body_length variable)
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    OksMethodImplementation(const std::string& language, const std::string& prototype, const std::string& body, OksMethod * p = nullptr);

    bool operator==(const class OksMethodImplementation &) const;
    friend std::ostream& operator<<(std::ostream&, const OksMethodImplementation&);


      /** Get method implementation language. */

    const std::string& get_language() const noexcept {return p_language;}


      /**
       *  \brief Set method implementation language.
       *
       *  Implementations linked with a method shall have different languages.
       *
       *  \param language    new method implementation language (max 16 bytes, see #s_max_language_length variable)
       *  In case of problems the oks::exception is thrown.
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void set_language(const std::string& language);


      /** Get prototype of method implementation. */

    const std::string& get_prototype() const noexcept {return p_prototype;}


      /**
       *  \brief Set method implementation prototype.
       *
       *  \param prototype    new method implementation prototype (max 1024 bytes, see #s_max_prototype_length variable)
       *  In case of problems the oks::exception is thrown.
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void set_prototype(const std::string& prototype);


      /** Get body of method implementation. */

    const std::string& get_body() const noexcept {return p_body;}


      /**
       *  \brief Set method implementation body.
       *
       *  \param body    new method implementation body (max 2048 bytes, see #s_max_body_length variable)
       *  In case of problems the oks::exception is thrown.
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void set_body(const std::string& body);


  private:

    std::string p_language;
    std::string p_prototype;
    std::string p_body;
    
    OksMethod * p_method;


      // to be used by OksMethod only

    OksMethodImplementation(OksXmlInputStream &, OksMethod *);
    void save(OksXmlOutputStream &) const;


      // valid xml tags and attributes

    static const char method_impl_xml_tag[];
    static const char language_xml_attr[];
    static const char prototype_xml_attr[];
    static const char body_xml_attr[];


};


  ///	OKS method class.
  /**
   *  	The class implements OKS method that is a part of OKS class.
   *  	It has name, description and implementations.
   *
   *    Methods can be used in generated Data Access Libraries (DAL).
   *    This allows to insert user-defined methods (also known as "DAL algorithms")
   *    into generated DAL classes. In such case the method should have
   *    appropriate implementations for c++ and/or Java languages.
   *    For more information on DAL generation see genconfig User's Guide.
   */

class OksMethod
{
  friend class OksKernel;
  friend class OksClass;
  friend class OksMethodImplementation;


  public:

      /**
       *  \brief OKS method constructor.
       *
       *  Create new OKS method providing name.
       *
       *  The parameter is:
       *  \param name     name of the method
       */
      
    OksMethod(const std::string& name, OksClass * p = nullptr);


      /**
       *  \brief OKS method constructor.
       *
       *  Create new OKS method providing name and descriptions.
       *
       *  The parameter is:
       *  \param name            name of the method (max 128 bytes, see #s_max_name_length variable)
       *  \param description     description of the method (max 2000 bytes, see #s_max_description_length variable)
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    OksMethod(const std::string& name, const std::string& description, OksClass * p = nullptr);


    ~OksMethod();

    bool operator==(const class OksMethod &) const;
    friend std::ostream& operator<<(std::ostream&, const OksMethod&);


      /** Get name of method. */

    const std::string& get_name() const noexcept {return p_name;}


      /**
       *  \brief Set method name.
       *
       *  Methods linked with the same class shall have different names.
       *
       *  \param name    new name of method (max 128 bytes, see #s_max_name_length variable)
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void set_name(const std::string& name);


      /** Get description of method. */

    const std::string& get_description() const noexcept {return p_description;}


      /**
       *  \brief Set method description.
       *
       *  \param description    new description of method (max 2000 bytes, see #s_max_description_length variable)
       *
       *  \throw oks::exception is thrown in case of problems.
       */

    void set_description(const std::string& description);


      /** Get implementations of method. */

    const std::list<OksMethodImplementation *> * implementations() const noexcept {return p_implementations;}


      /**
       *  \brief Find method implementation.
       *
       *  Find method implementation by language.
       *
       *  \param language    language of method implementation
       *  \return            the pointer on method implementation for given language, or 0, if it is not found
       */

    OksMethodImplementation * find_implementation(const std::string& language) const;



      /**
       *  \brief Add method implementation.
       *
       *  Add new method implementation.
       *  Implementations linked with method shall have different languages.
       *
       *  \param language     language of method implementation
       *  \param prototype    prototype of method implementation
       *  \param body         body of method implementation
       *
       *  In case of problems the oks::exception is thrown.
       */

    void add_implementation(const std::string& language, const std::string& prototype, const std::string& body);


      /**
       *  \brief Remove method implementation.
       *
       *  Remove method implementation by language.
       *
       *  \param language     language of method implementation
       *
       *  In case of problems the oks::exception is thrown.
       */

    void remove_implementation(const std::string& language);


  private:

    OksClass *  p_class;
    std::string p_name;
    std::string p_description;
    std::list<OksMethodImplementation *> * p_implementations;


      // to be used by OksClass only

    OksMethod (OksXmlInputStream &, OksClass*);
    void save(OksXmlOutputStream &) const;


      // valid xml tags and attributes

    static const char method_xml_tag[];
    static const char name_xml_attr[];
    static const char description_xml_attr[];

};


#endif
