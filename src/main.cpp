#include <iostream>

#include "antlr4-runtime.h"
#include "GOatLANGLexer.h"
#include "GOatLANGParser.h"
#include "Compiler.hpp"
#include "Runtime.hpp"

int main()
{
    const char prog[] =
    R"(
func main() {
    var x int
    var y int = 10
    if (y > 2) {
        x = 1
    } else {
        x = 2
    }
    iprint(x)
}
    )";

        // "func main() {\n"
        // "    var done chan int = make(chan struct{})\n"
        // "    go func(n int) {\n"
        // "        var x int = n * 2 + 3 / 5 % 9\n"
        // "        iprint(x)\n"
        // "    }(1)\n"
        // "}";
    antlr4::ANTLRInputStream input(prog);
    GOatLANGLexer lexer{&input};
    antlr4::CommonTokenStream tokens{&lexer};
    tokens.fill();
    std::cout << "number of tokens: " << tokens.size() << std::endl;
    for (auto token : tokens.getTokens()) {
        std::cout << token->toString() << std::endl;
    }
    GOatLANGParser parser{&tokens};
    auto tree = parser.sourceFile();
    std::cout << tree->toStringTree(&parser, true) << std::endl;

    Compiler compiler{};
    compiler.visitSourceFile(tree);
    for (auto& [name, index] : compiler.function_indices) {
        std::cout << name << ", " << index << std::endl;
    }
    Configuration configuration = Runtime::default_configuration();
    configuration.init_function_index = compiler.function_indices.at("main");
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
