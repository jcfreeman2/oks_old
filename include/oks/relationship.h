/**	
 *	\file oks/relationship.h
 *	
 *	This file is part of the OKS package.
 *      Author: Igor SOLOVIEV "https://phonebook.cern.ch/phonebook/#personDetails/?id=432778"
 *	
 *	This file contains the declarations for the OKS relationship.
 */

#ifndef OKS_RELATIONSHIP_H
#define OKS_RELATIONSHIP_H

#include <oks/defs.h>

#include <string>


class	OksClass;
class   OksXmlOutputStream;
class   OksXmlInputStream;

  /// @addtogroup oks


  /**
   *    @ingroup oks
   *
   *	\brief OKS relationship class.
   *	
   *  	The class implements OKS relationship that is a part of an OKS class.
   *
   *  	A relationship has name, class type and description.
   *  	A relationship is [zero|one]-to-one or [zero|one]-to-many.
   *
   *    There are two basic types of OKS relationships:
   *    - \b weak or \b general reference (relationship): default reference between two objects, it carries no special semantic;
   *    - \b strong or \b composite reference (relationship): the objects connected by composite references are considered as a whole object.
   *
   *    A composite reference may be exclusive or shared
   *    (an exclusive object is component of only one object,
   *    a shared object may be referenced by more than one object).
   *
   *    As well a composite reference can be dependent or independent.
   *    The existence of object, referenced thought dependent composite reference,
   *    is dependent from existence of their composite parents.
   *    The deleting of all composite parents results the deleting of
   *    their composite dependent child objects (i.e. reference count is zero).
   *
   *    \paragraph ex Example
   *    The following code:
\code
#include <oks/relationship.h>

int main()
{
  try
    {
      OksRelationship r(
        "consists of",                                   // name
        "Element",                                       // class type
        OksRelationship::Zero,                           // low cc in Zero
        OksRelationship::Many,                           // high cc is Many
        true,                                            // is composite
        true,                                            // is exclusive
        true,                                            // is dependent
        "A structure consists of zero or many elements"  // description
      );

      std::cout << r;
    }
  catch (const std::exception& ex)
    {
      std::cerr << "Caught exception:\n" << ex.what() << std::endl;
    }
  return 0;
}
\endcode
   *    produces:
\code
Relationship name: "consists of"
 class type: "Element"
 low cardinality constraint is zero
 high cardinality constraint is many
 is composite reference
 is exclusive reference
 is dependent reference
 has description: "A structure consists of zero or many elements"
\endcode
   */

class OksRelationship
{
  friend class	OksKernel;
  friend class	OksClass;
  friend struct	OksData;
  friend class	OksObject;


  public:

      /**
       *  Valid cardinality constraint types
       */

    enum CardinalityConstraint {
      Zero, One, Many
    };



    /**
     *  \brief OKS relationship simple constructor.
     *
     *  Create new OKS relationship providing name.
     *
     *  The parameter is:
     *  \param name     name of the attribute
     *  \param p        containing parent class
     *
     *  \throw oks::exception is thrown in case of problems.
     */

    OksRelationship (const std::string& name, OksClass * p = nullptr);


    /**
     *  \brief OKS relationship complete constructor.
     *
     *  Create new OKS relationship providing all properties.
     *
     *  The parameters are:
     *  \param name        name of the attribute (max 128 bytes, see #s_max_name_length variable)
     *  \param type        class type of relationship (i.e. name of existing OKS class)
     *  \param low_cc      lowest possible number of objects linked via this relationship ("zero" or "one")
     *  \param high_cc     highest possible number of objects linked via this relationship ("one" or "many")
     *  \param composite   if true, set strong dependency on children objects
     *  \param exclusive   if true, guaranty unique reference on children objects via such relationship by any object
     *  \param dependent   if true, existence of children objects depends on existence of parent
     *  \param description  description of the method (max 2000 bytes, see #s_max_description_length variable)
     *  \param p        containing parent class
     *
     *  \throw oks::exception is thrown in case of problems.
     */

    OksRelationship (const std::string& name,
                     const std::string& type,
                     CardinalityConstraint low_cc,
                     CardinalityConstraint high_cc,
                     bool composite,
                     bool exclusive,
                     bool dependent,
                     const std::string& description,
                     OksClass * p = nullptr);


    bool operator==(const class OksRelationship &) const;

    friend std::ostream& operator<<(std::ostream&, const OksRelationship&);


      /**
       *  Methods to access relationship properties.
       */

    const std::string& get_name() const noexcept {return p_name;}


      /**
       *  \brief Set relationship name.
       *
       *  Relationships linked with the same class shall have different names.
       *
       *  \param name    new name of relationship (max 128 bytes, see #s_max_name_length variable)
       *
       *  In case of problems the oks::exception is thrown.
       */

    void set_name(const std::string&);


      /** Get relationship type (pointer on OKS class). */

    OksClass * get_class_type() const noexcept {return p_class_type;}


      /** Get relationship type (string). */

    const std::string& get_type() const noexcept {return p_rclass;}


      /**
       *  \brief Set relationship type.
       *
       *  The type is a name of existing OKS class.
       *  The value of relationship can point on objects of that class
       *  or any objects of classes derived from this one.
       *
       *  \param type    new type of relationship
       *
       *  In case of problems the oks::exception is thrown.
       */

    void set_type(const std::string& type);


      /** Return description of relationship */

    const std::string& get_description() const noexcept {return p_description;}

      /**
       *  \brief Set relationship description.
       *
       *  \param description    new description of relationship (max 2000 bytes, see #s_max_description_length variable)
       *
       *  In case of problems the oks::exception is thrown.
       */


    void set_description(const std::string& description);


      /**
       *  \brief Get relationship low cardinality constraint.
       *
       *  See set_low_cardinality_constraint() for more information.
       *
       *  \return   the relationship low cardinality constraint
       */

    CardinalityConstraint get_low_cardinality_constraint() const noexcept {return p_low_cc;}


      /**
       *  \brief Set relationship low cardinality constraint.
       *      
       *  The low cardinality constraint defiles lowest possible
       *  number of objects linked via this relationship.
       *  It must be OksRelationship::Zero or OksRelationship::One.
       *
       *  \param cc   the low cardinality constraint
       *
       *  In case of problems the oks::exception is thrown.
       */

    void set_low_cardinality_constraint(CardinalityConstraint cc);


      /**
       *  \brief Get relationship high cardinality constraint.
       *
       *  See set_high_cardinality_constraint() for more information.
       *
       *  \return   the relationship high cardinality constraint
       */

    CardinalityConstraint get_high_cardinality_constraint() const noexcept {return p_high_cc;}


      /**
       *  \brief Set relationship high cardinality constraint.
       *
       *  The high cardinality constraint defiles highest possible
       *  number of objects linked via this relationship.
       *  It must be OksRelationship::One or OksRelationship::Many.
       *
       *  \param cc   the high cardinality constraint
       *
       *  In case of problems the oks::exception is thrown.
       */

    void set_high_cardinality_constraint(CardinalityConstraint);


      /** Returns the true, if relationship is composite and false, if not. */

    bool get_is_composite() const noexcept {return p_composite;}


      /**
       *  \brief Set the composite relationship property.
       *
       *  If relationship is composite, the parent object sets \b strong 
       *  references on children objects and it is considered as \a composite or \a compound object.
       *
       *  The composite relationships are presented in a special way (as a diamond)
       *  by the OKS data editor graphical views, e.g. document is compound of sections:
          \code
          ============    1..N ===========
          | Document |<>-------| Section |
          ============   has   ===========
          \endcode
       *
       *  Also, the genconfig package generates print() method of data access library
       *  in a different way depending on the composite relationship state:
       *  - objects referenced via non-composite relationship are printed as references from parent object
       *  - objects referenced via composite relationship are printed with details as part of  parent object
       *
       *  \param composite      pass true to make relationship composite
       *
       *  In case of problems the oks::exception is thrown.
       */

    void set_is_composite(bool composite);


      /** Returns the true, if relationship is exclusive and false, if not. */

    bool get_is_exclusive() const noexcept {return p_exclusive;}


      /**
       *  \brief Set the composite relationship exclusive property.
       *
       *  If relationship is exclusive, the child object can only be
       *  referenced by single parent via given relationship.
       *
       *  \param exclusive      pass true to make relationship exclusive
       *
       *  In case of problems the oks::exception is thrown.
       */

    void set_is_exclusive(bool exclusive);


      /** Returns the true, if relationship is dependent and false, if not. */

    bool get_is_dependent() const noexcept {return p_dependent;}


      /**
       *  \brief Set the composite relationship dependent property.
       *
       *  If relationship is dependent, the child object will be removed
       *  when its parent object is removed and the child is not referenced
       *  by another parents via dependent composite relationships.
       *
       *  \param dependent   pass true to make relationship dependent
       *
       *  In case of problems the oks::exception is thrown.
       */

    void set_is_dependent(bool dependent);


      /** Helper method to convert valid string to cardinality constraint */

    static CardinalityConstraint str2card(const char *) noexcept;


      /** Helper method to convert cardinality constraint to string */

    static const char * card2str(CardinalityConstraint) noexcept;


  private:

    std::string		  p_name;
    std::string		  p_rclass;
    CardinalityConstraint p_low_cc;
    CardinalityConstraint p_high_cc;
    bool		  p_composite;
    bool		  p_exclusive;
    bool		  p_dependent;
    std::string		  p_description;
    OksClass *		  p_class;
    OksClass *		  p_class_type;
    bool                  p_ordered;


      /**
       *  To be used by OksClass only
       */

    OksRelationship	  (OksXmlInputStream&, OksClass*); /// private constructor from XML stream
    void                  save(OksXmlOutputStream&) const; /// private method to save in XML stream



      /**
       *  Valid xml tags and attributes
       */

    static const char relationship_xml_tag[];
    static const char name_xml_attr[];
    static const char description_xml_attr[];
    static const char class_type_xml_attr[];
    static const char low_cc_xml_attr[];
    static const char high_cc_xml_attr[];
    static const char is_composite_xml_attr[];
    static const char is_exclusive_xml_attr[];
    static const char is_dependent_xml_attr[];
    static const char mv_implement_xml_attr[];
    static const char ordered_xml_attr[];

};


#endif
