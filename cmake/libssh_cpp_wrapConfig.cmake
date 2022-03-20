if (NOT TARGET libssh_cpp_wrap)
    find_package(libssh REQUIRED)

    # dll dependencies
    find_package(OpenSSL REQUIRED COMPONENTS SSL)
    find_package(zlib REQUIRED)

    get_filename_component(LIBSSH_CPP_WRAP_INCLUDE_DIR ${CMAKE_CURRENT_LIST_DIR}/../../include ABSOLUTE)

    add_library(libssh_cpp_wrap INTERFACE)
    target_include_directories(libssh_cpp_wrap INTERFACE
        ${LIBSSH_CPP_WRAP_INCLUDE_DIR}
    )

    target_compile_features(libssh_cpp_wrap INTERFACE cxx_std_20)

    target_link_libraries(libssh_cpp_wrap INTERFACE ssh)

    set(LIBSSH_CPP_WRAP_WIN_PATH "$<TARGET_FILE_DIR:ssh>;$<TARGET_FILE_DIR:OpenSSL::SSL>/../bin;$<TARGET_FILE_DIR:ZLIB::ZLIB>/../bin" CACHE INTERNAL "binary dependencies")
))
endif()