/**
 *  \file oks_dump.cpp
 *
 *  This file is part of the OKS package.
 *  Author: <Igor.Soloviev@cern.ch>
 *
 *  This file contains the implementation of the OKS application to dump
 *  contents of the OKS database files.
 *
 */

#include <vector>
#include <iostream>

#include <string.h>
#include <stdlib.h>

#include "oks/kernel.hpp"
#include "oks/query.hpp"
#include "oks/exceptions.hpp"


enum __OksDumpExitStatus__ {
  __Success__ = 0,
  __BadCommandLine__,
  __BadOksFile__,
  __BadQuery__,
  __NoSuchClass__,
  __FoundDanglingReferences__,
  __ExceptionCaught__
};


static void
printUsage(std::ostream& s)
{
  s << "Usage: oks_dump\n"
       "    [--files-only | --files-stat-only | --schema-files-only | --schema-files-stat-only | --data-files-only | --data-files-stat-only]\n"
       "    [--class name-of-class [--query query [--print-references recursion-depth [class-name*] [--]] [--print-referenced_by [name] [--]]]]\n"
       "    [--path object-from object-to query]\n"
       "    [--allow-duplicated-objects-via-inheritance]\n"
       "    [--version]\n"
       "    [--help]\n"
       "    [--input-from-files] database-file [database-file(s)]\n"
       "\n"
       "Options:\n"
       "    -f | --files-only                                 print list of oks files names\n"
       "    -F | --files-stat-only                            print list of oks files with statistic details (size, number of items\n" 
       "    -s | --schema-files-only                          print list of schema oks files names\n"
       "    -S | --schema-files-stat-only                     print list of oks schema files with statistic details\n"
       "    -d | --data-files-only                            print list of data oks files names\n"
       "    -D | --data-files-stat-only                       print list of oks data files with statistic details\n"
       "    -c | --class class_name                           dump given class (all objects or matching some query)\n"
       "    -q | --query query                                print objects matching query (can only be used with class)\n"
       "    -r | --print-references N C1 C2 ... CX            print objects referenced by found objects (can only be used with query), where:\n"
       "                                                       * the parameter N defines recursion depth for referenced objects (> 0)\n"
       "                                                       * the optional set of names {C1 .. CX} defines [sub-]classes for above objects\n"
       "    -b | --print-referenced_by [name]                 print objects referencing found objects (can only be used with query), where:\n"
       "                                                       * the optional parameter name defines name of relationship\n"
       "    -p | --path obj query                             print path from object \'obj\' to object of query expression\n"
       "    -i | --input-from-files                           read oks files to be loaded from file(s) instead of command line\n"
       "                                                      (to avoid problems with long command line, when there is huge number of files)\n"
       "    -a | --allow-duplicated-objects-via-inheritance   do not stop if there are duplicated object via inheritance hierarchy\n"
       "    -v | --version                                    print version\n"
       "    -h | --help                                       print this text\n"
       "\n"
       "Description:\n"
       "    Dumps contents of the OKS database.\n"
       "\n"
       "Return Status:\n"
       "    0 - no problems found\n"
       "    1 - bad command line parameter\n"
       "    2 - bad oks file(s)\n"
       "    3 - bad query passed via -q or -p options\n"
       "    4 - cannot find class passed via -c option\n"
       "    5 - loaded objects have dangling references\n"
       "    6 - caught an exception\n"
       "\n";
}

static void
no_param(const char * s)
{
  Oks::error_msg("oks_dump") << "no parameter(s) for command line argument \'" << s << "\' provided\n\n";
  exit(EXIT_FAILURE);
}

OksObject*
find_object(char * s, OksKernel& k)
{
  char * id = s;

  if((s = strchr(id, '@')) == 0) return 0;

  *s = '\0';
  s++;

  if(OksClass * c = k.find_class(s)) {
    if(OksObject * o = c->get_object(id)) {
      return o;
    }
    else {
      Oks::error_msg("oks_dump::find_object()") << "cannot find object \"" << id << '@' << s << "\"\n";
    }
  }
  else {
    Oks::error_msg("oks_dump::find_object()") << "cannot find class \"" << s << "\"\n";
  }

  return 0;
}

int
main(int argc, char **argv)
{
  if(argc == 1) {
    printUsage(std::cerr);
    return __BadCommandLine__;
  }

  OksKernel kernel;
  kernel.set_test_duplicated_objects_via_inheritance_mode(true);

  try {

    int dump_files_only = 0; // 0 - none, 12 - data, 1 - schema, 2 - schema & data
    const char * class_name = 0;
    const char * query = 0;
    const char * object_from = 0;
    const char * path_query = 0;
    long recursion_depth = 0;
    bool print_referenced_by = false;
    const char * ref_by_rel_name = "*";
    std::vector<std::string> ref_classes;
    bool input_from_files = false;
    bool print_files_stat = false;

    for(int i = 1; i < argc; i++) {
      const char * cp = argv[i];

      if(!strcmp(cp, "-h") || !strcmp(cp, "--help")) {
        printUsage(std::cout);		
        return __Success__;
      }
      else if(!strcmp(cp, "-v") || !strcmp(cp, "--version")) {
        std::cout << "OKS kernel version " << OksKernel::GetVersion() << std::endl;		
        return __Success__;
      }
      else if(!strcmp(cp, "-a") || !strcmp(cp, "--allow-duplicated-objects-via-inheritance")) {
        kernel.set_test_duplicated_objects_via_inheritance_mode(false);
      }
      else if(!strcmp(cp, "-f") || !strcmp(cp, "--files-only")) {
        dump_files_only = 2;
      }
      else if(!strcmp(cp, "-F") || !strcmp(cp, "--files-stat-only")) {
        dump_files_only = 2;
        print_files_stat = true;
      }
      else if(!strcmp(cp, "-s") || !strcmp(cp, "--schema-files-only")) {
        dump_files_only = 1;
      }
      else if(!strcmp(cp, "-S") || !strcmp(cp, "--schema-files-stat-only")) {
        dump_files_only = 1;
        print_files_stat = true;
      }
      else if(!strcmp(cp, "-d") || !strcmp(cp, "--data-files-only")) {
        dump_files_only = 12;
      }
      else if(!strcmp(cp, "-D") || !strcmp(cp, "--data-files-stat-only")) {
        dump_files_only = 12;
        print_files_stat = true;
      }
      else if(!strcmp(cp, "-c") || !strcmp(cp, "--class")) {
        if(++i == argc) { no_param(cp); } else { class_name = argv[i]; }
      }
      else if(!strcmp(cp, "-q") || !strcmp(cp, "--query")) {
        if(++i == argc) { no_param(cp); } else { query = argv[i]; }
      }
      else if(!strcmp(cp, "-i") || !strcmp(cp, "--input-from-files")) {
        input_from_files = true;
      }
      else if(!strcmp(cp, "-r") || !strcmp(cp, "--print-references")) {
        if(++i == argc) { no_param(cp); } else { recursion_depth = atol(argv[i]); }
        int j = 0;
        for(; j < argc - i - 1; ++j) {
          if(argv[i+1+j][0] != '-') { ref_classes.push_back(argv[i+1+j]); } else { break; }
        }
        i += j;
      }
      else if(!strcmp(cp, "-b") || !strcmp(cp, "--print-referenced_by")) {
        print_referenced_by = true;
	if((i+1) < argc && argv[i+1][0] != '-') {
	  ref_by_rel_name = argv[++i];
	}
      }
      else if(!strcmp(cp, "-p") || !strcmp(cp, "--path")) {
        if(++i > argc - 1) { no_param(cp); } else { object_from = argv[i]; path_query = argv[++i];}
      }
      else if(strcmp(cp, "--")) {
        if(input_from_files) {
          std::ifstream f(cp);
  	  if(f.good()) {
	    while(f.good() && !f.eof()) {
	      std::string file_name;
	      std::getline(f, file_name);
	      if(!file_name.empty() && kernel.load_file(file_name) == 0) {
                Oks::error_msg("oks_dump") << "\tCan not load file \"" << file_name << "\", exiting...\n";
                return __BadOksFile__;
              }
	    }
	  }
	  else {
            Oks::error_msg("oks_dump") << "\tCan not open file \"" << cp << "\" for reading, exiting...\n";
            return __BadCommandLine__;
	  }
        }
        else {
          if(kernel.load_file(cp) == 0) {
            Oks::error_msg("oks_dump") << "\tCan not load file \"" << cp << "\", exiting...\n";
            return __BadOksFile__;
          }
        }
      }
    }

    if(kernel.schema_files().empty()) {
      Oks::error_msg("oks_dump") << "\tAt least one oks file have to be provided, exiting...\n";
      return __BadCommandLine__;
    }

    if(query && !class_name) {
      Oks::error_msg("oks_dump") << "\tQuery can only be executed when class name is provided (use -c option), exiting...\n";
      return __BadCommandLine__;
    }

    if(dump_files_only) {
      long total_size = 0;
      long total_items = 0;
      const OksFile::Map * files [2] = {&kernel.schema_files(), &kernel.data_files()};
      for(int j = (dump_files_only / 10); j < (dump_files_only % 10); ++j) {
        if(!files[j]->empty()) {
          for(OksFile::Map::const_iterator i = files[j]->begin(); i != files[j]->end(); ++i) {
            if(print_files_stat) {
              total_size += i->second->get_size();
              total_items += i->second->get_number_of_items();
              std::cout << *(i->first) << " (" << i->second->get_number_of_items() << " items, " << i->second->get_size() << " bytes)" << std::endl;
            }
            else {
              std::cout << *(i->first) << std::endl;
            }
          }
        }
      }

      if(print_files_stat) {
        std::cout << "Total number of items: " << total_items << "\n"
                     "Total size of files: " << total_size << " bytes" << std::endl;
      }
    }
    else if(class_name && *class_name) {
      if(OksClass * c = kernel.find_class(class_name)) {
        if(query && *query) {
          OksQuery * q = new OksQuery(c, query);
  	  if(q->good()) {
	    OksObject::List * objs = c->execute_query(q);
	    size_t num = (objs ? objs->size() : 0);
	    std::cout << "Found " << num << " matching query \"" << query << "\" in class \"" << class_name << "\"";
	    if(num) {
	      std::cout << ':' << std::endl;
	      while(!objs->empty()) {
	        OksObject * o = objs->front();
	        objs->pop_front();
		
	        if(recursion_depth > 0 || print_referenced_by) {
		  if(recursion_depth) {
                    oks::ClassSet all_ref_classes;
                    kernel.get_all_classes(ref_classes, all_ref_classes);
	            OksObject::FSet refs;
		    o->references(refs, recursion_depth, false, &all_ref_classes);
	            std::cout << o << " references " << refs.size() << " object(s)" << std::endl;
		    for(OksObject::FSet::const_iterator i = refs.begin(); i != refs.end(); ++i) {
		      std::cout << " - " << *i << std::endl;
		    }
		  }
		  if(print_referenced_by) {
		    if(OksObject::FList * ref_by_objs = o->get_all_rels(ref_by_rel_name)) {
	              std::cout << o << " is referenced by " << ref_by_objs->size() << " object(s) via relationship \"" << ref_by_rel_name << "\":" << std::endl;

                      for(OksObject::FList::const_iterator i = ref_by_objs->begin(); i != ref_by_objs->end(); ++i) {
                        std::cout << " - " << *i << std::endl;
                      }
		      
		      delete ref_by_objs;
		    }
		    else {
	              std::cout << o << " is not referenced by any object via relationship \"" << ref_by_rel_name << '\"' << std::endl;
		    }
		  }
	        }
	        else {
	          std::cout << *o << std::endl;
	        }
	      }
	      delete objs;
	    }
	    else {
	      std::cout << std::endl;
	    }
	  }
	  else {
            Oks::error_msg("oks_dump") << "\tFailed to parse query \"" << query << "\" in class \"" << class_name << "\"\n";
            return __BadQuery__;
	  }
	  delete q;
        }
        else {
          std::cout << *c << std::endl;
        }
      }
      else {
        Oks::error_msg("oks_dump") << "\tCan not find class \"" << class_name << "\"\n";
        return __NoSuchClass__;
      }
    }
    else if(object_from && *object_from && path_query && *path_query) {
      OksObject * obj_from = find_object((char *)object_from, kernel);
      try {
        oks::QueryPath q(path_query, kernel);
        OksObject::List * objs = obj_from->find_path(q);

        size_t num = (objs ? objs->size() : 0);
        std::cout << "Found " << num << " objects in the path \"" << q << "\" from object " << obj_from << ":" << std::endl;

        if(num) {
          while(!objs->empty()) {
            OksObject * o = objs->front();
            objs->pop_front();
            std::cout << *o << std::endl;
          }
          delete objs;
        }
      }
      catch ( oks::bad_query_syntax& e ) {
        Oks::error_msg("oks_dump") << "\tFailed to parse query: " << e.what() << std::endl;
        return __BadQuery__;
      }
    }
    else {
      std::cout << kernel;
    }

    if(!kernel.get_bind_classes_status().empty())
      {
        Oks::error_msg("oks_dump") << "The schema contains dangling references to non-loaded classes:\n" << kernel.get_bind_classes_status();
      }

    if(!kernel.get_bind_objects_status().empty())
      {
        Oks::error_msg("oks_dump") << "\tThe data contain dangling references to non-loaded objects\n";
      }

    if(!kernel.get_bind_classes_status().empty() || !kernel.get_bind_objects_status().empty())
      {
        return __FoundDanglingReferences__;
      }
  }
  catch (oks::exception & ex) {
    std::cerr << "Caught oks exception:\n" << ex << std::endl;
    return __ExceptionCaught__;
  }
  catch (std::exception & e) {
    std::cerr << "Caught standard C++ exception: " << e.what() << std::endl;
    return __ExceptionCaught__;
  }
  catch (...) {
    std::cerr << "Caught unknown exception" << std::endl;
    return __ExceptionCaught__;
  }

  return __Success__;
}
