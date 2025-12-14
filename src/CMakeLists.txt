cmake_minimum_required(VERSION 3.28)

project(copier_test)


add_executable(copier ThreadListener.c)


target_include_directories(copier PUBLIC ../inc)
