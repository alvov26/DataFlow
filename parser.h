//
// Created by Aleksandr Govenko on 13/12/2023.
//

#pragma once

#include <utility>

#include "tokens.h"
#include "ast.h"

class Parser {
    std::istream &in_;
    std::optional<Token> current_token_;

    template<class TokenType>
    bool Peek(TokenType &t) {
        if (!current_token_.has_value()) return false;
        auto *token = std::get_if<TokenType>(&*current_token_);
        if (token != nullptr) {
            t = *token;
            return true;
        }
        return false;
    }

    template<class TokenType>
    bool Accept(TokenType &t) {
        if (!current_token_.has_value()) return false;
        auto *token = std::get_if<TokenType>(&*current_token_);
        if (token != nullptr) {
            t = *token;
            NextToken();
            return true;
        }
        return false;
    }

    template<class TokenType>
    TokenType Expect() {
        TokenType t;
        if (Accept<TokenType>(t) == false) {
            throw std::runtime_error("Expected token type mismatch");
        }
        return t;
    }

    void NextToken() {
        current_token_ = ::NextToken(in_);
    }

public:
    explicit Parser(std::istream &in) : in_(in) {
        NextToken();
    }

    std::shared_ptr<Program> ParseProgram() {
        return std::make_shared<Program>(ParseStatementList());
    }

    std::shared_ptr<StatementList> ParseStatementList() {
        auto stmt = ParseStatement();
        if (stmt == nullptr) {
            throw std::runtime_error("Expected statement");
        }
        std::vector<std::shared_ptr<Statement>> statements = {stmt};
        while (auto stmt2 = ParseStatement()) {
            statements.push_back(stmt2);
        }
        return std::make_shared<StatementList>(statements);
    }

    std::shared_ptr<Statement> ParseStatement() {
        if (NameToken token; Accept(token)) {
            Expect<AssignToken>();
            auto expr = ParseExpression();
            return std::make_shared<Assignment>(std::make_shared<Variable>(token.name), expr);
        }
        if (IfToken token; Accept(token)) {
            auto condition = ParseExpression();
            auto body = ParseStatementList();
            Expect<EndToken>();
            return std::make_shared<IfStatement>(condition, body);
        }
        if (WhileToken token; Accept(token)) {
            auto condition = ParseExpression();
            auto body = ParseStatementList();
            Expect<EndToken>();
            return std::make_shared<WhileStatement>(condition, body);
        }
        return nullptr;
    }

    std::shared_ptr<Expression> ParseExpression(int min_precedence = 0) {
        std::shared_ptr<Expression> expr;
        if (ConstantToken ct; Accept(ct)) {
            expr = std::make_shared<Constant>(ct.value);
        } else if (NameToken nt; Accept(nt)) {
            expr = std::make_shared<Variable>(nt.name);
        } else if (OpenParenToken pt; Accept(pt)) {
            expr = std::make_shared<PriorityExpression>(ParseExpression());
        }
        if (CloseParenToken pt; Accept(pt)) {
            return expr;
        }
        for (OperatorToken token; Peek<OperatorToken>(token) && token.precedence >= min_precedence;) {
            NextToken();
            auto right = ParseExpression(token.precedence + 1);
            expr = std::make_shared<BinaryExpression>(expr, token.op, right);
        }
        return expr;
    }
};
