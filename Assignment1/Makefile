CC=clang
CFLAGS=-I.
DEPS = 

dberror.o: dberror.c
	cc -o dberror.o -c dberror.c

storage_mgr.o: storage_mgr.c
	cc -o storage_mgr.o -c storage_mgr.c

test_assign1_1.o: test_assign1_1.c
	cc -o test_assign1_1.o -c test_assign1_1.c

test_assign1_2.o: test_assign1_2.c
	cc -o test_assign1_2.o -c test_assign1_2.c

assign1_1: test_assign1_1.o storage_mgr.o dberror.o
		cc -o assign1_1 test_assign1_1.o storage_mgr.o dberror.o -I.

assign1_2: test_assign1_2.o storage_mgr.o dberror.o
		cc -o assign1_2 test_assign1_2.o storage_mgr.o dberror.o -I.

clean:
	rm assign1_1 assign1_2 test_assign1_1.o test_assign1_2.o storage_mgr.o dberror.o
