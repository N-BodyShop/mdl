#
#       QMPI (MPI+Quadrics SHMEM library) makefile for MDL
#       Developed on Lemieux.psc.edu
#
#       Default cache line replacement scheme is LRU.  If you
#       want to try randomly selected lines, use mdl.rand.c.
#

default: mdl.o

mdl.o: mdl.c mdl.h
	cc -I/usr/opt/rms/include $(CC_FLAGS) -c mdl.c

#cc -I/usr/local/mpp/lib/include/mpp $(CC_FLAGS) -c mdl.c
