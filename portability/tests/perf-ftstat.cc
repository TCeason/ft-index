/*
 * Copyright (C) 2017 BohuTANG <overred.shuttler@gmail.com>
 *
 * 2*Intel(R) Core(TM) i5-7200U CPU @ 2.50GHz
 * toku_os_fstat count=10000000, cost=1.246280s, perf=8023879.5/sec
 *
 */

#include <stdio.h>
#include <sys/time.h>
#include "toku_portability.h"
#include <portability/toku_path.h>

int main(void)
{
	int i;
	int fd;
	int c = 10000000;
	float cost;
	toku_struct_stat st;
	struct timeval start, end;

	fd = toku_os_lock_file(TOKU_TEST_FILENAME);
	assert(fd != -1);

	gettimeofday(&start, NULL);
	for (i = 0; i < c; i++) {
		toku_os_fstat(fd, &st);
	}
	gettimeofday(&end, NULL);
	cost = (end.tv_sec - start.tv_sec) + 1e-6 * (end.tv_usec - start.tv_usec);
	printf("toku_os_fstat count=%d, cost=%.6fs, perf=%0.1f/sec\n", c, cost, c / cost);
	return 0;
}
