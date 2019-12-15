all:
	gcc-9 *.c -std=c11 -lpthread -o queue
	gcc-9 *.c -std=c11 -lpthread -DUSE_ATOMICS -o atomic_queue -O3
