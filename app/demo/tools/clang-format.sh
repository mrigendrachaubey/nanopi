#!/bin/bash
find .. -name "*.cc" -o -name "*.h" | xargs clang-format-8 -style=llvm -i
