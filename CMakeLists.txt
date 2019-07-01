cmake_minimum_required(VERSION 3.7)
project(binobject)

set(CMAKE_CXX_STANDARD 11)

add_subdirectory(deps/libmffcodec)

add_library(binobject SHARED src/custom-type.cpp src/node-encoder.cc src/node-decoder.cc src/node-binobject.cc)
target_compile_options(binobject PRIVATE -fPIC)
if(CMAKE_JS_VERSION)
    include_directories(${CMAKE_JS_INC})
    set_target_properties(binobject PROPERTIES PREFIX "" SUFFIX ".node")
    target_link_libraries(binobject PRIVATE ${CMAKE_JS_LIB} mffcodec)
else()
    target_include_directories(binobject PRIVATE node_modules/nan $ENV{NODE_PATH}/include/node)
    target_link_libraries(binobject PRIVATE mffcodec)
endif()
