# Not doing -march=native because we're scp-ying onto remote machines to run
CC=g++-7
#CC = clang++
#CFLAGS=-std=c++11 -O3 -fcilkplus
CFLAGS=-std=c++11 -march=native -funroll-loops -g -O3 -fcilkplus #-fcsi  -L/efs/tools/tapir-6/build/lib/clang/6.0.0/lib/linux/ -lclang_rt.cilkscale-x86_64
DEPS = main.cc partition.cc libc_partition.cc

main: $(DEPS)
	$(CC) -o $@ $^ $(CFLAGS) -lcilkrts -ldl
clean:
	rm -f main main_cpy $(OBJ)

