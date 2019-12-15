all:
	gcc *.c -std=c11 -lpthread -o queue
	gcc *.c -std=c11 -lpthread -DUSE_ATOMICS -o atomic_queue -O3
