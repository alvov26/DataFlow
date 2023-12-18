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
        unused.push_back(stmt.shared_from_this());
    }
    stmt.expression->GetNames(live_in_succ);
}

void LiveVariableAnalyser::Visit(IfStatement& stmt) {
    std::set<char> original_live_in_succ = live_in_succ;
    Visit(*stmt.body);
    live_in_succ.merge(original_live_in_succ);
    stmt.condition->GetNames(live_in_succ);
}

void LiveVariableAnalyser::Visit(WhileStatement& stmt) {
    std::set<char> original_live_in_succ = live_in_succ;
    const auto previous_size = unused.size();

    Visit(*stmt.body);
    unused.resize(previous_size);
    live_in_succ.insert(original_live_in_succ.begin(), original_live_in_succ.end());
    Visit(*stmt.body);

    live_in_succ.merge(original_live_in_succ);
    stmt.condition->GetNames(live_in_succ);
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
}

void PossibleValueAnalyzer::EvalExpr(Expression& expr, std::set<int>& values) {
    std::set<char> names;
    expr.GetNames(names);

    size_t combination_count = 1;
    for (auto it: names) {
        combination_count *= possible_values[it].size();
        if (combination_count == 0 || combination_count > kMaxCombinationCount) {
            return;
        }
    }

    std::vector<std::map<char, int>> combinations(combination_count);
    for (auto name: names) {
        for (const auto& possible_value: possible_values[name]) {
            for (size_t i = 0; i < combination_count; i += possible_values[name].size()) {
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
    for (const auto& value: values) {
        if (value) {
            can_be_true = true;
        } else {
            can_be_false = true;
        }
        if (can_be_true && can_be_false) {
            break;
        }
    }
    const bool always_true = can_be_true && not can_be_false;
    const bool always_false = can_be_false && not can_be_true;

    if (always_false) {
        never_happens.push_back(if_statement.shared_from_this());
        return;
    }
    if (always_true) {
        always_happens.push_back(if_statement.shared_from_this());
        Visit(*if_statement.body);
        return;
    }
    auto original_possible_values = possible_values;
    Visit(*if_statement.body);
    for (auto& [n, v]: original_possible_values) {
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

    const bool not_computable = values.empty();
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
    for (const auto value: values) {
        if (value) {
            can_be_true = true;
        } else {
            can_be_false = true;
        }
        if (can_be_true && can_be_false) {
            break;
        }
    }

    const bool always_true = can_be_true && not can_be_false;
    const bool always_false = can_be_false && not can_be_true;

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
        Visit(while_statement, depth + 1);
        return;
    }
    auto original_possible_values = possible_values;
    Visit(*while_statement.body);
    Visit(while_statement, depth + 1);
    for (auto& [n, v]: original_possible_values) {
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
    LiveVariableAnalyser::Analyse(p);
}

void MixedAnalyser::Visit(IfStatement& stmt) {
    using namespace std::ranges;
    const auto& never_happens = possible_value_analyzer.never_happens;
    const auto& always_happens = possible_value_analyzer.always_happens;
    if (find(never_happens, stmt.shared_from_this()) != end(never_happens)) {
        stmt.condition->GetNames(live_in_succ);
        AssignmentCollector c;
        c.Visit(*stmt.body);
        unused.insert(unused.end(), c.assignments.begin(), c.assignments.end());
        return;
    }
    if (find(always_happens, stmt.shared_from_this()) != end(always_happens)) {
        LiveVariableAnalyser::Visit(*stmt.body);
        stmt.condition->GetNames(live_in_succ);
        return;
    }
    LiveVariableAnalyser::Visit(stmt);
}

void MixedAnalyser::Visit(WhileStatement& stmt) {
    using namespace std::ranges;
    const auto& never_happens = possible_value_analyzer.never_happens;
    const auto& always_happens = possible_value_analyzer.always_happens;
    if (find(never_happens, stmt.shared_from_this()) != end(never_happens)) {
        stmt.condition->GetNames(live_in_succ);
        AssignmentCollector c;
        c.Visit(*stmt.body);
        unused.insert(unused.end(), c.assignments.begin(), c.assignments.end());
        return;
    }
    if (find(always_happens, stmt.shared_from_this()) != end(always_happens)) {
        std::set<char> original_live_in_succ = live_in_succ;
        const auto previous_size = unused.size();

        LiveVariableAnalyser::Visit(*stmt.body);
        unused.resize(previous_size);
        live_in_succ.merge(original_live_in_succ);
        LiveVariableAnalyser::Visit(*stmt.body);

        stmt.condition->GetNames(live_in_succ);
        return;
    }
    LiveVariableAnalyser::Visit(stmt);
}
