cmake_minimum_required(VERSION 3.12)
project(oks VERSION 0.0.0)

find_package(daq-cmake REQUIRED)

daq_setup_environment()

find_package(Boost COMPONENTS thread date_time regex REQUIRED)
find_package(ers REQUIRED)
find_package(logging REQUIRED)
find_package(system REQUIRED)

daq_add_library( *.cpp LINK_LIBRARIES ers::ers Boost::thread Boost::date_time Boost::regex stdc++fs system::system logging::logging)

# tdaq_add_library(oks 
#                  src/*.cpp
#                  INCLUDE_DIRECTORIES config
#                  LINK_LIBRARIES daq_tokens osw tdaq-common::ers Boost::thread Boost::date_time Boost::regex stdc++fs PRIVATE pam
#                  DEFINITIONS PRIVATE OKS_VERSION=\"${TDAQ_PACKAGE_VERSION}\")

# tdaq_add_executable(oks_dump bin/oks_dump.cpp LINK_LIBRARIES oks)
# tdaq_add_executable(oks_git_repository bin/oks_git_repository.cpp LINK_LIBRARIES oks)
# tdaq_add_executable(oks_clone_repository bin/oks_clone_repository.cpp LINK_LIBRARIES oks Boost::program_options)
# tdaq_add_executable(oks_validate_repository bin/oks_validate_repository.cpp LINK_LIBRARIES AccessManager oks Boost::program_options)

# tdaq_add_scripts(scripts/*)

# tdaq_add_jar(joks jsrc/oks/*.java OUTPUT_NAME oks)

# tdaq_add_jar_to_repo(oks.jar DESCRIPTION "implements CommitError class to parse oks-commit log")

daq_install()
