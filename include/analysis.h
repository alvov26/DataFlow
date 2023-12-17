//
// Created by Aleksandr Govenko on 16/12/2023.
//

#pragma once

#include <map>
#include <set>

#include "ast.h"

struct LiveVariableAnalyser : StatementVisitor {
    std::set<char> live_in_succ{};
    std::vector<std::shared_ptr<Statement>> unused{};

    virtual void Analyse(Program &p);

    void Visit(StatementList &sl) override;

    void Visit(Assignment &stmt) override;

    void Visit(IfStatement &stmt) override;

    void Visit(WhileStatement &stmt) override;
};

struct WriteNamesCollector : StatementVisitor {
    std::set<char> names{};

    void Visit(StatementList &sl) override;

    void Visit(Assignment &stmt) override;

    void Visit(IfStatement &stmt) override;

    void Visit(WhileStatement &stmt) override;
};

struct PossibleValueAnalyzer : StatementVisitor {
    constexpr static int kMaxCombinationCount = 32;
    constexpr static int kMaxDepth = 32;

    std::map<char, std::set<int>> possible_values{};
    std::vector<std::shared_ptr<Statement>> never_happens{};
    std::vector<std::shared_ptr<Statement>> always_happens{};

    virtual void Analyse(Program &p);

    void Visit(StatementList &sl) override;

    void EvalExpr(Expression& expr, std::set<int>& values);

    void Visit(Assignment &assignment) override;

    void Visit(IfStatement &if_statement) override;

    void Visit(WhileStatement &while_statement) override;

    void Visit(WhileStatement &while_statement, int depth);
};

struct AssignmentCollector : StatementVisitor {
    std::vector<std::shared_ptr<Statement>> assignments{};

    void Visit(StatementList &sl) override;

    void Visit(Assignment &stmt) override;

    void Visit(IfStatement &stmt) override;

    void Visit(WhileStatement &stmt) override;
};

struct MixedAnalyser : LiveVariableAnalyser {
    PossibleValueAnalyzer possible_value_analyzer{};

    void Analyse(Program &p) override;

    void Visit(IfStatement &stmt) override;

    void Visit(WhileStatement &stmt) override;
};