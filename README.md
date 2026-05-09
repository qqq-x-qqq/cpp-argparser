# CLI Argument Parser

Lightweight command-line argument parsing library written in C++.

## Features

- Supports integer, float, string, and boolean arguments
- Handles flags and positional arguments
- Supports repeated arguments
- Built-in help output
- Custom validation callbacks

## Tech Stack

- C++
- Command-line parsing
- Dynamic memory
- GoogleTest
- CMake

## Project Structure

- `lib/argparser.h` — public API
- `lib/argparser.cpp` — parser implementation
- `tests/` — parser tests
- `bin/` — example application

## Example

```bash
labwork3 --sum 2 4 6
labwork3 --mult 2 4 6
```

## Notes

This project is focused on API design, validation, and implementing a reusable parser with minimal language features.
