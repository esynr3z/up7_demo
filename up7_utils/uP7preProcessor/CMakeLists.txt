cmake_minimum_required(VERSION 3.0)

project(uP7preProcessor)

include_directories("${PATH_UP7_API}")
include_directories("${PATH_UP7_PLATFORM}")
include_directories("${PATH_UP7_LIB}")
include_directories("${PATH_SHARED}")
include_directories("${PATH_SHARED_PLATFORM}")
include_directories("${PATH_P7_API}")
include_directories("${PATH_UP7_PROXY_API}")

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/cfg")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}")

add_executable(${PROJECT_NAME} 
               "${CMAKE_CURRENT_SOURCE_DIR}/cfg/pugixml.cpp"
               "${CMAKE_CURRENT_SOURCE_DIR}/cfg/pugixml.hpp"
               "${CMAKE_CURRENT_SOURCE_DIR}/cfg/BNode.cpp"
               "${CMAKE_CURRENT_SOURCE_DIR}/cfg/BNode.h"
               "${CMAKE_CURRENT_SOURCE_DIR}/cfg/pugiconfig.hpp"
               "${CMAKE_CURRENT_SOURCE_DIR}/cfg/INode.h"
               uP7preCommon.h
               uP7preTools.h
               uP7preDefines.h
               uP7preProcessor.cpp
               uP7preConfig.h
               uP7sessionFile.cpp
               uP7sessionFile.h
               "${PATH_UP7_API}/uP7.h"
               "${PATH_UP7_LIB}/uP7hash.h"
               "${PATH_UP7_PROXY_API}/uP7proxyApi.h"
               uP7preFile.cpp               uP7preFile.h
               uP7preManager.cpp            uP7preManager.h
               uP7preFunction.cpp           uP7preFunction.h
              )

target_link_libraries (${PROJECT_NAME} uP7 p7)

set_ide_folder(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME} DESTINATION bin)