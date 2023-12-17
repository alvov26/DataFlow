//
// Created by Aleksandr Govenko on 13/12/2023.
//

#pragma once

#include "ast.h"
#include "tokens.h"

class Parser {
    std::istream& in_;
    std::optional<Token> current_token_;

    template<class TokenType>
    bool Peek(TokenType& t) {
        if (!current_token_.has_value()) return false;
        auto* token = std::get_if<TokenType>(&*current_token_);
        if (token != nullptr) {
            t = *token;
            return true;
        }
        return false;
    }

    template<class TokenType>
    bool Accept(TokenType& t) {
        if (!current_token_.has_value()) return false;
        auto* token = std::get_if<TokenType>(&*current_token_);
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
    explicit Parser(std::istream& in);

    std::shared_ptr<Program> ParseProgram();

    std::shared_ptr<StatementList> ParseStatementList();

    std::shared_ptr<Statement> ParseStatement();

    std::shared_ptr<Expression> ParseExpression(int min_precedence = 0);
};
