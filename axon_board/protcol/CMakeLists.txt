cmake_minimum_required(VERSION 3.16)

set(SRCS "target.c")

# 仮
if(DEFINED IDF_TARGET)
    idf_component_register(SRCS ${SRCS}
        INCLUDE_DIRS "." "include")
else()
    project(Target LANGUAGES C)
endif()