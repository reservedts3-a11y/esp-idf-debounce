# All .c files in src/ are compiled; only the task selected in config.h
# actually contains code (the others are switched off with #if)
FILE(GLOB_RECURSE app_sources ${CMAKE_SOURCE_DIR}/src/*.c)

idf_component_register(SRCS ${app_sources}
                       INCLUDE_DIRS ".")
