/* -*- mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
// vim: ft=cpp:expandtab:ts=8:sw=4:softtabstop=4:
#ident "$Id$"
/*
COPYING CONDITIONS NOTICE:

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation, and provided that the
  following conditions are met:

      * Redistributions of source code must retain this COPYING
        CONDITIONS NOTICE, the COPYRIGHT NOTICE (below), the
        DISCLAIMER (below), the UNIVERSITY PATENT NOTICE (below), the
        PATENT MARKING NOTICE (below), and the PATENT RIGHTS
        GRANT (below).

      * Redistributions in binary form must reproduce this COPYING
        CONDITIONS NOTICE, the COPYRIGHT NOTICE (below), the
        DISCLAIMER (below), the UNIVERSITY PATENT NOTICE (below), the
        PATENT MARKING NOTICE (below), and the PATENT RIGHTS
        GRANT (below) in the documentation and/or other materials
        provided with the distribution.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
  02110-1301, USA.

COPYRIGHT NOTICE:

  TokuDB, Tokutek Fractal Tree Indexing Library.
  Copyright (C) 2007-2013 Tokutek, Inc.

DISCLAIMER:

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

UNIVERSITY PATENT NOTICE:

  The technology is licensed by the Massachusetts Institute of
  Technology, Rutgers State University of New Jersey, and the Research
  Foundation of State University of New York at Stony Brook under
  United States of America Serial No. 11/760379 and to the patents
  and/or patent applications resulting from it.

PATENT MARKING NOTICE:

  This software is covered by US Patent No. 8,185,551.
  This software is covered by US Patent No. 8,489,638.

PATENT RIGHTS GRANT:

  "THIS IMPLEMENTATION" means the copyrightable works distributed by
  Tokutek as part of the Fractal Tree project.

  "PATENT CLAIMS" means the claims of patents that are owned or
  licensable by Tokutek, both currently or in the future; and that in
  the absence of this license would be infringed by THIS
  IMPLEMENTATION or by using or running THIS IMPLEMENTATION.

  "PATENT CHALLENGE" shall mean a challenge to the validity,
  patentability, enforceability and/or non-infringement of any of the
  PATENT CLAIMS or otherwise opposing any of the PATENT CLAIMS.

  Tokutek hereby grants to you, for the term and geographical scope of
  the PATENT CLAIMS, a non-exclusive, no-charge, royalty-free,
  irrevocable (except as stated in this section) patent license to
  make, have made, use, offer to sell, sell, import, transfer, and
  otherwise run, modify, and propagate the contents of THIS
  IMPLEMENTATION, where such license applies only to the PATENT
  CLAIMS.  This grant does not include claims that would be infringed
  only as a consequence of further modifications of THIS
  IMPLEMENTATION.  If you or your agent or licensee institute or order
  or agree to the institution of patent litigation against any entity
  (including a cross-claim or counterclaim in a lawsuit) alleging that
  THIS IMPLEMENTATION constitutes direct or contributory patent
  infringement, or inducement of patent infringement, then any rights
  granted to you under this License shall terminate as of the date
  such litigation is filed.  If you or your agent or exclusive
  licensee institute or order or agree to the institution of a PATENT
  CHALLENGE, then Tokutek may terminate any rights granted to you
  under this License.
*/

#ident "Copyright (c) 2007-2013 Tokutek Inc.  All rights reserved."
#ident "The technology is licensed by the Massachusetts Institute of Technology, Rutgers State University of New Jersey, and the Research Foundation of State University of New York at Stony Brook under United States of America Serial No. 11/760379 and to the patents and/or patent applications resulting from it."

// Generate a tree, do some left side deletions, and verify that all of the expected rows are present.
// A side effect is that basement nodes should be pruned, but there is no automated way to verify that.

#include "test.h"
#include <endian.h>
#ifndef htobe64
#include <byteswap.h>
#if __BYTE_ORDER == __LITTLE_ENDIAN
#define htobe64(x) __bswap_64(x)
#else
#define htobe64(x) (x)
#endif
#endif

static void populate(DB *db, DB_ENV *env, uint64_t n) {
    int r;
    
    DB_TXN *txn = nullptr;
    r = env->txn_begin(env, nullptr, &txn, 0);
    CKERR(r);

    for (uint64_t i = 0; i < n; i++) {

        uint64_t k = htobe64(i);
        DBT key = { .data = &k, .size = sizeof k };
        DBT val = { .data = &i, .size = sizeof i};
        r = db->put(db, txn, &key, &val, 0);
        CKERR(r);
    }

    r = txn->commit(txn, 0);
    CKERR(r);
}

static void deletesome(DB *db, DB_ENV *env, uint64_t n) {
    int r;
    
    DB_TXN *txn = nullptr;
    r = env->txn_begin(env, nullptr, &txn, 0);
    CKERR(r);

    for (uint64_t i = 0; i < n; i++) {
        uint64_t k = htobe64(i);
        DBT key = { .data = &k, .size = sizeof k };
        r = db->del(db, txn, &key, DB_DELETE_ANY);
        CKERR(r);
    }

    r = txn->commit(txn, 0);
    CKERR(r);

}

static void checkpoint(DB_ENV *env) {
    int r;
    r = env->txn_checkpoint(env, 0, 0, 0);
    CKERR(r);
}

static void populate_and_delete(uint64_t ninsert, uint64_t ndelete) {
    int r;

    // create the env
    DB_ENV *env = nullptr;
    r = db_env_create(&env, 0);
    CKERR(r);
    r = env->open(env, TOKU_TEST_FILENAME, 
                  DB_INIT_MPOOL|DB_CREATE|DB_THREAD |DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_TXN|DB_PRIVATE, 
                  S_IRWXU+S_IRWXG+S_IRWXO);
    CKERR(r);

    // create the db
    DB *db = nullptr;
    r = db_create(&db, env, 0);
    CKERR(r);
    r = db->open(db, nullptr, "foo.db", 0, DB_BTREE, DB_CREATE, S_IRWXU+S_IRWXG+S_IRWXO);
    CKERR(r);

    populate(db, env, ninsert);

    // optimize(db, env);

    checkpoint(env);

    deletesome(db, env, ndelete);

    checkpoint(env);

    r = db->close(db, 0);
    CKERR(r);

    r = env->close(env, 0);
    CKERR(r);
}

static void verify(uint64_t ninsert, uint64_t ndelete) {
    int r;

    // create the env
    DB_ENV *env = nullptr;
    r = db_env_create(&env, 0);
    CKERR(r);
    r = env->open(env, TOKU_TEST_FILENAME, 
                  DB_INIT_MPOOL|DB_CREATE|DB_THREAD |DB_INIT_LOCK|DB_INIT_LOG|DB_INIT_TXN|DB_PRIVATE, 
                  S_IRWXU+S_IRWXG+S_IRWXO);
    CKERR(r);

    // create the db
    DB *db = nullptr;
    r = db_create(&db, env, 0);
    CKERR(r);
    r = db->open(db, nullptr, "foo.db", 0, DB_BTREE, 0, S_IRWXU+S_IRWXG+S_IRWXO);
    CKERR(r);

    DB_TXN *txn = nullptr;
    r = env->txn_begin(env, nullptr, &txn, 0);
    CKERR(r);

    DBC *cursor = nullptr;
    r = db->cursor(db, txn, &cursor, 0);
    CKERR(r);

    while (1) {
        uint64_t k, v;
        DBT key = {}; key.data = &k; key.ulen = sizeof k; key.flags = DB_DBT_USERMEM;
        DBT val = {}; val.data = &v; val.ulen = sizeof v; val.flags = DB_DBT_USERMEM;
        r = cursor->c_get(cursor, &key, &val, DB_NEXT);
        if (r == DB_NOTFOUND)
            break;
        assert(key.size == key.ulen && val.size == val.ulen);
        assert(htobe64(k) == ndelete && v == ndelete);
        ndelete++;
    }
    assert(ndelete == ninsert);

    r = cursor->c_close(cursor);
    CKERR(r);
    
    r = txn->commit(txn, 0);
    CKERR(r);

    r = db->close(db, 0);
    CKERR(r);

    r = env->close(env, 0);
    CKERR(r);
}

int test_main (int argc, char *const argv[]) {
    default_parse_args(argc, argv);

    // init the env directory
    toku_os_recursive_delete(TOKU_TEST_FILENAME);
    int r = toku_os_mkdir(TOKU_TEST_FILENAME, S_IRWXU+S_IRWXG+S_IRWXO);   
    CKERR(r);

    // run the test
    populate_and_delete(1000000, 110000);
    verify(1000000, 110000);

    return 0;
}
