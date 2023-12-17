//
// Created by Aleksandr Govenko on 17/12/2023.
//

#include <ranges>
#include "analysis.h"

void LiveVariableAnalyser::Analyse(Program& p) {
    Visit(*p.statements);
}

void LiveVariableAnalyser::Visit(StatementList& sl) {
    for (const auto& stmt: sl | std::views::reverse) {
        stmt->Accept(*this);
    }
}

void LiveVariableAnalyser::Visit(Assignment& stmt) {
    const char write_name = stmt.variable->name;
    if (live_in_succ.erase(write_name) == 0) {
        std::shared_ptr<Statement> ptr = stmt.shared_from_this();
        unused.push_back(ptr);
    }
    stmt.expression->GetNames(live_in_succ);
}

void LiveVariableAnalyser::Visit(IfStatement& stmt) {
    std::set<char> original_live_in_succ = live_in_succ;

    Visit(*stmt.body);

    stmt.condition->GetNames(live_in_succ);
    live_in_succ.merge(original_live_in_succ);
}

void LiveVariableAnalyser::Visit(WhileStatement& stmt) {
    std::set<char> original_live_in_succ = live_in_succ;
    const auto previous_size = unused.size();

    Visit(*stmt.body);
    unused.resize(previous_size);
    live_in_succ.insert(original_live_in_succ.begin(), original_live_in_succ.end());
    Visit(*stmt.body);

    stmt.condition->GetNames(live_in_succ);
    live_in_succ.merge(original_live_in_succ);
}

void WriteNamesCollector::Visit(StatementList& sl) {
    for (const auto& stmt: sl) {
        stmt->Accept(*this);
    }
}

void WriteNamesCollector::Visit(Assignment& stmt) {
    names.insert(stmt.variable->name);
}

void WriteNamesCollector::Visit(IfStatement& stmt) {
    for (const auto& it: *stmt.body) {
        it->Accept(*this);
    }
}

void WriteNamesCollector::Visit(WhileStatement& stmt) {
    for (const auto& it: *stmt.body) {
        it->Accept(*this);
    }
}

void PossibleValueAnalyzer::Analyse(Program& p) {
    Visit(*p.statements);
}

void PossibleValueAnalyzer::Visit(StatementList& sl) {
    for (const auto& stmt: sl) {
        stmt->Accept(*this);
    }
    auto it = std::unique(never_happens.begin(), never_happens.end());
    never_happens.erase(it, never_happens.end());
}

void PossibleValueAnalyzer::EvalExpr(Expression& expr, std::set<int>& values) {
    std::set<char> names;
    expr.GetNames(names);

    int combination_count = 1;
    for (auto it: names) {
        combination_count *= possible_values[it].size();
        if (combination_count == 0 || combination_count > kMaxCombinationCount) {
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

void PossibleValueAnalyzer::Visit(Assignment& assignment) {
    std::set<int> values;
    EvalExpr(*assignment.expression, values);
    possible_values[assignment.variable->name] = std::move(values);
}

void PossibleValueAnalyzer::Visit(IfStatement& if_statement) {
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
        always_happens.push_back(if_statement.shared_from_this());
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
        if (possible_values[n].empty()) {
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

void PossibleValueAnalyzer::Visit(WhileStatement& while_statement) {
    Visit(while_statement, 0);
}

void PossibleValueAnalyzer::Visit(WhileStatement& while_statement, int depth) {
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
        if (depth == 0) {
            always_happens.push_back(while_statement.shared_from_this());
        }
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
        if (possible_values[n].empty()) {
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

void AssignmentCollector::Visit(StatementList& sl) {
    for (const auto& stmt: sl) {
        stmt->Accept(*this);
    }
}

void AssignmentCollector::Visit(Assignment& stmt) {
    assignments.push_back(stmt.shared_from_this());
}

void AssignmentCollector::Visit(IfStatement& stmt) {
    for (const auto& it: *stmt.body) {
        it->Accept(*this);
    }
}

void AssignmentCollector::Visit(WhileStatement& stmt) {
    for (const auto& it: *stmt.body) {
        it->Accept(*this);
    }
}

void MixedAnalyser::Analyse(Program& p) {
    possible_value_analyzer.Analyse(p);
    std::sort(possible_value_analyzer.never_happens.begin(),
              possible_value_analyzer.never_happens.end());
    std::sort(possible_value_analyzer.always_happens.begin(),
              possible_value_analyzer.always_happens.end());
    LiveVariableAnalyser::Analyse(p);
}

void MixedAnalyser::Visit(IfStatement& stmt) {
    if (std::find(
            possible_value_analyzer.never_happens.begin(),
            possible_value_analyzer.never_happens.end(),
            stmt.shared_from_this()) == possible_value_analyzer.never_happens.end()) {
        if (std::find(
                possible_value_analyzer.always_happens.begin(),
                possible_value_analyzer.always_happens.end(),
                stmt.shared_from_this()) == possible_value_analyzer.always_happens.end()) {
            LiveVariableAnalyser::Visit(stmt);
        } else {
            LiveVariableAnalyser::Visit(*stmt.body);
            stmt.condition->GetNames(live_in_succ);
        }
    } else {
        stmt.condition->GetNames(live_in_succ);
        AssignmentCollector assignmentCollector;
        assignmentCollector.Visit(*stmt.body);
        unused.insert(unused.end(),
                      assignmentCollector.assignments.begin(),
                      assignmentCollector.assignments.end());
    }
}

void MixedAnalyser::Visit(WhileStatement& stmt) {
    if (std::find(
            possible_value_analyzer.never_happens.begin(),
            possible_value_analyzer.never_happens.end(),
            stmt.shared_from_this()) == possible_value_analyzer.never_happens.end()) {
        if (std::find(
                possible_value_analyzer.always_happens.begin(),
                possible_value_analyzer.always_happens.end(),
                stmt.shared_from_this()) == possible_value_analyzer.always_happens.end()) {
            LiveVariableAnalyser::Visit(stmt);
        } else {
            std::set<char> original_live_in_succ = live_in_succ;
            auto previous_size = unused.size();
            LiveVariableAnalyser::Visit(*stmt.body);
            unused.resize(previous_size);
            live_in_succ.merge(original_live_in_succ);
            LiveVariableAnalyser::Visit(*stmt.body);
            stmt.condition->GetNames(live_in_succ);
        }
    } else {
        stmt.condition->GetNames(live_in_succ);
        AssignmentCollector assignmentCollector;
        assignmentCollector.Visit(*stmt.body);
        unused.insert(unused.end(),
                      assignmentCollector.assignments.begin(),
                      assignmentCollector.assignments.end());
    }
}
