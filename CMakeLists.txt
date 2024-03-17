cmake_minimum_required(VERSION 3.15)
project(GOatLANG)

set(CMAKE_CXX_STANDARD 17)

set(GOatLANG-GENERATED_SRC
    ${PROJECT_SOURCE_DIR}/generated/GOatLANGLexer.cpp
    ${PROJECT_SOURCE_DIR}/generated/GOatLANGParser.cpp
    ${PROJECT_SOURCE_DIR}/generated/GOatLANGParserBaseListener.cpp
    ${PROJECT_SOURCE_DIR}/generated/GOatLANGParserBaseVisitor.cpp
    ${PROJECT_SOURCE_DIR}/generated/GOatLANGParserListener.cpp
    ${PROJECT_SOURCE_DIR}/generated/GOatLANGParserVisitor.cpp
)

foreach(src_file ${GOatLANG-GENERATED_SRC})
    set_source_files_properties(
        ${src_file}
        PROPERTIES
        GENERATED TRUE
    )
endforeach(src_file ${GOatLANG-GENERATED_SRC})

add_custom_target(GenerateParser DEPENDS ${GOatLANG-GENERATED_SRC})
add_custom_command(OUTPUT ${GOatLANG-GENERATED_SRC}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_SOURCE_DIR}/generated/
    COMMAND antlr4 -Werror -Dlanguage=Cpp -listener -visitor -o ${PROJECT_SOURCE_DIR}/generated/ ${PROJECT_SOURCE_DIR}/GOatLANG.g4
    # we may add this option to the command above: -package <name> #
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    DEPENDS ${PROJECT_SOURCE_DIR}/GOatLANG.g4
)

include_directories(
    ${PROJECT_SOURCE_DIR}/generated
)
