/* -*- mode: C; c-basic-offset: 4 -*- */
#ident "$Id$"
#ident "Copyright (c) 2010 Tokutek Inc.  All rights reserved."
#ident "The technology is licensed by the Massachusetts Institute of Technology, Rutgers State University of New Jersey, and the Research Foundation of State University of New York at Stony Brook under United States of America Serial No. 11/760379 and to the patents and/or patent applications resulting from it."

#ifndef TOKU_BRTLOADER_ERROR_INJECTOR_H
#define TOKU_BRTLOADER_ERROR_INJECTOR_H

#if defined(__cplusplus)
extern "C" {
#if 0 
}
#endif
#endif

#include "toku_atomic.h"

static toku_pthread_mutex_t event_mutex = TOKU_PTHREAD_MUTEX_INITIALIZER;
static void lock_events(void) {
    int r = toku_pthread_mutex_lock(&event_mutex); assert(r == 0);
}
static void unlock_events(void) {
    int r = toku_pthread_mutex_unlock(&event_mutex); assert(r == 0);
}
static int event_count, event_count_trigger;

__attribute__((__unused__))
static void reset_event_counts(void) {
    lock_events();
    event_count = event_count_trigger = 0;
    unlock_events();
}

__attribute__((__unused__))
static void event_hit(void) {
}

__attribute__((__unused__))
static int event_add_and_fetch(void) {
    lock_events();
    int r = ++event_count;
    unlock_events();
    return r;
    // return toku_sync_increment_and_fetch_int32(&event_count);
}

static int do_user_errors = 0;

__attribute__((__unused__))
static int loader_poll_callback(void *UU(extra), float UU(progress)) {
    int r;
    if (do_user_errors && event_count_trigger == event_add_and_fetch()) {
        event_hit();
        r = 1;
    } else {
        r = 0;
    }
    return r;
}

static int do_write_errors = 0;

__attribute__((__unused__))
static size_t bad_fwrite (const void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t r;
    if (do_write_errors && event_count_trigger == event_add_and_fetch()) {
        event_hit();
	errno = ENOSPC;
	r = -1;
    } else {
	r = fwrite(ptr, size, nmemb, stream);
	if (r!=nmemb) {
	    errno = ferror(stream);
	}
    }
    return r;
}

__attribute__((__unused__))
static ssize_t bad_write(int fd, const void * bp, size_t len) {
    ssize_t r;
    if (do_write_errors && event_count_trigger == event_add_and_fetch()) {
        event_hit();
	errno = ENOSPC;
	r = -1;
    } else {
	r = write(fd, bp, len);
    }
    return r;
}

__attribute__((__unused__))
static ssize_t bad_pwrite(int fd, const void * bp, size_t len, toku_off_t off) {
    ssize_t r;
    if (do_write_errors && event_count_trigger == event_add_and_fetch()) {
        event_hit();
	errno = ENOSPC;
	r = -1;
    } else {
	r = pwrite(fd, bp, len, off);
    }
    return r;
}

static int do_malloc_errors = 0;
static int my_malloc_count = 0, my_big_malloc_count = 0;
static int my_realloc_count = 0, my_big_realloc_count = 0;
static size_t my_big_malloc_limit = 64*1024;
   
__attribute__((__unused__))
static void reset_my_malloc_counts(void) {
    my_malloc_count = my_big_malloc_count = 0;
    my_realloc_count = my_big_realloc_count = 0;
}

__attribute__((__unused__))
static void *my_malloc(size_t n) {
    (void) toku_sync_fetch_and_increment_int32(&my_malloc_count); // my_malloc_count++;
    if (n >= my_big_malloc_limit) {
        (void) toku_sync_fetch_and_increment_int32(&my_big_malloc_count); // my_big_malloc_count++;
        if (do_malloc_errors) {
            if (event_add_and_fetch() == event_count_trigger) {
                event_hit();
                errno = ENOMEM;
                return NULL;
            }
        }
    }
    return malloc(n);
}

static int do_realloc_errors = 0;

__attribute__((__unused__))
static void *my_realloc(void *p, size_t n) {
    (void) toku_sync_increment_and_fetch_int32(&my_realloc_count); // my_realloc_count++;
    if (n >= my_big_malloc_limit) {
        (void) toku_sync_increment_and_fetch_int32(&my_big_realloc_count); // my_big_realloc_count++;
        if (do_realloc_errors) {
            if (event_add_and_fetch() == event_count_trigger) {
                event_hit();
                errno = ENOMEM;
                return NULL;
            }
        }
    }
    return realloc(p, n);
}

#if defined(__cplusplus)
}
#endif

#endif
