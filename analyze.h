//
// Created by Aleksandr Govenko on 16/12/2023.
//

#pragma once

#include <algorithm>
#include <ranges>
#include <set>
#include <map>

#include "ast.h"

struct LiveVariableAnalyser : StatementVisitor {
    std::set<char> live_in_succ{};
    std::vector<std::shared_ptr<Statement>> unused{};

    virtual void Analyse(Program &p) {
        Visit(*p.statements);
    }

    void Visit(StatementList &sl) override {
        for (const auto &stmt: sl | std::views::reverse) {
            stmt->Accept(*this);
        }
    }

    void Visit(Assignment &stmt) override {
        std::set<char> read{};
        std::set<char> live_in{};
        std::set<char> &live_out = live_in_succ;

        char write_name = stmt.variable->name;
        stmt.expression->GetNames(read);

        for (auto it: live_out) {
            if (it != write_name) { live_in.insert(it); }
        }
        for (auto it: read) { live_in.insert(it); }

        if (not live_out.contains(write_name)) {
            std::shared_ptr<Statement> ptr = stmt.shared_from_this();
            unused.push_back(ptr);
        }

        live_in_succ = std::move(live_in);
    }

    void Visit(IfStatement &stmt) override {
        LiveVariableAnalyser inner_analyser;
        inner_analyser.live_in_succ = live_in_succ;

        std::set<char> read{};
        stmt.condition->GetNames(read);

        inner_analyser.Visit(*stmt.body);

        live_in_succ.merge(read);
        live_in_succ.merge(inner_analyser.live_in_succ);
        unused.insert(unused.end(), inner_analyser.unused.begin(), inner_analyser.unused.end());
    }

    void Visit(WhileStatement &stmt) override {
        LiveVariableAnalyser inner_analyser;
        inner_analyser.live_in_succ = live_in_succ;

        std::set<char> read{};
        stmt.condition->GetNames(read);

        inner_analyser.Visit(*stmt.body);
        inner_analyser.unused.clear();
        inner_analyser.Visit(*stmt.body);

        live_in_succ.merge(read);
        live_in_succ.merge(inner_analyser.live_in_succ);
        unused.insert(unused.end(), inner_analyser.unused.begin(), inner_analyser.unused.end());
    }
};

struct WriteNamesCollector : StatementVisitor {
    std::set<char> names{};

    void Visit(StatementList &sl) {
        for (const auto &stmt: sl) {
            stmt->Accept(*this);
        }
    }

    void Visit(Assignment &stmt) override {
        names.insert(stmt.variable->name);
    }

    void Visit(IfStatement &stmt) override {
        for (const auto& it : *stmt.body) {
            it->Accept(*this);
        }
    }

    void Visit(WhileStatement &stmt) override {
        for (const auto& it : *stmt.body) {
            it->Accept(*this);
        }
    }
};

struct PossibleValueAnalyzer : StatementVisitor {
    constexpr static int kMaxCombinationCount = 32;
    constexpr static int kMaxDepth = 32;

    std::map<char, std::set<int>> possible_values{};
    std::vector<std::shared_ptr<Statement>> never_happens{};

    virtual void Analyse(Program &p) {
        Visit(*p.statements);
    }

    void Visit(StatementList &sl) override {
        for (const auto &stmt: sl) {
            stmt->Accept(*this);
        }
        auto it = std::unique(never_happens.begin(), never_happens.end());
        never_happens.erase(it, never_happens.end());
    }

    void EvalExpr(Expression& expr, std::set<int>& values) {
        std::set<char> names;
        expr.GetNames(names);

        int combination_count = 1;
        for (auto it: names) {
            combination_count *= possible_values[it].size();
            if  (combination_count == 0 || combination_count > kMaxCombinationCount) {
                return;
            }
        }

        std::vector<std::map<char, int>> combinations(combination_count);
        for (auto name: names) {
            for (auto possible_value: possible_values[name]) {
                for (int i = 0; i < combination_count; i += possible_values[name].size()) {
                    combinations[i][name] = possible_value;
                }
            }
        }

        for (auto combination: combinations) {
            auto evaluated = expr.Evaluate(combination);
            values.insert(dynamic_pointer_cast<Constant>(evaluated)->value);
        }
    }

    void Visit(Assignment &assignment) override {
        std::set<int> values;
        EvalExpr(*assignment.expression, values);
        possible_values[assignment.variable->name] = std::move(values);
    }

    void Visit(IfStatement &if_statement) override {
        std::set<int> values;
        EvalExpr(*if_statement.condition, values);
        bool can_be_true = values.empty();
        bool can_be_false = values.empty();
        for (auto value: values) {
            if (value) {
                can_be_true = true;
            } else {
                can_be_false = true;
            }
            if (can_be_true && can_be_false) {
                break;
            }
        }
        bool always_true = can_be_true && not can_be_false;
        bool always_false = can_be_false && not can_be_true;

        if (always_false) {
            never_happens.push_back(if_statement.shared_from_this());
            return;
        }
        if (always_true) {
            Visit(*if_statement.body);
            return;
        }
        PossibleValueAnalyzer inner_analyzer;
        inner_analyzer.possible_values = possible_values;
        inner_analyzer.Visit(*if_statement.body);
        for (const auto& it: inner_analyzer.never_happens) {
            never_happens.push_back(it);
        }
        for (auto& [n, v]: inner_analyzer.possible_values) {
            if  (possible_values[n].empty()) {
                continue;
            }
            if (v.empty()) {
                possible_values[n].clear();
                continue;
            }
            possible_values[n].merge(v);
            if (possible_values[n].size() > kMaxCombinationCount) {
                possible_values[n].clear();
            }
        }
    }

    void Visit(WhileStatement &while_statement) override {
        Visit(while_statement, 0);
    }

    void Visit(WhileStatement &while_statement, int depth) {
        std::set<int> values;
        EvalExpr(*while_statement.condition, values);

        bool not_computable = values.empty();
        if (not_computable || depth > kMaxDepth) {
            WriteNamesCollector writeNamesGetter;
            writeNamesGetter.Visit(*while_statement.body);
            for (auto name: writeNamesGetter.names) {
                possible_values[name].clear();
            }
            return;
        }

        bool can_be_true = false;
        bool can_be_false = false;
        for (auto value: values) {
            if (value) {
                can_be_true = true;
            } else {
                can_be_false = true;
            }
            if (can_be_true && can_be_false) {
                break;
            }
        }

        bool always_true = can_be_true && not can_be_false;
        bool always_false = can_be_false && not can_be_true;

        if (always_false) {
            if (depth == 0) {
                never_happens.push_back(while_statement.shared_from_this());
            }
            return;
        }
        if (always_true) {
            Visit(*while_statement.body);
            return Visit(while_statement, depth + 1);
        }

        PossibleValueAnalyzer inner_analyzer;
        inner_analyzer.possible_values = possible_values;
        inner_analyzer.Visit(*while_statement.body);
        inner_analyzer.Visit(while_statement, depth + 1);
        for (const auto& it: inner_analyzer.never_happens) {
            never_happens.push_back(it);
        }
        for (auto& [n, v]: inner_analyzer.possible_values) {
            if  (possible_values[n].size() == 0) {
                continue;
            }
            if (v.size() == 0) {
                possible_values[n].clear();
                continue;
            }
            possible_values[n].merge(v);
            if (possible_values[n].size() > kMaxCombinationCount) {
                possible_values[n].clear();
            }
        }
    }
};

struct AssignmentCollector : StatementVisitor {
    std::vector<std::shared_ptr<Statement>> assignments{};

    void Visit(StatementList &sl) {
        for (const auto &stmt: sl) {
            stmt->Accept(*this);
        }
    }

    void Visit(Assignment &stmt) override {
        assignments.push_back(stmt.shared_from_this());
    }

    void Visit(IfStatement &stmt) override {
        for (const auto& it : *stmt.body) {
            it->Accept(*this);
        }
    }

    void Visit(WhileStatement &stmt) override {
        for (const auto& it : *stmt.body) {
            it->Accept(*this);
        }
    }
};

struct MixedAnalyser : LiveVariableAnalyser {
    PossibleValueAnalyzer possible_value_analyzer{};

    void Analyse(Program &p) override {
        possible_value_analyzer.Analyse(p);
        std::sort(possible_value_analyzer.never_happens.begin(),
                  possible_value_analyzer.never_happens.end());
        LiveVariableAnalyser::Analyse(p);
    }

    void Visit(IfStatement &stmt) override {
        if (std::find(
                possible_value_analyzer.never_happens.begin(),
                possible_value_analyzer.never_happens.end(),
                stmt.shared_from_this()) == possible_value_analyzer.never_happens.end()) {
            LiveVariableAnalyser::Visit(stmt);
        } else {
            AssignmentCollector assignmentCollector;
            assignmentCollector.Visit(*stmt.body);
            unused.insert(unused.end(),
                          assignmentCollector.assignments.begin(),
                          assignmentCollector.assignments.end());
        }
    }

    void Visit(WhileStatement &stmt) override {
        if (std::find(
                possible_value_analyzer.never_happens.begin(),
                possible_value_analyzer.never_happens.end(),
                stmt.shared_from_this()) == possible_value_analyzer.never_happens.end()) {
            LiveVariableAnalyser::Visit(stmt);
        } else {
            AssignmentCollector assignmentCollector;
            assignmentCollector.Visit(*stmt.body);
            unused.insert(unused.end(),
                          assignmentCollector.assignments.begin(),
                          assignmentCollector.assignments.end());
        }
    }
};