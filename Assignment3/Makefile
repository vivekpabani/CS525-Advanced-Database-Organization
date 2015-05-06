CC=clang
CFLAGS=-I.
DEPS =

dberror.o: dberror.c
	cc -o dberror.o -c dberror.c -g -Wall

storage_mgr.o: storage_mgr.c
	cc -o storage_mgr.o -c storage_mgr.c -g -Wall

buffer_mgr.o: buffer_mgr.c
	cc -o buffer_mgr.o -c buffer_mgr.c -g -Wall

buffer_mgr_stat.o: buffer_mgr_stat.c
	cc -o buffer_mgr_stat.o -c buffer_mgr_stat.c -g -Wall

record_mgr.o: record_mgr.c
	cc -o record_mgr.o -c record_mgr.c -g -Wall

rm_serializer.o: rm_serializer.c
	cc -o rm_serializer.o -c rm_serializer.c -g -Wall

expr.o: expr.c
	cc -o expr.o -c expr.c -g -Wall

test_expr.o: test_expr.c
	cc -o test_expr.o -c test_expr.c -g -Wall

test_assign3_1.o: test_assign3_1.c
	cc -o test_assign3_1.o -c test_assign3_1.c -g -Wall

assign3_1: test_assign3_1.o record_mgr.o rm_serializer.o expr.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o
	cc -o assign3_1 test_assign3_1.o record_mgr.o rm_serializer.o expr.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o -I. -g -Wall -lm

expr: test_expr.o record_mgr.o rm_serializer.o expr.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o
	cc -o expr test_expr.o record_mgr.o rm_serializer.o expr.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o -I. -g -Wall -lm

clean: test_assign3_1.o test_expr.o record_mgr.o rm_serializer.o expr.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o
	rm test_assign3_1.o test_expr.o record_mgr.o rm_serializer.o expr.o buffer_mgr.o buffer_mgr_stat.o storage_mgr.o dberror.o
