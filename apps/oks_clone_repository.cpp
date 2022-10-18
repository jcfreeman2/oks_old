/**
 *  \file oks_clone_repository.cpp
 *
 *  This file is part of the OKS package.
 *  Author: <Igor.Soloviev@cern.ch>
 */

#include "oks/kernel.hpp"

#include <string.h>
#include <stdlib.h>

#include <boost/program_options.hpp>

int
main(int argc, char **argv)
{
  std::string config_version, user_dir, branch_name;
  bool verbose = false;

  try
    {
      boost::program_options::options_description desc("Clone and checkout oks repository.\n\n"
          "By default the master branch is checkout. The \"branch\" command line option can be used to specify particular one. A branch will be created, if does not exist yet.\n"
          "TDAQ_DB_VERSION process environment variable or \"version\" command line option can be used to specify particular version.\n"
          "The output directory can be specified via command line, otherwise temporal directory will be created and its name will be reported.\n"
          "If TDAQ_DB_USER_REPOSITORY process environment variable is set, the utility makes no effect.\n\n"
          "The command line options are");

      std::vector<std::string> app_types_list;
      std::vector<std::string> segments_list;

      desc.add_options()
        ("branch,b", boost::program_options::value<std::string>(&branch_name), "checkout or create given branch name")
        ("version,e", boost::program_options::value<std::string>(&config_version), "oks config version in type:value format, where type is \"hash\", \"date\" or \"tag\"")
        ("output-directory,o", boost::program_options::value<std::string>(&user_dir), "output directory; if not defined, create temporal")
        ("verbose,v", "print verbose information")
        ("help,h", "print help message");

      boost::program_options::variables_map vm;
      boost::program_options::store(boost::program_options::parse_command_line(argc, argv, desc), vm);

      if (vm.count("help"))
        {
          std::cout << desc << std::endl;
          return EXIT_SUCCESS;
        }

      if (vm.count("verbose"))
        verbose = true;

      boost::program_options::notify(vm);
    }
  catch (std::exception& ex)
    {
      std::cerr << "Command line parsing errors occurred:\n" << ex.what() << std::endl;
      return EXIT_FAILURE;
    }

  if (!getenv("TDAQ_DB_USER_REPOSITORY"))
    {
      if(!user_dir.empty())
        setenv("TDAQ_DB_USER_REPOSITORY_PATH", user_dir.c_str(), 1);

      OksKernel k(user_dir.empty() && !verbose, verbose, false, true, config_version.empty() ? nullptr : config_version.c_str(), branch_name);

      if (!k.get_user_repository_root().empty())
        {
          k.unset_repository_created();

          if (user_dir.empty())
            std::cout << k.get_user_repository_root() << std::endl;
        }
    }

  return EXIT_SUCCESS;
}
