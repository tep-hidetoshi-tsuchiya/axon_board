cmake_minimum_required(VERSION 3.16)

set(SRCS "src/target.c"
         "src/s2a_packet.c")

# 仮
if(DEFINED IDF_TARGET)
    idf_component_register(SRCS ${SRCS}
        INCLUDE_DIRS "." "src")
else()
    project(Target LANGUAGES C)
endif()