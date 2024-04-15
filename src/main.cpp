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
    FunctionType(std::vector<Type*> args, std::vector<Type*> retType)
        : arguments(std::move(args)), returnType(std::move(retType)) {}

    std::string getType() const override {
        return "Function Type";
    }

    std::vector<Type*> getArguments() const {
        return arguments;
    }

    std::vector<Type*> getReturnType() const {
        return returnType;
    }

    std::vector<Type*> arguments;
    std::vector<Type*> returnType;
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

class UnaryType : public Type {
public:
    UnaryType(Type* arg, Type* retType)
        : argument(std::move(arg)), returnType(std::move(retType)) {}

    std::string getType() const override {
        return "Unary Type";
    }

    Type* getArgument() const {
        return argument;
    }

    Type* getReturnType() const {
        return returnType;
    }

    Type* argument;
    Type* returnType;
};

class BinaryType : public Type {
public:
    BinaryType(Type *first_arg, Type *second_arg, Type *retType) 
        : first_argument(std::move(first_arg)), second_argument(std::move(second_arg)), returnType(std::move(retType)) {}

    std::string getType() const override {
        return "Binary Type";
    }

    Type* getFirstArgument() const {
        return first_argument;
    }
    Type* getSecondArgument() const {
        return second_argument;
    }

    Type* getReturnType() const {
        return returnType;
    }

    Type* first_argument;
    Type* second_argument;
    Type* returnType;
};

Type *IntUnaryType = new UnaryType(new PrimitiveType("int"), new PrimitiveType("int"));
Type *LogicUnaryType = new UnaryType(new PrimitiveType("bool"), new PrimitiveType("bool"));
Type *IntBinaryType = new BinaryType(new PrimitiveType("int"), new PrimitiveType("int"), new PrimitiveType("int"));
Type *FloatBinaryType = new BinaryType(new PrimitiveType("float"), new PrimitiveType("float"), new PrimitiveType("float"));
Type *IntBoolBinaryType = new BinaryType(new PrimitiveType("int"), new PrimitiveType("int"), new PrimitiveType("bool"));
Type *FloatBoolBinaryType = new BinaryType(new PrimitiveType("float"), new PrimitiveType("float"), new PrimitiveType("bool"));
Type *LogicBinaryType = new BinaryType(new PrimitiveType("bool"), new PrimitiveType("bool"), new PrimitiveType("bool"));

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

std::map<void *, Type*> result_environment;
void annotate_context(void* ctx, Type* t) {
    result_environment[ctx] = t;
}
Type* get_type_of_context(void* ctx) {
    return result_environment[ctx];
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
            annotate_context(identifiers[i], stringToType(t->getText()));
            // std::cout << "identifiers[i]'s type is " << get_type(identifiers[i]->toString(), te)->getType() << std::endl;
        }

        // Complex type (TypeLitContext)
        Type *complex_type = type_check(typeLit, te);
        
        for (int i = 0; i < identifiers.size(); i++) {
            if (typeLit == nullptr) {
                break;
            }
            add_type_environment(identifiers[i]->toString(), complex_type, te); // add to type environment the type information of each identifier
            annotate_context(identifiers[i], complex_type);
            // std::cout << "identifiers[i]'s type is " << get_type(identifiers[i]->toString(), te)->getType() << std::endl;
        }
        type_check(ctx1->expressionList(), te);
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::IdentifierListContext*>(ctx)) {
        // unreachable


    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::FunctionDeclContext*>(ctx)) { // function declaration
        
        std::cout << "FunctionDecl: " << ctx1->getText() << std::endl;
        std::string function_name = ctx1->IDENTIFIER()->toString();
        std::cout << "Function name: " << function_name << std::endl;
        
        Pair* extended_te = extend_type_environment(te);
        Type *func_type = type_check(ctx1->function(), extended_te);
        add_type_environment(function_name, func_type, te);

        // auto func = dynamic_cast<FunctionType*>(get_type(function_name, te));
        // std::cout << "Type of " << function_name << ": " << func->getArguments()[1]->getType() << std::endl;

    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::FunctionContext*>(ctx)) {
        // std::cout << "Function: " << ctx1->toStringTree() << std::endl;
        // std::cout << "Function Signature: " << ctx1->signature();
        Type *func_type = type_check(ctx1->signature(), te);

        // body of function
        type_check(ctx1->block(), te);

        return func_type;



    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::SignatureContext*>(ctx)) { // determine function type, determine type for parameters
        std::cout << "Parameters: " << ctx1->parameters()->toStringTree() << std::endl;
        // std::cout << "Result: " << ctx1->result()->toStringTree() << std::endl;
        std::vector<GOatLANGParser::ParameterDeclContext*> parameterDeclContexts = ctx1->parameters()->parameterList()->parameterDecl();
        std::vector<Type*> argumentsType;
        for (int i = 0; i < parameterDeclContexts.size(); i++) { 
            std::string parameter_name = parameterDeclContexts[i]->identifierList()->IDENTIFIER()[0]->toString();
            Type* parameter_type = type_check(parameterDeclContexts[i]->goType(), te);
            add_type_environment(parameter_name, parameter_type, te);
            argumentsType.push_back(parameter_type);
        }
        std::vector<Type*> returnType;
        if (ctx1->result()->parameters() != nullptr) {
            std::vector<GOatLANGParser::ParameterDeclContext*> returnContexts = ctx1->result()->parameters()->parameterList()->parameterDecl(); 
            for (int i = 0; i < returnContexts.size(); i++) {
                returnType.push_back(type_check(returnContexts[i]->goType(), te));
            }
        }
        return new FunctionType(argumentsType, returnType);

    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::ParameterDeclContext*>(ctx)) {


    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::GoTypeContext*>(ctx)) {
        if (ctx1->typeName() != nullptr) { // primitive type
            return stringToType(ctx1->typeName()->getText());
        } else if (ctx1->typeLit() != nullptr) { // complex type
            Type *complex_type = type_check(ctx1->typeLit(), te);
            return complex_type;
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
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::OperandNameContext*>(ctx)) { // name occurrences
        // std::cout << "Name occurence: " << ctx1->IDENTIFIER()->toString() << std::endl;
        annotate_context(ctx1, get_type(ctx1->IDENTIFIER()->toString(), te));
        return get_type(ctx1->IDENTIFIER()->toString(), te);
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::BlockContext*>(ctx)) {
        // std::cout << "Block occurence: " << ctx1->toStringTree() << std::endl;
        Pair* extended_te = extend_type_environment(te);
        type_check(ctx1->statementList(), extended_te);
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::StatementListContext*>(ctx)) {
        std::vector<GOatLANGParser::StatementContext *> statements = ctx1->statement();
        for (int i = 0; i < statements.size(); i++) {
            type_check(statements[i], te);
        }
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::StatementContext*>(ctx)) {
        // std::cout << "statement: " << ctx1->toStringTree() << std::endl;
        type_check(ctx1->varDecl(), te);
        // type_check(ctx1->labeledStmt(), te);
        type_check(ctx1->simpleStmt(), te);
        // type_check(ctx1->goStmt(), te);
        type_check(ctx1->returnStmt(), te);
        type_check(ctx1->gotoStmt(), te);
        type_check(ctx1->block(), te);
        type_check(ctx1->ifStmt(), te);
        type_check(ctx1->selectStmt(), te);
        type_check(ctx1->forStmt(), te);
        type_check(ctx1->deferStmt(), te);
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::SimpleStmtContext*>(ctx)) {
        // type_check(ctx1->sendStmt(), te);
        type_check(ctx1->expressionStmt(), te);
        // type_check(ctx1->assignment(), te);
        // type_check(ctx1->emptyStmt(), te);
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::ExpressionStmtContext*>(ctx)) {
        type_check(ctx1->expression(), te);
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::ExpressionContext*>(ctx)) {
        type_check(ctx1->unaryExpr(), te);
        std::vector<GOatLANGParser::ExpressionContext *> expressions = ctx1->expression();
        for (int i = 0; i < expressions.size(); i++) {
            type_check(expressions[i], te);
        }
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::UnaryExprContext*>(ctx)) {
        Type* expressionType = nullptr;
        if (ctx1->unaryExpr() != nullptr) {
            expressionType = type_check(ctx1->unaryExpr(), te);
            annotate_context(ctx, expressionType);
            return expressionType;
        }
        expressionType = type_check(ctx1->primaryExpr(), te);
        annotate_context(ctx, expressionType);
        return expressionType;
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::PrimaryExprContext*>(ctx)) {
        Type* expressionType = type_check(ctx1->operand(), te);
        annotate_context(ctx, expressionType);
        return expressionType;
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::OperandContext*>(ctx)) {
        Type *t = nullptr;
        if (ctx1->literal() != nullptr) {
            t = type_check(ctx1->literal(), te);
            annotate_context(ctx, t);
            return t;
        } else if (ctx1->operandName() != nullptr) {
            t = type_check(ctx1->operandName(), te);
            annotate_context(ctx, t);
            return t;
        } else if (ctx1->expression() != nullptr) {
            t = type_check(ctx1->expression(), te);
            annotate_context(ctx, t);
            return t;
        }
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::LiteralContext*>(ctx)) {
        Type *t = nullptr;
        if (ctx1->basicLit() != nullptr) {
            t = type_check(ctx1->basicLit(), te);
            annotate_context(ctx, t);
            return t;
        } else if (ctx1->compositeLit() != nullptr) {
            t = type_check(ctx1->compositeLit(), te);
            annotate_context(ctx, t);
            return t;
        } else if (ctx1->functionLit() != nullptr) {
            t = type_check(ctx1->functionLit(), te);
            annotate_context(ctx, t);
            return t;
        }
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::BasicLitContext*>(ctx)) {
        if (ctx1->INT_LIT() != nullptr) {
            std::cout << "BasicLit: " << ctx1->INT_LIT()->toString() << std::endl;
            Type *t = new PrimitiveType("int");
            annotate_context(ctx, t);
            return t;
        } else if (ctx1->FLOAT_LIT() != nullptr) {
            std::cout << "BasicLit: " << ctx1->FLOAT_LIT()->toString() << std::endl;
            Type *t = new PrimitiveType("float");
            annotate_context(ctx, t);
            return t;
        } else if (ctx1->STRING_LIT() != nullptr) {
            Type *t = new PrimitiveType("string");
            annotate_context(ctx, t);
            return t;
        }
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::CompositeLitContext*>(ctx)) {
        Type *t = type_check(ctx1->literalType(), te);
        annotate_context(ctx, t);
        return t;
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::ArrayTypeContext*>(ctx)) {
        Type *t = type_check(ctx1->elementType()->goType(), te);
        Type *t1 = new ArrayType(t);
        annotate_context(ctx, t);
        return t1;
    } else if (auto ctx1 = dynamic_cast<GOatLANGParser::FunctionLitContext*>(ctx)) {
        Pair* extended_te = extend_type_environment(te);
        Type *func_type = type_check(ctx1->function(), extended_te);
        annotate_context(ctx, func_type);
        return func_type;
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
        // "func foo(a int, b bool) (chan int, float) {\n"
        // "var c chan int = make(chan int)\n"
        // "a + 1;\n"
        // "return c, 1.0\n"
        // "}\n"
        "func bar(d int) float {\n"
        "var e[2]int"
        "return d + 5.0\n"
        "}\n"

        // "func main() {\n"
        // "return 1\n"
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

    // Testing
    // annotate_context(rootContext, new Type());
    // std::cout << "result_environment: " << get_type_of_context(rootContext)->getType() << std::endl;

    // std::cout << "Type of tree: " << typeid(tree).name() << std::endl;
    // std::cout << tree->toStringTree(&parser, true) << std::endl;
    GOatLANGBaseListener listener;
    antlr4::tree::ParseTreeWalker::DEFAULT.walk(&listener, tree);
    Pair p(std::map<std::string, Type>, nullptr_t);
    return 0;
}
