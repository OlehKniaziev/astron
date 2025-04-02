# Astron
Astron is a collection of headers that are meant to use in conjunction with each other. The idea is
to have a single central header, with all other headers being able to only depend on it.

| Header                   | Description                                                                                    |
|--------------------------|------------------------------------------------------------------------------------------------|
| **[helios.h](helios.h)** | The central header, which provides the allocator interface and basic QOL procedures and types. |
| **[ermis.h](ermis.h)**   | Provides template-like data structure generation macros                                        |
