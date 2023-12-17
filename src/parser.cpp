//
// Created by Aleksandr Lvov on 17/12/2023.
//

#include "parser.h"

Parser::Parser(std::istream& in): in_(in) {
    NextToken();
}

std::shared_ptr<Program> Parser::ParseProgram() {
    return std::make_shared<Program>(ParseStatementList());
}

std::shared_ptr<StatementList> Parser::ParseStatementList() {
    const auto stmt = ParseStatement();
    if (stmt == nullptr) {
        throw std::runtime_error("Expected statement");
    }
    std::vector<std::shared_ptr<Statement>> statements = {stmt};
    while (auto stmt2 = ParseStatement()) {
        statements.push_back(stmt2);
    }
    return std::make_shared<StatementList>(statements);
}

std::shared_ptr<Statement> Parser::ParseStatement() {
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

std::shared_ptr<Expression> Parser::ParseExpression(int min_precedence) {
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
