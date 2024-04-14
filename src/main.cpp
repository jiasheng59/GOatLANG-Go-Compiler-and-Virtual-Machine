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
    PrimitiveType(std::string t1) {
        t = t1;
    }

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
    ArrayType(Type *elementType1) {
        elementType = elementType1;
    }

    std::string getType() const override {
        return "Array Type: " + getElementType()->getType();
    }

    Type* getElementType() const {
        return elementType;
    }

    Type* elementType;
};

class ChanType : public Type {
public:
    ChanType(Type *elementType1) {
        elementType = elementType1;
    }

    std::string getType() const override {
        return "Chan Type: " + getElementType()->getType();
    }

    Type* getElementType() const {
        return elementType;
    }

    Type* elementType;
};

class Pair {
public:
    Pair(std::map<std::string, Type*> h, Pair* t) {
        head = h;
        tail = t;
    }
    std::map<std::string, Type*> head;
    Pair * tail;
};

Type* stringToType(std::string s) {
    if (s == "int" || s == "bool" || s == "float") {
       return new PrimitiveType(s);
    }
    return new Type();
}

Pair *type_environment = nullptr;
Pair *extend_type_environment(Pair *te) {
    return new Pair(std::map<std::string, Type*>(), te);
}
void add_type_environment(std::string name, Type* t, Pair *te) {
    te->head[name] = t;
}
Type* get_type(std::string name, Pair *te) {
    while (te != nullptr) {
        if (te->head.find(name) != te->head.end()) {
            return te->head[name];
        }
        te = te->tail;
    }
    return nullptr;
}


Type *type_check(antlr4::ParserRuleContext *ctx, Pair *te) { // return type of the ctx
    if (auto ctx1 = dynamic_cast<GOatLANGParser::SourceFileContext *>(ctx)) {
        // std::cout << "SourceFileContext" << ctx1->getText() << std::endl;
        std::vector<GOatLANGParser::TopLevelDeclContext *> topLevelDecls = ctx1->topLevelDecl();
        for (int i = 0; i < topLevelDecls.size(); i++) {
            type_check(topLevelDecls[i], te);
        }
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::TopLevelDeclContext*>(ctx)) {
        // std::cout << "TopLevelContext" << ctx1->getText() << std::endl;
        type_check(ctx1 ->varDecl(), te);
        type_check(ctx1 ->functionDecl(), te);
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::VarDeclContext*>(ctx)) {
        type_check(ctx1->varSpec(), te);
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::VarSpecContext*>(ctx)) {
        // std::cout << "VarSpec" << ctx1->toStringTree() << std::endl;
        std::vector<antlr4::tree::TerminalNode *> identifiers = ctx1->identifierList()->IDENTIFIER();
        GOatLANGParser::TypeNameContext* t = ctx1->goType()->typeName(); // primitive type
        GOatLANGParser::TypeLitContext* typeLit = ctx1->goType()->typeLit(); // complex type

        // Primitive type (TypeNameContext)
        for (int i = 0; i < identifiers.size(); i++) {
            if (t == nullptr) {
                break;
            }
            add_type_environment(identifiers[i]->toString(), stringToType(t->getText()), te); // add to type environment the type information of each identifier
            // std::cout << "identifiers[i]'s type is " << get_type(identifiers[i]->toString(), te)->getType() << std::endl;
        }

        // Complex type (TypeLitContext)
        Type *complex_type = type_check(typeLit, te);
        
        for (int i = 0; i < identifiers.size(); i++) {
            if (typeLit == nullptr) {
                break;
            }
            add_type_environment(identifiers[i]->toString(), complex_type, te); // add to type environment the type information of each identifier
            // std::cout << "identifiers[i]'s type is " << get_type(identifiers[i]->toString(), te)->getType() << std::endl;
        }
        type_check(ctx1->expressionList(), te);
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::IdentifierListContext*>(ctx)) {
        // unreachable
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::GoTypeContext*>(ctx)) {
        if (ctx1->typeName() != nullptr) {
            return stringToType(ctx1->typeName()->getText());
        }
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::TypeLitContext*>(ctx)) {
        if (ctx1->arrayType() != nullptr) {
            Type *elementType = type_check(ctx1->arrayType()->elementType(), te);
            return new ArrayType(elementType);
        } else if (ctx1->channelType() != nullptr) {
            Type *elementType = type_check(ctx1->channelType()->elementType(), te);
            return new ChanType(elementType);
        }
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::ElementTypeContext*>(ctx)) {
        return type_check(ctx1->goType(), te);
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::FunctionDeclContext*>(ctx)) {
        type_check(ctx1->function(), te);
        type_check(ctx1->signature(), te);
        std::cout << "FunctionDecl" << ctx1->getText() << std::endl;
    }
    
    return nullptr;
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

        // "var a, b int = 1, 2\n"
        // "var c int = 3\n"
        // "func main() {\n"
        // "return 1\n"
        // "}\n";

        "var a [2]string\n"
        "var b chan int = make(chan int)\n"
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
    
    type_environment = new Pair(std::map<std::string, Type*>(), type_environment);
    type_check(rootContext, type_environment);

    // std::cout << "Type of tree: " << typeid(tree).name() << std::endl;
    // std::cout << tree->toStringTree(&parser, true) << std::endl;
    GOatLANGBaseListener listener;
    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);
    Pair p(std::map<std::string, Type>, nullptr_t);
    return 0;
}
