//
// Created by Aleksandr Govenko on 13/12/2023.
//

#pragma once

#include <iosfwd>
#include <string>

struct ConstantToken {
    int value = 0;
};

struct NameToken {
    char name = 'X';
};

struct OperatorToken {
    int precedence = 0;
    char op = 'O';
};

struct OpenParenToken {};
struct CloseParenToken {};
struct AssignToken {};
struct IfToken {};
struct WhileToken {};
struct EndToken {};

using Token = std::variant<
        ConstantToken,
        NameToken,
        OpenParenToken,
        CloseParenToken,
        OperatorToken,
        AssignToken,
        IfToken,
        WhileToken,
        EndToken>;

std::optional<Token> NextToken(std::istream &in_);
