CFLAGS=-Wall -Wextra -Wpedantic
OBJS=util.o rangegen.o rangegen-cash.o
PROGS=rangegen rangegen-cash
LIBS=-lstdc++
# OPTIMIZE
# -O0 = no optimization
# -O3 = good optimization
# -Ofast -march=native = aggressive optimization
# -Os = small file size
# -Og -g -ggdb debugging
CFLAGS+=-Ofast
CXXFLAGS+=-Ofast -fno-rtti
CC=gcc-8
CXX=g++-8
PLATFORM=$(shell uname -s)

#most: all
most: rangegen

all: rangegen rangegen-cash

rangegen: rangegen.o util.o sort.o
	$(CC) $^ -o $@ $(CFLAGS) $(LIBS)

rangegen-cash: rangegen-cash.o util.o sort.o
	$(CC) $^ -o $@ $(CFLAGS) $(LIBS)

clean:
# DON'T RUN IF YOU DO `make -f` or `--file`
	rm -rf *.dSYM *.out *.o rangegen rangegen-cash

format:
	clang-format -i -verbose -style=file util.h util.c rangegen.c rangegen-cash.c
