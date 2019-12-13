cmake_minimum_required(VERSION 3.14)
project(operating_system_course C)

set(CMAKE_C_STANDARD 11)

add_executable(operating_system_course chapter2/1_jiffies/jiffies.c chapter4/threads/main.c chapter5/simple_shell/shell.c)
include_directories(
        "/usr/src/linux-headers-5.0.0-32/include" ,
        "/usr/src/linux-headers-5.0.0-32/arch/x86/include" ,
        "/usr/src/linux-headers-5.0.0-32-generic/include" ,
        "/usr/src/linux-headers-5.0.0-32-generic/arch/x86/include")