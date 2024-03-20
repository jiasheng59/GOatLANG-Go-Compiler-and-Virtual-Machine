#include <iostream>

#include "antlr4-runtime.h"
#include "GOatLANGLexer.h"
#include "GOatLANGParser.h"

int main()
{
    antlr4::ANTLRInputStream input{
        "func main(foo int) {\n"
        "   Println(1)\n"
        "}"
    };
    GOatLANGLexer lexer{&input};
    antlr4::CommonTokenStream tokens{&lexer};
    for (auto token : tokens.getTokens()) {
        std::cout << token->toString() << std::endl;
    }
    
    GOatLANGParser parser{&tokens};
    auto tree = parser.sourceFile();
    std::cout << tree->toStringTree(true) << std::endl;
    return 0;
}
