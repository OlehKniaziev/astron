for %%f in (.\tests\*) do (
    clang -Wall -Wextra -pedantic -Werror -o "%%f.exe" %%f
    ".\%%f.exe"
    del "%%f.exe"
)
