cmake_minimum_required(VERSION 3.15)
project(operating_system_course C)

set(CMAKE_C_STANDARD 11)

add_executable(operating_system_course main.c jiffies/jiffies.c)
include_directories(
        "/usr/src/linux-headers-4.15.0-66/include" ,
        "/usr/src/linux-headers-4.15.0-66/arch/x86/include" ,
        "/usr/src/linux-headers-4.15.0-66-generic/include" ,
        "/usr/src/linux-headers-4.15.0-66-generic/arch/x86/include")