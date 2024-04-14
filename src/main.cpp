#include <iostream>
#include <string>

#include "antlr4-runtime.h"
#include "GOatLANGLexer.h"
#include "GOatLANGParser.h"
#include "GOatLANGBaseListener.h"

// class Listener : public GOatLANGBaseListener {
// public:
//     void enterEveryRule(antlr4::ParserRuleContext *ctx) override {
//         std::cout << "Entering rule: " << ctx->getText() << std::endl;
//     }

//     void exitEveryRule(antlr4::ParserRuleContext *ctx) override {
//         std::cout << "Exiting rule: " << ctx->getText() << std::endl;
//     }
// };


class Type {
public:
    virtual std::string getType() const {
        return "Base Type";
    }
};

class PrimitiveType : public Type {
public:
    PrimitiveType(std::string t)
        : t(std::move(t)) {}

    std::string getType() const override {
        return t;
    }

    std::string t;
};

class FunctionType : public Type {
public:
    FunctionType(std::vector<Type> args, std::vector<Type> retType)
        : arguments(std::move(args)), returnType(std::move(retType)) {}

    std::string getType() const override {
        return "Function Type";
    }

    std::vector<Type> getArguments() const {
        return arguments;
    }

    std::vector<Type> getReturnType() const {
        return returnType;
    }

    std::vector<Type> arguments;
    std::vector<Type> returnType;
};

class ArrayType : public Type {
public:
    ArrayType(Type elementType)
        : elementType(std::move(elementType)) {}

    std::string getType() const override {
        return "Array Type";
    }

    Type getElementType() const {
        return elementType;
    }

    Type elementType;
};

class ChanType : public Type {
public:
    ChanType(Type elementType)
        : elementType(std::move(elementType)) {}

    std::string getType() const override {
        return "Chan Type";
    }

    Type getElementType() const {
        return elementType;
    }

    Type elementType;
};

std::string type_check(antlr4::ParserRuleContext *ctx) { // return type of the ctx
    if (auto ctx1 = dynamic_cast<GOatLANGParser::SourceFileContext *>(ctx)) {
        std::vector<GOatLANGParser::TopLevelDeclContext *> topLevelDecls = ctx1->topLevelDecl();
        for (int i = 0; i < topLevelDecls.size(); i++) {
            type_check(topLevelDecls[i]);
        }
        // do nothing
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::TopLevelDeclContext*>(ctx)) {
        type_check(ctx1 ->varDecl());
        type_check(ctx1 ->functionDecl());
        // do nothing
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::VarDeclContext*>(ctx)) {
        type_check(ctx1->varSpec());
        // do nothing
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::VarSpecContext*>(ctx)) { // map identifiers to their type
        std::vector<antlr4::tree::TerminalNode *> identifiers = ctx1->identifierList()->IDENTIFIER();
        GOatLANGParser::TypeNameContext* t = ctx1->goType()->typeName();
        std::cout << "TypeName" << t->getText() << std::endl;
        type_check(ctx1->expressionList());
        std::cout << "VarSpec" << ctx1->getText() << std::endl;
        // add to type environment the type information of identifier
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::IdentifierListContext*>(ctx)) {
        // unreachable
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::IdentifierListContext*>(ctx)) {
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::FunctionDeclContext*>(ctx)) {
        type_check(ctx1->function());
        type_check(ctx1->signature());
        std::cout << "FunctionDecl" << ctx1->getText() << std::endl;
    }
    
    return "";
}

std::map<antlr4::ParserRuleContext*, Type> type_environment; // context to type

void addTypeInformation(antlr4::ParserRuleContext *ctx, Type type, std::map<antlr4::ParserRuleContext*, Type> te) {
    type_environment[ctx] = type;
}

Type getTypeInformation(antlr4::ParserRuleContext *ctx, Type type, std::map<antlr4::ParserRuleContext*, Type> te) {
    return type_environment[ctx];
}



int main()
{
    const char prog[] =
        // "func main() {\n"
        // "   Println(\"Hello, World!\")"
        // "   var done chan int = make(chan struct{})\n"
        // "   go func() {\n"
        // "       select {\n"
        // "       case <-done:\n"
        // "           return\n"
        // "       }\n"
        // "   }()\n"
        // "   close(done)\n"
        // "}";
        "var a int = 1\n"
        "var b int = 2\n"
        "func main() {\n"
        "return 1\n"
        "}\n";
    antlr4::ANTLRInputStream input(prog);
    GOatLANGLexer lexer{&input};
    antlr4::CommonTokenStream tokens{&lexer};
    tokens.fill();
    std::cout << "number of tokens: " << tokens.size() << std::endl;
    for (auto token : tokens.getTokens()) {
        // std::cout << token->toString() << std::endl;
    }
    GOatLANGParser parser{&tokens};
    antlr4::tree::ParseTree *tree = parser.sourceFile();
    antlr4::ParserRuleContext *rootContext = dynamic_cast<antlr4::ParserRuleContext *>(tree);
    type_check(rootContext);

    // std::cout << "Type of tree: " << typeid(tree).name() << std::endl;
    // std::cout << tree->toStringTree(&parser, true) << std::endl;
    GOatLANGBaseListener listener;
    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);
    return 0;
}
