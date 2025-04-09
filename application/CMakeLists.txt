cmake_minimum_required(VERSION 3.16)

project(application)

include(../cmake/CommonMacros.txt)

SETUP_APP(application "Application")

target_link_libraries(application SharedUtils)
