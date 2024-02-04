CC	=	/opt/ohpc/pub/mpi/openmpi3-gnu7/3.1.0/bin/mpicc
CCLINK	=	/opt/ohpc/pub/mpi/openmpi3-gnu7/3.1.0/bin/mpicc
SHELL	=	/bin/sh

EXEC	= bitonic_sort_mpi

main: bitonic_sort_mpi.c
	$(CC) -o $(EXEC) $(EXEC).c

clean:
	/bin/rm -f $(EXEC) $(EXEC)*.o $(EXEC)*.s
