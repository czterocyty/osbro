cmake_minimum_required(VERSION 3.16)
project(osbro C)

set(CMAKE_C_STANDARD 99)

find_package(PkgConfig)
pkg_check_modules(libusb-1.0 REQUIRED libusb-1.0)

add_executable(osbro main.c)
add_executable(raw2pnm raw2pnm.c)

target_link_libraries(osbro ${libusb-1.0_LIBRARIES})
