cmake_minimum_required(VERSION 3.24)

# Prefer the in-tree vcpkg clone unless the user provided their own.
set(VCPKG_ROOT_HINT "${CMAKE_CURRENT_LIST_DIR}/vcpkg")
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
    if(DEFINED ENV{VCPKG_ROOT} AND EXISTS "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "vcpkg toolchain" FORCE)
        message(STATUS "Using vcpkg toolchain from $ENV{VCPKG_ROOT}")
    elseif(EXISTS "${VCPKG_ROOT_HINT}/scripts/buildsystems/vcpkg.cmake")
        set(CMAKE_TOOLCHAIN_FILE "${VCPKG_ROOT_HINT}/scripts/buildsystems/vcpkg.cmake" CACHE FILEPATH "vcpkg toolchain" FORCE)
        message(STATUS "Using vcpkg toolchain from ${VCPKG_ROOT_HINT}")
    endif()
endif()

project(ecl-protobuf VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(protobuf CONFIG REQUIRED)

option(ECL_PROTOBUF_ENABLE_WARNINGS "Enable recommended compiler warnings" ON)
option(ECL_PROTOBUF_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(ECL_PROTOBUF_BUILD_TESTS "Enable tests in the tests directory" ON)
option(ECL_PROTOBUF_BUILD_EXAMPLES "Enable example executables in the examples directory" OFF)

set(ECL_PROTOBUF_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(ECL_PROTOBUF_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(ECL_PROTOBUF_GENERATED_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/generated")
set(ECL_PROTOBUF_PROTO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/proto")
set(ECL_PROTOBUF_PROTO_FILE "${ECL_PROTOBUF_PROTO_DIR}/addressbook.proto")
set(ECL_PROTOBUF_APP_SOURCE "${ECL_PROTOBUF_SRC_DIR}/main.cpp")

set(ECL_PROTOBUF_SOURCES)
if(EXISTS "${ECL_PROTOBUF_SRC_DIR}")
    file(GLOB_RECURSE ECL_PROTOBUF_SOURCES CONFIGURE_DEPENDS
        "${ECL_PROTOBUF_SRC_DIR}/*.c"
        "${ECL_PROTOBUF_SRC_DIR}/*.cc"
        "${ECL_PROTOBUF_SRC_DIR}/*.cxx"
        "${ECL_PROTOBUF_SRC_DIR}/*.cpp"
    )
endif()

if(EXISTS "${ECL_PROTOBUF_APP_SOURCE}")
    list(REMOVE_ITEM ECL_PROTOBUF_SOURCES "${ECL_PROTOBUF_APP_SOURCE}")
endif()

if(EXISTS "${ECL_PROTOBUF_PROTO_FILE}")
    set(ECL_PROTOBUF_PROTOC_EXECUTABLE "")
    if(DEFINED VCPKG_INSTALLED_DIR AND DEFINED VCPKG_TARGET_TRIPLET)
        set(_vcpkg_protoc "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/protobuf/protoc")
        if(EXISTS "${_vcpkg_protoc}")
            set(ECL_PROTOBUF_PROTOC_EXECUTABLE "${_vcpkg_protoc}")
        endif()
    endif()
    if(NOT ECL_PROTOBUF_PROTOC_EXECUTABLE)
        set(_local_protoc "${CMAKE_CURRENT_LIST_DIR}/vcpkg_installed/x64-linux/tools/protobuf/protoc")
        if(EXISTS "${_local_protoc}")
            set(ECL_PROTOBUF_PROTOC_EXECUTABLE "${_local_protoc}")
        endif()
    endif()
    if(NOT ECL_PROTOBUF_PROTOC_EXECUTABLE)
        find_program(ECL_PROTOBUF_PROTOC_EXECUTABLE NAMES protoc DOC "Protobuf compiler")
    endif()
    if(NOT ECL_PROTOBUF_PROTOC_EXECUTABLE)
        message(FATAL_ERROR "Could not locate protoc. Run ./vcpkg/vcpkg install to provision protobuf.")
    endif()

    set(ECL_PROTOBUF_GENERATED_CC "${ECL_PROTOBUF_GENERATED_DIR}/addressbook.pb.cc")
    set(ECL_PROTOBUF_GENERATED_H "${ECL_PROTOBUF_GENERATED_DIR}/addressbook.pb.h")
    list(REMOVE_ITEM ECL_PROTOBUF_SOURCES "${ECL_PROTOBUF_GENERATED_CC}")

    add_custom_command(
        OUTPUT "${ECL_PROTOBUF_GENERATED_CC}" "${ECL_PROTOBUF_GENERATED_H}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${ECL_PROTOBUF_GENERATED_DIR}"
        COMMAND "${ECL_PROTOBUF_PROTOC_EXECUTABLE}"
                --proto_path "${ECL_PROTOBUF_PROTO_DIR}"
                --cpp_out "${ECL_PROTOBUF_GENERATED_DIR}"
                "${ECL_PROTOBUF_PROTO_FILE}"
        DEPENDS "${ECL_PROTOBUF_PROTO_FILE}"
        COMMENT "Generating C++ sources from ${ECL_PROTOBUF_PROTO_FILE}"
        VERBATIM
    )

    list(APPEND ECL_PROTOBUF_SOURCES "${ECL_PROTOBUF_GENERATED_CC}")
endif()

if(ECL_PROTOBUF_SOURCES)
    add_library(ecl_protobuf ${ECL_PROTOBUF_SOURCES})
else()
    message(STATUS "No implementation sources detected. Creating an interface target for now.")
    add_library(ecl_protobuf INTERFACE)
endif()

add_library(ecl::protobuf ALIAS ecl_protobuf)

if(ECL_PROTOBUF_SOURCES)
    target_link_libraries(ecl_protobuf PUBLIC protobuf::libprotobuf)
else()
    target_link_libraries(ecl_protobuf INTERFACE protobuf::libprotobuf)
endif()

target_compile_features(ecl_protobuf PUBLIC cxx_std_20)

set(_ecl_protobuf_public_includes)
if(EXISTS "${ECL_PROTOBUF_INCLUDE_DIR}")
    list(APPEND _ecl_protobuf_public_includes "$<BUILD_INTERFACE:${ECL_PROTOBUF_INCLUDE_DIR}>")
endif()
if(ECL_PROTOBUF_GENERATED_CC)
    list(APPEND _ecl_protobuf_public_includes "$<BUILD_INTERFACE:${ECL_PROTOBUF_GENERATED_DIR}>")
endif()
if(_ecl_protobuf_public_includes)
    list(APPEND _ecl_protobuf_public_includes "$<INSTALL_INTERFACE:include>")
    target_include_directories(ecl_protobuf PUBLIC ${_ecl_protobuf_public_includes})
endif()

if(EXISTS "${ECL_PROTOBUF_APP_SOURCE}")
    add_executable(ecl_protobuf_demo "${ECL_PROTOBUF_APP_SOURCE}")
    target_link_libraries(ecl_protobuf_demo PRIVATE ecl::protobuf)
    set_target_properties(ecl_protobuf_demo PROPERTIES OUTPUT_NAME addressbook_demo)
endif()

if(NOT BUILD_SHARED_LIBS)
    set_property(TARGET ecl_protobuf PROPERTY POSITION_INDEPENDENT_CODE ON)
endif()

if(ECL_PROTOBUF_ENABLE_WARNINGS)
    if(MSVC)
        target_compile_options(ecl_protobuf PRIVATE /W4 $<$<BOOL:${ECL_PROTOBUF_WARNINGS_AS_ERRORS}>:/WX>)
    else()
        target_compile_options(ecl_protobuf PRIVATE -Wall -Wextra -Wpedantic $<$<BOOL:${ECL_PROTOBUF_WARNINGS_AS_ERRORS}>:-Werror>)
    endif()
endif()

include(GNUInstallDirs)
install(TARGETS ecl_protobuf EXPORT ecl-protobuf-targets)
if(EXISTS "${ECL_PROTOBUF_INCLUDE_DIR}")
    install(DIRECTORY "${ECL_PROTOBUF_INCLUDE_DIR}/" DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
install(EXPORT ecl-protobuf-targets
    FILE ecl-protobufTargets.cmake
    NAMESPACE ecl::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ecl-protobuf
)

include(CTest)
if(BUILD_TESTING AND ECL_PROTOBUF_BUILD_TESTS AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_subdirectory(tests)
endif()

if(ECL_PROTOBUF_BUILD_EXAMPLES AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/examples")
    add_subdirectory(examples)
endif()
