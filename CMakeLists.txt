cmake_minimum_required(VERSION 3.15)

project(GOatLANG)

set(CMAKE_CXX_STANDARD 20)
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Java COMPONENTS Runtime REQUIRED)
find_package(Threads REQUIRED)

set(ANTLR_LIB_DEFAULT_PATH "/usr/local/lib/libantlr4-runtime.a")

if(NOT ANTLR_LIB)
    if (EXISTS ${ANTLR_LIB_DEFAULT_PATH})
        set(ANTLR_LIB ${ANTLR_LIB_DEFAULT_PATH})
    else()
        set(ANTLR_LIB ${PROJECT_SOURCE_DIR}/libantlr4-runtime.a)
    endif()
endif()

set(GOatLANG_GENERATED_SRC
    ${PROJECT_SOURCE_DIR}/generated/GOatLANGLexer.cpp
    ${PROJECT_SOURCE_DIR}/generated/GOatLANGParser.cpp
    ${PROJECT_SOURCE_DIR}/generated/GOatLANGBaseVisitor.cpp
    ${PROJECT_SOURCE_DIR}/generated/GOatLANGVisitor.cpp
    # ${PROJECT_SOURCE_DIR}/generated/GOatLANGBaseListener.cpp
    # ${PROJECT_SOURCE_DIR}/generated/GOatLANGListener.cpp
)

foreach(src_file ${GOatLANG_GENERATED_SRC})
    set_source_files_properties(
        ${src_file}
        PROPERTIES
        GENERATED TRUE
    )
endforeach(src_file ${GOatLANG_GENERATED_SRC})

add_custom_target(GenerateParser DEPENDS ${GOatLANG_GENERATED_SRC})
add_custom_command(OUTPUT ${GOatLANG_GENERATED_SRC}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/generated/
    COMMAND antlr4 -Werror -Dlanguage=Cpp -visitor -o ${PROJECT_SOURCE_DIR}/generated/ ${PROJECT_SOURCE_DIR}/GOatLANG.g4
    # we may add this option to the command above: -package <name> #
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    DEPENDS ${PROJECT_SOURCE_DIR}/GOatLANG.g4
)

include_directories(
    ${PROJECT_SOURCE_DIR}/generated
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/antlr4-runtime-copy  # Adjusted include path
)

set(GOatLANG_SRC
    ${PROJECT_SOURCE_DIR}/src/Heap.cpp
    ${PROJECT_SOURCE_DIR}/src/Native.cpp
    ${PROJECT_SOURCE_DIR}/src/Thread.cpp
    ${PROJECT_SOURCE_DIR}/src/Runtime.cpp
    ${PROJECT_SOURCE_DIR}/src/main.cpp
    ${GOatLANG_GENERATED_SRC}
)

set(CXX_DEBUG_FLAGS "-g -Wall -Wpedantic -Wextra -Wno-unused-parameter -Wno-missing-field-initializers")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CXX_DEBUG_FLAGS}")

add_executable(GOatLANG ${GOatLANG_SRC})
add_dependencies(GOatLANG GenerateParser)

target_link_libraries(GOatLANG ${ANTLR_LIB})
target_link_libraries(GOatLANG Threads::Threads)
