//
// Created by Aleksandr Govenko on 13/12/2023.
//

#pragma once

#include <iosfwd>
#include <utility>
#include <set>
#include <map>

struct StatementVisitor;

struct Statement : std::enable_shared_from_this<Statement> {
    virtual void Print(std::ostream &os) const = 0;

    virtual void Accept(StatementVisitor &visitor) = 0;

    virtual ~Statement() = default;
};

using StatementList = std::vector<std::shared_ptr<Statement>>;

std::ostream &operator<<(std::ostream &os, const Statement &statement);

std::ostream &operator<<(std::ostream &os, const StatementList &statement);

struct Expression {
    virtual void GetNames(std::set<char> &names) const = 0;

    virtual std::shared_ptr<Expression> Evaluate(std::map<char, int> &variables) = 0;

    virtual void Print(std::ostream &os) const = 0;

    virtual ~Expression() = default;
};

std::ostream &operator<<(std::ostream &os, const Expression &expression);

struct Variable : Expression {
    char name;

    explicit Variable(char name);

    void GetNames(std::set<char> &names) const override;

    std::shared_ptr<Expression> Evaluate(std::map<char, int> &variables) override;

    void Print(std::ostream &os) const override;
};

struct Constant : Expression {
    int value;

    explicit Constant(int value);

    void GetNames(std::set<char> &names) const override;

    std::shared_ptr<Expression> Evaluate(std::map<char, int> &variables) override;

    void Print(std::ostream &os) const override;
};

struct BinaryExpression : Expression {
    std::shared_ptr<Expression> left;
    std::shared_ptr<Expression> right;
    char operation;

    BinaryExpression(std::shared_ptr<Expression> left, char operation, std::shared_ptr<Expression> right);

    void GetNames(std::set<char> &names) const override;

    std::shared_ptr<Expression> Evaluate(std::map<char, int> &variables) override;

    void Print(std::ostream &os) const override;
};

struct PriorityExpression : Expression {
    std::shared_ptr<Expression> expression;

    explicit PriorityExpression(std::shared_ptr<Expression> expression);

    void GetNames(std::set<char> &names) const override;

    std::shared_ptr<Expression> Evaluate(std::map<char, int> &variables) override;

    void Print(std::ostream &os) const override;
};

struct Assignment : Statement {
    std::shared_ptr<Variable> variable;
    std::shared_ptr<Expression> expression;

    Assignment(std::shared_ptr<Variable> variable, std::shared_ptr<Expression> expression);

    void Print(std::ostream &os) const override;

    void Accept(StatementVisitor &visitor) override;
};

struct IfStatement : Statement {
    std::shared_ptr<Expression> condition;
    std::shared_ptr<StatementList> body;

    IfStatement(std::shared_ptr<Expression> condition, std::shared_ptr<StatementList> body);

    void Print(std::ostream &os) const override;

    void Accept(StatementVisitor &visitor) override;
};

struct WhileStatement : Statement {
    std::shared_ptr<Expression> condition;
    std::shared_ptr<StatementList> body;

    WhileStatement(std::shared_ptr<Expression> condition, std::shared_ptr<StatementList> body);

    void Print(std::ostream &os) const override;

    void Accept(StatementVisitor &visitor) override;
};

struct Program {
    std::shared_ptr<StatementList> statements;

    explicit Program(std::shared_ptr<StatementList> statements);
};

std::ostream &operator<<(std::ostream &os, const Program &program);

struct StatementVisitor {
    virtual void Visit(Assignment &assignment) = 0;

    virtual void Visit(IfStatement &if_statement) = 0;

    virtual void Visit(WhileStatement &while_statement) = 0;

    virtual ~StatementVisitor() = default;
};