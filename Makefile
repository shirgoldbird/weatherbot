CC = gcc
FLAGS = -g -Wall --std=c11
LINKOPTS = -lcurl

all: weatherbot

LinkedList.o: LinkedList.c LinkedList.h
	$(CC) -o $@ $< -c $(FLAGS)

frozen.o: frozen.c frozen.h
	$(CC) -o $@ $< -c $(FLAGS)

weatherbot: weatherbot.c LinkedList.o frozen.o
	$(CC) $^ $(FLAGS) $(LINKOPTS) -o $@

.PHONY: clean
clean:
	rm -f LinkedList.o frozen.o weatherbot
