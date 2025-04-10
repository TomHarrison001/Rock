cmake_minimum_required(VERSION 3.19)

project(application)

include(../cmake/CommonMacros.txt)

SETUP_APP(application "application")

target_link_libraries(application SharedUtils)
