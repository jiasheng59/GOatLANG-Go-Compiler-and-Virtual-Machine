#include <iostream>

#include "antlr4-runtime.h"
#include "GOatLANGVisitor.h"

class TypeChecker : public GOatLANGBaseVisitor
{
    std::any visitLiteral(GOatLANGParser::LiteralContext *context)
    {
        /* Add the type of the expression to the Type Environment */
        /* visit any children if there is any */
    }

private:
    /* Type Environment */
};
