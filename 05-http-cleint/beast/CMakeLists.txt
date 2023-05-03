cmake_minimum_required(VERSION 3.11)

project(http-client)

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

add_executable(${PROJECT_NAME} main.cpp)
if(WIN32)
target_link_libraries(${PROJECT_NAME}  PRIVATE CRYPT32.LIB)
endif()
target_link_libraries(${PROJECT_NAME}  PRIVATE ${Boost_LIBRARIES}  ${OPENSSL_LIBRARIES})