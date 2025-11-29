cmake_minimum_required(VERSION 3.20)
project(QRticketingSystem)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# -----------------------------
# Set build type
# -----------------------------
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# -----------------------------
# QR Codegen (single-file static library)
# -----------------------------
add_library(qrcodegen STATIC
    "${CMAKE_SOURCE_DIR}/dependencies/src/qrcodegen.cpp"
)

target_include_directories(qrcodegen PUBLIC
    "${CMAKE_SOURCE_DIR}/dependencies/include"
)

# -----------------------------
# Include directories for the executable
# -----------------------------
include_directories(
    "${CMAKE_SOURCE_DIR}/dependencies/include"
    "${CMAKE_SOURCE_DIR}/src"
)

# -----------------------------
# Link directories for static libs
# -----------------------------
link_directories("${CMAKE_SOURCE_DIR}/dependencies/debug/lib")

# -----------------------------
# Source files
# -----------------------------
file(GLOB SOURCES
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
)

# -----------------------------
# Build executable
# -----------------------------
add_executable(QRticketingSystem ${SOURCES})

# -----------------------------
# Link libraries (static and DLL-based)
# -----------------------------
target_link_libraries(QRticketingSystem
    qrcodegen
    hpdf
    drogon
    trantor
    libssl
    libcrypto
    zlibd
    jsoncpp
    cares
    brotlienc
    brotlidec
    brotlicommon
    libpq         # PostgreSQL client library
)

# -----------------------------
# Copy required DLLs (for drogon and other DLL-based libs)
# -----------------------------
add_custom_command(TARGET QRticketingSystem POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/drogon.dll"
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/trantor.dll"
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/libssl-3-x64.dll"
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/libcrypto-3-x64.dll"
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/zlibd1.dll"
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/jsoncpp.dll"
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/cares.dll"
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/brotlienc.dll"
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/brotlidec.dll"
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/brotlicommon.dll"
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/hpdf.dll"
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/libpq.dll"      # PostgreSQL DLL
        "${CMAKE_SOURCE_DIR}/dependencies/debug/bin/libpng16d.dll"
        $<TARGET_FILE_DIR:QRticketingSystem>
)
