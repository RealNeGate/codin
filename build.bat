@echo off
clang unity.c c11threads\threads_msvc.c -I c11threads -g -Wno-microsoft-enum-forward-reference -D_CRT_SECURE_NO_WARNINGS -o codin.exe
