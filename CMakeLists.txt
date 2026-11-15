cmake_minimum_required(VERSION 3.16)

project(AVLTreeDeletionRebalancing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(avl_tree
    src/AVLTree.cpp
    src/AVLBalance.cpp
    src/AVLInsert.cpp
    src/AVLDelete.cpp
    src/AVLTraversal.cpp
)

target_include_directories(avl_tree PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(avl_demo main.cpp)
target_link_libraries(avl_demo PRIVATE avl_tree)

include(CTest)
if(BUILD_TESTING)
    add_executable(avl_architecture_tests tests/tests.cpp)
    target_link_libraries(avl_architecture_tests PRIVATE avl_tree)
    add_test(NAME avl_architecture_tests COMMAND avl_architecture_tests)
endif()
