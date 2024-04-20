#include <iostream>

#include "antlr4-runtime.h"
#include "GOatLANGLexer.h"
#include "GOatLANGParser.h"
#include "Compiler.hpp"

int main()
{
    const char prog[] =
        "func main() {\n"
        "    var done chan int = make(chan struct{})\n"
        "    go func(n int) {\n"
        "        var x int = n * 2 + 3 / 5 % 9\n"
        "        println(x)\n"
        "    }(1)\n"
        "    close(done)\n"
        "}";
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
    Compiler compiler{};
    compiler.visitSourceFile(parser.sourceFile());
    std::cout << tree->toStringTree(&parser, true) << std::endl;
    return 0;
}
