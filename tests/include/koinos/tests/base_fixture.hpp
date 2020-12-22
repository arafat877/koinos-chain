#pragma once

#include <boost/filesystem.hpp>

#include <koinos/log.hpp>

struct base_fixture
{
   base_fixture()
   {
      auto temp = boost::filesystem::temp_directory_path() / boost::filesystem::unique_path();
      boost::filesystem::create_directory( temp );
      koinos::initialize_logging( temp, "koinos_tests_%3N.log" );
   }

   ~base_fixture()
   {
      boost::log::core::get()->remove_all_sinks();
   }
};
