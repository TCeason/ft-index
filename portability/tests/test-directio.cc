/*
 * Copyright (C) 2017 BohuTANG <overred.shuttler@gmail.com>
 *
 */

#include <test.h>
#include <fcntl.h>
#include <toku_assert.h>
#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <portability/toku_path.h>

struct dio_entry {
	int64_t wbufsize;
	int64_t woffset;
	int64_t walign;

	int64_t rbufsize;
	int64_t roffset;
	int64_t ralign;
};
const char *test_directio_fname = "test_directio.in";

int test_main(int UU(argc), char *const UU(argv[]))
{
	struct dio_entry dios[2];

	// 512
	dios[0].wbufsize = 512;
	dios[0].woffset = 512;
	dios[0].walign = 512;
	dios[0].rbufsize = 512;
	dios[0].roffset = 512;
	dios[0].ralign = 512;

	// 4K
	dios[1].wbufsize = 4096 * 2;
	dios[1].woffset = 4096;
	dios[1].walign = 4096;
	dios[1].rbufsize = 4096;
	dios[1].roffset = 4096;
	dios[1].ralign = 4096;

	int len = sizeof(dios) / sizeof(dio_entry);
	for (int i = 0; i < len; i++) {
		int r;

		unlink(test_directio_fname);
		int fdo = open(test_directio_fname, O_WRONLY | O_CREAT | O_TRUNC | O_DIRECT, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
		assert(fdo >= 0);

		char *XMALLOC_N_ALIGNED(dios[i].walign, dios[i].wbufsize, wbuf);
		memset(wbuf, 0, dios[i].wbufsize);
		strcpy(wbuf, "hello");
		toku_os_full_pwrite(fdo, wbuf, dios[i].wbufsize, dios[i].woffset);
		r = close(fdo);
		assert(r == 0);

		int fdi = open(test_directio_fname, O_RDONLY | O_DIRECT);
		char *MALLOC_N_ALIGNED(dios[i].ralign, dios[i].rbufsize, rbuf);
		int64_t rsize = toku_os_pread(fdi, rbuf, dios[i].rbufsize, dios[i].roffset);
		assert(rsize == dios[i].rbufsize);
		toku_free(rbuf);
		r = close(fdi);
		assert(r == 0);
	}
	return 0;
}
