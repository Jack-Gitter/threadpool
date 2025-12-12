.PHONY: all

all: main.c
	clang -g -std=c2x -o main main.c
