//
// Created by Aleksandr Govenko on 16/12/2023.
//

#include <iostream>
#include "ast.h"

Variable::Variable(char name) : name(name) {}

void Variable::GetNames(std::set<char> &names) const {
    names.insert(name);
}

void Variable::Print(std::ostream &os) const {
    os << name;
}

Constant::Constant(int value) : value(value) {}

void Constant::GetNames(std::set<char> &names) const {}

void Constant::Print(std::ostream &os) const {
    os << value;
}

BinaryExpression::BinaryExpression(std::shared_ptr<Expression> left, char operation, std::shared_ptr<Expression> right)
        : left(std::move(left)), right(std::move(right)), operation(operation) {}

void BinaryExpression::GetNames(std::set<char> &names) const {
    left->GetNames(names);
    right->GetNames(names);
}

void BinaryExpression::Print(std::ostream &os) const {
    os << *left << ' ' << operation << ' ' << *right;
}

PriorityExpression::PriorityExpression(std::shared_ptr<Expression> expression) : expression(std::move(expression)) {}

void PriorityExpression::GetNames(std::set<char> &names) const {
    expression->GetNames(names);
}

void PriorityExpression::Print(std::ostream &os) const {
    os << '(' << *expression << ')';
}

Assignment::Assignment(std::shared_ptr<Variable> variable, std::shared_ptr<Expression> expression)
        : variable(std::move(variable)), expression(std::move(expression)) {}

void Assignment::Print(std::ostream &os) const {
    os << *variable << " = " << *expression;
}

void Assignment::Accept(StatementVisitor &visitor) {
    visitor.Visit(*this);
}

IfStatement::IfStatement(std::shared_ptr<Expression> condition, std::shared_ptr<StatementList> body)
        : condition(std::move(condition)), body(std::move(body)) {}

void IfStatement::Print(std::ostream &os) const {
    os << "if " << *condition << '\n';
    for (const auto &stmt: *body) {
        os << "  " << *stmt << '\n';
    }
    os << "end";
}

void IfStatement::Accept(StatementVisitor &visitor) {
    visitor.Visit(*this);
}

WhileStatement::WhileStatement(std::shared_ptr<Expression> condition, std::shared_ptr<StatementList> body)
        : condition(std::move(condition)), body(std::move(body)) {}

void WhileStatement::Print(std::ostream &os) const {
    os << "while " << *condition << '\n';
    for (const auto &stmt: *body) {
        os << "  " << *stmt << '\n';
    }
    os << "end";
}

void WhileStatement::Accept(StatementVisitor &visitor) {
    visitor.Visit(*this);
}

Program::Program(std::shared_ptr<StatementList> statements) : statements(std::move(statements)) {}

std::ostream &operator<<(std::ostream &os, const Statement &statement) {
    statement.Print(os);
    return os;
}

std::ostream &operator<<(std::ostream &os, const StatementList &list) {
    for (const auto &stmt: list) {
        os << *stmt << '\n';
    }
    return os;
}

std::ostream &operator<<(std::ostream &os, const Expression &expression) {
    expression.Print(os);
    return os;
}

std::ostream &operator<<(std::ostream &os, const Program &program) {
    os << *program.statements;
}
