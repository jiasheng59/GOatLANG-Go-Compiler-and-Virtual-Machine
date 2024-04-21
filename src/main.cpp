#include <iostream>
#include <fstream>

#include "antlr4-runtime.h"
#include "GOatLANGLexer.h"
#include "GOatLANGParser.h"
#include "Compiler.hpp"
#include "Runtime.hpp"

int main(int argc, const char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <input_file>" << std::endl;
        return 1;
    }
    std::ifstream fs{argv[1]};
    antlr4::ANTLRInputStream input{fs};
    GOatLANGLexer lexer{&input};
    antlr4::CommonTokenStream tokens{&lexer};
    tokens.fill();
    for (auto token : tokens.getTokens()) {
        std::cout << token->toString() << std::endl;
    }
    GOatLANGParser parser{&tokens};
    auto tree = parser.sourceFile();
    std::cout << tree->toStringTree(&parser, true) << std::endl;

    Compiler compiler{};
    compiler.visitSourceFile(tree);
    Configuration configuration = Runtime::default_configuration();
    configuration.main_function_index = compiler.function_indices.at("main");
    configuration.channel_type = compiler.type_names.at("chan");
    Runtime runtime{
        configuration,
        std::move(compiler.function_table),
        std::move(compiler.native_function_table),
        std::move(compiler.type_table),
        std::move(compiler.string_pool)
    };
    runtime.start();
    std::cout << "success!" << std::endl;
    return 0;
}
