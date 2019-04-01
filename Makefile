# Not doing -march=native because we're scp-ying onto remote machines to run
CC=g++-7
CFLAGS=-std=c++11 -march=native -funroll-loops -g -O3 -fcilkplus  -lcilkrts -ldl #-fcsi  -L/efs/tools/tapir-6/build/lib/clang/6.0.0/lib/linux/ -lclang_rt.cilkscale-x86_64

main: main.o partition.o libc_partition.o cache_friendly_partition.o
	$(CC) $(CFLAGS) -o $@ main.o partition.o libc_partition.o cache_friendly_partition.o
cache_friendly_partition.o: cache_friendly_partition.cpp
	$(CC) -c  cache_friendly_partition.cpp $(CFLAGS)
partition.o: partition.h partition.cc params.h
	$(CC) -c  partition.cc $(CFLAGS)
libc_partition.o: libc_partition.h libc_partition.cc params.h
	$(CC) -c  libc_partition.cc $(CFLAGS)
main.o: main.cc params.h 
	$(CC)  -c  main.cc $(CFLAGS)
clean:
	rm -f main *.o
