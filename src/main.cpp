#include <iostream>

#include "antlr4-runtime.h"
#include "GOatLANGLexer.h"
#include "GOatLANGParser.h"

int main()
{
    const char prog[] =
        "func main() {\n"
        "   Println(\"Hello, World!\")"
        "   var done chan int = make(chan struct{})\n"
        "   go func() {\n"
        "       select {\n"
        "       case <-done:\n"
        "           return\n"
        "       }\n"
        "   }()\n"
        "   close(done)\n"
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
    std::cout << tree->toStringTree(&parser, true) << std::endl;
    return 0;
}
