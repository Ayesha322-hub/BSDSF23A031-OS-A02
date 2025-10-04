# Makefile for ls-v1.0.0
CC = gcc
SRC = src/lsv1.0.0.c
OUT = bin/ls

all: $(OUT)

$(OUT): $(SRC)
	mkdir -p bin
	$(CC) $(SRC) -o $(OUT)

clean:
	rm -f $(OUT)
