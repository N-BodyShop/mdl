#
#       SPX makefile for MDL
#
CC_FLAGS = -O3

default: mdl.o

mdl.o: mdl.c mdl.h
	mpicc $(CC_FLAGS) -c mdl.c
