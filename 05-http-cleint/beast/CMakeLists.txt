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

add_executable(${PROJECT_NAME} main.cpp)
target_link_libraries(${PROJECT_NAME}  PRIVATE ${Boost_LIBRARIES})