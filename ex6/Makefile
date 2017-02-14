listfiles : listfiles.c
	gcc -Wall -g -o listfiles listfiles.c

test : listfiles 
	listfiles testdir > testdir.actual
	compare_result testdir
