#ifndef COMPILER_HPP
#define COMPILER_HPP

#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Code.hpp"
#include "GOatLANGBaseVisitor.h"

class TopLevelScanner : public GOatLANGBaseVisitor
{
public:
    std::vector<Function>& function_table;
    std::unordered_map<std::string, u64>& function_indices;

    TopLevelScanner(
        std::vector<Function>& function_table,
        std::unordered_map<std::string, u64>& function_indices) : function_table{function_table},
                                                                  function_indices{function_indices}
    {
    }

    virtual std::any visitTopLevelDecl(GOatLANGParser::TopLevelDeclContext* ctx) override
    {
        if (auto function_decl = ctx->functionDecl(); function_decl) {
            visitFunctionDecl(function_decl);
        }
        return {};
    }

    virtual std::any visitFunctionDecl(GOatLANGParser::FunctionDeclContext* ctx) override
    {
        u64 function_index = function_table.size();
        Function function{};
        function.index = function_index;
        function_table.push_back(std::move(function));
        function_indices.try_emplace(ctx->IDENTIFIER()->getText(), function_index);
        return {};
    }
};

enum class VariableCategory
{
    free,
    bound,
    escaped,
};

struct Variable
{
    VariableCategory category;
    u64 index = 0;
};

struct VariableFrame
{
    using variable_map = std::unordered_map<std::string, Variable>;
    std::vector<variable_map::value_type*> parameters;
    std::vector<variable_map::value_type*> locals;
    std::vector<variable_map::value_type*> captures;
    variable_map variables;
};

class VariableAnalyzer : public GOatLANGBaseVisitor
{
public:
    std::unordered_map<void*, VariableFrame>& variable_frames;
    VariableFrame* current_frame;

    VariableAnalyzer(
        std::unordered_map<void*, VariableFrame>& variable_frames) : variable_frames{variable_frames},
                                                                     current_frame{nullptr}
    {
    }

    virtual std::any visitFunction(GOatLANGParser::FunctionContext* ctx) override
    {
        VariableFrame* enclosing_frame = current_frame;
        VariableFrame* current_frame;
        {
            auto [it, _] = variable_frames.try_emplace(ctx);
            current_frame = &it->second;
        }
        this->current_frame = current_frame;

        visitSignature(ctx->signature());
        visitBlock(ctx->block());

        {
            u64 index = 0;
            for (auto ptr : current_frame->captures) {
                ptr->second.index = index++;
            }
            for (auto ptr : current_frame->parameters) {
                ptr->second.index = index++;
            }
            for (auto ptr : current_frame->locals) {
                ptr->second.index = index++;
            }
        }

        this->current_frame = enclosing_frame;
        if (!enclosing_frame) {
            return {};
        }

        auto& current_variables = current_frame->variables;
        auto& enclosing_variables = enclosing_frame->variables;

        for (auto& [name, variable] : current_variables) {
            if (variable.category != VariableCategory::free) {
                continue;
            }
            auto it = enclosing_variables.find(name);
            if (it == enclosing_variables.end()) {
                auto [new_it, _] = enclosing_variables.try_emplace(
                    std::move(name),
                    Variable{.category = VariableCategory::free});
                enclosing_frame->captures.push_back(&*new_it);
                continue;
            }
            Variable& enclosing_variable = it->second;
            if (enclosing_variable.category == VariableCategory::bound) {
                enclosing_variable.category = VariableCategory::escaped;
            }
        }
        return {};
    }

    virtual std::any visitSignature(GOatLANGParser::SignatureContext* ctx) override
    {
        visitParameters(ctx->parameters());
        return {};
    }

    virtual std::any visitParameterDecl(GOatLANGParser::ParameterDeclContext* ctx) override
    {
        if (auto identifier = ctx->IDENTIFIER(); identifier) {
            auto [it, _] = current_frame->variables.try_emplace(
                identifier->getText(),
                Variable{.category = VariableCategory::bound});
            current_frame->parameters.push_back(&*it);
        }
        return {};
    }

    virtual std::any visitVarSpec(GOatLANGParser::VarSpecContext* ctx) override
    {
        auto [it, _] = current_frame->variables.try_emplace(
            ctx->IDENTIFIER()->getText(),
            Variable{.category = VariableCategory::bound});
        current_frame->locals.push_back(&*it);
        return {};
    }

    virtual std::any visitOperandName(GOatLANGParser::OperandNameContext* ctx) override
    {
        auto name = ctx->IDENTIFIER()->getText();
        auto& current_variables = current_frame->variables;
        if (current_variables.contains(name)) {
            return {};
        }
        auto [it, _] = current_variables.try_emplace(
            std::move(name),
            Variable{.category = VariableCategory::free});
        current_frame->captures.push_back(&*it);
        return {};
    }
};

class BaseType
{
public:
    virtual const std::string& get_name() = 0;
    virtual Type to_type() = 0;
};

class IntType : public BaseType
{
    std::string name = "int";

public:
    virtual const std::string& get_name() override
    {
        return name;
    }

    virtual Type to_type() override
    {
        return Type{.memc = 0, .size = 8};
    }
};

class FloatType : public BaseType
{
    std::string name = "float";

public:
    virtual const std::string& get_name() override
    {
        return name;
    }

    virtual Type to_type() override
    {
        return Type{.memc = 0, .size = 8};
    }
};

class ChanType : public BaseType
{
public:
    virtual const std::string& get_name() override
    {
        return "";
    }

    virtual Type to_type() override
    {
    }
};

class RecvChanType : public ChanType
{
};

class SendChanType : public ChanType
{
};

class StructType : public BaseType
{
};

class FunctionType : public BaseType
{
};

class SliceType : public BaseType
{
};

class TypeChecker : public GOatLANGBaseVisitor
{
public:
    virtual std::any visitVarSpec(GOatLANGParser::VarSpecContext* ctx) override
    {
        // add the type of var spec to the type enviroment
    }

    virtual std::any visitParameterDecl(GOatLANGParser::ParameterDeclContext* ctx) override
    {
    }

    virtual std::any visitBinaryExpr(GOatLANGParser::BinaryExprContext* ctx) override
    {
    }

    virtual std::any visitUnaryExpr(GOatLANGParser::UnaryExprContext* ctx) override
    {
    }

    virtual std::any visitCallExpr(GOatLANGParser::CallExprContext* ctx) override
    {
    }
};

class Compiler : public GOatLANGBaseVisitor
{
public:
    static constexpr u64 new_thread_index = 0;
    static constexpr u64 new_chan_index = 0;
    static constexpr u64 chan_send_index = 0;
    static constexpr u64 chan_recv_index = 0;

    std::vector<Function> function_table;
    std::unordered_map<std::string, u64> function_indices;

    Function* current_function;
    VariableFrame* variable_frame;

    // std::vector<Type> type_table;
    // std::unordered_map<std::string, CompilerType*> type_names;
    std::vector<BaseType> types;
    std::unordered_map<std::string, BaseType*> variable_types;
    std::unordered_map<void*, BaseType*> expression_types;

    /*
    std::vector<NativeFunction> native_function_table;
    std::unordered_map<std::string, Type> identifier_types;
    std::unordered_map<void*, Type> node_types;
    */
    std::unordered_map<void*, VariableFrame> variable_frames;

    std::any visitExpression(GOatLANGParser::ExpressionContext* ctx)
    {
        return ctx->accept(this);
    }

    std::any visitPrimaryExpr(GOatLANGParser::PrimaryExprContext* ctx)
    {
        return ctx->accept(this);
    }

    virtual std::any visitSourceFile(GOatLANGParser::SourceFileContext* ctx) override
    {
        TopLevelScanner scanner{function_table, function_indices};
        scanner.visitSourceFile(ctx);
        return visitChildren(ctx);
    }

    virtual std::any visitTopLevelDecl(GOatLANGParser::TopLevelDeclContext* ctx) override
    {
        if (auto function_decl = ctx->functionDecl(); function_decl) {
            visitFunctionDecl(function_decl);
        }
        return {};
    }

    virtual std::any visitFunctionDecl(GOatLANGParser::FunctionDeclContext* ctx) override
    {
        Function* saved_function = current_function;
        u64 function_index = function_indices.at(ctx->IDENTIFIER()->getText());
        current_function = &function_table[function_index];

        // before we visit this function, we need to analyze the variables
        // variable_frames.clear();
        VariableAnalyzer analyzer{variable_frames};
        analyzer.visitFunctionDecl(ctx);

        visitFunction(ctx->function());
        current_function = saved_function;
        return {};
    }

    virtual std::any visitFunction(GOatLANGParser::FunctionContext* ctx) override
    {
        variable_frame = &variable_frames.at(ctx);
        visitSignature(ctx->signature());
        visitBlock(ctx->block());
        return {};
    }

    virtual std::any visitVarSpec(GOatLANGParser::VarSpecContext* ctx) override
    {
        auto expression = ctx->expression();
        if (!expression) {
            return {};
        }
        visitExpression(expression);

        auto name = ctx->IDENTIFIER()->getText();
        auto& variable = variable_frame->variables.at(name);
        auto& code = current_function->code;

        switch (variable.category) {
            case VariableCategory::bound: {
                code.push_back(Instruction{.opcode = Opcode::store, .index = variable.index});
                break;
            }
            case VariableCategory::escaped: {
                BaseType* type = expression_types.at(expression);
                code.push_back(Instruction{.opcode = Opcode::new_, .index = 0 /* TODO */});
                code.push_back(Instruction{.opcode = Opcode::dup});
                code.push_back(Instruction{.opcode = Opcode::store, .index = variable.index});
                code.push_back(Instruction{.opcode = Opcode::wstore, .index = 0});
                break;
            }
            case VariableCategory::free: {
                /* error */
                break;
            }
        }
        return {};
    }

    virtual std::any visitExpressionStmt(GOatLANGParser::ExpressionStmtContext* ctx) override
    {
        visitExpression(ctx->expression());
        current_function->code.push_back(Instruction{.opcode = Opcode::pop});
        return {};
    }

    virtual std::any visitSendStmt(GOatLANGParser::SendStmtContext* ctx) override
    {
        auto name = ctx->IDENTIFIER()->getText();
        auto& variable = variable_frame->variables.at(name);
        auto& code = current_function->code;
        switch (variable.category) {
            case VariableCategory::bound: {
                code.push_back(Instruction{.opcode = Opcode::load, .index = variable.index});
                break;
            }
            case VariableCategory::escaped:
            case VariableCategory::free: {
                code.push_back(Instruction{.opcode = Opcode::load, .index = variable.index});
                code.push_back(Instruction{.opcode = Opcode::wload, .index = 0});
                break;
            }
        }
        code.push_back(Instruction{.opcode = Opcode::new_, .index = 0});
        code.push_back(Instruction{.opcode = Opcode::dup});
        visitExpression(ctx->expression());
        code.push_back(Instruction{.opcode = Opcode::wstore, .index = 0});
        code.push_back(Instruction{.opcode = Opcode::invoke_native, .index = chan_send_index});
        return {};
    }

    virtual std::any visitAssignmentStmt(GOatLANGParser::AssignmentStmtContext* ctx) override
    {
        visitExpression(ctx->expression());

        auto name = ctx->IDENTIFIER()->getText();
        auto& variable = variable_frame->variables.at(name);
        auto& code = current_function->code;

        switch (variable.category) {
            case VariableCategory::bound: {
                code.push_back(Instruction{.opcode = Opcode::store, .index = variable.index});
                break;
            }
            case VariableCategory::escaped:
            case VariableCategory::free: {
                code.push_back(Instruction{.opcode = Opcode::load, .index = variable.index});
                code.push_back(Instruction{.opcode = Opcode::wstore, .index = 0});
                break;
            }
        }
        return {};
    }

    virtual std::any visitReturnStmt(GOatLANGParser::ReturnStmtContext* ctx) override
    {
        visitExpression(ctx->expression());
        current_function->code.push_back(Instruction{.opcode = Opcode::ret});
        return {};
    }

    virtual std::any visitIfStmt(GOatLANGParser::IfStmtContext* ctx) override
    {
        visitExpression(ctx->expression());

        auto& code = current_function->code;
        u64 if_f_index = code.size();
        code.push_back(Instruction{
            .opcode = Opcode::if_f,
            .index = 0, /* determined later */
        });

        visitBlock(ctx->block(0));

        auto block = ctx->block(1);
        auto if_stmt = ctx->ifStmt();
        u64 goto_index;

        if (block || if_stmt) {
            goto_index = code.size();
            code.push_back(Instruction{
                .opcode = Opcode::goto_,
                .index = 0, /* determined later */
            });
        }
        code[if_f_index].index = code.size();

        if (block) {
            visitBlock(block);
        } else if (if_stmt) {
            visitIfStmt(if_stmt);
        }

        if (block || if_stmt) {
            code[goto_index].index = code.size();
        }
        return {};
    }

    virtual std::any visitForStmt(GOatLANGParser::ForStmtContext* ctx) override
    {
        auto& code = current_function->code;
        auto expression = ctx->expression();
        u64 for_index = code.size();
        u64 if_f_index;

        if (expression) {
            visitExpression(expression);
            if_f_index = code.size();
            code.push_back(Instruction{.opcode = Opcode::if_f});
        }

        visitBlock(ctx->block());

        code.push_back(Instruction{.opcode = Opcode::goto_, .index = for_index});
        if (expression) {
            code[if_f_index].index = code.size();
        }
        return {};
    }

    virtual std::any visitGoStmt(GOatLANGParser::GoStmtContext* ctx) override
    {
        auto call_expr = dynamic_cast<GOatLANGParser::CallExprContext*>(ctx->expression());
        auto& code = current_function->code;
        // TODO
        visitPrimaryExpr(call_expr->primaryExpr());
        visitArguments(call_expr->arguments());
        code.push_back(Instruction{.opcode = Opcode::invoke_native, .index = new_thread_index});
        return {};
    }

    virtual std::any visitParameterList(GOatLANGParser::ParameterListContext* ctx) override
    {
        auto parameter_decls = ctx->parameterDecl();
        /* the parameters are visited in reverse order */
        for (auto it = parameter_decls.rbegin(); it != parameter_decls.rend(); ++it) {
            visitParameterDecl(*it);
        }
        return {};
    }

    virtual std::any visitParameterDecl(GOatLANGParser::ParameterDeclContext* ctx) override
    {
        auto identifier = ctx->IDENTIFIER();
        auto& code = current_function->code;
        if (!identifier) {
            code.push_back(Instruction{.opcode = Opcode::pop});
            return {};
        }
        auto name = identifier->getText();
        auto& variable = variable_frame->variables.at(name);
        switch (variable.category) {
            case VariableCategory::bound: {
                code.push_back(Instruction{.opcode = Opcode::store, .index = variable.index});
                break;
            }
            case VariableCategory::escaped: {
                code.push_back(Instruction{.opcode = Opcode::new_, .index = 0 /* TODO type */});
                code.push_back(Instruction{.opcode = Opcode::dup});
                code.push_back(Instruction{.opcode = Opcode::store, .index = variable.index});
                code.push_back(Instruction{.opcode = Opcode::wstore, .index = 0});
                break;
            }
            case VariableCategory::free: {
                /* TODO error */
                break;
            }
        }
        return {};
    }

    virtual std::any visitBasicLit(GOatLANGParser::BasicLitContext* ctx) override
    {
        Word word;
        if (auto int_lit = ctx->INT_LIT(); int_lit) {
            auto text = int_lit->getText();
            u64 value = std::stoull(text);
            word = bitcast<u64, Word>(value);
        } else if (auto float_lit = ctx->FLOAT_LIT(); float_lit) {
            auto text = float_lit->getText();
            f64 value = std::stod(text);
            word = bitcast<f64, Word>(value);
        }
        // TODO: handle string
        current_function->code.push_back(Instruction{.opcode = Opcode::push, .value = word});
        return {};
    }

    virtual std::any visitOperandName(GOatLANGParser::OperandNameContext* ctx) override
    {
        auto name = ctx->IDENTIFIER()->getText();
        auto& variable = variable_frame->variables.at(name);
        auto& code = current_function->code;

        switch (variable.category) {
            case VariableCategory::bound: {
                code.push_back(Instruction{.opcode = Opcode::load, .index = variable.index});
                break;
            }
            case VariableCategory::escaped:
            case VariableCategory::free: {
                code.push_back(Instruction{.opcode = Opcode::load, .index = variable.index});
                code.push_back(Instruction{.opcode = Opcode::wload, .index = 0});
                break;
            }
        }
        return {};
    }

    virtual std::any visitFunctionLit(GOatLANGParser::FunctionLitContext* ctx) override
    {
        Function* saved_function = current_function;
        u64 function_index = function_table.size();
        Function function{};
        function.index = function_index;

        current_function = &function;
        auto& code = current_function->code;
        auto function_ctx = ctx->function();

        // we need to know the lambda

        code.push_back(Instruction{.opcode = Opcode::new_, .index = 0 /* TODO: Closure index */});
        // and we need to load the captures as well
        // sigh

        visitFunction(function_ctx);
        function_table.push_back(std::move(function));
        current_function = saved_function;
        return {};
    }

    virtual std::any visitArguments(GOatLANGParser::ArgumentsContext* ctx) override
    {
        if (auto go_type = ctx->goType(); go_type) {
            // do something here
            // not necessary here!
        }
        if (auto expression_list = ctx->expressionList(); expression_list) {
            visitExpressionList(expression_list);
        }
        return {};
    }

    virtual std::any visitUnaryExpr(GOatLANGParser::UnaryExprContext* ctx) override
    {
        auto unary_op = ctx->unary_op->getText();
        auto& code = current_function->code;
        if (unary_op == "&") {
            // special case, special visitor
            return {};
        }

        auto expression = ctx->expression();
        visitExpression(expression);
        if (unary_op == "<-") {
            code.push_back(Instruction{.opcode = Opcode::invoke_native, .index = chan_recv_index});
            code.push_back(Instruction{.opcode = Opcode::wload, .index = 0});
            return {};
        }
        if (unary_op == "*") {
            code.push_back(Instruction{.opcode = Opcode::wload, .index = 0});
            return {};
        }
        if (unary_op == "+") {
            return {};
        }
        Opcode opcode;
        auto& type = expression_types.at(expression)->get_name();
        if (unary_op == "-") {
            opcode = type == "int" ? Opcode::ineg : Opcode::fneg;
        } else if (unary_op == "!") {
            opcode = Opcode::lnot;
        } else if (unary_op == "^") {
            opcode = Opcode::inot;
        } else {
            // error
        }
        code.push_back(Instruction{.opcode = opcode});
        return {};
    }

    virtual std::any visitBinaryExpr(GOatLANGParser::BinaryExprContext* ctx) override
    {
        auto binary_op = ctx->binary_op->getText();
        auto& code = current_function->code;
        auto left = ctx->expression(0);
        auto right = ctx->expression(1);
        if (binary_op == "||") {
            visitExpression(left);
            code.push_back(Instruction{.opcode = Opcode::dup});
            u64 if_t_index = code.size();
            code.push_back(Instruction{.opcode = Opcode::if_t});
            code.push_back(Instruction{.opcode = Opcode::pop});
            visitExpression(right);
            code[if_t_index].index = code.size();
            return {};
        }
        if (binary_op == "&&") {
            visitExpression(left);
            code.push_back(Instruction{.opcode = Opcode::dup});
            u64 if_f_index = code.size();
            code.push_back(Instruction{.opcode = Opcode::if_f});
            code.push_back(Instruction{.opcode = Opcode::pop});
            visitExpression(right);
            code[if_f_index].index = code.size();
            return {};
        }
        visitExpression(ctx->expression(0));
        visitExpression(ctx->expression(1));
        Opcode opcode;
        auto& type = expression_types.at(left)->get_name();

        if (binary_op == "==") {
            opcode = type == "int" ? Opcode::ieq : Opcode::feq;
        } else if (binary_op == "!=") {
            opcode = type == "int" ? Opcode::ine : Opcode::fne;
        } else if (binary_op == "<") {
            opcode = type == "int" ? Opcode::ilt : Opcode::flt;
        } else if (binary_op == "<=") {
            opcode = type == "int" ? Opcode::ile : Opcode::fle;
        } else if (binary_op == ">") {
            opcode = type == "int" ? Opcode::igt : Opcode::fgt;
        } else if (binary_op == ">=") {
            opcode = type == "int" ? Opcode::ige : Opcode::fge;
        } else if (binary_op == "+") {
            opcode = type == "int" ? Opcode::iadd : Opcode::fadd;
        } else if (binary_op == "-") {
            opcode = type == "int" ? Opcode::isub : Opcode::fsub;
        } else if (binary_op == "|") {
            opcode = Opcode::ior;
        } else if (binary_op == "^") {
            opcode = Opcode::ixor;
        } else if (binary_op == "*") {
            opcode = type == "int" ? Opcode::imul : Opcode::fmul;
        } else if (binary_op == "/") {
            opcode = type == "int" ? Opcode::idiv : Opcode::fdiv;
        } else if (binary_op == "%") {
            opcode = Opcode::irem;
        } else if (binary_op == "<<") {
            opcode = Opcode::ishl;
        } else if (binary_op == ">>") {
            opcode = Opcode::ishr;
        } else if (binary_op == "&") {
            opcode = Opcode::iand;
        } else {
        }
        current_function->code.push_back(Instruction{.opcode = opcode});
        return {};
    }

    virtual std::any visitCallExpr(GOatLANGParser::CallExprContext* ctx) override
    {
        auto primary_expr = ctx->primaryExpr();
        if (auto arguments = ctx->arguments(); arguments) {
            visitArguments(arguments);
        }
        auto& code = current_function->code;
        if (auto operand = dynamic_cast<GOatLANGParser::Operand_Context*>(primary_expr); operand) {
            // invoke native here as well, but whatever, we shall not change it right now
            auto name = operand->operand()->operandName()->IDENTIFIER()->getText();
            auto function_index = function_indices.at(name);
            code.push_back(Instruction{.opcode = Opcode::invoke_static, .index = function_index});
            return {};
        }
        visitPrimaryExpr(primary_expr);
        code.push_back(Instruction{.opcode = Opcode::invoke_dynamic});
        return {};
    }
};

#endif
