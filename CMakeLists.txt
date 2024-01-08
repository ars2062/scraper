cmake_minimum_required(VERSION 3.26.4)
include_directories(/home/arshia/tools/vcpkg/installed/x64-linux/include)
link_directories(/home/arshia/tools/vcpkg/installed/x64-linux/lib)

project(scraper)

add_executable(${PROJECT_NAME} main.cpp)
set(BUILD_M)
set(CMAKE_PREFIX_PATH "/home/arshia/tools/vcpkg/installed/x64-linux/share")
find_package(LibXml2 REQUIRED)
target_link_libraries(scraper PRIVATE LibXml2::LibXml2)

set(CMAKE_PREFIX_PATH "/home/arshia/tools/vcpkg/installed/x64-linux/share")
find_package(cpr CONFIG REQUIRED)
target_link_libraries(scraper PRIVATE cpr::cpr)