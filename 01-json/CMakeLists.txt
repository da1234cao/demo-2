cmake_minimum_required(VERSION 3.11)

project(json_test)

find_package( 
 Boost REQUIRED 
 COMPONENTS  json 
)

add_execublable(anyExecutable main.cpp )
target_link_libraries( exeLINK_PUBLIC ${Boost_LIBRARIES})