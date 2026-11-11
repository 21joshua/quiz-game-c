# Quiz Game (C)

A terminal-based quiz game written in C. Questions, answer options,
correct answers, time limits, and prize amounts are stored in an
external text file.

## Features

- Multiple-choice questions
- Case-insensitive answers
- Fifty-Fifty lifeline
- Skip Question lifeline
- Prize money tracking

## Requirements

- GCC compiler
- Linux/Unix terminal

## Build

    gcc -Wall -Wextra -g quiz.c -o quiz

## Run

    ./quiz

## Question File Format

The questions.txt file contains eight lines per question:

1. Question text
2. Option A
3. Option B
4. Option C
5. Option D
6. Correct answer (A, B, C, or D)
7. Time limit in seconds
8. Prize amount

The time limit is currently displayed but not enforced.
