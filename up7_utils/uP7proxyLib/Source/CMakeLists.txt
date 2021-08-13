cmake_minimum_required(VERSION 3.0)

project(uP7proxy)

include_directories(${PATH_UP7_API})
include_directories(${PATH_UP7_LIB})
include_directories(${PATH_SHARED})
include_directories(${PATH_SHARED_PLATFORM})
include_directories(${PATH_P7_API})
include_directories(${PATH_UP7_PROXY_API})
include_directories(${PATH_UP7_PLATFORM})

set(PATH_UP7_PROXY_LIB ${CMAKE_CURRENT_SOURCE_DIR} CACHE INTERNAL "")

add_library(uP7proxy STATIC 
            "${PATH_UP7_API}/uP7.h"
            "${PATH_UP7_API}/uP7helpers.h"
            "${PATH_UP7_API}/uP7protocol.h"
            "${PATH_UP7_PLATFORM}/uP7platform.h"
            "${PATH_UP7_PROXY_API}/uP7proxyApi.h"
            uP7common.h
            uP7proxyPreprocessor.h
            uP7proxyClient.h
            uP7proxyLinearPool.h
            uP7chunks.h
            uP7Fifo.cpp                 uP7Fifo.h
            uP7proxy.cpp                uP7proxy.h
            uP7proxyCpu.cpp             uP7proxyCpu.h 
            uP7proxyStream.cpp          uP7proxyStream.h
            uP7proxyTrace.cpp           uP7proxyTrace.h
            uP7proxyTelemetry.cpp       uP7proxyTelemetry.h
           )

           
set_property(TARGET uP7proxy PROPERTY POSITION_INDEPENDENT_CODE ON) 

set_ide_folder(${PROJECT_NAME})




