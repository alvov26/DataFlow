# Data Flow Analysis

## Disclaimer
Understanding that this is a toy example, I concentrated more on readability and expressiveness of my program, rather than performance. For example, I could easily replace `std::set<char>` with `std::bitset<27>` (since variables are `a..z`), so that copying it would be much faster, and unroll recursions into loops, and so on. In real compiler specialised data structures are probably used for that purpose. On the other hand, I wanted my code to be compact and express ideas of algorithms I used. As to performance, it should still be enough for reasonably complex programs written in this toy language.

## Usage
```shell
$ DataFlow <filename>
```
I created some examples, to run on them, run
```shell
$ DataFlow testfile.txt
```

## Parser
I use recursive descent, combined with precedence climbing to parse expressions.

## Analysis
In my solution I combine two analysis algorithms to get the best result:
Firstly, `PossibleValueAnalyser` goes through the program from top to bottom, calculating possible values for each variable at each point in the program (The idea is taken from [here](https://clang.llvm.org/docs/DataFlowAnalysisIntro.html)). So, for example:
```
x = 5
if (x > 10)
  x = 13
end
```
In this code if-statement will marked as 'never happens', and ignored during further analysis. On the contrary, in this code
```
x = a
if (x > 10)
  x = 13
end
```
value of `x` is unknown, and if-statement will be taken into consideration later. This algorithm has one limitation, though. When performing calculations on loops, it cannot determine when the loop will halt, so the number of maximum iterations is hardcoded (i chose number 32). So here:
```
x = 1
while (x < 32)
  x = x + 1
end
```
the analyser will know, that `x = 32` after the loop, but here:
```
x = 1
while (x < 34)
  x = x + 1
end
```
it will already say that `x` cannot be determined.

After `PossibleValueAnalyser` has done its job, `LiveVariableAnalyser` starts working. It goes through the program from bottom to top. When it sees statements marked as 'never happens', it marks all assignments inside them as unused. It performs analysis based on algorithm described [here](https://en.wikipedia.org/w/index.php?title=Live-variable_analysis&oldformat=true), checking that the variable is read after it is written to. If it finds a write without consequent read, it marks that write as unused.
```
x = 5
x = 6
a = x
```
Here `x = 5` and `a = x` will be marked unused.
Combining two algorithms allows to work on such code:
```
x = 1
while (x < 13)
  x = x + 1
end
if (x > 13)
  x = 5
end
a = x
```
Here `x = 5` and `a = x` will be marked as unused.
