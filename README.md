# DA Puzzle
The DA Puzzle is a puzzle, which is described as follows.

A matrix M, of size NxN, is given, which can contain 0s or 1s.\
The transition operator is T(i, j): flip bit at M[i, j], M[i-1, j], M[i+1, j], M[i, j-1], M[i, j+1], wherever applicable.\
The initial and goal state can vary, but are usually all 0s and all 1s respectively.

This repository aims to explore different ways of solving this puzzle, for increasing values of N. The challenge is that the search space grows exponentially in regards to N.\
Currently, the following algorithms have been implemented:
* BFS (Python, C)
* IDS (Python)
* BestFS (Python)

It is interesting to note that the Python implementation of BFS will require a few hours on an average computer to find the solution for N=5 (optimal solution depth: 15), whereas the C implementation requires less than a minute.\
BestFS is by far the fastest algorithm. Using the heuristic function h(n): total 0s in matrix of state n, a solution can be found for N=5 in seconds. However, that solution is obviously not optimal.
