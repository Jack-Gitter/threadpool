.PHONY: all

all: main.c
	clang -std=c2x -o main main.c
