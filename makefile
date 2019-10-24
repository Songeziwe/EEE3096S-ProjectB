.RECIPEPREFIX +=
CC = g++
CFLAGS = -Wall -lm -lrt -lwiringPi -lpthread -lmosquitto -I ./blynk-library/linux

PROG = bin/*
OBJS = obj/*

default:
    mkdir -p bin obj
    $(CC) $(CFLAGS) -c src/Project.cpp -o obj/Project.o
    $(CC) $(CFLAGS) -c src/CurrentTime.c -o obj/CurrentTime.o
    $(CC) $(CFLAGS) obj/Project.o obj/CurrentTime.o -o bin/Project

run:
    sudo ./bin/Project

clean:
    rm $(PROG) $(OBJS)
