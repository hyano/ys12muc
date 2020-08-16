#
# Makefile for ys12muc
#
# Copyright (c) 2020 Hirokuni Yano
#
# Released under the MIT license.
# see https://opensource.org/licenses/MIT
#
CC	= gcc
CFLAGS	= -g -Wall -Wextra

all: ys12muc

clean:
	rm -f ys12muc ys12muc.o

ys12muc.o: ys12muc.c

ys12muc: ys12muc.o
	$(CC) ys12muc.o -o ys12muc
