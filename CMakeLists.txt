cmake_minimum_required(VERSION 3.24)

project(AVLTreeDeletionRebalancing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(SQLite3 REQUIRED)

include(FetchContent)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_ZSTD_IF_AVAILABLE OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_CERTS_FROM_MACOSX_KEYCHAIN OFF CACHE BOOL "" FORCE)
set(HTTPLIB_USE_NON_BLOCKING_GETADDRINFO OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
    httplib
    URL https://github.com/yhirose/cpp-httplib/archive/refs/tags/v0.24.0.tar.gz
    URL_HASH SHA256=bb788d1ccf6654af77b63f4f3f01f9d9a5b549a8773d02eb88b33ef252a9f78e
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(httplib)

include(cmake/AVLTreeSources.cmake)

add_library(avl_tree ${AVL_TREE_SOURCES})

target_include_directories(avl_tree PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(avl_demo main.cpp)
target_link_libraries(avl_demo PRIVATE avl_tree)

add_executable(avl_server
    web_server_main.cpp
    src/Reservation.cpp
    src/ReservationStore.cpp
    src/WebApplication.cpp
)
target_include_directories(avl_server PRIVATE include)
target_link_libraries(avl_server PRIVATE avl_tree SQLite3::SQLite3 httplib::httplib)

set(AVL_STUDIO_ASSET_DIR "${CMAKE_CURRENT_BINARY_DIR}/web-assets")
file(MAKE_DIRECTORY "${AVL_STUDIO_ASSET_DIR}")
configure_file(web/styles.css "${AVL_STUDIO_ASSET_DIR}/styles.css" COPYONLY)
configure_file(web/public/favicon.svg "${AVL_STUDIO_ASSET_DIR}/favicon.svg" COPYONLY)
target_compile_definitions(avl_server PRIVATE
    AVL_STUDIO_ASSET_DIR="${AVL_STUDIO_ASSET_DIR}"
)

include(CTest)
if(BUILD_TESTING)
    add_executable(avl_architecture_tests tests/tests.cpp)
    target_link_libraries(avl_architecture_tests PRIVATE avl_tree)
    add_test(NAME avl_architecture_tests COMMAND avl_architecture_tests)

    add_executable(reservation_tests
        tests/reservation_tests.cpp
        src/Reservation.cpp
        src/ReservationStore.cpp
    )
    target_include_directories(reservation_tests PRIVATE include)
    target_link_libraries(reservation_tests PRIVATE avl_tree SQLite3::SQLite3)
    add_test(NAME reservation_tests COMMAND reservation_tests)
endif()
