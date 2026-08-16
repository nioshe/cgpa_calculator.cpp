# C++ Word Scramble and CGPA Experiments

A learning repository containing two separate programming experiments:

1. A substantial C++17 command-line word-scramble game
2. An early, incomplete browser UI concept for a CGPA calculator

The C++ word-scramble game is the main working implementation in the repository. The frontend CGPA files are not currently a complete runnable application.

## C++ word-scramble game

The program in `cgpa_calculator.cpp` implements:

- Random word selection and scrambling
- Easy, medium, and hard difficulty levels
- Case-insensitive guess checking
- Input validation and duplicate-word prevention
- Custom word loading from files
- Score calculation and per-player leaderboard entries
- Accuracy, timing, file-I/O, and memory-estimate metrics
- Difficulty-aware score multipliers

### Build

A compiler with C++17 support is required.

```bash
g++ -std=c++17 -O2 -o word-scramble cgpa_calculator.cpp
```

### Run

macOS or Linux:

```bash
./word-scramble
```

Windows PowerShell:

```powershell
.\word-scramble.exe
```

## CGPA frontend status

The `frontend/` directory contains an unfinished React-style interface fragment for entering courses, credits, and grades. It does not currently include the complete component logic, build configuration, or a runnable HTML entry point.

## Technology

- C++17 standard library
- File I/O
- Regular expressions
- Randomization
- STL containers and algorithms
- Early React/JavaScript UI experimentation

## Repository note

The current repository name reflects the original CGPA experiment but does not accurately describe the main C++ program. A future rename would make the project easier to understand, but no repository rename is included in this documentation update.
