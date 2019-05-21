# Author        : Achmad Satria Putera
# Email         : satria@bahasakita.co.id
# Date          : 20 Mei 2018
# Filename      : Makefile
# Description   :
# Makefile for compiling client smart speaker service

# the variable CC will be the compiler to use.
CC=gcc
CFLAGS=-g -Wall


LIBS=-lzmq -ljson-c -lasound

all: client-ss-service

client-ss-service: src/client-ss-service.c
	$(CC) -o client-ss-service src/client-ss-service.c $(LIBS)

clean: 
	rm -rf *.o client-ss-service

