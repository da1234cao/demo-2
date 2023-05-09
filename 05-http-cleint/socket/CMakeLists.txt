cmake_minimum_required(VERSION 3.11)

project(http-client)

include(FetchContent)

# http-parser
# FetchContent_Declare(
#   http-parser
#   GIT_REPOSITORY git@github.com:nodejs/http-parser.git
#   GIT_TAG main)
# FetchContent_MakeAvailable(http-parser)
# include_directories(${http-parser_SOURCE_DIR})
# message(STATUS "fetch http-parser")

# boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED 
    COMPONENTS)

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found")
endif()
include_directories((${Boost_INCLUDE_DIRS}))

# openssl
set(OPENSSL_USE_STATIC_LIBS true)
find_package(OpenSSL REQUIRED)
include_directories(${OPENSSL_INCLUDE_DIR})
link_directories(${OPENSSL_LIBRARIES})

include_directories(${CMAKE_SOURCE_DIR})
set(SRC ${CMAKE_SOURCE_DIR}/main.c 
        ${CMAKE_SOURCE_DIR}/3rd/http_parser.c 
        ${CMAKE_SOURCE_DIR}/utils.c
        ${CMAKE_SOURCE_DIR}/net.c)

add_executable(${PROJECT_NAME} ${SRC})
if(WIN32)
target_link_libraries(${PROJECT_NAME}  PRIVATE CRYPT32.LIB)
endif()
target_link_libraries(${PROJECT_NAME}  PRIVATE ${Boost_LIBRARIES}  ${OPENSSL_LIBRARIES})