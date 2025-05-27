for %%f in (.\tests\*.c) do (
    clang -fsanitize=undefined -g -Wall -Wextra -pedantic -Werror -o "%%f.exe" %%f
    ".\%%f.exe"
)
