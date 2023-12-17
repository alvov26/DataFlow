//
// Created by Aleksandr Govenko on 17/12/2023.
//

#include <iostream>
#include "tokens.h"

std::optional<Token> NextToken(std::istream& in_) {
    char c;
    in_ >> c;
    switch (c) {
        case '<':
        case '>':
            return OperatorToken{.precedence = 1, .op = c};
        case '+':
        case '-':
            return OperatorToken{.precedence = 2, .op = c};
        case '*':
        case '/':
            return OperatorToken{.precedence = 3, .op = c};
        case '=':
            return AssignToken{};
        case '(':
            return OpenParenToken{};
        case ')':
            return CloseParenToken{};
        default:
            break;
    }
    if (std::isdigit(c)) {
        int value = 0;
        do {
            value = value * 10 + (c - '0');
            c = in_.get();
        } while (std::isdigit(c));
        in_.unget();
        return ConstantToken{value};
    }
    if (c >= 'a' && c <= 'z') {
        std::string name;
        do {
            name.push_back(c);
            c = in_.get();
        } while (c >= 'a' && c <= 'z');
        in_.unget();
        if (name == "if") return IfToken{};
        if (name == "while") return WhileToken{};
        if (name == "end") return EndToken{};
        return NameToken{name.front()};
    }
    return {};
}
