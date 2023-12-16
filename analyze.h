//
// Created by Aleksandr Govenko on 16/12/2023.
//

#pragma once

#include <ranges>

#include "ast.h"

struct LiveVariableAnalyser : StatementVisitor {
    std::set<char> live_in_succ{};
    std::vector<std::shared_ptr<Statement>> unused{};

    void Analyse(StatementList &sl) {
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

        inner_analyser.Analyse(*stmt.body);

        live_in_succ.merge(read);
        live_in_succ.merge(inner_analyser.live_in_succ);
        unused.insert(unused.end(), inner_analyser.unused.begin(), inner_analyser.unused.end());
    }

    void Visit(WhileStatement &stmt) override {
        LiveVariableAnalyser inner_analyser;
        inner_analyser.live_in_succ = live_in_succ;

        std::set<char> read{};
        stmt.condition->GetNames(read);

        inner_analyser.Analyse(*stmt.body);
        inner_analyser.unused.clear();
        inner_analyser.Analyse(*stmt.body);

        live_in_succ.merge(read);
        live_in_succ.merge(inner_analyser.live_in_succ);
        unused.insert(unused.end(), inner_analyser.unused.begin(), inner_analyser.unused.end());
    }
};