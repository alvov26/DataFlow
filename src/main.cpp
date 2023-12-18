//
// Created by Aleksandr Govenko on 13/12/2023.
//

#include <fstream>
#include <iostream>
#include <ranges>

#include "analysis.h"
#include "ast.h"
#include "parser.h"

void Analyze(Program &p) {
    // std::cout << "Live variables:" << std::endl;
    // LiveVariableAnalyser analyzer;
    // analyzer.Analyse(p);
    // for (const auto &statement: analyzer.unused | std::views::reverse) {
    //     std::cout << *statement << std::endl;
    // }
    //
    // std::cout << "Never happens:" << std::endl;
    // PossibleValueAnalyzer valueAnalyzer;
    // valueAnalyzer.Analyse(p);
    // for (const auto &statement: valueAnalyzer.never_happens) {
    //     std::cout << *statement << std::endl;
    // }
    //
    // std::cout << "Mixed analysis:" << std::endl;
    MixedAnalyser mixedAnalyser;
    mixedAnalyser.Analyse(p);
    for (const auto &statement: mixedAnalyser.unused | std::views::reverse) {
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
