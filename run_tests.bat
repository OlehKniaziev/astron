for %%f in (.\tests\*.c) do (
    clang -O0 -g -Wall -Wextra -pedantic -Werror -o "%%f.exe" %%f
    ".\%%f.exe"
)
