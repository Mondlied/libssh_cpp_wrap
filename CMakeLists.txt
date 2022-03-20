# libssh_cpp_wrap (A C++ wrapper for libssh)
# 
# Copyright(C) 2022 Fabian Klein
#
# This library is free software; you can redistribute itand /or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110 - 1301
# USA

cmake_minimum_required(VERSION 3.21)

project(LibSshCppWrap)

function(set_if_undefined VAR VALUE)
    if (NOT DEFINED ${VAR} AND NOT DEFINED CACHE{${VAR}})
        set(${VAR} ${VALUE} PARENT_SCOPE)
    endif()
endfunction()

set_if_undefined(CMAKE_INSTALL_INCLUDEDIR include)
set_if_undefined(CMAKE_INSTALL_BINDIR bin)
set_if_undefined(CMAKE_INSTALL_LIBDIR lib)

find_package(libssh REQUIRED)

# dll dependencies
find_package(OpenSSL REQUIRED COMPONENTS SSL)
find_package(zlib REQUIRED)

#add_library(libssh_cpp_wrap STATIC
add_library(libssh_cpp_wrap INTERFACE)

# add sources to linking target for autocompletion
target_sources(libssh_cpp_wrap PUBLIC
    include/libssh_cpp_wrap/connection.hpp
    include/libssh_cpp_wrap/command_execution_channel.hpp
    include/libssh_cpp_wrap/ip.hpp
    include/libssh_cpp_wrap/session_options.hpp
    include/libssh_cpp_wrap/session.hpp
    include/libssh_cpp_wrap/sftp_channel.hpp
)

target_include_directories(libssh_cpp_wrap INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_compile_features(libssh_cpp_wrap INTERFACE cxx_std_20)

target_link_libraries(libssh_cpp_wrap INTERFACE ssh)

add_executable(Example ssh.cpp)
target_link_libraries(Example PRIVATE libssh_cpp_wrap)
set_target_properties(Example PROPERTIES VS_DEBUGGER_ENVIRONMENT
    # note $<TARGET_FILE_DIR:OpenSSL::SSL> and $<TARGET_FILE_DIR:ZLIB::ZLIB> point to the lib directories
    "PATH=$<TARGET_FILE_DIR:ssh>;$<TARGET_FILE_DIR:OpenSSL::SSL>/../bin;$<TARGET_FILE_DIR:ZLIB::ZLIB>/../bin;$ENV{PATH}"
)

set(LIBSSH_CPP_WRAP_NOINSTALL True CACHE BOOL "don't add installation logic to libssh_cpp_wrap")

if (NOT LIBSSH_CPP_WRAP_NOINSTALL)

    install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILE_PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )

    install(FILES cmake/libssh_cpp_wrapConfig.cmake
        DESTINATION lib/libssh_cpp_wrap
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ
    )

    install(IMPORTED_RUNTIME_ARTIFACTS ssh
            LIBRARY DESTINATION ${CMAKE_INSTALL_BINDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_LIBDIR}
    )
endif()
