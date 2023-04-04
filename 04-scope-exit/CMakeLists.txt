cmake_minimum_required(VERSION 3.11)

project(scope-exit)

# boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED 
    COMPONENTS)

if(NOT Boost_FOUND)
  message(FATAL_ERROR "Boost not found")
endif()
include_directories((${Boost_INCLUDE_DIRS}))

add_executable(boost-scope-exit boost-scope-exit.cpp)
target_link_libraries(boost-scope-exit PRIVATE ${Boost_LIBRARIES})


add_executable(lambda-scope-exit lambda-scope-exit.cpp)
target_link_libraries(lambda-scope-exit PRIVATE ${Boost_LIBRARIES})

add_executable(boost-scope-exit-deep boost-scope-exit-deep.cpp)
target_link_libraries(boost-scope-exit-deep PRIVATE ${Boost_LIBRARIES})