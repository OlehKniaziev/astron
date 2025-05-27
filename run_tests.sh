#!/bin/sh

set -e

CC="${CC:-cc}"

for test_file in ./tests/*.c; do
    $CC -fsanitize=undefined -Wall -Wextra -pedantic -Werror -o "$test_file.o" $test_file
    ./"$test_file.o" || (echo "Failed to run test $test_file" && exit 1)
    echo "Ran test $test_file successfully"
    rm ./"$test_file.o"
done
