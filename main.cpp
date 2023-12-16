//
// Created by Aleksandr Govenko on 13/12/2023.
//

#include <iostream>
#include <fstream>
#include <ranges>

#include "parser.h"
#include "ast.h"
#include "analyze.h"

void Analyze(const Program &p) {
    LiveVariableAnalyser analyzer;
    analyzer.Analyse(*p.statements);
    for (const auto &statement: analyzer.unused | std::views::reverse) {
        std::cout << *statement << std::endl;
    }
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }
    std::ifstream file(argv[1]);
    Parser parser(file);
    auto program = parser.ParseProgram();
    Analyze(*program);
    return 0;
}
