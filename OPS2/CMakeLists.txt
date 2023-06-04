cmake_minimum_required(VERSION 3.22)
project(OPS2)

set(CMAKE_CXX_STANDARD 17)

############# LAB 3 ##############
# exercise task
add_executable(lab3.exercise.server lab3/exercise/server.c mysocklib/mysocklib.c mysocklib/mysocklib.h)
add_executable(lab3.exercise.client lab3/exercise/client.c mysocklib/mysocklib.c mysocklib/mysocklib.h)

# lab task
add_executable(lab3.lab.server lab3/lab/server.c mysocklib/mysocklib.c mysocklib/mysocklib.h)

# lab udp task
add_executable(lab3.lab-udp-task.server lab3/lab-udp-task/server.c mysocklib/mysocklib.c mysocklib/mysocklib.h)
add_executable(lab3.lab-udp-task.client lab3/lab-udp-task/client.c mysocklib/mysocklib.c mysocklib/mysocklib.h)

# tcp quiz app
add_executable(lab3.tcp-quiz-app.server lab3/tcp-quiz-app/server.c mysocklib/mysocklib.c mysocklib/mysocklib.h)
add_executable(lab3.tcp-quiz-app.client lab3/tcp-quiz-app/client.c mysocklib/mysocklib.c mysocklib/mysocklib.h)

############# LAB 4 ##############
# exercise 1
add_executable(lab4.exercise1.server lab4/exercise1/server.c mysocklib/mysocklib.c mysocklib/mysocklib.h)
add_executable(lab4.exercise1.client lab4/exercise1/client.c mysocklib/mysocklib.c mysocklib/mysocklib.h)

# exercise 2
add_executable(lab4.exercise2.server lab4/exercise2/server.c mysocklib/mysocklib.c mysocklib/mysocklib.h)

# exercise 3
add_executable(lab4.exercise3.server lab4/exercise3/server.c mysocklib/mysocklib.c mysocklib/mysocklib.h)

add_compile_options(-Wall -fsanitize=address,undefined -ansi -pedantic)



