cmake_minimum_required(VERSION 3.22)
project(OPS2)

set(CMAKE_CXX_STANDARD 17)

############# LAB 3 ##############
# exercise task
add_executable(lab3.exercise.server lab3/exercise/server.c lab3/library/mysocklib.c lab3/library/mysocklib.h)
add_executable(lab3.exercise.client lab3/exercise/client.c lab3/library/mysocklib.c lab3/library/mysocklib.h)

# lab task
add_executable(lab3.lab.server lab3/lab/server.c lab3/library/mysocklib.c lab3/library/mysocklib.h)

add_compile_options(-Wall -fsanitize=address,undefined -ansi -pedantic)