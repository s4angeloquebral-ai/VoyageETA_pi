cmake_minimum_required(VERSION 3.15)
project(voyage_eta_pi)

set(PACKAGE_NAME voyage_eta)
set(PLUGIN_NAME voyage_eta)
set(PLUGIN_VERSION_MAJOR 1)
set(PLUGIN_VERSION_MINOR 0)
set(PLUGIN_VERSION_PATCH 0)
set(PLUGIN_VERSION_DATE "2026-07-11")
set(OCPN_API_VERSION "1.18")
set(PACKAGE_CONTACT "Captain Voyage ETA")
set(PACKAGE_DESCRIPTION "Live route ETA using OpenCPN own-ship SOG")
set(PACKAGE_TITLE "Voyage ETA")
set(PACKAGE_SUMMARY "Live ETA to any waypoint on an OpenCPN route")

# This project is intended to be overlaid on the official OpenCPN testplugin
# template. If Plugin.cmake is available, use the normal template build.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Plugin.cmake")
  include(cmake/Plugin.cmake)
  add_subdirectory(src)
else()
  message(FATAL_ERROR
    "OpenCPN plugin template files are missing. Run scripts/prepare-template.ps1 "
    "or build using the included GitHub Actions workflow.")
endif()
