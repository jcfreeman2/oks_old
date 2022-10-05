  /**
   *  \file Schema.h This file contains several schema classes,
   *  which can be used to get information about classes and their properties.
   *  \author Igor Soloviev
   *  \brief config schema description
   */

#ifndef CONFIG_SCHEMA_H_
#define CONFIG_SCHEMA_H_

#include <string>
#include <vector>
#include <iostream>

namespace daq {
  namespace config {

      /** The supported attribute types. */

    enum type_t {
      bool_type,       /*!< the boolean type */
      s8_type,         /*!< the 8-bits signed integer type */
      u8_type,         /*!< the 8-bits unsigned integer type */
      s16_type,        /*!< the 16-bits signed integer type */
      u16_type,        /*!< the 16-bits unsigned integer type */
      s32_type,        /*!< the 32-bits signed integer type */
      u32_type,        /*!< the 32-bits unsigned integer type */
      s64_type,        /*!< the 64-bits signed integer type */
      u64_type,        /*!< the 64-bits unsigned integer type */
      float_type,      /*!< the float type */
      double_type,     /*!< the double type */
      date_type,       /*!< the date type */
      time_type,       /*!< the time type */
      string_type,     /*!< the string type */
      enum_type,       /*!< the enumeration type */
      class_type       /*!< the class reference type */
    };


      /** The numeration format to be used for integer value representation. */

    enum int_format_t {
      oct_int_format,  /*!< use octal numeration for attribute value */
      dec_int_format,  /*!< use decimal numeration for attribute value */
      hex_int_format,  /*!< use hexadecimal numeration for attribute value */
      na_int_format    /*!< not applicable, the attribute type is not integer */
    };


      /** The description of attribute. */

    struct attribute_t {

      std::string p_name;            /*!< the attribute name */
      type_t p_type;                 /*!< the attribute type */
      std::string p_range;           /*!< the attribute range in UML syntax (e.g.: "A,B,C..D,*..F,G..*" => value can be A, B, greater or equal to C and less or equal to D, less or equal to F, greater or equal to G) */
      int_format_t p_int_format;     /*!< the representation format for integer values */
      bool p_is_not_null;            /*!< if true, the value of attribute cannot be null */
      bool p_is_multi_value;         /*!< if true, the value of attribute is a list of primitive values (e.g. list of strings, list of integer numbers, etc.) */
      std::string p_default_value;   /*!< the default value of attribute */
      std::string p_description;     /*!< the description text of attribute */

        /** Create attribute description */

      attribute_t(
        const std::string& name,
        type_t type,
        const std::string& range,
        int_format_t int_format,
        bool is_not_null,
        bool is_multi_value,
        const std::string& default_value,
        const std::string& description
      );


        /** Default constructor for Python binding */

      attribute_t() { ; }


        /** Print description of attribute to stream. */

      void print(
        std::ostream& out,               /*!< the output stream */
	const std::string& prefix = ""   /*!< optional shift output using prefix */
      ) const;

        /** Return string corresponding to given data type */
      static const char * type2str(type_t type /*!< the data type */);

      /** Return short string (oks data type) corresponding to given data type */
      static const char * type(type_t type /*!< the data type */);

      /** Return string corresponding to given integer value representation format */
      static const char * format2str(int_format_t format /*!< the integer value representation format */);

    };


      /** The relationship cardinality. */

    enum cardinality_t {
      zero_or_one,    /*!< relationship references zero or one object */
      zero_or_many,   /*!< relationship references zero or many objects */
      only_one,       /*!< relationship always references one object */
      one_or_many     /*!< relationship references one or many objects */
    };


      /** The description of relationship. */

    struct relationship_t {

      std::string p_name;           /*!< the relationship name */
      std::string p_type;           /*!< the relationship class type */
      cardinality_t p_cardinality;  /*!< the relationship cardinality */
      bool p_is_aggregation;        /*!< if true, the relationship is an aggregation (composite); otherwise the relationship is simple (weak) */
      std::string p_description;    /*!< the description text of relationship */

        /** Create relationship description */

      relationship_t(
        const std::string& name,
        const std::string& type,
	bool can_be_null,
	bool is_multi_value,
        bool is_aggregation,
        const std::string& description
      );


        /** Default constructor for Python binding */

      relationship_t() { ; }


        /** Print description of relationship to stream. */

      void print(
        std::ostream& out,               /*!< the output stream */
	const std::string& prefix = ""   /*!< optional shift output using prefix */
      ) const;


        /** Return string corresponding to given cardinality */

      static const char * card2str(cardinality_t cardinality);

    };


      /** The description of class. */

    struct class_t {

      std::string p_name;                                  /*!< the class name */
      std::string p_description;                           /*!< the description text of class */
      bool p_abstract;                                     /*!< if true, the class is abstract and has no objects */
      const std::vector<std::string> p_superclasses;       /*!< the names of direct superclasses */
      const std::vector<std::string> p_subclasses;         /*!< the names of direct subclasses */
      const std::vector<attribute_t> p_attributes;         /*!< the all attributes of the class */
      const std::vector<relationship_t> p_relationships;   /*!< the all relationships of the class */

        /** Create class description */

      class_t(
        const std::string& name,
        const std::string& description,
	bool is_abstract
      );


        /** Default constructor for Python binding */

      class_t() { ; }


        /** Print description of class to stream. */

      void print(
        std::ostream& out,               /*!< the output stream */
	const std::string& prefix = ""   /*!< optional shift output using prefix */
      ) const;

    };

    const char * bool2str(bool value);

    std::ostream& operator<<(std::ostream& out, const attribute_t &);
    std::ostream& operator<<(std::ostream& out, const relationship_t &);
    std::ostream& operator<<(std::ostream& out, const class_t &);

  }
}

#endif // CONFIG_SCHEMA_H_
