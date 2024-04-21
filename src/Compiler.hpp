#ifndef COMPILER_HPP
#define COMPILER_HPP

#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "Code.hpp"
#include "Native.hpp"
#include "StringPool.hpp"

#include "GOatLANGBaseVisitor.h"

class FunctionScanner : public GOatLANGBaseVisitor
{
public:
    std::vector<Function>& function_table;
    std::unordered_map<std::string, u64>& function_indices;
    std::unordered_map<void*, u64>& node_functions;

    u64 current_function_index;

    FunctionScanner(
        std::vector<Function>& function_table,
        std::unordered_map<std::string, u64>& function_indices,
        std::unordered_map<void*, u64>& node_functions) : function_table{function_table},
                                                          function_indices{function_indices},
                                                          node_functions{node_functions},
                                                          current_function_index{}
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
        current_function_index = function_table.size();
        function_table.emplace_back(Function{.index = current_function_index});
        function_indices.try_emplace(ctx->IDENTIFIER()->getText(), current_function_index);
        node_functions.try_emplace(ctx, current_function_index);
        visitFunction(ctx->function());
        return {};
    }

    virtual std::any visitFunctionLit(GOatLANGParser::FunctionLitContext* ctx) override
    {
        current_function_index = function_table.size();
        function_table.emplace_back(Function{.index = current_function_index});
        node_functions.try_emplace(ctx, current_function_index);
        visitFunction(ctx->function());
        return {};
    }

    virtual std::any visitFunction(GOatLANGParser::FunctionContext* ctx) override
    {
        node_functions.try_emplace(ctx, current_function_index);
        return visitChildren(ctx);
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
    std::vector<Function>& function_table;
    std::unordered_map<std::string, u64>& function_indices;
    std::unordered_map<void*, u64>& node_functions;
    std::unordered_map<std::string, u64>& native_function_indices;
    std::unordered_map<void*, VariableFrame>& variable_frames;
    VariableFrame* current_frame;

    VariableAnalyzer(
        std::vector<Function>& function_table,
        std::unordered_map<std::string, u64>& function_indices,
        std::unordered_map<void*, u64>& node_functions,
        std::unordered_map<std::string, u64>& native_function_indices,
        std::unordered_map<void*, VariableFrame>& variable_frames)
        : function_table{function_table},
          function_indices{function_indices},
          node_functions{node_functions},
          native_function_indices{native_function_indices},
          variable_frames{variable_frames},
          current_frame{nullptr}
    {
    }

    void analyze_reference(antlr4::tree::TerminalNode* identifier)
    {
        auto name = identifier->getText();
        if (function_indices.contains(name) || native_function_indices.contains(name)) {
            return;
        }
        auto& current_variables = current_frame->variables;
        if (current_variables.contains(name)) {
            return;
        }
        auto [it, _] = current_variables.try_emplace(
            std::move(name),
            Variable{.category = VariableCategory::free});
        current_frame->captures.push_back(&*it);
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

        u64 function_index = node_functions.at(ctx);
        Function& function = function_table[function_index];
        function.capc = current_frame->captures.size();
        function.argc = current_frame->parameters.size();
        function.varc = current_frame->variables.size();
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
        auto identifier = ctx->IDENTIFIER();
        auto [it, _] = current_frame->variables.try_emplace(
            identifier->getText(),
            Variable{.category = VariableCategory::bound});
        current_frame->locals.push_back(&*it);
        return visitChildren(ctx);
    }

    virtual std::any visitSendStmt(GOatLANGParser::SendStmtContext* ctx) override
    {
        analyze_reference(ctx->IDENTIFIER());
        return visitChildren(ctx);
    }

    virtual std::any visitAssignmentStmt(GOatLANGParser::AssignmentStmtContext* ctx) override
    {
        analyze_reference(ctx->IDENTIFIER());
        return visitChildren(ctx);
    }

    virtual std::any visitOperandName(GOatLANGParser::OperandNameContext* ctx) override
    {
        analyze_reference(ctx->IDENTIFIER());
        return visitChildren(ctx);
    }
};

class TypeAnnotator : public GOatLANGBaseVisitor
{
    std::vector<std::unique_ptr<Type>>& type_table;
    std::unordered_map<std::string, Type*>& type_names;
    std::unordered_map<void*, Type*>& node_types;
    std::unordered_map<void*, VariableFrame>& variable_frames;

    std::vector<std::unordered_map<std::string, Type*>> type_environment;
    std::vector<Type*> arg_types;
    std::string* function_name;
    bool in_function_type = false;

    template <typename T>
    Type* register_type(const T& type)
    {
        std::string name = type.get_name();
        auto it = type_names.find(name);
        if (it != type_names.end()) {
            return it->second;
        }
        u64 type_index = type_table.size();
        auto smart_ptr = std::unique_ptr<Type>(new T{type});
        auto raw_ptr = smart_ptr.get();
        raw_ptr->index = type_index;
        type_table.push_back(std::move(smart_ptr));
        type_names.try_emplace(name, raw_ptr);
        return raw_ptr;
    }

    Type* lookup(const std::string& name)
    {
        for (auto it = type_environment.rbegin(); it != type_environment.rend(); ++it) {
            auto& frame = *it;
            if (frame.contains(name)) {
                return frame[name];
            }
        }
        throw std::runtime_error("lookup: cannot find '" + name + "'");
    }

    Type* wrap_callable_type(Type* type)
    {
        auto function_type = dynamic_cast<FunctionType*>(type);
        if (function_type) {
            return register_type(CallableType{function_type});
        }
        return type;
    }

public:
    TypeAnnotator(
        std::vector<std::unique_ptr<Type>>& type_table,
        std::unordered_map<std::string, Type*>& type_names,
        std::unordered_map<void*, Type*>& node_types,
        std::unordered_map<void*, VariableFrame>& variable_frames)
        : type_table{type_table},
          type_names{type_names},
          node_types{node_types},
          variable_frames{variable_frames},
          type_environment{},
          arg_types{},
          function_name{nullptr}
    {
        auto& top_level_frame = type_environment.emplace_back();
        auto new_chan_type = register_type(FunctionType{{type_names.at("int")}, type_names.at("chan")});
        auto sprint_type = register_type(FunctionType{{type_names.at("string")}, nullptr});
        auto iprint_type = register_type(FunctionType{{type_names.at("int")}, nullptr});
        auto fprint_type = register_type(FunctionType{{type_names.at("float")}, nullptr});
        top_level_frame.try_emplace("make", new_chan_type);
        top_level_frame.try_emplace("sprint", sprint_type);
        top_level_frame.try_emplace("iprint", iprint_type);
        top_level_frame.try_emplace("fprint", fprint_type);
    }

    std::any visitExpression(GOatLANGParser::ExpressionContext* ctx)
    {
        return ctx->accept(this);
    }

    std::any visitPrimaryExpr(GOatLANGParser::PrimaryExprContext* ctx)
    {
        return ctx->accept(this);
    }

    virtual std::any visitPrimaryExpr_(GOatLANGParser::PrimaryExpr_Context* ctx) override
    {
        auto primary_expr = ctx->primaryExpr();
        visitPrimaryExpr(primary_expr);
        auto type = node_types.at(primary_expr);
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitFunctionDecl(GOatLANGParser::FunctionDeclContext* ctx) override
    {
        auto name = ctx->IDENTIFIER()->getText();
        function_name = &name;
        auto function = ctx->function();
        visitFunction(function);
        auto type = node_types.at(function);
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitFunction(GOatLANGParser::FunctionContext* ctx) override
    {
        auto env_index = type_environment.size() - 1;
        type_environment.emplace_back();
        auto signature = ctx->signature();
        visitSignature(signature);
        auto type = node_types.at(signature);
        if (function_name) {
            type_environment.at(env_index).try_emplace(*function_name, type);
        }
        node_types.try_emplace(ctx, type);
        visitBlock(ctx->block());
        type_environment.pop_back();
        return {};
    }

    virtual std::any visitSignature(GOatLANGParser::SignatureContext* ctx) override
    {
        arg_types.clear();
        visitParameters(ctx->parameters());
        Type* result_type = nullptr;
        if (auto result = ctx->result(); result) {
            visitResult(result);
            result_type = node_types.at(result);
        }
        auto function_type = FunctionType{arg_types, result_type};
        auto type = register_type(function_type);
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitParameterDecl(GOatLANGParser::ParameterDeclContext* ctx) override
    {
        auto go_type = ctx->goType();
        visitGoType(go_type);
        auto type = wrap_callable_type(node_types.at(go_type));
        arg_types.push_back(type);
        node_types.try_emplace(ctx, type);
        if (in_function_type) {
            return {};
        }
        if (auto identifier = ctx->IDENTIFIER(); identifier) {
            auto& type_frame = type_environment[type_environment.size() - 1];
            auto name = identifier->getText();
            type_frame.try_emplace(name, type);
        }
        return {};
    }

    virtual std::any visitResult(GOatLANGParser::ResultContext* ctx) override
    {
        auto go_type = ctx->goType();
        visitGoType(go_type);
        auto type = wrap_callable_type(node_types.at(go_type));
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitGoType(GOatLANGParser::GoTypeContext* ctx) override
    {
        Type* type = nullptr;
        if (auto type_name = ctx->typeName(); type_name) {
            visitTypeName(type_name);
            type = node_types.at(type_name);
        } else if (auto type_lit = ctx->typeLit(); type_lit) {
            visitTypeLit(type_lit);
            type = node_types.at(type_lit);
        } else if (auto go_type = ctx->goType(); go_type) {
            visitGoType(go_type);
            type = node_types.at(go_type);
        }
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitTypeLit(GOatLANGParser::TypeLitContext* ctx) override
    {
        Type* type = nullptr;
        if (auto struct_type = ctx->structType(); struct_type) {
        } else if (auto pointer_type = ctx->pointerType(); pointer_type) {
        } else if (auto function_type = ctx->functionType(); function_type) {
            visitFunctionType(function_type);
            type = node_types.at(function_type);
        } else if (auto slice_type = ctx->sliceType(); slice_type) {
        } else if (auto channel_type = ctx->channelType(); channel_type) {
            visitChannelType(channel_type);
            type = node_types.at(channel_type);
        }
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitFunctionType(GOatLANGParser::FunctionTypeContext* ctx) override
    {
        in_function_type = true;
        auto signature = ctx->signature();
        visitSignature(signature);
        auto type = node_types.at(signature);
        node_types.try_emplace(ctx, type);
        in_function_type = false;
        return {};
    }

    virtual std::any visitChannelType(GOatLANGParser::ChannelTypeContext* ctx)
    {
        auto go_type = ctx->goType();
        visitGoType(go_type);
        auto element_type = node_types.at(go_type);
        auto channel_type = ChannelType{element_type};
        auto type = register_type(channel_type);
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitTypeName(GOatLANGParser::TypeNameContext* ctx) override
    {
        auto name = ctx->IDENTIFIER()->getText();
        auto type = type_names.at(name);
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitVarSpec(GOatLANGParser::VarSpecContext* ctx) override
    {
        auto name = ctx->IDENTIFIER()->getText();
        auto go_type = ctx->goType();
        visitGoType(go_type);
        auto type = wrap_callable_type(node_types.at(go_type));
        node_types.try_emplace(ctx, type);
        type_environment[type_environment.size() - 1].try_emplace(name, type);
        if (auto expression = ctx->expression(); expression) {
            visitExpression(expression);
        }
        return {};
    }

    virtual std::any visitLiteral(GOatLANGParser::LiteralContext* ctx) override
    {
        Type* type = nullptr;
        if (auto basic_lit = ctx->basicLit(); basic_lit) {
            visitBasicLit(basic_lit);
            type = node_types.at(basic_lit);
        } else if (auto composite_lit = ctx->compositeLit(); composite_lit) {
            visitCompositeLit(composite_lit);
            type = node_types.at(composite_lit);
        } else if (auto function_lit = ctx->functionLit(); function_lit) {
            visitFunctionLit(function_lit);
            type = node_types.at(function_lit);
        }
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitBasicLit(GOatLANGParser::BasicLitContext* ctx) override
    {
        Type* type = nullptr;
        if (auto int_lit = ctx->INT_LIT(); int_lit) {
            type = type_names.at("int");
        } else if (auto float_lit = ctx->FLOAT_LIT(); float_lit) {
            type = type_names.at("float");
        } else if (auto string_lit = ctx->STRING_LIT(); string_lit) {
            type = type_names.at("string");
        }
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitFunctionLit(GOatLANGParser::FunctionLitContext* ctx) override
    {
        function_name = nullptr;
        auto function = ctx->function();
        visitFunction(function);
        auto function_type = dynamic_cast<FunctionType*>(node_types.at(function));
        if (!function_type) {
            throw std::runtime_error("function lit: is not function type");
        }
        auto& variable_frame = variable_frames.at(function);
        u64 capc = variable_frame.captures.size();
        auto closure_type = ClosureType{function_type, capc};
        auto type = register_type(closure_type);
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitOperand(GOatLANGParser::OperandContext* ctx) override
    {
        Type* type;
        if (auto literal = ctx->literal(); literal) {
            visitLiteral(literal);
            type = node_types.at(literal);
        } else if (auto operand_name = ctx->operandName(); operand_name) {
            visitOperandName(operand_name);
            type = node_types.at(operand_name);
        } else if (auto expression = ctx->expression(); expression) {
            visitExpression(expression);
            type = node_types.at(expression);
        }
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitOperandName(GOatLANGParser::OperandNameContext* ctx) override
    {
        auto name = ctx->IDENTIFIER()->getText();
        auto type = lookup(name);
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitOperand_(GOatLANGParser::Operand_Context* ctx) override
    {
        auto operand = ctx->operand();
        visitOperand(operand);
        auto type = node_types.at(operand);
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitCallExpr(GOatLANGParser::CallExprContext* ctx) override
    {
        auto primary_expr = ctx->primaryExpr();
        visitPrimaryExpr(primary_expr);
        auto type = node_types.at(primary_expr);
        FunctionType* function_type = nullptr;
        if (auto closure_type = dynamic_cast<ClosureType*>(type); closure_type) {
            function_type = closure_type->function_type;
        } else if (auto callable_type = dynamic_cast<CallableType*>(type); callable_type) {
            function_type = callable_type->function_type;
        } else {
            function_type = dynamic_cast<FunctionType*>(type);
        }
        if (!function_type) {
            throw std::runtime_error("call expr: operand is not a callable");
        }
        type = function_type->return_type;
        node_types.try_emplace(ctx, type);
        if (auto arguments = ctx->arguments(); arguments) {
            visitArguments(ctx->arguments());
        }
        return {};
    }

    virtual std::any visitBinaryExpr(GOatLANGParser::BinaryExprContext* ctx) override
    {
        auto binary_op = ctx->binary_op->getText();
        auto left = ctx->expression(0);
        auto right = ctx->expression(1);

        visitExpression(left);
        visitExpression(right);

        auto left_type = node_types.at(left);
        auto right_type = node_types.at(right);

        if (left_type != right_type) {
            std::string error_msg = "binary expr: operands have different types";
            error_msg += "\n    left type: " + (left_type ? left_type->get_name() : "void");
            error_msg += "\n    right type: " + (right_type ? right_type->get_name() : "void");
            throw std::runtime_error(std::move(error_msg));
        }

        Type* type = nullptr;
        if (binary_op == "||") {
            type = type_names.at("bool");
        } else if (binary_op == "&&") {
            type = type_names.at("bool");
        } else if (binary_op == "==") {
            type = type_names.at("bool");
        } else if (binary_op == "!=") {
            type = type_names.at("bool");
        } else if (binary_op == "<") {
            type = type_names.at("bool");
        } else if (binary_op == "<=") {
            type = type_names.at("bool");
        } else if (binary_op == ">") {
            type = type_names.at("bool");
        } else if (binary_op == ">=") {
            type = type_names.at("bool");
        } else if (binary_op == "|") {
            type = left_type;
        } else if (binary_op == "^") {
            type = left_type;
        } else if (binary_op == "%") {
            type = type_names.at("int");
        } else if (binary_op == "<<") {
            type = type_names.at("int");
        } else if (binary_op == ">>") {
            type = type_names.at("int");
        } else if (binary_op == "&") {
            type = left_type;
        } else if (binary_op == "+") {
            type = left_type;
        } else if (binary_op == "-") {
            type = left_type;
        } else if (binary_op == "*") {
            type = left_type;
        } else if (binary_op == "/") {
            type = left_type;
        } else {
        }
        node_types.try_emplace(ctx, type);
        return {};
    }

    virtual std::any visitUnaryExpr(GOatLANGParser::UnaryExprContext* ctx) override
    {
        auto unary_op = ctx->unary_op->getText();
        if (unary_op == "&") {
            // special case, special visitor
            return {};
        }

        auto expression = ctx->expression();
        visitExpression(expression);
        auto expression_type = node_types.at(expression);

        Type* type = nullptr;
        if (unary_op == "<-") {
            auto channel_type = dynamic_cast<ChannelType*>(expression_type);
            if (!channel_type) {
                throw new std::runtime_error("unary expr: operand is not a channel");
            }
            type = channel_type->element_type;
        } else if (unary_op == "&") {
        } else if (unary_op == "*") {
        } else if (unary_op == "+") {
            type = expression_type;
        } else if (unary_op == "-") {
            type = expression_type;
        } else if (unary_op == "!") {
            type = type_names.at("bool");
        } else if (unary_op == "^") {
            type = type_names.at("int");
        } else {
            // error
        }
        node_types.try_emplace(ctx, type);
        return {};
    }
};

class FunctionResolver : public GOatLANGBaseVisitor
{
    std::vector<Function>& function_table;
    std::unordered_map<std::string, u64>& function_indices;
    std::unordered_map<void*, u64>& node_functions;
    std::unordered_map<std::string, u64>& native_function_indices;
    std::unordered_map<void*, u64>& node_native_functions;

public:
    FunctionResolver(
        std::vector<Function>& function_table,
        std::unordered_map<std::string, u64>& function_indices,
        std::unordered_map<void*, u64>& node_functions,
        std::unordered_map<std::string, u64>& native_function_indices,
        std::unordered_map<void*, u64>& node_native_functions)
        : function_table{function_table},
          function_indices{function_indices},
          node_functions{node_functions},
          native_function_indices{native_function_indices},
          node_native_functions{node_native_functions}
    {
    }

    std::any visitPrimaryExpr(GOatLANGParser::PrimaryExprContext* ctx)
    {
        return ctx->accept(this);
    }

    virtual std::any visitOperand_(GOatLANGParser::Operand_Context* ctx) override
    {
        auto operand = ctx->operand();
        visitOperand(operand);
        if (node_functions.contains(operand)) {
            node_functions.try_emplace(ctx, node_functions.at(operand));
        } else if (node_native_functions.contains(operand)) {
            node_native_functions.try_emplace(ctx, node_native_functions.at(operand));
        }
        return {};
    }

    virtual std::any visitOperand(GOatLANGParser::OperandContext* ctx) override
    {
        if (auto operand_name = ctx->operandName(); operand_name) {
            visitOperandName(operand_name);
            if (node_functions.contains(operand_name)) {
                node_functions.try_emplace(ctx, node_functions.at(operand_name));
            } else if (node_native_functions.contains(operand_name)) {
                node_native_functions.try_emplace(ctx, node_native_functions.at(operand_name));
            }
        }
        return {};
    }

    virtual std::any visitOperandName(GOatLANGParser::OperandNameContext* ctx) override
    {
        auto name = ctx->IDENTIFIER()->getText();
        if (function_indices.contains(name)) {
            u64 function_index = function_indices.at(name);
            node_functions.try_emplace(ctx, function_index);
        } else if (native_function_indices.contains(name)) {
            u64 native_function_index = native_function_indices.at(name);
            node_native_functions.try_emplace(ctx, native_function_index);
        }
        return {};
    }
};

struct FunctionContext
{
    bool has_return_stmt = false;
    std::unordered_map<std::string, u64> labels;
    VariableFrame& variable_frame;
};

class Compiler : public GOatLANGBaseVisitor
{
public:
    static constexpr u64 new_thread_index = 0;
    static constexpr u64 new_chan_index = 1;
    static constexpr u64 chan_send_index = 2;
    static constexpr u64 chan_recv_index = 3;
    static constexpr u64 sprint_index = 4;
    static constexpr u64 iprint_index = 5;
    static constexpr u64 fprint_index = 6;

    std::vector<Function> function_table;
    std::unordered_map<std::string, u64> function_indices;
    std::unordered_map<void*, u64> node_functions;
    Function* current_function = nullptr;
    FunctionContext* current_function_context = nullptr;

    std::vector<NativeFunction> native_function_table;
    std::unordered_map<std::string, u64> native_function_indices;
    std::unordered_map<void*, u64> node_native_functions;

    std::unordered_map<void*, VariableFrame> variable_frames;
    std::vector<std::unique_ptr<Type>> type_table;
    std::unordered_map<std::string, Type*> type_names;
    std::unordered_map<void*, Type*> node_types;

    StringPool string_pool;

    template <typename T>
    Type* register_type(const T& type)
    {
        std::string name = type.get_name();
        auto it = type_names.find(name);
        if (it != type_names.end()) {
            return it->second;
        }
        u64 type_index = type_table.size();
        auto smart_ptr = std::unique_ptr<Type>(new T{type});
        auto raw_ptr = smart_ptr.get();
        raw_ptr->index = type_index;
        type_table.push_back(std::move(smart_ptr));
        type_names.try_emplace(name, raw_ptr);
        return raw_ptr;
    }

    Compiler()
    {
        register_type(IntType{});
        register_type(FloatType{});
        register_type(BoolType{});
        register_type(StringType{});
        register_type(ChannelType{nullptr});

        native_function_table.push_back(new_thread);
        native_function_table.push_back(new_chan);
        native_function_table.push_back(chan_send);
        native_function_table.push_back(chan_recv);
        native_function_table.push_back(sprint);
        native_function_table.push_back(iprint);
        native_function_table.push_back(fprint);

        native_function_indices.try_emplace("make", new_chan_index);
        native_function_indices.try_emplace("sprint", sprint_index);
        native_function_indices.try_emplace("iprint", iprint_index);
        native_function_indices.try_emplace("fprint", fprint_index);
    }

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
        FunctionScanner scanner{function_table, function_indices, node_functions};
        scanner.visitSourceFile(ctx);
        VariableAnalyzer analyzer{
            function_table,
            function_indices,
            node_functions,
            native_function_indices,
            variable_frames};
        analyzer.visitSourceFile(ctx);
        TypeAnnotator annotator{type_table, type_names, node_types, variable_frames};
        annotator.visitSourceFile(ctx);
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
        visitFunction(ctx->function());
        current_function = saved_function;
        return {};
    }

    virtual std::any visitFunction(GOatLANGParser::FunctionContext* ctx) override
    {
        FunctionContext* saved_function_context = current_function_context;
        FunctionContext new_function_context{
            .has_return_stmt = false,
            .variable_frame = variable_frames.at(ctx),
        };
        current_function_context = &new_function_context;

        visitSignature(ctx->signature());
        visitBlock(ctx->block());

        if (!current_function_context->has_return_stmt) {
            current_function->code.push_back(Instruction{.opcode = Opcode::ret});
        }

        current_function_context = saved_function_context;
        return {};
    }

    virtual std::any visitVarSpec(GOatLANGParser::VarSpecContext* ctx) override
    {
        auto expression = ctx->expression();
        if (!expression) {
            return {};
        }
        auto name = ctx->IDENTIFIER()->getText();
        auto& variable = current_function_context->variable_frame.variables.at(name);
        auto& code = current_function->code;

        if (variable.category != VariableCategory::bound) {
            auto type = node_types.at(ctx);
            code.push_back(Instruction{.opcode = Opcode::new_, .index = type->index});
            code.push_back(Instruction{.opcode = Opcode::dup});
        }

        visitExpression(expression);

        if (variable.category == VariableCategory::bound) {
            code.push_back(Instruction{.opcode = Opcode::store, .index = variable.index});
        } else {
            code.push_back(Instruction{.opcode = Opcode::wstore, .index = 0});
            code.push_back(Instruction{.opcode = Opcode::store, .index = variable.index});
        }
        return {};
    }

    virtual std::any visitExpressionStmt(GOatLANGParser::ExpressionStmtContext* ctx) override
    {
        auto expression = ctx->expression();
        visitExpression(expression);
        auto type = node_types.at(expression);
        if (type) {
            current_function->code.push_back(Instruction{.opcode = Opcode::pop});
        }
        return {};
    }

    virtual std::any visitSendStmt(GOatLANGParser::SendStmtContext* ctx) override
    {
        auto name = ctx->IDENTIFIER()->getText();
        auto& variable = current_function_context->variable_frame.variables.at(name);
        auto& code = current_function->code;

        if (variable.category == VariableCategory::bound) {
            code.push_back(Instruction{.opcode = Opcode::load, .index = variable.index});
        } else {
            code.push_back(Instruction{.opcode = Opcode::load, .index = variable.index});
            code.push_back(Instruction{.opcode = Opcode::wload, .index = 0});
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
        auto name = ctx->IDENTIFIER()->getText();
        auto& variable = current_function_context->variable_frame.variables.at(name);
        auto& code = current_function->code;

        if (variable.category != VariableCategory::bound) {
            code.push_back(Instruction{.opcode = Opcode::load, .index = variable.index});
        }

        visitExpression(ctx->expression());

        if (variable.category == VariableCategory::bound) {
            code.push_back(Instruction{.opcode = Opcode::store, .index = variable.index});
        } else {
            code.push_back(Instruction{.opcode = Opcode::wstore, .index = 0});
        }
        return {};
    }

    virtual std::any visitReturnStmt(GOatLANGParser::ReturnStmtContext* ctx) override
    {
        visitExpression(ctx->expression());
        current_function->code.push_back(Instruction{.opcode = Opcode::ret});
        current_function_context->has_return_stmt = true;
        return {};
    }

    virtual std::any visitIfStmt(GOatLANGParser::IfStmtContext* ctx) override
    {
        visitExpression(ctx->expression());

        auto& code = current_function->code;
        u64 if_f_index = code.size();
        code.push_back(Instruction{.opcode = Opcode::if_f});

        visitBlock(ctx->block(0));

        auto block = ctx->block(1);
        auto if_stmt = ctx->ifStmt();
        u64 goto_index;

        if (block || if_stmt) {
            goto_index = code.size();
            code.push_back(Instruction{.opcode = Opcode::goto_});
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

    virtual std::any visitParameterList(GOatLANGParser::ParameterListContext* ctx) override
    {
        auto parameter_decls = ctx->parameterDecl();
        /* the parameters are visited in reverse order */
        for (auto it = parameter_decls.rbegin(); it != parameter_decls.rend(); ++it) {
            visitParameterDecl(*it);
        }
        return {};
    }

    virtual std::any visitLabeledStmt(GOatLANGParser::LabeledStmtContext* ctx) override
    {
        current_function_context->labels.try_emplace(
            ctx->IDENTIFIER()->getText(),
            current_function->code.size());
        visitStatement(ctx->statement());
        return {};
    }

    virtual std::any visitGotoStmt(GOatLANGParser::GotoStmtContext* ctx) override
    {
        u64 index = current_function_context->labels.at(ctx->IDENTIFIER()->getText());
        current_function->code.push_back(Instruction{.opcode = Opcode::goto_, .index = index});
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
        auto& variable = current_function_context->variable_frame.variables.at(name);

        if (variable.category == VariableCategory::bound) {
            code.push_back(Instruction{.opcode = Opcode::store, .index = variable.index});
        } else { /* escaped */
            auto type = node_types.at(ctx);
            code.push_back(Instruction{.opcode = Opcode::new_, .index = type->index});
            code.push_back(Instruction{.opcode = Opcode::dup});
            code.push_back(Instruction{.opcode = Opcode::store, .index = variable.index});
            code.push_back(Instruction{.opcode = Opcode::swap});
            code.push_back(Instruction{.opcode = Opcode::wstore, .index = 0});
        }
        return {};
    }

    virtual std::any visitGoType(GOatLANGParser::GoTypeContext* ctx) override
    {
        return {};
    }

    virtual std::any visitBasicLit(GOatLANGParser::BasicLitContext* ctx) override
    {
        Word word;
        auto& code = current_function->code;
        if (auto int_lit = ctx->INT_LIT(); int_lit) {
            auto text = int_lit->getText();
            u64 value = std::stoull(text);
            word = bitcast<u64, Word>(value);
            code.push_back(Instruction{.opcode = Opcode::push, .value = word});
        } else if (auto float_lit = ctx->FLOAT_LIT(); float_lit) {
            auto text = float_lit->getText();
            f64 value = std::stod(text);
            word = bitcast<f64, Word>(value);
            code.push_back(Instruction{.opcode = Opcode::push, .value = word});
        } else if (auto string_lit = ctx->STRING_LIT(); string_lit) {
            auto text = string_lit->getText();
            auto type = type_names.at("string");
            u64 string_index = string_pool.new_string(std::move(text));
            code.push_back(Instruction{.opcode = Opcode::new_, .index = type->index});
            code.push_back(Instruction{.opcode = Opcode::dup});
            code.push_back(Instruction{.opcode = Opcode::push, .value = bitcast<u64, Word>(string_index)});
            code.push_back(Instruction{.opcode = Opcode::wstore});
        } else {
        }
        return {};
    }

    virtual std::any visitOperandName(GOatLANGParser::OperandNameContext* ctx) override
    {
        auto& code = current_function->code;
        auto name = ctx->IDENTIFIER()->getText();
        auto type = node_types.at(ctx);
        if (auto function_type = dynamic_cast<FunctionType*>(type); function_type) {
            u64 function_index = function_indices.at(name);
            auto callable_type = register_type(CallableType{function_type});
            auto closure_type = register_type(ClosureType{function_type, 0});
            code.push_back(Instruction{.opcode = Opcode::new_, .index = closure_type->index});
            code.push_back(Instruction{.opcode = Opcode::dup});
            code.push_back(Instruction{.opcode = Opcode::push, .value = bitcast<u64, Word>(function_index)});
            code.push_back(Instruction{.opcode = Opcode::wstore, .index = 0});
            return {};
        }
        auto& variable = current_function_context->variable_frame.variables.at(name);
        if (variable.category == VariableCategory::bound) {
            code.push_back(Instruction{.opcode = Opcode::load, .index = variable.index});
        } else {
            code.push_back(Instruction{.opcode = Opcode::load, .index = variable.index});
            code.push_back(Instruction{.opcode = Opcode::wload, .index = 0});
        }
        return {};
    }

    virtual std::any visitArguments(GOatLANGParser::ArgumentsContext* ctx) override
    {
        if (auto go_type = ctx->goType(); go_type) {
        }
        if (auto expression_list = ctx->expressionList(); expression_list) {
            visitExpressionList(expression_list);
        }
        return {};
    }

    virtual std::any visitGoStmt(GOatLANGParser::GoStmtContext* ctx) override
    {
        auto primary_expr = dynamic_cast<GOatLANGParser::PrimaryExpr_Context*>(ctx->expression());
        auto call_expr = dynamic_cast<GOatLANGParser::CallExprContext*>(primary_expr->primaryExpr());
        if (!call_expr) {
            throw new std::runtime_error("go stmt: expression is not a call expression");
        }
        auto& code = current_function->code;
        if (auto arguments = call_expr->arguments(); arguments) {
            visitArguments(arguments);
        }
        visitPrimaryExpr(call_expr->primaryExpr());
        code.push_back(Instruction{.opcode = Opcode::invoke_native, .index = new_thread_index});
        return {};
    }

    virtual std::any visitFunctionLit(GOatLANGParser::FunctionLitContext* ctx) override
    {
        Function* saved_function = current_function;
        current_function = &function_table[node_functions.at(ctx)];
        u64 function_index = current_function->index;
        auto function_ctx = ctx->function();
        visitFunction(function_ctx);
        current_function = saved_function;

        auto& code = current_function->code;
        auto type = node_types.at(ctx);
        code.push_back(Instruction{.opcode = Opcode::new_, .index = type->index});
        code.push_back(Instruction{.opcode = Opcode::dup});
        code.push_back(Instruction{.opcode = Opcode::push, .value = bitcast<u64, Word>(function_index)});
        code.push_back(Instruction{.opcode = Opcode::wstore, .index = 0});
        u64 slot = 1;

        auto& captures = variable_frames.at(function_ctx).captures;
        auto& current_variables = current_function_context->variable_frame.variables;
        for (auto ptr : captures) {
            u64 index = current_variables.at(ptr->first).index;
            code.push_back(Instruction{.opcode = Opcode::dup});
            code.push_back(Instruction{.opcode = Opcode::load, .index = index});
            code.push_back(Instruction{.opcode = Opcode::wstore, .index = slot});
            slot++;
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
        auto expression_type = node_types.at(expression);
        if (unary_op == "-") {
            opcode = expression_type == type_names.at("int") ? Opcode::ineg : Opcode::fneg;
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
        auto left_type = node_types.at(left);
        auto int_type = type_names.at("int");
        if (binary_op == "==") {
            opcode = left_type == int_type ? Opcode::ieq : Opcode::feq;
        } else if (binary_op == "!=") {
            opcode = left_type == int_type ? Opcode::ine : Opcode::fne;
        } else if (binary_op == "<") {
            opcode = left_type == int_type ? Opcode::ilt : Opcode::flt;
        } else if (binary_op == "<=") {
            opcode = left_type == int_type ? Opcode::ile : Opcode::fle;
        } else if (binary_op == ">") {
            opcode = left_type == int_type ? Opcode::igt : Opcode::fgt;
        } else if (binary_op == ">=") {
            opcode = left_type == int_type ? Opcode::ige : Opcode::fge;
        } else if (binary_op == "+") {
            opcode = left_type == int_type ? Opcode::iadd : Opcode::fadd;
        } else if (binary_op == "-") {
            opcode = left_type == int_type ? Opcode::isub : Opcode::fsub;
        } else if (binary_op == "|") {
            opcode = Opcode::ior;
        } else if (binary_op == "^") {
            opcode = Opcode::ixor;
        } else if (binary_op == "*") {
            opcode = left_type == int_type ? Opcode::imul : Opcode::fmul;
        } else if (binary_op == "/") {
            opcode = left_type == int_type ? Opcode::idiv : Opcode::fdiv;
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
        code.push_back(Instruction{.opcode = opcode});
        return {};
    }

    virtual std::any visitCallExpr(GOatLANGParser::CallExprContext* ctx) override
    {
        auto primary_expr = ctx->primaryExpr();
        auto& code = current_function->code;

        FunctionResolver resolver{
            function_table,
            function_indices,
            node_functions,
            native_function_indices,
            node_native_functions};
        resolver.visitPrimaryExpr(primary_expr);

        if (auto arguments = ctx->arguments(); arguments) {
            visitArguments(arguments);
        }

        if (node_functions.contains(primary_expr)) {
            u64 function_index = node_functions.at(primary_expr);
            code.push_back(Instruction{.opcode = Opcode::invoke_static, .index = function_index});
            return {};
        }

        if (node_native_functions.contains(primary_expr)) {
            auto native_function_index = node_native_functions.at(primary_expr);
            code.push_back(Instruction{.opcode = Opcode::invoke_native, .index = native_function_index});
            return {};
        }

        visitPrimaryExpr(primary_expr);
        code.push_back(Instruction{.opcode = Opcode::invoke_dynamic});
        return {};
    }
};

#endif
