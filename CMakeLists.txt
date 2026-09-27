# Please see README.md for build instructions

cmake_minimum_required(VERSION 3.16.4)

set(CMAKE_SYSTEM_NAME Generic)

set(CMAKE_SYSTEM_PROCESSOR arm)

# Use add_library() with the STATIC option to name the source file in the generated project.
# This avoids running the linker and is intended for use with cross-compiling toolchains that
# cannot link without custom flags or linker scripts
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_CONFIGURATION_TYPES Debug Release RelWithDebInfo MinSizeRel)

if(BUILD_KIT)
	message(NOTICE "Build Kit is defined as ${BUILD_KIT}")
endif()

message(NOTICE "CMAKE_CURRENT_BINARY_DIR is defined as ${CMAKE_CURRENT_BINARY_DIR}")

get_property(isMultiConfig GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

if(isMultiConfig)
    message(NOTICE "Multi-config generator(${CMAKE_GENERATOR}) is active")
else()
    message(NOTICE "Single-config generator(${CMAKE_GENERATOR}) is active")
	if(DEFINED CMAKE_BUILD_TYPE)
	 	message(NOTICE "CMAKE_BUILD_TYPE is defined as ${CMAKE_BUILD_TYPE}")
	elseif(DEFINED BUILD_TYPE)
		set(CMAKE_BUILD_TYPE ${BUILD_TYPE})
		message(NOTICE "CMAKE_BUILD_TYPE is set to BUILD_TYPE ENV ${CMAKE_BUILD_TYPE}")
	else()
		message(FATAL_ERROR "Please specify the CMAKE_BUILD_TYPE with -DCMAKE_BUILD_TYPE=")
	endif()
    #set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE})
	message(NOTICE "CMAKE_RUNTIME_OUTPUT_DIRECTORY is defined as ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

cmake_minimum_required(VERSION 3.16.4)

# Project configuration
project(console_rtt
	VERSION 1.0.0
	LANGUAGES C CXX ASM)

set(CMAKE_CROSSCOMPILING "TRUE")
set(CMAKE_EXPORT_COMPILE_COMMANDS "TRUE")

include(${CMAKE_CURRENT_LIST_DIR}/Config.cmake)

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GeneratedCfg.cmake)

# If required user can amend RASC generated flags here, e.g. list(REMOVE_ITEM RASC_CMAKE_C_FLAGS "-Wall")

include(${CMAKE_CURRENT_LIST_DIR}/cmake/GeneratedSrc.cmake)

# ─────────────────────────────────────────────────────────────────────────────
# CONSOLE_RTT integration  (added on top of the RASC-generated project)
# ─────────────────────────────────────────────────────────────────────────────
# GeneratedSrc.cmake above defines the executable target ${PROJECT_NAME}.elf
# and recursively globs ra/, ra_gen/ and src/, so the CONSOLE_RTT sources are
# compiled DIRECTLY into that ELF (not as a static archive) with no extra
# target_sources() needed:
#   src/app/            application use-case (console_rtt_app.c/.h)
#   src/rs_console_rtt/ CONSOLE_RTT PES: portable RTT logic + newlib syscalls
#   src/segger_rtt/     official SEGGER RTT V7.96e engine
# Their include directories are listed in GeneratedSrc.cmake as well.
#
# Constraints honoured here:
#   - No UART, no FreeRTOS, no Picolibc, no logger.
#   - SEGGER_RTT_printf.c and SEGGER_RTT_Syscalls_GCC.c are deliberately NOT
#     part of src/segger_rtt/: rs_console_rtt_syscalls.c is the sole newlib
#     syscall provider (_write/_read/_close/_fstat/_isatty/_lseek).
#   - ra_gen/main.c owns main(); src/hal_entry.c calls console_rtt_app_run().

# newlib syscall stubs: nano.specs (from RASC) provides the reduced libc; add
# nosys.specs so the syscalls we do NOT implement (_getpid/_kill/_exit ...)
# resolve to weak stubs. The strong versions in rs_console_rtt_syscalls.c are
# compiled directly into the ELF and therefore win over the nosys weak ones —
# no multiple-definition conflict, no "-u" needed.
target_link_options(${PROJECT_NAME}.elf PRIVATE --specs=nosys.specs)
