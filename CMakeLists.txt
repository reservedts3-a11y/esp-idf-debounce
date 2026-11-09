cmake_minimum_required(VERSION 3.16.0)
# Fixed version: otherwise ESP-IDF reads it from git and fails without commits
set(PROJECT_VER "1.0")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp-idf-debounce)
