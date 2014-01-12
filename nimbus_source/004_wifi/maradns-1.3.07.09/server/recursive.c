/* Copyright (c) 2002-2007 Sam Trenholme
 *
 * TERMS
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * This software is provided 'as is' with no guarantees of correctness or
 * fitness for purpose.
 */

/* This is the only code that uses threads */
#include <pthread.h>

/* The MaraDNS includes */
#include "../libs/MaraHash.h"
#include "../MaraDns.h"
#include "../qual/qual_timestamp.h"
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <sys/types.h>
#ifndef MINGW32
#include <sys/resource.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#else /* MINGW32 */
#include <winsock.h>
#include <wininet.h>
#endif /* MINGW32 */
#include <fcntl.h>
#include <sys/param.h>
#include "../dns/functions_dns.h"
#include "../parse/functions_parse.h"
#include "../parse/Csv2_database.h"
#include "../parse/Csv2_read.h"
#include "../parse/Csv2_functions.h"
/* BEGIN RNG USING CODE */
#include "../rng/rng-api-fst.h"
/* END RNG USING CODE */
#include "functions_server.h"
#include "timestamp.h"

/* The locks used for multithreaded purposes */
pthread_mutex_t big_lock = PTHREAD_MUTEX_INITIALIZER;
int in_big_lock = 0;
pthread_mutex_t logwrite_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t rng_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t thr_lock = PTHREAD_MUTEX_INITIALIZER;


/* How long records will stay in the cache by default */
#define DEFAULT_TTL 86400

mhash *dnscache = 0;

/* The number of threads currently running */
int num_of_threads_running = 0;
int maximum_num_of_threads = 96;


/* The maximum allowed glueless level */
int max_glueless_level = 10;
/* Maximum allowed number of queries */
int max_queries_total = 32;
/* Maximum time to wait for a DNS server to respond */
int timeout_seconds = 2;
#define INCOMPLETE_CNAME_LIFETIME (timeout_seconds * 15)
fila *top = 0; /* The first record in the circular bidirectional linked list
                  of the order in which to get rid of unused RRs */

/* This is set to 0 if we send non-recursive queries to other servers;
   This is set to 1 if we send recursive queries to other servers and if
   we never follow NS referrals (to avoid certain nasty feedback loops) */
int root_or_upstream = 0;

/* This is 0 if we don't verbosely query every single query
 * This is 1 if we do */
int verbose_query = 0;

/* Minimum TTL for CNAME records and Mimimum TTL for non-CNAME records,
   in seconds */
int min_ttl_cname = 300;
int min_ttl_normal = 300;

/* The amount of logging that the end user desires; this does not
   output anything in the recursive code unless it is four or
   higher */
int rlog_level = 1;

/* How to handle the case of getting no reply at all from any of the
 * remote servers */
int handle_noreply = 2;

/* The number of times we try contacting all of the DNS servers to try
 * and resolve a name */
int retry_cycles = 2;

/* The port we connect to when contacting remote DNS servers */
int upstream_port = 53;

/* The lowest numbered port we will bind to and the number of ports
 * in our binding range */
int recurse_min_bind_port = 15000;
int recurse_number_ports = 4095;

typedef struct {
    int id;
    int sock;
    struct sockaddr_in client;
    js_string *query;
    } dnsreq; /* DNS request */

/* The final argument to the functions that add elements to the cache */
#define OVERWRITE 1
#define APPEND 2

/* Function prototypes so gcc doesn't complain when -Wall is set */
extern int udpany(int id,int sock,struct sockaddr_in *client,
                  js_string *query,int rr_set, mhash *bighash, int rd_val,
                  conn *ect, int called_from_recursive, js_string *origq);
extern int udperror(int sock,js_string *raw, struct sockaddr_in *from,
                    js_string *question,
                    int error,char *why, int min_log_level, int rd_val,
                    conn *ect,int log_msg);
extern int udpnotfound(rr *where, int id, int sock,
                       struct sockaddr_in *client,js_string *query,
                       int qtype, int rd_val, conn *ect,
                       int recursive_call);
extern int udpsuccess(rr *where, int id, int sock, struct sockaddr_in
                      *client, js_string *query, void **rotate_point,
                      int show_cname_a, int rd_val, conn *ect,
                      int force_authoritative, int ra_value);
extern int mhash_add_ip();
extern int mhash_put_data();


int recurse_call(int id, int sock, struct sockaddr_in client,
                 js_string *query, int queries_sent, int glueless_level,
                 uint32 *ipret, js_string *ptrret);
int unlink_rr(rr *fatma,int depth);
int unlink_closer(closer *fatma);
int in_bailiwick(js_string *host, js_string *bailiwick);
int cmp_ips(js_string *compare, int offset, uint32 ip);
int add_closer_jsip(js_string *zone, js_string *ipjs, int if_exists);
int add_closer_jsip_offset(js_string *js, int offset, js_string *ipjs,
                           int if_exists);
int add_closer_js_offset(js_string *js, int offset1, int offset2,
                         int if_exists);
int add_closer_jsip_offset(js_string *js, int offset, js_string *ipjs,
                           int if_exists);
int init_rng(js_string *seedfile, int rekey);
extern rr *init_ra_data();


/* BEGIN RNG USING CODE */
/* The variables that the srng (secure random-number generator) uses */
MARA_BYTE r_inBlock[17],r_outBlock[17],r_binSeed[17];
MARA_BYTE r_seedMaterial[320]; /* We may not eventually need this */
keyInstance r_seedInst;
cipherInstance r_cipherInst;
u_int32_t r_counter = 0;
/* END RNG USING CODE */
int r_place = 0;

int custodian_mode = 0; /* custodian mode: erase records from the cache
                           every time someone adds a records to the cache */
int cache_max = 0;

/* A bogus "not here" which is cached for 60 seconds handling the case
 * of being unable to contact any nameserver for a given domain */
rr *rra_data = 0;

/* A list of spam-friendly DNS servers (e.g. azmalink.net, etc.) */
ipv4pair spammers[512];

void log_lock() {
    pthread_mutex_lock(&logwrite_lock);
    }

void log_unlock() {
    pthread_mutex_unlock(&logwrite_lock);
    }

void do_big_lock() {
    pthread_mutex_lock(&big_lock);
    in_big_lock = 1;
    }

void big_unlock() {
    if(in_big_lock != 1) {
        log_lock();
        show_timestamp();
        printf("WARNING: Attempting to unlock when not locked\n");
        log_unlock();
        return;
        }
    in_big_lock = 0;
    pthread_mutex_unlock(&big_lock);
    }

void srng_lock() {
    pthread_mutex_lock(&rng_lock);
    }

void srng_unlock() {
    pthread_mutex_unlock(&rng_lock);
    }

void tcount_lock() {
    pthread_mutex_lock(&thr_lock);
    }

void tcount_unlock() {
    pthread_mutex_unlock(&thr_lock);
    }

/* Tell a function calling this function the number of threads currently
   running; used for debugging purposes
   Input: None
   Output: An integer which tells us the number of threads currently running
*/

int how_many_threads() {
    int ret;
    tcount_lock();
    ret = num_of_threads_running;
    tcount_unlock();
    return ret;
    }

/* Tell a function calling this function how many elements are in the
   DNS cache
   Input: None
   Output: An integer which tells us the number of elements in the
           DNS cache
*/

int cache_elements() {
    int ret;
    ret = dnscache->spots;
    return ret;
    }

/* Routine to set the handle_noreply value (how to handle the case of
   there being no reply at all from any of the remote DNS servers)
 */

int init_handle_noreply(int value) {
    handle_noreply = value;
    return value;
}

/* Routine used to determine if a given DNS label ends with the
   string ".arpa"; we use this information to determine whether we
   chase down the A or PTR record when given a CNAME record

   Input: JS string in the format of a DNS domain label followed
   by the two-byte query type (we ignore the query type)

   Output: 0 if it doesn't end in arpa, 1 if it does end in arpa,
           JS_ERROR on fatal error.

 */

int arpa_at_end_p(js_string *query) {
    js_string *match; /* Used for matching */
    int result;
    int counter = 0;

    if((match = js_create(10,1)) == 0) {
        return JS_ERROR;
        }

    /* Make the query we look for in the string '\004arpa\000' */
    if(js_qstr2js(match,"XarpaX") == JS_ERROR) {
        js_destroy(match);
        return JS_ERROR;
        }
    *(match->string + 0) = 4;
    *(match->string + 5) = 0;

    result = js_fgrep(match,query);
    while(result != -2 && result < query->unit_count - 8) {
        result = js_fgrep_offset(match,query,result + 1);
        if(counter++ > 45)
            break;
        }

    if(result != query->unit_count - 8) {
        js_destroy(match);
        return 0;
        }

    js_destroy(match);
    return 1;
    }

/* Code that handles the "custodial" maintainence of the cache. */

/* This is code which moves a given "fila" (line) structure to the
   top of the line.
   Input: Pointer to "fila" structure which we wish to move to the
          top.
   Output: JS_SUCCESS if we were able to move it to the top.
           JS_ERROR if something really bad happened
*/

int move_to_top(fila *element) {
    fila *before, *after;
    if(element == NULL)
        return JS_SUCCESS;
    /* No need to move an element already at the top to the top */
    if(element == top)
        return JS_SUCCESS;

    /* Remove this element from wherever it is in the line */
    before = element->previous;
    after = element->siguiente;
    if(before != 0)
        before->siguiente = after;
    if(after != 0)
        after->previous = before;

    /* And move it to the top */
    if(top != 0) {
        top->previous->siguiente = element;
        element->previous = top->previous;
        element->siguiente = top;
        top->previous = element;
        }
    else { /* Make a 1-element circular linked list */
        element->previous = element;
        element->siguiente = element;
        }
    top = element;

    return JS_SUCCESS;
    }

/* This is code which removes a given "fila" (line) structure
   Input: Pointer to the "fila" structure we wish to destroy
   Output: JS_SUCCESS on success, JS_ERROR on error
   Note: This only removes the fila structure, not whatever the fila
         structure points to
*/

int remove_fila(fila *zap) {
    fila *before, *after;

    /* Remove this element from wherever it is in the line */
    before = zap->previous;
    after = zap->siguiente;
    if(before != 0)
        before->siguiente = after;
    if(after != 0)
        after->previous = before;
    /* Handle the special case of this being the top element */
    if(zap == top) {
        /* Don't forget the even more special case of making this
           a zero-length list again */
        if(after != top)
            top = after;
        else
            top = 0;
        }
    /* OK, now that we have removed all pointers to this record, remove
       the record itself */
    if(zap->nukable_hp >= 1) {
        js_destroy(zap->hash_point);
        }
    return js_dealloc(zap);
    }

/* This is code which creates a new "fila" (line) structure, and places
   said structure at the top of the list.
   Input: Pointer to the "rr" or "closer" data structure;
          Whether the data structure is a "rr" structure (0), or a
          "closer" structure (1)
          The query used to point to this element in the hash;
          Whether said query is destroyable or not (whether the
          query was dynanically created for this zap element)
          nukable_ip values: 0: nuke nothing 1: Nuke the hquery string
                             2: The data structure is a rr; nuke
                                point->query also
   Output: Pointer to the fila data structure, 0 if something bad
           happened
*/

fila *new_fila(void *point, unsigned char type, js_string *hquery,
               char nukable_hp) {
    fila *new;

    /* Sanity checks */
    if(point == 0 || type > 1)
        return 0;

    /* Allocate memory */
    if((new = js_alloc(1,sizeof(fila))) == 0)
        return 0;

    /* The next record is whatever is at the top right now */

    /* The previous element is whatever is at the bottom right now */
    if(top != 0) {
        top->previous->siguiente = new;
        new->siguiente = top;
        new->previous = top->previous;
        top->previous = new;
        }
    else {
        /* 1-length circular linked list */
        new->siguiente = new;
        new->previous = new;
        }

    /* The data type is supplied by the user */
    new->datatype = type;
    /* The pointer to the record is also user-supplied */
    new->record = point;
    /* Also user supplied: The hash query which points to this element */
    new->hash_point = hquery;
    /* And whether we destroy it or not when removeing this fila element */
    new->nukable_hp = nukable_hp;

    /* Now, move the new record to the top */
    top = new;

    /* And return the pointer */
    return new;
    }

/* If we hit the maximum number of elements in the cache, this hits
   "custodian" mode, which erases elements as needed
   Input: None  (global variables used)
   Output: JS_ERROR if something really bad happened, otherwise
           JS_SUCCESS
*/

int custodian() {
    int counter;
    closer *cpoint;
    fila *zap;
    void *bye;
    /* Sanity check */
    if(dnscache == 0 || cache_max == 0)
        return JS_ERROR;
    /* If we are within our limits, do nothing */
    if(dnscache->spots < cache_max && custodian_mode <= 0)
        return JS_SUCCESS;
    /* If we have exceeded the maximum size, go in to "custodian"
       mode, where we destroy one percent of the records in the cache */
    if(dnscache->spots >= cache_max)
        custodian_mode = cache_max / 100;

    /* Eight records get the axe */

    counter = 8;
    while(counter > 0) {

        /* Remove the record at the bottom of the fila list */
        if(top != 0)
            zap = top->previous;
        else
            return JS_ERROR;

        bye = zap->record;

        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Custodian is zapping record ");
            show_esc_stdout(zap->hash_point);
            printf(" at %p\n",zap->record);
            log_unlock();
            }

        /* Remove the pointer to this hash element */
        if(mhash_undef(dnscache,zap->hash_point) == 0) {
            rr *rrb;

            rrb = (rr *)zap->record;

            show_timestamp();
            printf("Unable to remove element from hash.\n");
            printf("I thought I fixed this bug.  Please report this\n");
            printf("to list@maradns.org.\n");
            printf("The element is (for debugging purposes): %p %p ",
                    zap->hash_point,zap->record);
            if(zap->datatype == 0) /* rr */ {
                printf("--rr-- %p ",rrb->query);
                show_esc_stdout(rrb->query);
                }
            else if(zap->datatype == 1) /* closer */ {
                printf("--closer--");
                }
            printf(" ||| ");
            show_esc_stdout(zap->hash_point);
            printf("\n");
            exit(1);
            }

        /* Destroy the hash element pointed to by the fila pointer */
        switch(zap->datatype) {
            case 0: /* rr */
                if(bye != NULL)
                    unlink_rr((rr *)bye,0);
                break;
            case 1: /* closer */
                cpoint = bye;
                if(bye != NULL &&
                   cpoint->ttd != 0 /* Never nuke root server entries */
                   )
                    unlink_closer(cpoint);
            }

        counter--;
        }

    custodian_mode -= 8;
    return JS_SUCCESS;

    }

/* A cryptgraphically secure random number with a value between
   0 and 65535
   Input: None
   Output: A number between 0 and 65535
*/
uint16 srng() {
     uint16 ret;
     srng_lock();
     /* BEGIN RNG USING CODE */
     /* If needed, rerun the encryption to create 128 more random bits */
     if(r_place >= 16) {
         /* Four bytes are from the previous ciphertext.
            This is akin to OFB mode */
         r_inBlock[8] = r_outBlock[12];
         r_inBlock[9] = r_outBlock[13];
         r_inBlock[10] = r_outBlock[14];
         r_inBlock[11] = r_outBlock[15];
         /* Four bytes of the "plaintext" are a counter that increments
            every time we create a new cipertext block.
            This is akin to "counter" mode */
         r_inBlock[12] = (r_counter >> 24) & 0xff;
         r_inBlock[13] = (r_counter >> 16) & 0xff;
         r_inBlock[14] = (r_counter >> 8) & 0xff;
         r_inBlock[15] = r_counter & 0xff;
         r_counter++;
         /* Up to eight bytes of the "plaintext" is a timestamp that changes
            every second. */
         time((time_t *)&r_inBlock[0]);
         /* Re-key every 1,000,000 encryptions; this protects against
          * cache timing attacks */
         if((r_counter & 0xfffff) == 0xfffff) {
             if(rlog_level >= 3) {
                 printf("\nRe-keying rng seed...\n");
                 }
             if(init_rng(0,1) < 0) {
                 printf("WARNING: Problem rekeying\n");
                 }
             }
         blockEncrypt(&r_cipherInst,&r_seedInst,r_inBlock,128,r_outBlock);
         r_place = 0;
         }
     ret = ((r_outBlock[r_place] & 0xff) << 8) |
            (r_outBlock[r_place + 1] & 0xff);
     r_place += 2;
     /* END RNG USING CODE */
     srng_unlock();
     return ret;
     }

/* Destory a given linked list of "closer" records.
   Input:  Pointer to linked list we wish to destroy
   Output: JS_ERROR on error, JS_SUCCESS on success
   Note: This does not destroy whatever element in the hash points
         to this element
*/

int unlink_closer(closer *fatma) {

    closer *remember;

    while(fatma != NULL) {
        switch(fatma->datatype) {
            case RR_NS:
                if(fatma->data != NULL)
                    js_destroy(fatma->data);
                break;
            default:
                if(fatma->data != NULL)
                    js_dealloc(fatma->data);
                break;
            }
        remember = fatma;

        if(remember->zap != NULL)
            remove_fila(remember->zap);
        fatma = fatma->next;
        js_dealloc(remember);
        }

    return JS_SUCCESS;
    }

/* Destory a given linked list of resource records.
   Input:  Pointer to linked list we wish to destroy
   Output: JS_ERROR on error, JS_SUCCESS on success
   Note: This does not destroy whatever element in the hash points to
         this element
*/

int unlink_rr(rr *fatma,int depth) {

    /* There are times when we want to zap fatma->query, and there are
       times when we do not want to zap fatma->query, depending on whether
       fatma->query points directly to the hash key or is a copy of the
       hash key (possibly modified).  This code assumes that, in the second
       case, either one of two things are true:

       1) fatma->zap->nukable_hp is equal to 2
       2) We are pointing to the IP sub-branch of the record in question.

       There really needs to be a better way of handling this.
    */

    rr *remember;

    if(depth > 32) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf(" unlinking depth exceeded\n");
            log_unlock();
            }
        return JS_ERROR; /* Maximum allowed depth is 32 */
        }

    while(fatma != 0) {

        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Unlinking RR at %p, depth %d, next %p\n",fatma,depth,
                   fatma->next);
            log_unlock();
            }

        if(fatma->ip != 0)
            unlink_rr(fatma->ip,depth + 1);

        /* We normally do not destroy fatma->query because that js string
           object has normally already been destroyed */
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Destroying data at %p\n",fatma->data);
            log_unlock();
            }
        js_destroy(fatma->data);
        if(fatma->ptr != 0) {
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                printf("Destroying ptr at %p\n",fatma->ptr);
                log_unlock();
                }
            js_destroy(fatma->ptr);
            }
        /* A depth greater than 1 means that the query is a
           separate js_string object that also needs to be nuked */
        if(fatma->zap == NULL && depth >= 1) {
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                printf("Destroying query at %p\n",fatma->query);
                log_unlock();
                }
            js_destroy(fatma->query);
            }
        remember = fatma;
        fatma = fatma->next;
        if(remember->zap != NULL) {
            /* A nukable_hp value of 2 means that the query is a
               separate js_string object that also needs to be nuked */
            if(remember->zap->nukable_hp == 2) {
                js_destroy(remember->query);
                }
            remove_fila(remember->zap);
            }

        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Destroying remember at %p\n",remember);
            log_unlock();
            }

        js_dealloc(remember);
        }

    return JS_SUCCESS;
    }

/* Determine if a given substring of one long string is the same value as
   another string.
   Input: Pointer to first long js_string obejct, offset from which we
          begin comparison, length of subsction of string we compare,
          Pointer to second short js_string object
   Ouput: JS_ERROR on fatal error, 0 if they are different, 1 if they
          are the same
*/

int substring_issame(js_string *longjs, int offset, int length,
                     js_string *shortjs) {
    js_string *compare;
    int ret;
    /* Sanity checks */
    if(longjs->unit_size != 1 || shortjs->unit_size != 1)
        return JS_ERROR;
    if(offset < 0 || length < 1)
        return JS_ERROR;
    /* Create the temporary string used for comparision purposes */
    if((compare = js_create(length + 1,1)) == 0)
        return JS_ERROR;
    if(js_substr(longjs,compare,offset,length) == JS_ERROR) {
        js_destroy(compare);
        return JS_ERROR;
        }
    ret = js_issame(shortjs,compare);
    js_destroy(compare);
    return ret;
    }

/* Determine if a given substring of one long string is the same value as
   another string, taking in to account that this may be case-insensitive,
   and taking in to account that a query for "RR_ANY" can return any record
   type.
   Input: Pointer to first long js_string obejct, offset from which we
          begin comparison, length of subsction of string we compare,
          Pointer to second short js_string object
   Ouput: JS_ERROR on fatal error, 0 if they are different, 1 if they
          are the same
*/

int substring_issame_case(js_string *longjs, int offset, int length,
                          js_string *shortjs) {
    js_string *compare, *lower;
    int ret, case_folded1, case_folded2, counter, qtype_long,
    qtype_short;
    lower = 0;
    /* Sanity checks */
    if(longjs->unit_size != 1 || shortjs->unit_size != 1)
        return JS_ERROR;
    if(offset < 0 || length < 1)
        return JS_ERROR;
    /* Create the temporary string used for comparision purposes */
    if((compare = js_create(length + 1,1)) == 0)
        return JS_ERROR;
    if(js_substr(longjs,compare,offset,length) == JS_ERROR) {
        js_destroy(compare);
        return JS_ERROR;
        }
    /* Get the query types of the short and the compare strings */
    qtype_long = get_rtype(compare);
    qtype_short = get_rtype(shortjs);
    /* Yet another sanity check */
    if(qtype_long == JS_ERROR || qtype_short == JS_ERROR) {
        js_destroy(compare);
        return JS_ERROR;
        }
    /* If the short js is an ANY query, it will match whatever query
       type the long js has */
    if(qtype_short == RR_ANY) {
        if(change_rtype(shortjs,qtype_long) == JS_ERROR) {
            js_destroy(compare);
            return JS_ERROR;
            }
        }

    /* Do the first actual comparison */
    ret = js_issame(shortjs,compare);
    /* Sanity check */
    if(ret == JS_ERROR) {
        js_destroy(compare);
        change_rtype(shortjs,qtype_short);
        return JS_ERROR;
        }
    /* If they do not match, pweform a case-insensitive search */
    if(ret == 0) {
        if((lower = js_create(shortjs->unit_count + 1,1)) == 0) {
            js_destroy(compare);
            change_rtype(shortjs,qtype_short);
            return JS_ERROR;
            }
        ret = JS_ERROR; /* Return appropraite error if we go to cleanup */
        if(js_copy(shortjs,lower) == JS_ERROR)
            goto cleanup;
        case_folded1 = fold_case(lower);
        case_folded2 = fold_case(compare);
        /* If either string had folded case
           (one or more upper case letters) */
        if(case_folded1 == JS_SUCCESS || case_folded2 == JS_SUCCESS) {
            /* See if the case-insensitive forms are the same.  If not,
               destroy strings and return 0. */
            ret = js_issame(lower,compare);
            if(ret == JS_ERROR)
                goto cleanup;
            if(ret == 0)
                goto cleanup;
            /* OK, they match when both are lowercase.  Now fold the case
               of the strings we sent to the routine */
            /* Folding the case of the short string is simple (and, yes,
               we could have done this above, but this is simpler to read) */
            if(case_folded1 == JS_SUCCESS)
                fold_case(shortjs);
            /* Folding the case of a substring of the longer string
               is not-so-simple */
            if(case_folded2 == JS_SUCCESS) {
                counter = offset;
                while(counter + 2 < offset + length) {
                    if(*(longjs->string + counter) >= 'A' &&
                       *(longjs->string + counter) <= 'Z') {
                        *(longjs->string + counter) += 32;
                        }
                    counter++;
                    }
                }
            }
        else {
            ret = 0;  /* Both lower case, so they are different */
            }
        }
    cleanup:
        if(lower != 0)
            js_destroy(lower);
        js_destroy(compare);
        change_rtype(shortjs,qtype_short);
        return ret;
    }

/* This is how we are both case-insensitive and
   case-perserving with domain names.  If the case
   of the answer differs from the case of the
   question, we make what we add to the cache
   lower-case (case-insensitive) */

int check_case_of_answer(js_string *uindata, js_string *query, int offset) {
    js_string *copy;
    int result;
    if((copy = js_create(query->unit_count + 3,1)) == 0)
        return JS_ERROR;
    if(js_copy(query,copy) == JS_ERROR) {
        js_destroy(copy);
        return JS_ERROR;
        }
    /* Since it is the NS records we need to change the case of, make
       copy a NS query */
    if(change_rtype(copy,RR_NS) == JS_ERROR) {
        js_destroy(copy);
        return JS_ERROR;
        }

    result = 0;
    do {
        /* We change the case of the query as appropriate */
        result = substring_issame_case(uindata,offset,
                    dlabel_length(uindata,offset) + 2,
                    copy);
        } while(bobbit_label(copy) > 0 && result <= 0);

    js_destroy(copy);
    return JS_SUCCESS;
    }

/* Add a substring of a js_string object to the cache hash.
   Input: Pointer to hash to add element to, Pointer to long string,
          offset of substr, length of substr, ttl of query, pointer to
          query, whether to put new element (1) or to add to end of
          existing object (2)
   output: JS_ERROR on error, JS_SUCCESS on success
*/

int substring_add_rr(mhash *hashp, js_string *longjs, int offset, int length,
                 uint32 ttl, js_string *query, int action, int datatype,
                 int rcode) {
    js_string *sub = 0;
    int ret;
    uint32 ttd;
    /* Sanity checks */
    if(longjs->unit_size != 1 || query->unit_size != 1)
        return JS_ERROR;
    if(offset < 0 || length < 1)
        return JS_ERROR;
    if(action < 1 || action > 2)
        return JS_ERROR;

    /* Create and set the value of the substr that we will put the value in */
    if((sub = js_create(length + 1,1)) == 0)
        return JS_ERROR;
    if(js_substr(longjs,sub,offset,length) == JS_ERROR) {
        js_destroy(sub);
        return JS_ERROR;
        }

    /* Make the ttl the time this record dies */
    ttd = qual_get_time() + ttl;
    /* Note that mhash_put_data actually doesn't use the ttd value
       (Yuk) */
    ret = mhash_put_data(hashp,query,sub,ttl,0,ttd,datatype,255,action,rcode);
    custodian();
    js_destroy(sub);
    return ret;
    }

/* Determine if a host label with is a substring of a long js
   object is in bailiwick.
   Input: String with host name, offset of beginning of host name,
          bailiwick we are allowed to be in
   Output: JS_ERROR (-1) on fatal error, 0 if host name is out
           of bailiwick, 1 if host name is in bailiwick
*/

int offset_bailiwick(js_string *js, int offset, js_string *bailiwick) {
    js_string *sub;
    int len, ret, len_save;

    /* Sanity check */
    if(bailiwick->unit_size != 1)
        return JS_ERROR;

    /* Determine how long the domain label is */
    len = dlabel_length(js,offset);
    if(len == JS_ERROR)
        return JS_ERROR;

    /* Make a string which holds just the domain name label */
    if((sub = js_create(len + 1,1)) == 0)
        return JS_ERROR;
    if(js_substr(js,sub,offset,len) == JS_ERROR) {
        js_destroy(sub);
        return JS_ERROR;
        }

    /* Just in case Bailiwick is longer than the dlabel at the top of the
       Bailiwick, truncate the bailiwick */
    len_save = bailiwick->unit_count;
    len = dlabel_length(bailiwick,0);
    if(len == JS_ERROR) {
        js_destroy(sub);
        return JS_ERROR;
        }
    bailiwick->unit_count = len;
    /* Run in_bailiwick with the substring */
    ret = in_bailiwick(sub,bailiwick);
    bailiwick->unit_count = len_save;
    js_destroy(sub);
    return ret;
    }

/* Determine if a given host name is in the bailiwick of another
   domain name.
   Input: Host name we are querying, bailiwick we are allowed to be in
          These host names are *not* followed by a two-byte rr type
   Output: JS_ERROR (-1) on fatal error, 0 if host name is out
           of bailiwick, 1 if host name is in bailiwick
   Note: This is case-insensitive due to occassional bailiwick rejections
         from MaraDNS' attempts to have some level of case sensitivity
*/

int in_bailiwick(js_string *host, js_string *bailiwick) {
    js_string *get,*b_lower;
    int result;
    if((get = js_create(host->unit_count + 3,1)) == 0)
        return JS_ERROR;
    if((b_lower = js_create(bailiwick->unit_count + 3,1)) == 0) {
        js_destroy(get);
        return JS_ERROR;
        }
    if(js_copy(host,get) == JS_ERROR) {
        js_destroy(get);
        js_destroy(b_lower);
        return JS_ERROR;
        }
    if(js_copy(bailiwick,b_lower) == JS_ERROR) {
        js_destroy(get);
        js_destroy(b_lower);
        return JS_ERROR;
        }

    get->encoding = JS_US_ASCII;
    b_lower->encoding = JS_US_ASCII;

    /* Make both strings lower-case before doing the comparison */
    if(js_tolower(get) == JS_ERROR) {
        js_destroy(get);
        js_destroy(b_lower);
        return JS_ERROR;
        }
    if(js_tolower(b_lower) == JS_ERROR) {
        js_destroy(get);
        js_destroy(b_lower);
        return JS_ERROR;
        }

    get->encoding = bailiwick->encoding;
    b_lower->encoding = bailiwick->encoding;

    result = 0; /* Out of bailiwick */
    if(js_issame(get,b_lower) == 1) {
        js_destroy(get);
        js_destroy(b_lower);
        return 1; /* In bailiwick */
        }
    while(result == 0 && get->unit_count > b_lower->unit_count) {
        if(bobbit_label(get) == JS_ERROR) {
            js_destroy(get);
            js_destroy(b_lower);
            return JS_ERROR;
            }
        if(js_issame(get,b_lower) == 1) {
            js_destroy(get);
            js_destroy(b_lower);
            return 1; /* In bailiwick */
            }
        }
    js_destroy(get);
    js_destroy(b_lower);
    return result; /* Out of bailiwick */
    }

/* This is similiar to in_bailiwick, but the host names in question
   *do* have 2-byte query-type suffixes.  See the in_bailiwick comments
   above for arguments, etc. */
int q_bailiwick(js_string *host, js_string *bailiwick) {
    int ret;
    if(host->unit_count < 3) {
        return JS_ERROR;
        }
    if(bailiwick->unit_count < 3) {
        return JS_ERROR;
        }
    host->unit_count -= 2;
    bailiwick->unit_count -= 2;
    ret = in_bailiwick(host,bailiwick);
    host->unit_count += 2;
    bailiwick->unit_count += 2;
    return ret;
    }

/* Add a name to the dns cache
   Input: Pointer to hash, host name and type, value to put there,
          host ttl, authoritative flag,
          expire, datatype (whether this is a positive or negative answer),
          rtype of data (255 means it uses the rtype that query has), and
          action to perform (1: Destory any element that already exists
          at the hash spot in question, if applicable.  2: Append to any
          element that is already exists at the hash spot in question)
   Note: rtype is always called with a value of 255; code which uses other
         rtype values no longer exists
   Output: JS_ERROR on error, JS_SUCCESS on success
   Yuk: The expire argument is actually not used
*/

int mhash_put_data(mhash *hash, js_string *query, js_string *value, uint32 ttl,
                   uint32 authoritative, uint32 expire, int datatype,
                   int rtype, int action, int rcode) {

    rr *data = 0, *point = 0;
    js_string *new = 0, *new_query = 0, *zap_query = 0;
    int retz = JS_ERROR;
    int rrtype;
    int add_zap = 0;
    int use_immutable_key = 0;
    mhash_e spot_data;

    /* Create a structure for putting the rr data in */
    if((data = js_alloc(1,sizeof(rr))) == 0)
        return JS_ERROR;

    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        printf("Adding RR/psudo-NXDOMAIN ");
        show_esc_stdout(query);
        printf(" to cache at %p\n",data);
        log_unlock();
        }

    /* First, clear out all the fields */
    init_rr(data);
    /* The following is really fluff */
    data->expire = data->ttl = data->authoritative = data->rr_type = 0;
    data->next = data->ip = 0;
    data->query = data->data = 0;
    data->ptr = 0;
    data->seen = 0;
    data->zap = 0;

    /* Store the simple data in the rr data */
    data->authoritative = authoritative;
    if(rcode == 3) {
            data->rcode = 3;
    } else {
            data->rcode = 0;
    }
    /* Get the rr type from the query string */
    if(datatype == MARA_DNS_NEG)
        rrtype = RR_SOA;
    else if(rtype == 255)
        rrtype = get_rtype(query);
    else
        rrtype = rtype;
    if(rrtype == JS_ERROR) {
        js_dealloc(data);
        return JS_ERROR;
        }
    if(rrtype < 0 || rrtype > 65535) {
        js_dealloc(data);
        return JS_ERROR;
        }
    data->rr_type = rrtype;

    /* The minimum TTL is determined by whether this is CNAME or non-CNAME
       data */
    data->ttl = ttl;
    if(rrtype == RR_CNAME) {
        if(ttl < min_ttl_cname) {
            data->expire = qual_get_time() + min_ttl_cname;
            data->ttl = min_ttl_cname;
        } else {
            data->expire = qual_get_time() + ttl;
        }
    } else {
        if(ttl < min_ttl_normal) {
            data->expire = qual_get_time() + min_ttl_normal;
            data->ttl = min_ttl_normal;
        } else {
            data->expire = qual_get_time() + ttl;
        }
    }
    /* Thanks to Hugo Vanwoerkom for pointing out that Aww4.janus.com.
       doesn't reaolve in Mozilla; it has a TTL of 0 (ugh), which
       confuses stub resolver libraries.  This code is not necessary
       now because of the above code */
    /* if(ttl > 30)
        data->ttl = ttl;
    else
        data->ttl = 30; */

    /* Create a js_string object to store the raw binary answer */
    if((new=js_create(value->unit_count + 1,value->unit_size)) == 0) {
        js_dealloc(data);
        return JS_ERROR;
        }
    if(js_copy(value,new) == JS_ERROR) {
        js_dealloc(data);
        js_destroy(new);
        return JS_ERROR;
        }
    /* And put a pointer to that js_string in the rr data */
    data->data = new;
    /* This is a new record, so the pointers to other records this uses
       are blank */
    data->ip = data->next = 0;

    /* The structure that we use needs to have a pointer to the query,
       so that udpsuccess can form answers.  Usually, this will simply
       point to the hash element, but we need to handle the special
       case of SOA negative responses */

    /* Note that rtype is *always* 255 */
    if(datatype == MARA_DNS_NEG || rtype != 255) {
        if((new_query = js_create(query->unit_count + 1,1)) == 0) {
            js_dealloc(data);
            js_destroy(new);
            return JS_ERROR;
            }
        if(js_copy(query,new_query) == JS_ERROR) {
            js_destroy(new_query);
            js_dealloc(data);
            js_destroy(new);
            return JS_ERROR;
            }
        if(change_rtype(new_query,rrtype) == JS_ERROR) {
            js_destroy(new_query);
            js_dealloc(data);
            js_destroy(new);
            return JS_ERROR;
            }
        if((zap_query = js_create(query->unit_count + 1,1)) == 0) {
            js_destroy(new_query);
            js_dealloc(data);
            js_destroy(new);
            return JS_ERROR;
            }
        if(js_copy(query,zap_query) == JS_ERROR) {
            js_destroy(new_query);
            js_destroy(zap_query);
            js_dealloc(data);
            js_destroy(new);
            return JS_ERROR;
            }
        data->query = new_query;
        add_zap = -1;
        }
    else {
        /* Since the hash element has not been created yet, we need to
           flag that we will point to the key in the hash after making
           said key */
        use_immutable_key = 1;
        }

    /* OK, now add the data to the big hash */

    /* Handle the case of there already being an element in the hash at the
       desired point */
    spot_data = mhash_get(hash,query);
    if(spot_data.value != 0) {
        /* If the action is to overwrite, delete the element that is
           currently there */
        if(action == OVERWRITE || spot_data.datatype == MARA_DNS_NS) {
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                printf("Element already in hash at %p, zapping\n",
                       spot_data.value);
                log_unlock();
                }

            if(add_zap == 0)
                add_zap = 1;
            /* We also overwrite in the [should never happen] case of the
               element in question being a MARA_DNS_NS element */
            switch(spot_data.datatype) {
                case MARA_DNSRR:
                case MARA_DNS_NEG:
                    unlink_rr(spot_data.value,0);
                    mhash_undef(hash,query);
                    break;
                case MARA_DNS_NS:
                    unlink_closer(spot_data.value);
                    mhash_undef(hash,query);
                    break;
                default: /* There should not be any other data types
                            in the hash */
                    js_dealloc(data);
                    js_destroy(new);
                    if(add_zap == -1) {
                        js_destroy(new_query);
                        js_destroy(zap_query);
                        }
                    return JS_ERROR;
                }
            }
        else if(action == 2) { /* Append */
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                printf("Appending element in hash at %p\n",
                       spot_data.value);
                log_unlock();
                }
            point = spot_data.value;
            while(point->next != NULL)
                point = point->next;
            point->next = data;
            }
        }

    /* Store that data in the big hash, but only if it is appropriate
       to create a new element */
    if(action == OVERWRITE ||
       spot_data.value == 0 /* Nothing to append to: Create new element */ ) {
        retz = mhash_put(hash,query,data,datatype);
        if(retz == JS_ERROR) {
            js_dealloc(data);
            js_destroy(new);
            if(add_zap == -1) {
                js_destroy(new_query);
                js_destroy(zap_query);
                }
            return JS_ERROR;
            }
        if(add_zap == 0)
            add_zap = 1;
        }

    /* If appropriate, have data->query point to the newly created hash
       element */
    if(use_immutable_key == 1)
        data->query = mhash_get_immutable_key(hash,query);

    /* Also, set up the place where this is on the "zap" list (The list
       the custodian uses to zap [get rid of] records) */
    if(add_zap == 1)
        data->zap = new_fila(data,0,data->query,0);
    else if(add_zap == -1)
        data->zap = new_fila(data,0,zap_query,2);

    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        printf("Sucessfully added ");
        show_esc_stdout(query);
        printf(" to cache at %p\n",data);
        log_unlock();
        }

    return JS_SUCCESS;
    }

/* If we were given a negative answer, we have to add the fact that
   the record does not exist to our cache, and return the negative
   answer back to the stub resolver.
   Input: The query that they sent us;
          A pointer to the js_string with the uncompressed reply from the
          DNS server; offset from the beginning of that string where the
          negative SOA reply begins; pointer to header of reply
   Output: JS_ERROR if something nasty happened, JS_SUCCESS if we managed
           to add the data to our cache.
   Global variables used: The dns cache, of course.
*/

int handle_negative_data(js_string *query, js_string *server_reply,
                int offset, q_header *header) {
    int query_type, len;
    uint32 ttl;
    int rdlength, ret;

    /* Determine the original query type they asked for */
    query_type = get_rtype(query);
    if(query_type == JS_ERROR)
        return JS_ERROR;

    /* Dealing with negativity is such a pain.
       What our code does is create a special element in the cache that
       can easily be used by the already existing udpnotfound routine.
       We make the data an ordinary rr.  We can determine the expire by
       the ttd in the rr, and the contents the RR points to is the data
       that we show the end user.
     */

    /* Determine the length of the answer */
    len = dlabel_length(server_reply,offset);
    if(len == JS_ERROR)
        return JS_ERROR;
    /* Add the type to the length */
    len += 2;
    /* Go past the query, type, and class */
    offset += len + 2;
    /* Get the TTL */
    /* XXX: RFC1034 says you use the SOA minimum as the TTL.  Instead, we
       will use the TTL of the SOA record itself */
    ttl = js_readuint32(server_reply,offset);
    if(ttl == JS_ERROR)
        return JS_ERROR;
    offset += 4;
    /* Get the rdlength of the SOA record */
    rdlength = js_readuint16(server_reply,offset);
    if(rdlength == JS_ERROR)
        return JS_ERROR;
    /* Add the rddata to the big hash */
    offset += 2;
    ret = substring_add_rr(dnscache,server_reply,offset,rdlength,ttl,query,
                            1,MARA_DNS_NEG,header->rcode);
    return ret;
    }

/* Given two pointers at differnet places in the same js_string object,
   both of which point to a dns-style domain name (in binary format),
   determine if the two objects point to the same domain name.
   Input: Pointer to the JS string we are doing the comparison of, index of
          the first point we start our comparison at, index of the second
          point we start our comparison at.
   Output: JS_ERROR (-1) on fatal error, 0 if they do not match, 1 if
           they do match */
int cmp_dnames(js_string *js, int p1, int p2) {
    int temp, length;

    if(js_has_sanity(js) == JS_ERROR)
        return JS_ERROR;

    /* Make sure p1 is smaller than p2 */
    if(p1 > p2) {
        temp = p1;
        p1 = p2;
        p2 = temp;
        }

    /* If they point to the same place, they are the same (obviously) */
    if(p1 == p2)
        return 1;

    /* If p1 or p2 are out of bounds, then return error
       Optimization: p2 is greater then p1, so we only have to check to see
       if p1 is less then zero */
    if(p1 < 0 || p1 > js->unit_count || p2 > js->unit_count)
        return JS_ERROR;

    length = dlabel_length(js,p1);

    /* They are not the same if they overlap */
    if(p1 + length > p2)
        return 0;

    /* They are not the same if they have different lengths */
    if(length != dlabel_length(js,p2))
        return 0;

    /* Make sure we are in bounds (optimization: we know p2 is greater than
       p1) */
    if(p2 + length > js->unit_count)
        return JS_ERROR;

    /* OK, now compare the two dlabels byte by byte */
    temp = 0;
    while(temp < length) {
        /* Since we occassionally have a server where the NS referrals and
         * the NSes have different cases (namely, in the process of resolving
         * www.gnu.org) */
        char cg_l, cg_r;
        cg_l = *(js->string + p1 + temp);
        cg_r = *(js->string + p2 + temp);
        if(cg_l >= 'A' && cg_l <= 'Z') { cg_l += 32; } /* Case conversion */
        if(cg_r >= 'A' && cg_r <= 'Z') { cg_r += 32; } /* Case conversion */
        if(cg_l != cg_r) {
            return 0;
            }
        temp++;
        }

    /* There is no difference between the labels.  Return 1 */
    return 1;

    }

/* Query a nameserver for the answer to our question.  Add the answer
   (it being a closer nameserver or the question being asked)
   If answer found, add it to the cache.  If pointer to other name servers
   given, add those pointers to this cache.
   Input: IP of server we will contact, query to send to the server,
          bailiwick of the IP of the server we will contact
   Output: JS_ERROR on fatal error, -2 if the nameserver could not
           be contacted (timeout), -3 if the namserver refuses to give us the
           the appropriate information, -4 if it is a lame delegation
           (non-authoritative data), JS_SUCCESS if we find a name server,
           2 if we got the final answer, 3 if we got the final answer and
           it is a CNAME record
*/

int query_nameserver(int remote_ip, js_string *query, js_string *bailiwick) {
    /* Lots of cut and paste from askmara.c here */
    struct sockaddr_in dns_udp, server;
    int len_inet; /* Length */
    int s; /* Socket */
    int nspoint, offset, len;
    int type, class, rdlength;
    int cname_original_record = 0; /* Use a variable for two purposes:
                                      - Determine if we cname'd this record
                                      - If so, what was the Query ID of their
                                        original query.
                                   */
    uint32 ttl;
    uint16 sid; /* Securly generated query id */
    uint16 ns_record_type; /* Whether this is a "host not found"/NXDOMAIN or a
                              NS referral */
    int record_added = 0; /* Boolean: has a record been added to the cache */
    int return_code_check = 0;
    js_string *outdata; /* Outgoing data */
    js_string *indata, *uindata; /* Incoming data (uncompressed version) */
    js_string *jsip; /* Place where we put the IP of the answer */
    uint32 ipq; /* IP which we pass the address of as an argument for
                   determining the IP that a CNAME records points to */
    js_string *ptrq = 0;
    q_header header; /* header data */
    int counter,count,ret;
    /* Select() [which we are using *only* because of its timeout ability]
       requires four variables to be used */
    fd_set rx_set; int maxd; struct timeval tv; int n;
    /* We keep track of what NS servers are servers for a given
       branch of the "domain" tree in the answer */
    uint16 ns_record[32]; /* Who is going to have more than 32 NS records
                             for a given domain name? */
    /* We keep track of that branch of the DNS "tree" the records are
       authoritative for in the answer */
    uint16 ns_domain[32];
    /* Jaakko changed this from struct sockaddr to struct sockaddr_in */
    struct sockaddr_in dummy; /* "dummy" so we can call recurse_call when
                              trying to determine the IP for a CNAME
                              record */
    mhash_e point; /* Used for the CNAME code */
    rr *spot;

    if(remote_ip == 0xffffffff || remote_ip == 0) { /* 255.255.255.255
                                                       and 0.0.0.0 */
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Invalid ip ");
            debug_show_ip(remote_ip);
            printf(" rejected\n");
            log_unlock();
            }
        return JS_ERROR;
        }

    /* End variable declaration, begin code */

    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        printf("Querying DNS server with ip ");
        debug_show_ip(remote_ip);
        printf(" for ");
        show_esc_stdout(query);
        printf(" with bailiwick ");
        show_esc_stdout(bailiwick);
        printf("\n");
        log_unlock();
        }

    /* Make sure they are not a spammer.  If they are, return an error */
    counter = 0;
    while(counter < 500 && (spammers[counter]).ip != 0xffffffff) {
        if((remote_ip & (spammers[counter]).mask) ==
            ((spammers[counter]).ip & (spammers[counter]).mask)) {
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                debug_show_ip(remote_ip);
                printf(" is a DNS server we won't talk to\n");
                log_unlock();
                }
            return JS_ERROR; /* We do not welcome replies from spam-friendly
                                DNS servers */
            }
        counter++;
        }

    /* Initialize ns_record and ns_domain */
    ns_record[0] = 0;
    ns_domain[0] = 0;

    /* Allocate memory for some strings */
    if((indata = js_create(512,1)) == 0) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Unable to allocate indata string\n");
            log_unlock();
            }
        return JS_ERROR;
        }
    if((uindata = js_create(2048,1)) == 0) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Unable to allocate uindata string\n");
            log_unlock();
            }
        js_destroy(indata);
        return JS_ERROR;
        }
    if((outdata = js_create(512,1)) == 0) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Unable to allocate outdata string\n");
            log_unlock();
            }
        js_destroy(indata);
        js_destroy(uindata);
        return JS_ERROR;
        }

    /* Create a server socket address to use with sendto() */
    memset(&server,0,sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(upstream_port);
    if((server.sin_addr.s_addr = htonl(remote_ip)) == INADDR_NONE) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Failure running htonl\n");
            log_unlock();
            }
        goto cleanup;
        }
    /* Create a secure psudo-random port number to bind to.
       Give credit where credit is due: This is loosely modelled after
       an example UDP client I found on http://www.pont.net */
    memset(&dns_udp,0,sizeof(dns_udp));
    dns_udp.sin_family = AF_INET;
    /* XXX also make this user-configurable (v1.2 feature) */
    dns_udp.sin_addr.s_addr = htons(INADDR_ANY);

    len_inet = sizeof(dns_udp);

    /* Format a DNS request */
    sid = srng();
    header.id = sid;
    header.qr = 0;
    header.opcode = 0;
    header.aa = 0;
    header.tc = 0;
    /* Root_or_upstream: Whether the server we are first contacting is
       one which is recursive itself, and which we use the recursion
       of, or if it is a server which we do not request recursion
       from.  0: No recursion desired from upstream server; 1: Recursion
       desired from upstream server */
    header.rd = root_or_upstream;
    header.ra = 0;
    header.z = 0;
    header.rcode = 0;
    header.qdcount = 1;
    header.ancount = 0;
    header.nscount = 0;
    header.arcount = 0;

    /* Make a beginning of a DNS query from that header */
    if(make_hdr(&header,outdata) == JS_ERROR) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Failure running make_hdr\n");
            log_unlock();
            }
        goto cleanup;
        }
    if(js_append(query,outdata) == JS_ERROR) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Failure running js_append\n");
            log_unlock();
            }
        goto cleanup;
        }
    if(js_adduint16(outdata,1) == JS_ERROR) { /* Adding class to query */
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Failure running js_adduint16\n");
            log_unlock();
            }
        goto cleanup;
        }

    /* OK, the DNS request has been formed.  Send it out */
    /* Note that rng-disabled versions of MaraDNS do not
       do this, since there is a chance that the underlying operating
       system will generate a secure source UDP port number if the
       source port is not bound */

    /* Create a UDP client socket */
    if((s = socket(AF_INET,SOCK_DGRAM,0)) == -1) {
        if(rlog_level >= 2) {
            log_lock();
            show_timestamp();
            printf("WARNING: Failure creating socket\n");
            log_unlock();
            }
        goto cleanup;
        }

    /* BEGIN RNG USING CODE */
    /* Bind to a secure psudo-random address and port */
    counter = 0;
    /* Try 10 times just in case the port is already bound */
    do {
        if(rlog_level >= 4 && counter > 0) {
            log_lock();
            show_timestamp();
            printf("Bind failed, trying again\n");
            log_unlock();
            }
        /* To add: Read mararc parameters which determine the range of this */
        dns_udp.sin_port = htons(recurse_min_bind_port +
                                 (srng() & recurse_number_ports));
        counter++;
        } while(bind(s,(struct sockaddr *)&dns_udp,sizeof(dns_udp)) < 0 &&
                counter < 10);

    if(counter >= 10) {
        close(s);
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Failure binding, giving up\n");
            log_unlock();
            }
        goto cleanup;
        }

    /* END RNG USING CODE */
    /* (End code snippet removed in rng-disabled version) */

    /* And send, on the same socket, the message to the server */
    /* Thanks to Rani Assaf for pointing out that you can actually
     * connect with a UDP connection */
#ifdef SELECT_PROBLEM
    /* Set socket to non-blocking mode to work around select() being
       unreliable in linux; packet may have been dropped.
     */
    fcntl(s, F_SETFL, O_NONBLOCK);
#endif
    connect(s, (struct sockaddr *)&server, sizeof(server));
    counter = send(s,outdata->string,outdata->unit_count,0);
    if(counter < 0) {
        close(s);
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Sendto failure\n");
            log_unlock();
            }
        goto cleanup;
        }

    /* Wait for a reply from the DNS server */
    FD_ZERO(&rx_set);
    FD_SET(s,&rx_set);
    maxd = s + 1;
    tv.tv_sec = timeout_seconds;
    tv.tv_usec = 0;
    /* Since a number of different threads will be at this point waiting
     * for a reply from a remote DNS server on a heavily loaded resolver,
     * we let each thread do a select() at the same time */
    big_unlock();
    n = select(maxd,&rx_set,NULL,NULL,&tv);
    /* OK, we're done waiting for the slow remote DNS server.  Lock the
     * thread again */
    do_big_lock();
    if(n == -1)  /* select error */ {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Select error\n");
            log_unlock();
            }
        close(s);
        goto cleanup;
        }
    if(n == 0) /* Timeout */ {
        close(s);
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Timeout contacting remote nameserver\n");
            log_unlock();
            }
        goto minus2;
        }
    /* Get the actual reply from the DNS server */
    if((count = recv(s,indata->string,indata->max_count,0)) < 0) {
        close(s);
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("recvfrom error\n");
            log_unlock();
            }
        goto minus3;
        }
    /* Now that we are done with the socket, close it */
    close(s);
    /* Process the reply */
    indata->unit_count = count;
    if(decompress_data(indata,uindata) == JS_ERROR) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Decompress failure: ");
            show_esc_stdout(indata);
            printf("\n");
            log_unlock();
            }
        goto minus3;
        }
    if(rlog_level >= 5) {
        log_lock();
        show_timestamp();
        printf("Decompressed packet: ");
        show_esc_stdout(uindata);
        printf("\n");
        log_unlock();
        }
    if(read_hdr(uindata,&header) == JS_ERROR) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("read_hdr failure\n");
            log_unlock();
            }
        goto minus3;
        }

    /* OK, there are two school of thoughts here.  If we get a different
       Query ID than the one we asked for, there are two possibilities here:

       * Somone who should not has sent us a packet in an attampt to spoof
         us a packet.

       * For whatever reason, this particular recvfrom() got the wrong
         packet.

       In terms of security against a spoofer, we have two choices:

       * We allow a spoofer to perform a DOS attack against us.

       * We allow a spoofer to have multiple chances to sucessfully
         spoof a packet sent to us.

       I opt to allow a spoofer to DOS us, since coding this is easier.

     */

    if(header.id != sid) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Bad id from remote server (possible spoof attempt)\n");
            log_unlock();
            }
        goto cleanup;
        }

    /* This bug was found when trying to help resolve macslash.net;
       this may be why the mysterious DNS faeries aren't so
       keen on resolving macslash.net */
    if(header.rcode != 0 && header.rcode != 3) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Bad rcode from remote server\n");
            log_unlock();
            }
        goto cleanup;
        }

    /* Let us find the offset of the first answer */
    count = header.qdcount;
    offset = 12; /* Point it at the first question/answer */
    while(count > 0) {
        len = dlabel_length(uindata,offset);
        if(len == JS_ERROR) {
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                printf("Bad dlabel_length\n");
                log_unlock();
                }
            goto cleanup;
            }
        offset += len;
        offset += 4; /* Move past the query type and query class */
        count--;
        }

    /* Determine if this is an answer or a referral */
    if(header.ancount > 0) {
        /* If this is an answer, add the answer to our cache */
        /* We get the following pieces of information:
           - The Time to live (and its delta from right now)
           - The exact query this is an answer for
           - A js string object which is the answer for the query
             (newly created)
        */
        nspoint = 1; /* We use ns point here to tell us if we are pointing
                        to the first record or to a later record */
        counter = header.ancount;
        do {
            int is_arpa_address = 0;
            /* Determine the length of the answer */
            len = dlabel_length(uindata,offset);
            if(len == JS_ERROR) {
                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    printf("Bad dlabel_length\n");
                    log_unlock();
                    }
                goto cleanup;
                }
            /* Add the type to the length */
            len += 2;
            /* Make sure that the answer is the same as the query.
               This routine also folds case as appropriate */
            return_code_check =
                substring_issame_case(uindata,offset,len,query);
            if(return_code_check == JS_ERROR) {
                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    printf("Bad return_code_check\n");
                    log_unlock();
                    }
                goto cleanup;
                }
            cname_original_record = 0;
            if(return_code_check != 1) {
                /* Perhaps this is a CNAME answer, which we also add
                   to the cache as appropriate */
                cname_original_record = get_rtype(query);
                if(cname_original_record == JS_ERROR) {
                    if(rlog_level >= 4) {
                        log_lock();
                        show_timestamp();
                        printf("Bad cname_original_record\n");
                        log_unlock();
                        }
                    goto cleanup;
                    }
                if(change_rtype(query,RR_CNAME) == JS_ERROR) {
                    if(rlog_level >= 4) {
                        log_lock();
                        show_timestamp();
                        printf("change_rtype problem\n");
                        log_unlock();
                        }
                    goto cleanup;
                    }
                /* Check to see if this is the CNAME form of the answer.
                 * If not... */
                if(substring_issame_case(uindata,offset,len,query) != 1) {
                    int rdlength;
                    /* We look at the next answer given and discard this
                     * one; we used to discard the entire DNS packet
                     * at this point but that, alas, breaks microsoft.com */
                    if(rlog_level >= 4) {
                        log_lock();
                        show_timestamp();
                        printf("substring_issame_case is skipping answer\n");
                        log_unlock();
                        }
                    /* Jump past CLASS and TTL */
                    offset += len + 6;
                    /* Bounds checking, as always */
                    if(offset + 2 > uindata->unit_count) {
                        if(rlog_level >= 4) {
                            log_lock();
                            show_timestamp();
                            printf("uindata string truncated\n");
                            log_unlock();
                            }
                        goto minus2;
                        }
                    /* Find out how long this answer is */
                    rdlength = ((*(uindata->string + offset ) & 0xff) << 8)
                               + *(uindata->string + offset + 1);
                    /* And jump past this answer */
                    offset += rdlength + 2;
                    /* Again, bounds checking */
                    if(offset >= uindata->unit_count) {
                        if(rlog_level >= 4) {
                            log_lock();
                            show_timestamp();
                            printf("uindata string ended\n");
                            log_unlock();
                            }
                        goto minus2;
                        }
                    /* And now let's reset some stuff so we can inspect
                     * the next answer the remote server gave us */
                    counter--;
                    is_arpa_address = 0;
                    if(change_rtype(query,cname_original_record) == JS_ERROR) {
                        if(rlog_level >= 4) {
                            log_lock();
                            show_timestamp();
                            printf("change_rtype restore problem\n");
                            log_unlock();
                            }
                        goto cleanup;
                        }
                    /* And jump back to the beginning of this do { loop */
                    continue;
                    }
                /* If it is a CNAME answer, DNS servers have this way
                   of politely also giving us the IP the CNAME points to
                   in the same query.  Because of how MaraDNS is structured,
                   this information does not help us, and is, in fact, an
                   error condition if we try grokking the helpful additional
                   answer.  Hence, we don't look at records after a CNAME
                   record in the answer */
                counter = 0;
                }
            /* Go past 'query', 'type', and 'class' */
            offset += len + 2;
            /* Get the ttl */
            ttl = js_readuint32(uindata,offset);

            if(ttl == JS_ERROR) {
                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    printf("bad ttl\n");
                    log_unlock();
                    }
                goto minus2;
                }
            /* Keep the TTL within sane bounds; should fix ALBATROS.BE
               problems that Franky reported */
            if(ttl < 20)
                ttl = 20;
            if(ttl > 63072000) /* Two years */
                ttl = 63072000;
            /* If this is a CNAME answer then we don't store it for over
             * 15 minutes */
            if(ttl > 900 && cname_original_record != 0)
                ttl = 900;

            offset += 4;
            /* Get the rdlength */
            rdlength = js_readuint16(uindata,offset);
            if(rdlength == JS_ERROR) {
                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    printf("bad rdlength\n");
                    log_unlock();
                    }
                goto minus2;
                }
            /* Add the rddata to the big hash */
            offset += 2;
            ret = substring_add_rr(dnscache,uindata,offset,rdlength,ttl,
                                   query,nspoint,MARA_DNSRR,header.rcode);
            /* If the record in question is a CNAME, we get one of the
               corresponding A records always.  If they want a non-A record,
               we still need to add the A record to the cache */
            /* Determine the IP that the CNAME record points to */
            if((jsip = js_create(256,1)) == 0) {
                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    printf("bad jsip\n");
                    log_unlock();
                    }
                goto cleanup;
                }
            ipq = 0;
            /* Find out what the possibly CNAME answer is so we can
             * query for it */
            point = mhash_get(dnscache,query);
            if(point.value != 0 && point.datatype == MARA_DNSRR) {
                spot = point.value;
                /* Put this element at the top of the line, so it
                   won't get nuked */
                move_to_top(spot->zap);
                /* If it is a CNAME */
                if(spot->data != 0 && spot->rr_type == RR_CNAME) {
                    /* In this case "jsip" actually has the
                       name that the CNAME record points so,
                       which we extract so that we can find out
                       the A record attached to the CNAME */
                    if(js_copy(spot->data,jsip) == JS_ERROR) {
                        if(rlog_level >= 4) {
                            log_lock();
                            show_timestamp();
                            printf("bad js_copy\n");
                            log_unlock();
                            }
                        goto cleanup_jsip;
                        }
                    /* Normally we want to add an A record.
                       However, in the case of a record in the *.arpa.
                       address space, we are usually much more interested
                       in the PTR record which a given CNAME record
                       points to */
                    is_arpa_address = arpa_at_end_p(query);
                    if(is_arpa_address == 1) {
                        if(js_adduint16(jsip,RR_PTR) == JS_ERROR) {
                            if(rlog_level >= 4) {
                                log_lock();
                                show_timestamp();
                                printf("bad js_adduint16 RR_PTR\n");
                                log_unlock();
                                }
                            goto cleanup_jsip;
                            }
                        if((ptrq = js_create(257,1)) == 0) {
                            goto cleanup_jsip;
                            }
                        if(rlog_level >= 4) {
                            log_lock();
                            show_timestamp();
                            printf("About to recursively chase ptr ");
                                    show_esc_stdout(jsip);
                                    printf("\n");
                            log_unlock();
                            }
                        if(recurse_call(0,0,dummy,jsip,0,0,0,ptrq) ==
                           JS_ERROR) {
                            js_destroy(ptrq);
                            goto cleanup_jsip;
                            }
                        ipq = 0;
                        }
                    else {
                        if(js_adduint16(jsip,RR_A) == JS_ERROR) {
                            if(rlog_level >= 4) {
                                log_lock();
                                show_timestamp();
                                printf("bad js_adduint16 RR_A\n");
                                log_unlock();
                                }
                            goto cleanup_jsip;
                            }
                        if(recurse_call(0,0,dummy,jsip,0,0,&ipq,0) == JS_ERROR)
                            goto cleanup_jsip;
                        }
                    }
                }
            /* Make the data pointed to by ipq a jsip
               (this is a DAV) */
            if(ipq != 0) {
                *(jsip->string) = (ipq & 0xff000000) >> 24;
                *(jsip->string + 1) = (ipq & 0x00ff0000) >> 16;
                *(jsip->string + 2) = (ipq & 0x0000ff00) >> 8;
                *(jsip->string + 3) = (ipq & 0x000000ff);
                jsip->unit_count = 4;
                mhash_add_ip(dnscache,query,jsip);
                /* jsip is copied in the mhash_add_ip routine */
                js_destroy(jsip);
                }
            else if(ptrq != 0 && !js_qissame("NotQual",ptrq)) {
                mhash_add_ptr(dnscache,query,ptrq);
                js_destroy(ptrq);
                js_destroy(jsip); /* Since this is always created */
                }
            else {
                /* Lets plug this memory leak */
                if(ptrq != 0) {
                        js_destroy(ptrq);
                }
                /* If we don't add jsip to the cache, we need to clear the
                   memory it uses */
                js_destroy(jsip);
                }
            nspoint = 2;
            counter--;
            offset += rdlength;
            if(cname_original_record != 0) {
                if(change_rtype(query,cname_original_record) == JS_ERROR) {
                    if(rlog_level >= 4) {
                        log_lock();
                        show_timestamp();
                        printf("bad change_rtype return code\n");
                        log_unlock();
                        }
                    goto cleanup;
                    }
                }
            } while(counter > 0);

        js_destroy(indata);
        js_destroy(uindata);
        js_destroy(outdata);
        if(cname_original_record == 0) /* If this was not a CNAME answer */
            return 2; /* Final non-CNAME answer given */
        else
            return 3; /* CNAME answer given */
        }

    /* It is a referral --or-- it is a "host not here" */

    /* If there are no authority records either, then it is an error */
    if(header.nscount == 0)
        goto minus2;

    /* Offset now points at the first authority (NS) record */
    count = header.nscount;
    nspoint = 0;
    /* We only look at the first 29 NS records */
    if(count > 29)
       count = 29;

    /* Start filling up the ns_record array with pointers to the
       various authority records that are ns_records.  Later on,
       we will see which ns records have corresponding records in
       the additional records section. */

    nspoint = 0;
    while(nspoint < count) {
        /* Determine the length of the answer returned */
        len = dlabel_length(uindata,offset);
        if(len == JS_ERROR)
            goto cleanup;
        /* Point to the question they asked
           (but only if this is a NS record) */
        /* Handle SOA records (read: negative caching) in the NS section */
        ns_record_type = js_readuint16(uindata,offset + len);

        /* If this is a "that does not exist" answer, we give the answer
           special processing. */
        if(nspoint == 0 && ns_record_type == RR_SOA) {
            js_destroy(indata);
            js_destroy(outdata);
            ret = handle_negative_data(query,uindata,offset,&header);
            js_destroy(uindata);
            return ret;
            }

        /* Otherwise, if we are using an upstream instead of a root
           NS server, we need to make sure to *not* follow NS referrals
         */
        if(root_or_upstream != 0) {
            if(rlog_level >= 5) {
printf("WARNING: We do not follow NS referrals when using upstream servers\n");
                }
            goto cleanup;
            }

        /* OK, we did not request recursion in our query; time to play
           chase-the-server-with-the-knowledge-we-are-seeking */

        if(ns_record_type == RR_NS && offset > 0)
            ns_domain[nspoint] = offset;
        else {
            ns_domain[nspoint] = 1; /* Invalid data */
            }
        /* Answers that are out of bailiwick are invalid */
        if(offset_bailiwick(uindata,offset,bailiwick) != 1) {
            ns_domain[nspoint] = 1; /* Invalid data */
            }
        ns_domain[nspoint + 1] = 0; /* End of ns records */
        offset += len + 8; /* Go past name, type, class, and ttl */
        len = js_readuint16(uindata,offset); /* rdlength */
        if(len == JS_ERROR)
            goto cleanup;
        /* We also keep a note of what the NS record is for
           this domain */
        if(ns_domain[nspoint] > 2) { /* If this is a valid NS record */
            ns_record[nspoint] = offset + 2;
            }
        else {
            ns_record[nspoint] = 1; /* invalid data */
            }
        ns_record[nspoint + 1] = 0;
        offset += len + 2; /* Go past rdlength and rddata */
        nspoint++;
        }

    /* This string is used to pass arguments to add_closer_jsip */
    if((jsip = js_create(6,1)) == 0)
        goto cleanup;

    /* Start looking at the additional records.  See which ones are IPs
       which match ns records.  When we see such an IP, we add the record
       to the the big cache. */

    count = header.arcount;
    while(count > 0) {
        /* Determine the length of the question we are answering */
        len = dlabel_length(uindata,offset);
        if(len == JS_ERROR)
            goto cleanup_jsip;
        /* We will only add this record to the cache if:
         * 1) The answer is a NS record
         * 2) The answer is in bailiwick (check done above)
         * 3) There is a ns record above corresponding to the answer
         */
        type = js_readuint16(uindata,offset + len);
        class = js_readuint16(uindata,offset + len + 2);
        ttl = js_readuint32(uindata,offset + len + 4);
        rdlength = js_readuint16(uindata,offset + len + 8);
        if(type == JS_ERROR || class == JS_ERROR || ttl == JS_ERROR ||
           rdlength == JS_ERROR)
            goto cleanup_jsip;
        /* Move the offset pointer to point to the beginning of the
         * rddata */
        if(type != RR_A || class != 1 || rdlength != 4) {
            offset += len + rdlength + 10; /* Rdlength plus ten byte header,
                                              plus length of domain name
                                              label */
            count--;
            continue;
            }
        /* Start comparing the dlabel for this RR with the various NS
         * RRs in the authority section */
        nspoint = 0;
        while(ns_record[nspoint] != 0) {
            /* Give the jsip string a value */
            if(js_substr(uindata,jsip,len + offset + 10,4) == JS_ERROR)
                goto cleanup_jsip;
            /* If the domain name label for this node share the same
               name as a ns record */
            if(ns_record[nspoint] > 8 &&
                cmp_dnames(uindata,ns_record[nspoint],offset) == 1) {
                /* Check to make sure we do not have a lame delegation
                 * (A name server which lists itself as a referring name
                 *  server) */
                if(cmp_ips(uindata,offset,remote_ip) != 0) {
                    js_destroy(jsip);
                    goto minus4;
                    }
                if(record_added == 0) {
                    record_added = 1;
                    /* Handle case */
                    check_case_of_answer(uindata,query,ns_domain[nspoint]);
                    if(add_closer_jsip_offset(uindata,ns_domain[nspoint],
                                              jsip,OVERWRITE) == JS_ERROR) {
                        goto cleanup_jsip;
                        }
                    }
                else {
                    /* Handle case */
                    check_case_of_answer(uindata,query,ns_domain[nspoint]);
                    if(add_closer_jsip_offset(uindata,ns_domain[nspoint],
                                              jsip,APPEND) == JS_ERROR) {
                        goto cleanup_jsip;
                        }
                    }
                /* Mark this as "already added" */
                /* Note that we may need to change this to handle
                   hosts where the NS record points to a list of
                   IPs, such as, as of October 9 2001,
                   proliant.salvanet.com.mx (Usar una lista de IPs para
                   un NS es mala!) */
                /* Mark the pointer to information regarding what part of the
                   subtree of the domain space that this NS record covers as
                   "already added" */
                /* Disabled so that hosts like proliant.salvanet.com.mx can
                   resolve */
                /* ns_domain[nspoint] = 2; */
                /* And mark the name of the NS server that covers this
                   domain space as "already added" */
                /* Disabled so that hosts like proliant.salvanet.com.mx can
                   resolve */
                /* ns_record[nspoint] = 2; */
                }
            nspoint++;
            }
        offset += len + rdlength + 10;
        count--;
        }

    /* All of the glued records have been added.  Now add the glueless
       records to the cache */

    nspoint = 0;
    while(ns_record[nspoint] != 0) {
        /* If the record is valid and has not been added */
        if(ns_record[nspoint] > 8) {
            if(record_added == 0) {
                record_added = 1;
                /* Handle case */
                check_case_of_answer(uindata,query,ns_domain[nspoint]);
                if(add_closer_js_offset(uindata,ns_domain[nspoint],
                                ns_record[nspoint],OVERWRITE) == JS_ERROR) {
                    goto cleanup_jsip;
                    }
                }
            else {
                /* Handle case */
                check_case_of_answer(uindata,query,ns_domain[nspoint]);
                if(add_closer_js_offset(uindata,ns_domain[nspoint],
                                      ns_record[nspoint],APPEND) == JS_ERROR) {
                    goto cleanup_jsip;
                    }
                }
            }
        nspoint++;
        }

    js_destroy(jsip);
    js_destroy(indata);
    js_destroy(uindata);
    js_destroy(outdata);
    if(record_added == 1)
        return JS_SUCCESS;
    else
        return -4; /* No data we can add */

    cleanup_jsip:
        js_destroy(jsip);
    cleanup:
        js_destroy(indata);
        js_destroy(uindata);
        js_destroy(outdata);
        return JS_ERROR;

    minus2:
        js_destroy(indata);
        js_destroy(uindata);
        js_destroy(outdata);
        return -2;

    minus3:
        js_destroy(indata);
        js_destroy(uindata);
        js_destroy(outdata);
        return -3;

    minus4:
        js_destroy(indata);
        js_destroy(uindata);
        js_destroy(outdata);
        return -4;

    }

/* cmp_ips: Compare an IP inside a reply with an IP that is an integer
   input: js_string with the IP, offset of the beginning of the
          A record (we need to get past the name, type, class, ttl, and
          rdlength, and then endian convert the ip itself), ip to compare
          with.
   output: -1 (JS_ERROR) on fatal error, 0 if they are different, 1 if they
           are the same
*/
int cmp_ips(js_string *compare, int offset, uint32 ip) {

    int ip_compare;

    int len;

    /* Sanity checks */
    if(js_has_sanity(compare) == JS_ERROR)
       return JS_ERROR;
    if(compare->unit_size != 1)
       return JS_ERROR;
    if(offset > compare->unit_count)
       return JS_ERROR;

    /* Go past the name */
    len = dlabel_length(compare,offset);
    if(len == JS_ERROR)
       return JS_ERROR;
    offset += len;
    /* Make sure the type is an A record */
    if(js_readuint16(compare,offset) != RR_A)
        return 0;
    /* Go past type, class, and ttl */
    offset += 8;
    /* Make sure the record is four bytes long */
    if(js_readuint16(compare,offset) != 4)
        return 0;
    /* Go past rdlength */
    offset += 2;
    /* See if the IP matches */
    if(compare->unit_count < offset + 3)
        return JS_ERROR;
    ip_compare = ((*(compare->string + offset) << 24) & 0xff000000) |
                 ((*(compare->string + offset + 1) << 16) & 0x00ff0000) |
                 ((*(compare->string + offset + 2) << 8) & 0x0000ff00) |
                 (*(compare->string + offset + 3) & 0x000000ff);
    /* If they match, return 1 */
    if(ip_compare == ip)
        return 1;
    /* No match.  Return 0 */
    return 0;

    }

/* Give answer: Give an answer from the cache.
   Input: the pointer to the element in the hash with the data, the data type
          of the element in the hash with the data, a pointer to where
          we store the IP we get (if appropriate), the query the end user
          sent to the program, a pointer to wher ein the hash we found the
          answer, the id of the query the end-user sent, the fd of the socket
          the query is on, the ip and other related informaton about the
          client who sent the query to us, the recursion depth we are
          currently at, the glueless level we are currently at,
          if this is a cname answer the original query type, otherwise 0
   Output: 0 if we didn't find any data, JS_SUCCESS if we successfully
           sent data to the end user -or- we set ipret
*/

int give_answer(void *value, int datatype, void **rotate_point,
                uint32 *ipret, js_string *ptrret, js_string *query,
                js_string *hash_pointer,
                /* Jaakko changed struct sockaddr to struct sockaddr_in */
                int id, int sock, struct sockaddr_in client, int queries_sent,
                int glueless_level, int cname_answer) {


    rr *lookatrr;

    /* Make sure the RR has not expired */
    lookatrr = value;
    move_to_top(lookatrr->zap);
    /* If the data expires (is not 0), and the data has expired...
       HACK: If the record is an "incomplete" CNAME record (A CNAME
       record without an IP nor a "there is no IP attached to this
       node" datapoint), then the record will live for a short time */
    if(lookatrr->expire != 0 && (lookatrr->expire < qual_get_time() ||
       (lookatrr->rr_type == RR_CNAME && lookatrr->ip == NULL &&
        lookatrr->expire - lookatrr->ttl < qual_get_time() -
        INCOMPLETE_CNAME_LIFETIME))) {
        /* Then destroy the data */
        /* Delete the answer from the cache */
        lookatrr = mhash_undef(dnscache,hash_pointer);
        unlink_rr(lookatrr,0);
        /* And re-run recurse_call with the revised cache */
        if(recurse_call(id,sock,client,query,queries_sent + 1,
                     glueless_level,ipret,ptrret) == JS_ERROR)
            goto cleanup;
        goto success;
        }

    /* If we are set up to return an answer to the client instead
       of placing the answer in a pointer (used for out-of-bailiwick
       and cname-to-ip lookups), do so */
    if(ipret == NULL && ptrret == NULL) { /* If we send a UDP packet
                                     as an answer */
        if(datatype == MARA_DNS_NEG) {
            udpnotfound(lookatrr,id,sock,&client,query,0,1,0,1);
            }
        else {
            udpsuccess(value,id,sock,&client,query,rotate_point,
                       cname_answer,1,0,0,1);
            }
        }
    else if(ipret != NULL) { /* If we are simply changint the value of
                        ipret (giving out an IP answer) */
        lookatrr = value;
        /* If this is a "this host does not exist" answer, return an
           "ip" of 255.255.255.255 */
        if(datatype == MARA_DNS_NEG) {
            *ipret = 0xffffffff;
            goto success;
            }
        /* Make sure this is an A record */
        if(lookatrr->rr_type != RR_A && cname_answer != 1) {
            *ipret = 0;
            goto success;
            }
        /* If it is a cname answer, guess what: We get to find out
           what IP that cname (eventually) points to */
        if(cname_answer == 1) {
            /* Copy over lookatrr->data */
            if(js_copy(lookatrr->data,hash_pointer) == JS_ERROR)
                goto cleanup;
            if(js_adduint16(hash_pointer,RR_A) == JS_ERROR)
                goto cleanup;
            if(recurse_call(id,sock,client,hash_pointer,queries_sent + 1,
                    glueless_level + 1,ipret,ptrret) == JS_ERROR)
                goto cleanup;
            /* Recursively calling recurse_call changes the value
               of ipret; so we don't have to change it here */
            goto success;
            }
        /* Convert spot_data.value->data in to a uint32 */
        *ipret = ((*(lookatrr->data->string) << 24) & 0xff000000) |
                 ((*(lookatrr->data->string + 1) << 16) & 0x00ff0000) |
                 ((*(lookatrr->data->string + 2) <<  8) & 0x0000ff00) |
                 (*(lookatrr->data->string + 3) & 0x000000ff);
        return JS_SUCCESS;
        }
    else if(ptrret != NULL) { /* We want to give them a PTR answer */
        lookatrr = value;
        /* Handle the case of the CNAME pointing to a black hole
           "This host does not exist" record */
        if(datatype == MARA_DNS_NEG) {
            if(js_qstr2js(ptrret,"NotQual") == JS_ERROR) {
                return JS_ERROR;
                }
            return JS_SUCCESS;
            }
        /* Make sure this is a PTR record */
        if(lookatrr->rr_type != RR_PTR && cname_answer != 1) {
            if(js_qstr2js(ptrret,"NotQual") == JS_ERROR) {
                return JS_ERROR;
                }
            goto success;
            }
        /* If it is a cname answer, guess what: We get to find out
           what PTR that cname (eventually) points to */
        if(cname_answer == 1) {
            /* Copy over lookatrr->data */
            if(js_copy(lookatrr->data,hash_pointer) == JS_ERROR)
                goto cleanup;
            if(js_adduint16(hash_pointer,RR_PTR) == JS_ERROR)
                goto cleanup;
            if(recurse_call(id,sock,client,hash_pointer,queries_sent + 1,
                    glueless_level + 1,ipret,ptrret) == JS_ERROR)
                goto cleanup;
            /* Recursively calling recurse_call changes the value
        of ptrret; so we don't have to change it here */
            goto success;
            }
        /* Copy over lookatrr->data to ptrret */
        if(js_copy(lookatrr->data,ptrret) == JS_ERROR) {
            return JS_ERROR;
            }
        return JS_SUCCESS;
        }

    success:
        return JS_SUCCESS;
    cleanup:
        return JS_ERROR;
    }

/* Recurse thread: This is a special routine that launches its own thread
   input: Pointer to dnsreq query
   output: JS_SUCCESS (never looked at--this is a detached thread)
*/

int recurse_thread(dnsreq *req) {

    int ret;

    /* Increment the number of threads running */
    tcount_lock();
    num_of_threads_running++;
    tcount_unlock();

    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        tcount_lock();
        printf("In thread; ready to begin recursion; Threads in use: %d\n",
        num_of_threads_running);
        tcount_unlock();
        log_unlock();
        }

    do_big_lock();
    ret = recurse_call(req->id, req->sock, req->client, req->query, 0, 0,
                       NULL,NULL);
    big_unlock();

    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        tcount_lock();
        printf("Ready to terminate thread; threads in use %d\n",
        num_of_threads_running);
        tcount_unlock();
        log_unlock();
        }

    js_destroy(req->query);
    js_dealloc(req);

    /* Decrement the number of threads running */
    tcount_lock();
    num_of_threads_running--;
    tcount_unlock();

    return ret;

    }

/* We made this a separate function so that we can call it recursivly
   without having to allocate memory on the heap for req objects.
   Input: id of request, socket of request, location request came from,
          desired DNS query, recursion_depth (which we use to limit the
          number of queries), gluless_level (the number of times we had
          to perform a glueless lookup), ipret: If this is a request for
          a glueless IP, return the IP here.  If this is NULL, give the
          client the answer to the question they asked.
   Output: JS_SUCCESS, I believe
*/

/* Jaakko changed struct sockaddr to struct sockaddr_in */

int recurse_call(int id, int sock, struct sockaddr_in client,
                 js_string *query, int queries_sent, int glueless_level,
                 uint32 *ipret, js_string *ptrret) {

    mhash_e spot_data;
    js_string *copy;
    closer *cpoint;
    uint32 nsip;
    int result, qtype, case_folded = 0;
    int case_folded_found = 0;
    /* Variables used to copy over the chain of closer name servers */
    closer *local_c, *local_c_head = NULL, *local_c_save;
    uint32 *i32_copy;
    js_string *jstr_copy, *glueless_query, *lower;
    rr *lookatrr;
    int current_retry_cycle = 0;

    /* Make sure we haven't overloaded the recursion depth or maximum glueless
       level */
    if(queries_sent > max_queries_total ||
       glueless_level > max_glueless_level) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            show_esc_stdout(query);
            printf(" queries_total/glueless_level exceeded\n");
            log_unlock();
            }
        return JS_ERROR;
        }

    if(ipret != NULL)
        *ipret = 0;

    /* See if we have to do case-insensitive searching */
    if((lower = js_create(query->unit_count + 3,1)) == 0)
        return JS_ERROR;
    if(js_copy(query,lower) == JS_ERROR)
        goto cleanup_nojs;
    /* leave on error */
    if(case_folded == JS_ERROR)
        goto cleanup_nojs;

    /* We deal with the case of RR_ANY thusly:
       1) Look for RR_A and RR_MX for the query in question
       2) Use the udpany call to "sew" together the data
       3) Return a SOA if nothing was found */
    qtype = get_rtype(query);
    if(qtype == RR_ANY) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            show_esc_stdout(query);
            printf(" is an ANY query\n");
            log_unlock();
            }
        /* Make sure the RR_A and RR_NS are placed in the dns cache */
        change_rtype(query,RR_A);
if(recurse_call(id,sock,client,query,queries_sent + 1,
                     glueless_level,&nsip,0) == JS_ERROR) {
            goto cleanup_nojs;
    }
        change_rtype(query,RR_MX);
        if(recurse_call(id,sock,client,query,queries_sent + 1,
                     glueless_level,&nsip,0) == JS_ERROR) {
            goto cleanup_nojs;
    }
        change_rtype(query,RR_ANY);
        /* If something was found, we have success */
        if(udpany(id,sock,&client,query,3,dnscache,1,0,1,0) == JS_SUCCESS) {
            js_destroy(lower);
            return JS_SUCCESS;
            }
        /* Return a SOA answer if an A or MX answer was not found. */
        lookatrr = 0;
        change_rtype(query,RR_A);
        spot_data = mhash_get(dnscache,query);
        if(spot_data.value != 0 && spot_data.datatype == MARA_DNS_NEG)
            lookatrr = spot_data.value;
        else {
            change_rtype(query,RR_MX);
            spot_data = mhash_get(dnscache,query);
            if(spot_data.value != 0 && spot_data.datatype == MARA_DNS_NEG)
                lookatrr = spot_data.value;
            }
        if(lookatrr != 0) {
            move_to_top(lookatrr->zap);
            udpnotfound(lookatrr,id,sock,&client,query,255,1,0,1);
            }
        js_destroy(lower);
        return JS_SUCCESS;
        }

    /* Allocate memoey for the 'copy' string */
    if((copy = js_create(257,1)) == 0)
        goto cleanup_nojs;

    /* Look for query in cache */
    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        printf("Looking for ");
        show_esc_stdout(lower);
        printf(" in cache\n");
        log_unlock();
        }
    spot_data = mhash_get(dnscache,lower);

    /* If there is a "this host does not exist" in the cache, return that
       answer */
    if(spot_data.value != 0 && spot_data.datatype == MARA_DNS_NEG) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            show_esc_stdout(lower);
            printf(" found in cache (psudo-NXDOMAIN) at %p\n",spot_data.value);
            log_unlock();
            }
        if(give_answer(spot_data.value,spot_data.datatype,spot_data.point,
                       ipret,ptrret,query,lower,id,sock,client,queries_sent,
                       glueless_level,0) == JS_SUCCESS)
            goto success;
goto cleanup;
        }

    /* Return cached data if found */
    if(spot_data.value != 0 && spot_data.datatype == MARA_DNSRR) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            show_esc_stdout(lower);
            printf(" found in cache (RR) at %p\n",spot_data.value);
            log_unlock();
            }
        if(give_answer(spot_data.value,spot_data.datatype,spot_data.point,
                       ipret,ptrret,query,lower,id,sock,client,queries_sent,
                       glueless_level,0) == JS_SUCCESS)
            goto success;
goto cleanup;
        }

    /* If it is not found, see if we have a CNAME record instead */
    if(js_copy(lower,copy) == JS_ERROR)
        goto cleanup;
    if(change_rtype(copy,RR_CNAME) == JS_ERROR)
        goto cleanup;
    spot_data = mhash_get(dnscache,copy);
    if(spot_data.value != 0 && spot_data.datatype == MARA_DNSRR) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            show_esc_stdout(lower);
            printf(" found in cache (CNAME RR) at %p\n",spot_data.value);
            log_unlock();
            }
        if(give_answer(spot_data.value,spot_data.datatype,spot_data.point,
                       ipret,ptrret,query,
                       copy,id,sock,client,queries_sent,
                       glueless_level,qtype) == JS_SUCCESS)
            goto success;
        goto cleanup;
        }


    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        show_esc_stdout(lower);
        printf(" not found in cache\n");
        log_unlock();
        }

    /* If we still haven't found something, perhaps a lower-case version of
       the same exist */
    case_folded = fold_case(lower);
    /* If we could fold the case, look for a lower-case version of the same */
    if(case_folded == 1) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Looking for ");
            show_esc_stdout(lower);
            printf(" (lowercase)\n");
            log_unlock();
            }
        spot_data = mhash_get(dnscache,lower);

        /* If there is a "this host does not exist" in the cache, return that
           answer */
        if(spot_data.value != 0 && spot_data.datatype == MARA_DNS_NEG) {
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                show_esc_stdout(lower);
                printf(" found (lowercase psudo-NXDOMAIN) at %p\n",
                       spot_data.value);
                log_unlock();
                }
            if(give_answer(spot_data.value,spot_data.datatype,
                       spot_data.point,ipret,ptrret,query,
                       lower,id,sock,client,queries_sent,
                       glueless_level,0) == JS_SUCCESS)
                goto success;
    goto cleanup;
            }

        /* Return cached data if found */
        if(spot_data.value != 0 && spot_data.datatype == MARA_DNSRR) {
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                show_esc_stdout(lower);
                printf(" found (lowercase RR) at %p\n",spot_data.value);
                log_unlock();
                }
            if(give_answer(spot_data.value,spot_data.datatype,
                       spot_data.point,ipret,ptrret,query,
                       lower,id,sock,client,queries_sent,
                       glueless_level,0) == JS_SUCCESS)
                goto success;
    goto cleanup;
            }

        /* If it is not found, see if we have a lower-case CNAME record
           instead */
        if(js_copy(lower,copy) == JS_ERROR)
            goto cleanup;
        if(change_rtype(copy,RR_CNAME) == JS_ERROR)
            goto cleanup;
        spot_data = mhash_get(dnscache,copy);
        if(spot_data.value != 0 && spot_data.datatype == MARA_DNSRR) {
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                show_esc_stdout(lower);
                printf(" found (CNAME lowercase RR) at %p\n",spot_data.value);
                log_unlock();
                }
            if(give_answer(spot_data.value,spot_data.datatype,spot_data.point,
                       ipret,ptrret,query,copy,id,sock,client,queries_sent,
                       glueless_level,qtype) == JS_SUCCESS)
                goto success;
    goto cleanup;
            }

        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            show_esc_stdout(lower);
            printf(" not found in cache either\n");
            log_unlock();
            }

        }

    /* OK, we don't have data in the cache.  See which nameserver we
       have to bug */
    if(js_copy(query,copy) == JS_ERROR)
        goto cleanup;

    /* See if there is a name server in the cache for the exact query we are
       asking for */
    if(change_rtype(copy,252) == JS_ERROR)
        goto cleanup;
    /* OK, look for the "lowest" branch in the tree which will lead us to
       the data we are seeking */
    do {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Looking for ");
            show_esc_stdout(copy);
            printf(" in cache (NS referral)\n");
            log_unlock();
            }

        spot_data = mhash_get(dnscache,copy);

        /* Do a lower case version of the same query if needed */
        case_folded_found = 0;
        /* If not found, so lower case version of query */
        if(spot_data.value == 0 || spot_data.datatype != MARA_DNS_NS) {
            js_copy(copy,lower);
            if(fold_case(lower) == JS_SUCCESS) { /* If we can fold the case */
                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    printf("Looking for ");
                    show_esc_stdout(lower);
                    printf(" in cache (lowercase NS ref)\n");
                    log_unlock();
                    }
                spot_data = mhash_get(dnscache,lower);
                if(spot_data.value != 0 && spot_data.datatype == MARA_DNS_NS)
                    case_folded_found = 1;
                }
            }

        /* Query nameserver if found */
        if(spot_data.value != 0 && spot_data.datatype == MARA_DNS_NS) {
            cpoint = spot_data.value;
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                if(case_folded_found == 1)
                    show_esc_stdout(lower);
                else
                    show_esc_stdout(copy);
                printf(" found at %p\n",spot_data.value);
                log_unlock();
                }
            move_to_top(cpoint->zap);
            /* If the data expires (is not 0), and the data has expired... */
            if(cpoint->ttd != 0 && cpoint->ttd < qual_get_time()) {
                /* The destroy the data */
                /* Make the read-only lock a read-write lock */

                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    if(case_folded_found == 1)
                        show_esc_stdout(lower);
                    else
                        show_esc_stdout(copy);
                    printf(" at %p has expired, zapping\n",spot_data.value);
                    log_unlock();
                    }

                if(case_folded_found == 0)
                    cpoint = mhash_undef(dnscache,copy);
                else
                    cpoint = mhash_undef(dnscache,lower);
                unlink_closer(cpoint);
                /* Re-run this routine with the revised cache */
                if(recurse_call(id,sock,client,query,queries_sent + 1,
                             glueless_level,ipret,ptrret) == JS_ERROR) {
                    goto cleanup;
            }
                goto success;
                }

            /* Code that makes a local copy of the entire
               cpoint chain, so that other threads can use (or destroy) the
               original */

            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                printf("Making cpoint copy of ");
                if(case_folded_found == 1)
                    show_esc_stdout(lower);
                else
                    show_esc_stdout(copy);
                printf(" at %p\n",spot_data.value);
                log_unlock();
                }

            if((local_c = js_alloc(1,sizeof(closer))) == 0)
                goto cleanup;
            local_c_head = local_c;
            while(local_c != NULL) {
                local_c->num_elements = cpoint->num_elements;
                local_c->ttd = cpoint->ttd;
                local_c->zap = NULL; /* Not on zap list */
                switch(cpoint->datatype) {
                    case RR_A:
                        if((i32_copy = js_alloc(1,sizeof(uint32))) == 0)
                            goto cleanup;
                        *i32_copy = *(uint32 *)(cpoint->data);
                        local_c->datatype = RR_A;
                        local_c->data = i32_copy;
                        break;
                    case RR_NS:
                        if(js_has_sanity(cpoint->data) == JS_ERROR)
                            goto cleanup;
                        jstr_copy = cpoint->data;
                        if (( jstr_copy =
                                js_create(jstr_copy->unit_count + 1,1)) == 0)
                            goto cleanup;
                        if(js_copy(cpoint->data,jstr_copy) == JS_ERROR) {
                            js_destroy(jstr_copy);
                            goto cleanup;
                            }
                        local_c->datatype = RR_NS;
                        local_c->data = jstr_copy;
                        break;
                    default:
                        local_c->datatype = RR_NULL; /* NULL RR number,
                                                        since this
                                                        data is invalid; this
                                                        hopefully does not
                                                        stop the NULL RR
                                                        from being used by
                                                        MaraDNS */
                        local_c->data = NULL;
                    }
                if(cpoint->next != NULL) {
                    local_c_save = local_c;
                    if((local_c = js_alloc(1,sizeof(closer))) == 0)
                        goto cleanup;
                    local_c_save->next = local_c;
                    cpoint = cpoint->next;
                    }
                else {
                    local_c->next = NULL;
                    break;
                    }
                }

            local_c = local_c_head;
            /* Select a NS server at random (weak RNG because this is just
               to be fair to the name servers) */
#ifndef MINGW32
            result = random() % local_c->num_elements; /* I know, using
                                                          an unreleared
                                                          variable as a
                                                          temporary counter
                                                       */
#else
            result = rand() % local_c->num_elements;
#endif /* MINGW32 */
            while(result > 0) {
                local_c = local_c->next;
                if(local_c == NULL)
                    local_c = local_c_head;
                result--;
                }
            local_c_save = local_c;
            result = 0;
            /* Go through all of the NS servers we have an IP for.  Since
               we are using a circular linked list, this code is a little
               tricky.  Do this retry_cycles number of times. */
            current_retry_cycle = 0;
            do {
             do {
                if(local_c->datatype == RR_A) {
                    nsip = *(uint32 *)(local_c->data);
                    /* Query the nameserver for the answer */
                    if(case_folded_found == 0)
                        result = query_nameserver(nsip,query,copy);
                    else
                        result = query_nameserver(nsip,query,lower);
                    }
                /* Go to the next nameserver on the list */
                local_c = local_c->next;
                if(local_c == NULL)
                    local_c = local_c_head;
                } while (result <= 0 && local_c != local_c_save);
            /* If we did not find any glued name servers, do a DNS lookup
               for the glueless server(s) */
            if(result <= 0) { /* If nothing found yet */
                /* Go to randomly selected element above */
                local_c = local_c_save;
                /* Do an A lookup for the glueless nameserver in question,
                   then, if successful, query that nameserver */
                do {
                    if(local_c->datatype == RR_NS) {
                        /* Look for the ip for the name */
                        /* Create a copy of the destired query */
                        glueless_query = local_c->data;
                        if(js_has_sanity(glueless_query) == JS_ERROR)
                            goto cleanup;
                        if((glueless_query =
                            js_create(glueless_query->unit_count + 2,1)) == 0)
                            goto cleanup;
                        if(js_copy(local_c->data,glueless_query) == JS_ERROR) {
                            js_destroy(glueless_query);
                            goto cleanup;
                            }
                        /* Make sure the nameserver name in question is
                           *out*-of-bailwick; there is no sense in chasing
                           an in-bailiwick glueless name */
                        nsip = 0;
                        if(case_folded_found == 0 &&
                           q_bailiwick(glueless_query,copy) == 1) {
                            nsip = 0;
                            }
                        else if(case_folded_found == 1 &&
                                q_bailiwick(glueless_query,lower) == 1) {
                            nsip = 0;
                            }
                        /* Try to get the number */
                        else if(recurse_call(id,sock,client,glueless_query,
                           queries_sent + 1, glueless_level + 1,&nsip,0) ==
                           JS_ERROR) {
                            js_destroy(glueless_query);
                            goto cleanup;
                            }
                        js_destroy(glueless_query);
                        /* If we got an IP, query that nameserver, update the
                           cache */
                        if(nsip != 0) {
                            if(case_folded_found == 0)
                                result = query_nameserver(nsip,query,copy);
                            else
                                result = query_nameserver(nsip,query,lower);
                            }
                        }
                    /* Go to the next nameserver on the list */
                    local_c = local_c->next;
                    if(local_c == NULL)
                        local_c = local_c_head;
                    } while(result <= 0 && local_c != local_c_save);
                }
            current_retry_cycle++;
            } while(result <= 0 && current_retry_cycle < retry_cycles);
            js_destroy(copy);
            js_destroy(lower);
            /* Since we are done with our local copy of the data, we can
               get rid of it */
            if(local_c_head != NULL) /* gdb tells me this is NULL sometimes */
                unlink_closer(local_c_head);
            if(result > 0) /* If a nameserver was found,
                              then do a lookup again based
                              on the revised cache */ {
                if(recurse_call(id,sock,client,query,queries_sent + 1,
                             glueless_level,ipret,ptrret) == JS_ERROR) {
                        return JS_ERROR;
                    }
                return JS_SUCCESS; /* Since we did find something */
                }
            /* We couldn't contact any nameservers */
            /* Return a bogus SOA record */
            /* Return either bogus "not here", "server fail", or
             * nothing based on user's preferences */

            if(ipret == NULL && ptrret == NULL) { /* If we send a UDP packet
                                     as an answer */
                if(handle_noreply == 2) {
                    udpnotfound(rra_data,id,sock,&client,query,0,1,0,3);
                    }
                else if(handle_noreply == 1) {
                    /* udperror with "server fail" */
                    js_string *synthesized_header;
                    /* udperror with "server fail" */
                    if((synthesized_header = js_create(24,1)) == 0) {
                        return JS_ERROR;
                        }
                    if(js_adduint16(synthesized_header,id) == JS_ERROR) {
                        js_destroy(synthesized_header);
                        return JS_ERROR;
                        }
                    udperror(sock,synthesized_header,&client,query,SERVER_FAIL,
                             "No reply from remote servers",2,1,0,0);
                    js_destroy(synthesized_header);
                    }
                }
            else if(ptrret != NULL) {
                 if(js_qstr2js(ptrret,"NotQual") == JS_ERROR) {
                     return JS_ERROR;
                     }
                 }
            return JS_SUCCESS;
            }
        } while(bobbit_label(copy) > 0);

    /* We could not find a "closer" name server, which is an error
       condition */
    /* Return a bogus SOA record */
    /* CODE HERE: Return either bogus "not here", "server fail", or
                  nothing based on user's preferences */
    if(ipret == NULL && ptrret == NULL) { /* If we send a UDP packet
                                           * as an answer */
        if(handle_noreply == 2) {
            udpnotfound(rra_data,id,sock,&client,query,0,1,0,3);
            }
        else if(handle_noreply == 1) {
            js_string *synthesized_header;
            /* udperror with "server fail" */
            if((synthesized_header = js_create(24,1)) == 0) {
                return JS_ERROR;
                }
            if(js_adduint16(synthesized_header,id) == JS_ERROR) {
                js_destroy(synthesized_header);
                return JS_ERROR;
                }
            udperror(sock,synthesized_header,&client,query,SERVER_FAIL,
                 "No reply from remote servers",2,1,0,0);
            js_destroy(synthesized_header);
            }
        }
    /* Clean up the request */
    success:
        js_destroy(copy);
        js_destroy(lower);
        return JS_SUCCESS;

    cleanup:
        if(local_c_head != NULL)
            unlink_closer(local_c_head);
        js_destroy(copy);
    cleanup_nojs:
        js_destroy(lower);
        return JS_ERROR;
    }

/* Routine which prepares the launch of a detached thread which will
   recursivly look for the DNS name in question
   Input:  id of query, socket of query, sockaddr struct of query,
           js_string with host name they are looking for
   Output: JS_ERROR on error, JS_SUCCESS on success
*/

int launch_thread(int id, int sock,
        struct sockaddr_in client, js_string *query) {
    /* We have to copy the js_string over because the thread needs her
       own copy */
    js_string *copy, *synthesized_header;
    dnsreq *req;
    uint16 original_rtype;
    pthread_t thread;
    pthread_attr_t attr;
    mhash_e spot_data;
    rr *lookatrr;

    if(verbose_query > 0) {
        /* Debugging information to tell us the query being asked */
        printf("%d:",(int)time(0));
        human_readable_dns_query(query,0);
        printf("\n");
        fflush(stdout);
    }

    /* First of all, before launching a thread, we need to see if
       the data is already in the cache.  If so, there is no need to
       set up and launch a thread. */
    do_big_lock();
    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        printf("Looking for ");
        show_esc_stdout(query);
        printf(" in DNS cache\n");
        log_unlock();
        }
    spot_data = mhash_get(dnscache,query);
    if(spot_data.value != 0 && (spot_data.datatype == MARA_DNSRR ||
       spot_data.datatype == MARA_DNS_NEG)) {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            show_esc_stdout(query);
            printf(" found at %p\n",spot_data.value);
            log_unlock();
            }
        lookatrr = spot_data.value;
        move_to_top(lookatrr->zap);
        /* If the record has expired, continue
           HACK: "incomplete" CNAME records expire after a short time */
        if(lookatrr->expire != 0 && (lookatrr->expire < qual_get_time() ||
           (lookatrr->rr_type == RR_CNAME && lookatrr->ip == NULL &&
           lookatrr->expire - lookatrr->ttl < qual_get_time() -
           INCOMPLETE_CNAME_LIFETIME))) {
            /* Do almost nothing */
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                show_esc_stdout(query);
                printf(" has expired at %p\n",spot_data.value);
                log_unlock();
                }
            }
        else if(spot_data.datatype == MARA_DNS_NEG) {
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                show_esc_stdout(query);
                printf(" is a psudo-NXDOMAIN at %p\n",spot_data.value);
                log_unlock();
                }
            udpnotfound(lookatrr,id,sock,&client,query,0,1,0,1);
            big_unlock();
            return JS_SUCCESS;
            }
        else {
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                show_esc_stdout(query);
                printf(" is an RR at %p\n",spot_data.value);
                log_unlock();
                }
            udpsuccess(spot_data.value,id,sock,&client,query,
                       spot_data.point,0,1,0,0,1);
            big_unlock();
            return JS_SUCCESS;
            }
        }
    else {
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            show_esc_stdout(query);
            printf(" not found in cache\n");
            log_unlock();
            }
        }

    /* We also check to see if there is a CNAME element with the same
       value as the element we are looking for before going off and
       spwawning a thread */

    original_rtype = get_rtype(query);
    /*if(original_rtype == JS_ERROR)
        return JS_ERROR; */

    if(original_rtype != RR_CNAME) {
        if(change_rtype(query,RR_CNAME) == JS_ERROR) {
            big_unlock();
            return JS_ERROR;
            }
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("Looking for ");
            show_esc_stdout(query);
            printf(" in DNS cache (CNAME lookup)\n");
            log_unlock();
            }
        spot_data = mhash_get(dnscache,query);
        if(spot_data.value != 0 && (spot_data.datatype == MARA_DNSRR ||
           spot_data.datatype == MARA_DNS_NEG)) {
            lookatrr = spot_data.value;
            move_to_top(lookatrr->zap);
            /* If the record has expired, continue
               HACK: "incomplete" CNAME records expire after a short period */
            if(lookatrr->expire != 0 && (lookatrr->expire < qual_get_time() ||
               (lookatrr->rr_type == RR_CNAME && lookatrr->ip == NULL &&
               lookatrr->expire - lookatrr->ttl < qual_get_time() -
               INCOMPLETE_CNAME_LIFETIME))) {
                /* Do almost nothing */
                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    show_esc_stdout(query);
                    printf(" has expired (CNAME) at %p\n",spot_data.value);
                    log_unlock();
                    }
                }
            else if(spot_data.datatype == MARA_DNS_NEG) {
                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    show_esc_stdout(query);
                    printf(" is a psudo-NXDOMAIN (CNAME) at %p\n",
                           spot_data.value);
                    log_unlock();
                    }
                udpnotfound(lookatrr,id,sock,&client,query,0,1,0,1);
                big_unlock();
                return JS_SUCCESS;
                }
            else {
                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    show_esc_stdout(query);
                    printf(" is an RR (CNAME) at %p\n",spot_data.value);
                    log_unlock();
                    }
                if(change_rtype(query,original_rtype) == JS_ERROR) {
                    big_unlock();
                    return JS_ERROR;
                    }
                if(original_rtype > 0)
                    udpsuccess(spot_data.value,id,sock,&client,query,
                               spot_data.point,original_rtype,1,0,0,1);
                else
                    udpsuccess(spot_data.value,id,sock,&client,query,
                               spot_data.point,0,1,0,0,1);
        big_unlock();
                return JS_SUCCESS;
                }
            }
        else {
            if(rlog_level >= 4) {
                log_lock();
                show_timestamp();
                show_esc_stdout(query);
                printf(" not found in cache (CNAME)\n");
                log_unlock();
                }
            }
        }

    big_unlock();
    if(change_rtype(query,original_rtype) == JS_ERROR)
        return JS_ERROR;

    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        printf("initializing thread\n");
        log_unlock();
        }

    /* We need to create some thread-safe data structures.
       We have a special dnsreq data structure that we use because a
       thread, when initialized, can only be initialized with a single
       argument. */

    /* Create the req data structure */
    if((req = js_alloc(1,sizeof(dnsreq))) == 0)
        return JS_ERROR;

    /* Create the 'copy' js_string */
    if((copy = js_create(query->unit_count + 3,1)) == 0) {
        js_dealloc(req);
        return JS_ERROR;
        }

    /* Copy over query to 'copy' */
    if(js_copy(query,copy) == JS_ERROR) {
        js_dealloc(req); js_destroy(copy);
        return JS_ERROR;
        }

    /* Fill out the rest of the req structure */
    req->id = id;
    req->sock = sock;
    req->client = client;
    req->query = copy;

    /* Make sure we can spwawn the thread. */
    tcount_lock();
    if(num_of_threads_running > maximum_num_of_threads) {
        tcount_unlock();
        js_dealloc(req); js_destroy(copy);
        if(rlog_level >= 4) {
            log_lock();
            show_timestamp();
            printf("too many threads running\n");
            log_unlock();
            }
        /* Return a "server fail" error message if we can not spawn
           a thread.  We need to synthesize a header to do this */
        if((synthesized_header = js_create(24,1)) == 0) {
            return JS_ERROR;
            }
        if(js_adduint16(synthesized_header,id) == JS_ERROR) {
            js_destroy(synthesized_header);
            return JS_ERROR;
            }
        udperror(sock,synthesized_header,&client,query,FORMAT_ERROR,
                 "Didn't spawn thread",2,1,0,0);
        js_destroy(synthesized_header);
        return JS_ERROR;
        }
    tcount_unlock();

    /* Spawn the thread */
#ifdef NOTHREAD
    recurse_thread(req);
#else
    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        printf("About to launch thread...\n");
        log_unlock();
        }
    /* Set up the attributes for the thread.  Make this a detached
       thread because we don't look at the return value (the thread
       is talking to the client and maybe updating the cache) */
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);

    if(pthread_create(&thread,&attr,(void *)recurse_thread,(void *)req) != 0)
        {
        js_dealloc(req); js_destroy(copy);
        return JS_ERROR;
        }

    /* Linux does not use pthread_attr_destroy, so this does nothing on Linux.
       However, Solaris needs this done, otherwise it leaks 36 bytes every
       time a thread is created.  Thanks to Christophe Colle for pointing
       this out to me. */
    pthread_attr_destroy(&attr);
#endif /* NOTHREAD */

    /* Leave...it is now up to the thread to handle the query and free
       memory used */
    return JS_SUCCESS;

    }

/* Add an IP as a server "closer" to the answer for a given
   branch of the DNS tree
   Input:  js_string of zone (raw UDP format with 2-byte suffix),
           js_string with ddip,
           action to take if a node is already here:
           1) Overwrite 2) Append 3) "Mask" currect table
   Output: JS_ERROR on fatal error, JS_SUCCESS on success
*/

int add_closer_jsddip(js_string *zone, js_string *ddip, int if_exists) {
    closer *close;
    js_string *ipjs;
    uint32 ip, *ipp=0;
    closer *point, *point_save;
    mhash_e spot_data;

    /* Allocate the memory for a new spot on the dnscache hash */
    if((close = js_alloc(1,sizeof(closer))) == 0)
        return JS_ERROR;

    /* Allocate the memory for the IP we will put on the hash */
    if((ipjs = js_create(6,1)) == 0) {
        js_dealloc(close);
        return JS_ERROR;
        }

    /* ddip_2_ip uses js_atoi, which means we have to set the encoding */
    ddip->encoding = MARA_LOCALE;

    /* Convert their ddip in to an ip */
    if(ddip_2_ip(ddip,ipjs,0) == JS_ERROR)
        goto cleanup;

    /* Make sure the raw zone is of the right rtype */
    if(change_rtype(zone,252) == JS_ERROR)
        goto cleanup;

    /* Convert ipjs in to a format that sendto() can use */
    if(ipjs->unit_count < 4)
        goto cleanup;
    ip = (*(ipjs->string) << 24) | (*(ipjs->string + 1) << 16) |
         (*(ipjs->string + 2) << 8) | *(ipjs->string + 3);
    js_destroy(ipjs);

    /* Fill up the "close" structure */
    close->num_elements = 1;
    close->datatype = RR_A;
    if((ipp = js_alloc(1,sizeof(uint32))) == 0)
        goto cleanup_noipjs;
    *ipp = ip;
    close->data = ipp;
    close->next = NULL;
    close->ttd = qual_get_time() + DEFAULT_TTL; /* XXX: Use the
                                                   server-supplied ttl */
    close->ttd = 0; /* Hack: The root name server IPs never expire */
    close->zap = NULL; /* This kind of record will never be zapped (removed)
                          by the custodian */

    /* Add the data to the hash */

    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        printf("Adding ");
        show_esc_stdout(zone);
        printf(" to cache at %p (jsddip)\n",close);
        log_unlock();
        }

    /* See if the data is already there.  If so, act based on the value
       of if_exists */
    spot_data = mhash_get(dnscache,zone);
    if(spot_data.value != 0 && spot_data.datatype == MARA_DNS_NS) {
        point = spot_data.value;
        switch(if_exists) {
            case OVERWRITE:
                /* Destroy the pointer to the element in the hash */
                mhash_undef(dnscache,zone);
                /* Destroy the element itself.  Go down the
                   linked list ans destroy each element one by one,
                   starting at the top. */
                while(point != 0) {
                   point_save = point->next;
                   if(point->datatype == RR_A && point->data != 0)
                       js_dealloc(point->data);
                   else if(point->datatype == RR_NS && point->data != 0)
                       js_destroy(point->data);
                   else if(point->data != 0)
                       js_dealloc(point->data);
                   js_dealloc(point);
                   point = point_save;
                   }
                /* OK, now that we have nuked the old element, add the new
                   element */
                if(mhash_put(dnscache,zone,close,MARA_DNS_NS) == JS_ERROR)
                    goto cleanup_noipjs;
                break;
            case 2: /* Append */
                while(point->next != 0)
                    point = point->next;
                point->next = close;
                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    printf("That's an append\n");
                    log_unlock();
                    }
                return JS_SUCCESS;

            /* XXX to do: mask */
            }
        }
    /* This error condition should never happen */
    else if(spot_data.value != 0)
        goto cleanup_noipjs;
    /* It's a (possibly newly) empty spot--add a new entry to the hash
       (w/ error checking) */
    else if(mhash_put(dnscache,zone,close,MARA_DNS_NS) == JS_ERROR)
        goto cleanup_noipjs;
    /* Make sure we do not overflow the cache */
    custodian();

    return JS_SUCCESS;
    cleanup:
        js_destroy(ipjs);
    cleanup_noipjs:
        if (ipp != 0 ) {
                js_dealloc(ipp);
                }
        js_dealloc(close);
        return JS_ERROR;

    }

/* Add an IP as a server "closer" to the answer for a given
   branch of the DNS tree, where the record in question is a root name server
   Input:  js_string of zone (raw UDP format with 2-byte suffix),
           js_string with ddip,
           action to take if a node is already here:
           1) Overwrite 2) Append 3) "Mask" currect table
   Output: JS_ERROR on fatal error, JS_SUCCESS on success
*/

int add_closer_ipv4pair(js_string *zone, ipv4pair *pair, int if_exists) {
    closer *close = 0;
    uint32 ip, *ipp = 0;
    closer *point, *point_save;
    mhash_e spot_data;

    /* Allocate the memory for a new spot on the dnscache hash */
    if((close = js_alloc(1,sizeof(closer))) == 0)
        return JS_ERROR;

    /* Make sure the raw zone is of the right rtype */
    if(change_rtype(zone,252) == JS_ERROR)
        goto cleanup;

    /* Convert the data (real simple in this case) */
    ip = pair->ip;

    /* Fill up the "close" structure */
    close->num_elements = 1;
    close->datatype = RR_A;
    if((ipp = js_alloc(1,sizeof(uint32))) == 0)
        goto cleanup;
    *ipp = ip;
    close->data = ipp;
    close->next = NULL;
    close->ttd = qual_get_time() + DEFAULT_TTL; /* XXX: Use the
                                                   server-supplied ttl */
    close->ttd = 0; /* Hack: The root name server IPs never expire */
    /* This record will never be zapped, being a root name server */
    close->zap = NULL;

    /* Add the data to the hash */

    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        printf("Adding ");
        show_esc_stdout(zone);
        printf(" to cache at %p (ipv4pair)\n",close);
        log_unlock();
        }

    /* See if the data is already there.  If so, act based on the value
       of if_exists */
    spot_data = mhash_get(dnscache,zone);
    if(spot_data.value != 0 && spot_data.datatype == MARA_DNS_NS) {
        point = spot_data.value;
        switch(if_exists) {
            case OVERWRITE:
                /* Destroy the pointer to the element in the hash */
                mhash_undef(dnscache,zone);
                /* Destroy the element itself.  Go down the
                   linked list ans destroy each element one by one,
                   starting at the top. */
                while(point != 0) {
                   point_save = point->next;
                   if(point->datatype == RR_A && point->data != 0)
                       js_dealloc(point->data);
                   else if(point->datatype == RR_NS && point->data != 0)
                       js_destroy(point->data);
                   else if(point->data != 0)
                       js_dealloc(point->data);
                   js_dealloc(point);
                   point = point_save;
                   }
                /* OK, now that we have nuked the old element, add the new
                   element */
                if(mhash_put(dnscache,zone,close,MARA_DNS_NS) == JS_ERROR)
                    goto cleanup;
                break;
            case 2: /* Append */
                while(point->next != 0)
                    point = point->next;
                point->next = close;
                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    printf("That's an append\n");
                    log_unlock();
                    }
                return JS_SUCCESS;
            /* XXX to do: mask */
            }
        }
    /* This error condition should never happen */
    else if(spot_data.value != 0)
        goto cleanup;
    /* It's a (possibly newly) empty spot--add a new entry to the hash
       (w/ error checking) */
    else if(mhash_put(dnscache,zone,close,MARA_DNS_NS) == JS_ERROR)
        goto cleanup;
    /* Make sure we do not overflow the cache */
    custodian();

    return JS_SUCCESS;
    cleanup:
        if(ipp != 0) {
            js_dealloc(ipp);
            }
        js_dealloc(close);
        return JS_ERROR;

    }

/* Add the name of a server "closer" to the answer for a given
   branch of the DNS tree
   Input:  js_string of zone (raw UDP format with 2-byte suffix),
           js_string with server name
           action to take if a node is already here:
           1) Overwrite 2) Append 3) "Mask" currect table
   Output: JS_ERROR on fatal error, JS_SUCCESS on success
*/

int add_closer_js(js_string *zone, js_string *name, int if_exists) {
    closer *close = 0;
    mhash_e spot_data;
    closer *point;
    js_string *new = 0, *zone_copy = 0;

    /* Allocate the memory for a new spot on the dnscache hash */
    if((close = js_alloc(1,sizeof(closer))) == 0)
        return JS_ERROR;
    if((new = js_create(name->unit_count + 1,1)) == 0) {
        js_dealloc(close);
        return JS_ERROR;
        }
    if((zone_copy = js_create(zone->unit_count + 1,1)) == 0) {
        js_dealloc(close);
        js_destroy(new);
        return JS_ERROR;
        }

    if(js_copy(name,new) == JS_ERROR) {
        js_dealloc(close); js_destroy(new); js_destroy(zone_copy);
        return JS_ERROR;
        }
    if(js_copy(zone,zone_copy) == JS_ERROR) {
        js_dealloc(close); js_destroy(new); js_destroy(zone_copy);
        return JS_ERROR;
        }

    /* Make sure the raw zone is of the right rtype */
    if(change_rtype(zone,252) == JS_ERROR)
        goto cleanup;

    /* Fill up the "close" structure */
    close->num_elements = 1;
    close->datatype = RR_NS;
    close->data = new;
    close->next = NULL;
    close->ttd = qual_get_time() + DEFAULT_TTL; /* XXX: Use
                                                   the server-supplied ttl */
    close->zap = 0;

    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        printf("Adding ");
        show_esc_stdout(zone);
        printf(" to cache at %p (js)\n",close);
        log_unlock();
        }

    /* Add the data to the hash */
    /* See if the data is already there.  If so, add this to the end of
       the chain */
    spot_data = mhash_get(dnscache,zone);

    if(spot_data.value != 0 && spot_data.datatype == MARA_DNS_NS) {
        point = spot_data.value;
        switch(if_exists) {
            case 1: /* Overwrite */
                /* Destroy the pointer to the element in the hash */
                mhash_undef(dnscache,zone);
                /* Destroy the element itself. */
                unlink_closer(point);
                /* Add this record to the list of records which
                   the custodian can potentially nuke */
                close->zap = new_fila(close,1,zone_copy,1);
                break;
            case 2: /* Append */
                point->num_elements++;
                close->zap=NULL; /* When we zap the top element, that
                                    zaps all of the elements */
                while(point->next != 0)
                    point = point->next;
                point->next = close;
                if(rlog_level >= 4) {
                    log_lock();
                    show_timestamp();
                    printf("That's an append\n");
                    log_unlock();
                    }
                js_destroy(zone_copy);
                return JS_SUCCESS;
            /* XXX to do: mask */
            }
        }
    /* This error condition should never happen */
    else if(spot_data.value != 0)
        goto cleanup;
    else {
        /* Add this record to the list of records which
           the custodian can potentially nuke */
        close->zap = new_fila(close,1,zone_copy,1);
        }

    /* It's an empty spot--add a new entry to the hash (w/ error checking) */
    if(mhash_put(dnscache,zone,close,MARA_DNS_NS) == JS_ERROR) {
        if(close->zap != 0)
            remove_fila(close->zap);
        js_dealloc(close); js_destroy(new);
        return JS_ERROR;
        }
    /* Make sure we do not overflow the cache */
    custodian();

    return JS_SUCCESS;
    cleanup:
        js_dealloc(close); js_destroy(new); js_destroy(zone_copy);
        return JS_ERROR;

    }

/* Wrapper that allows a substring of another js_string object to be
   the zone we add a "closer" jsip for.
   Input: js_string object with desired dlabel as substring
          offset of where the dlabel begins
          ipjs of IP to add
          action to do if node is already there
   Output: JS_ERROR on fatal error, JS_SUCCESS on success
*/

int add_closer_jsip_offset(js_string *js, int offset, js_string *ipjs,
                           int if_exists) {

    js_string *sub;
    int len, ret;

    /* Determine how long the domain label is */
    len = dlabel_length(js,offset);
    if(len == JS_ERROR)
        return JS_ERROR;

    /* Make a string which holds just the domain name label */
    if((sub = js_create(len + 4,1)) == 0)
        return JS_ERROR;
    if(js_substr(js,sub,offset,len) == JS_ERROR) {
        js_destroy(sub);
        return JS_ERROR;
        }
    if(js_adduint16(sub,252) == JS_ERROR) {
        js_destroy(sub);
        return JS_ERROR;
        }

    /* Run the actual add_closer_jsip function */
    ret = add_closer_jsip(sub,ipjs,if_exists);
    js_destroy(sub);
    return ret;
    }

/* Wrapper that allows a substring of another js_string object to be
   the zone we add a "closer" js for.
   Input: js_string object with desired dlabel as substring
          offset of where the dlabel (question being answered) begins
          offset of where the answer itself begins (in same js object)
          action to do if node is already there
   Output: JS_ERROR on fatal error, JS_SUCCESS on success
*/

int add_closer_js_offset(js_string *js, int offset1, int offset2,
                         int if_exists) {

    js_string *sub1, *sub2;
    int len1, len2, ret;

    /* Determine how long the labels are */
    len1 = dlabel_length(js,offset1);
    if(len1 == JS_ERROR)
        return JS_ERROR;
    len2 = dlabel_length(js,offset2);
    if(len2 == JS_ERROR)
        return JS_ERROR;

    /* Make a string which holds just the question */
    if((sub1 = js_create(len1 + 4,1)) == 0)
        return JS_ERROR;
    if(js_substr(js,sub1,offset1,len1) == JS_ERROR) {
        js_destroy(sub1);
        return JS_ERROR;
        }
    if(js_adduint16(sub1,252) == JS_ERROR) {
        js_destroy(sub1);
        return JS_ERROR;
        }

    /* Make a string which holds just the answer */
    if((sub2 = js_create(len2 + 4,1)) == 0) {
        js_destroy(sub1);
        return JS_ERROR;
        }
    if(js_substr(js,sub2,offset2,len2) == JS_ERROR) {
        js_destroy(sub1);
        js_destroy(sub2);
        return JS_ERROR;
        }
    if(js_adduint16(sub2,1) == JS_ERROR) {
        js_destroy(sub1);
        js_destroy(sub2);
        return JS_ERROR;
        }

    /* Run the actual add_closer_js function */
    ret = add_closer_js(sub1,sub2,if_exists);
    js_destroy(sub1);
    js_destroy(sub2);
    return ret;
    }

/* Add an IP as a server "closer" to the answer for a given
   branch of the DNS tree
   Input:  js_string of zone (raw UDP format with 2-byte suffix),
           js_string with ip
           action to take if a node is already here:
           1) Overwrite 2) Append 3) "Mask" currect table
   Output: JS_ERROR on fatal error, JS_SUCCESS on success
*/

int add_closer_jsip(js_string *zone, js_string *ipjs, int if_exists) {
    closer *close = 0;
    uint32 ip, *ipp = 0;
    closer *point;
    mhash_e spot_data;
    js_string *zone_copy = 0;

    /* Allocate the memory for a new spot on the dnscache hash */
    if((close = js_alloc(1,sizeof(closer))) == 0)
        return JS_ERROR;
    if((zone_copy = js_create(zone->unit_count + 1,1)) == 0) {
        js_dealloc(close);
        return JS_ERROR;
        }

    /* Make sure the raw zone is of the right rtype */
    if(change_rtype(zone,252) == JS_ERROR) {
        js_dealloc(close); js_destroy(zone_copy);
        return JS_ERROR;
        }

    /* Make the "zone_copy", which is used so that the fila knows
       what element to remove from the hash */
    if(js_copy(zone,zone_copy) == JS_ERROR) {
        js_dealloc(close); js_destroy(zone_copy);
        return JS_ERROR;
        }

    /* Convert ipjs in to a format that sendto() can use */
    if(ipjs->unit_count < 4) {
        js_dealloc(close); js_destroy(zone_copy);
        return JS_ERROR;
        }
    ip = (*(ipjs->string) << 24) | (*(ipjs->string + 1) << 16) |
         (*(ipjs->string + 2) << 8) | *(ipjs->string + 3);

    /* Fill up the "close" structure */
    close->num_elements = 1;
    close->datatype = RR_A;
    if((ipp = js_alloc(1,sizeof(uint32))) == 0) {
        goto cleanup;
        }
    *ipp = ip;
    close->data = ipp;
    close->next = NULL;
    close->ttd = qual_get_time() + DEFAULT_TTL; /* XXX: Use the
                                                   server-supplied ttl */
    close->zap = 0;

    if(rlog_level >= 4) {
        log_lock();
        show_timestamp();
        printf("Adding ");
        show_esc_stdout(zone);
        printf(" to cache at %p (jsip)\n",close);
        log_unlock();
        }

    /* Add the data to the hash */
    /* See if the data is already there.  If so, add this to the end of
       the chain */
    spot_data = mhash_get(dnscache,zone);

    if(spot_data.value != 0 && spot_data.datatype == MARA_DNS_NS) {
        point = spot_data.value;
        switch(if_exists) {
            case 1: /* Overwrite */
                /* Destroy the pointer to the element in the hash */
                mhash_undef(dnscache,zone);
                /* Destroy the element itself */
                unlink_closer(point);
                /* Add the new record to the list of records which the
                   custodian can potentially nuke */
                close->zap = new_fila(close,1,zone_copy,1);
                break;
            case 2: /* Append */
                point->num_elements++;
                close->zap = NULL; /* When we zap the element at the
                                      head, that zaps all of the elements */
                while(point->next != 0)
                    point = point->next;
                point->next = close;
                js_destroy(zone_copy);
                return JS_SUCCESS;
            /* XXX to do: mask */
            }
        }
    /* This error condition should never happen */
    else if(spot_data.value != 0)
        goto cleanup;
    else {
        /* Add this record to the list of records which the
           custodian can potentially nuke */
        close->zap = new_fila(close,1,zone_copy,1);
        }

    /* It's an empty spot--add a new entry to the hash (w/ error checking) */
    if(mhash_put(dnscache,zone,close,MARA_DNS_NS) == JS_ERROR) {
        /* Get rid of the superflous element, which also
           destroys the zone_copy string */
        if(close->zap != 0)
            remove_fila(close->zap);
        js_dealloc(ipp); js_dealloc(close);
        return JS_ERROR;
        }
    /* Make sure we do not fill up the cache */
    custodian();

    return JS_SUCCESS;
    cleanup:
        js_dealloc(close);
        if (ipp != 0) {
                js_dealloc(ipp);
                }
        js_destroy(zone_copy);
        return JS_ERROR;

    }

/* Add an IP as a server "closer" to the answer for a given
   branch of the DNS tree
   Input:  null-terminated string of zone, nt-string of ddip
   Output: JS_ERROR on fatal error, JS_SUCCESS on success
*/

int add_closer_ip(char *zone, char *ddip) {
    js_string *zone_js, *ddip_js;
    int zone_len, ddip_len, ret;

    /* Sanity checks */
    if(zone == 0 || *zone == 0)
        return JS_ERROR;
    if(ddip == 0 || *ddip == 0)
        return JS_ERROR;

    zone_len = js_strnlen(zone,1000000);
    ddip_len = js_strnlen(ddip,1000000);

    /* Initialize the js string objects */
    if((zone_js = js_create(zone_len + 3,1)) == 0)
        return JS_ERROR;

    if((ddip_js = js_create(ddip_len + 1,1)) == 0) {
        js_destroy(zone_js);
        return JS_ERROR;
        }

    /* Copy over the null-terminated strings to the js string objects */
    if(js_str2js(zone_js,zone,zone_len,1) == JS_ERROR)
        goto cleanup;
    if(js_str2js(ddip_js,ddip,ddip_len,1) == JS_ERROR)
        goto cleanup;

    /* Convert the zone name in to a binary string */
    if(hname_2rfc1035(zone_js) <= 0)
        goto cleanup;
    /* Add a two-byte "query type" to the end of "zone_js" */
    if(js_adduint16(zone_js,252) == JS_ERROR)
        goto cleanup;

    /* Call the "real" function that uses the js_string objects */
    ret = add_closer_jsddip(zone_js,ddip_js,OVERWRITE);
    js_destroy(zone_js);
    js_destroy(ddip_js);
    return ret;

    cleanup:
        js_destroy(ddip_js);
        js_destroy(zone_js);
        return JS_ERROR;
    }

/* Initialize the list of spam-friendly DNS servers which we will refuse
   to obtain DNS information from.

   Input: Pointer to js_string object with list of spammers
   Output: JS_SUCCESS on success, JS_ERROR on error
   Global Variables used: ipv4pair spammers[512]

*/

int init_spammers(js_string *spam_list) {
    int counter;

    /* Clear out the spammers ipv4 pair */
    for(counter=0;counter<511;counter++)
        spammers[counter].ip = 0xffffffff;

    /* Initialize the list, if applicable */
    if(spam_list != 0)
        return make_ip_acl(spam_list,spammers,500,0);
    else
        return JS_SUCCESS;

    }

/* Set the range of ports that the recursive resolver will bind to
 * when making requests to other DNS servers */

int set_port_range(int a, int b) {
    recurse_min_bind_port = a;
    recurse_number_ports = b - 1;
    return JS_SUCCESS;
    }

/* Set the upstream_port; the port we use to contact to remote DNS
 * servers */

int set_upstream_port(int num) {
    if(num < 1 || num > 65530) {
        printf("Warning: upstream_port out of range; using default value"
        " of 53\n");
        upstream_port = 53;
        return 0;
        }
    upstream_port = num;
    return 1;
    }

/* Set the minimim TTLs
   Input: The minimum TTL for non-CNAME records, the minimum TTL for
          CNAME records
   Output: JS_ERROR on fail, 1 on success
*/

int set_min_ttl(int norm, int cname) {

     /* Some limits here */
     if(norm < 0)
         norm = 300;
     if(norm < 180)
         norm = 180;
     if(cname < 0)
         cname = norm;
     if(cname < 180)
         cname = 180;

     min_ttl_cname = cname;
     min_ttl_normal = norm;
     return 1;
     }

/* Add a given "root" nameserver for a given zone to the cache.  This is a
 * non-removable entry; this tells the cache what the nameservers are under
 * a given sub-tree of the DNS tree.  For example, a nameserver for "com."
 * will service all of the otherwise uncached requests for DNS names that
 * end in ".com".  A nameserver for "." is a root nameserver */

int add_hardcoded_ns(js_string *rootns, js_string *rootns_zone) {

    int ret = 0;
    int counter = 0;
    ipv4pair root_servers[512];
    js_string *zone;

    /* Initialize the list of root servers */
    for(counter=0;counter<511;counter++) {
        root_servers[counter].ip = 0xffffffff;
    }

    zone = js_create(256,1);
    if(zone == 0) {
        return 0;
    }

    /* Reset the locale so it works with the hash routines */
    js_set_encode(rootns,1);
    js_set_encode(rootns_zone,1);
    js_set_encode(zone,1);

    /* Convert rootns_zone in to a binary packet with "query type" */
    ret = -8;
    if(js_qstr2js(zone,"A") == JS_ERROR)
        goto cleanup;
    if(js_append(rootns_zone,zone) == JS_ERROR)
        goto cleanup;
    ret = -9;
    if(hname_2rfc1035(zone) <= 0) {
        printf("\n***************************************\n");
        printf("WARNING: Invalid zone name ");
        show_esc_stdout(rootns_zone);
        printf("\n(Perhaps you forgot the final dot)\n");
        printf("Please remove or edit the line from your mararc that looks "
               "like this:\n\n");
        if(root_or_upstream == 1) {
                printf("upstream");
        } else {
                printf("root");
        }
        printf("_servers[\"");
        show_esc_stdout(rootns_zone);
        printf("\"] = \"");
        show_esc_stdout(rootns);
        printf("\"\n\n");
        js_destroy(zone);
        printf("This line is being ignored by MaraDNS\n");
        printf("***************************************\n\n");
        return JS_SUCCESS;
        }
    ret = -10;
    if(js_adduint16(zone,252) == JS_ERROR)
        goto cleanup;
    /* Convert the rootns in to a list of ipv4 pairs */
    ret = -11;
    if(make_ip_acl(rootns,root_servers,500,0) == JS_ERROR) {
        printf("\n***************************************\n");
        printf("WARNING: Invalid IP list for zone ");
        show_esc_stdout(rootns_zone);
        printf("\nPlease remove or edit the line from your mararc that looks "
               "like this:\n\n");
        if(root_or_upstream == 1) {
                printf("upstream");
        } else {
                printf("root");
        }
        printf("_servers[\"");
        show_esc_stdout(rootns_zone);
        printf("\"] = \"");
        show_esc_stdout(rootns);
        printf("\"\n\n");
        js_destroy(zone);
        printf("This line is being ignored by MaraDNS\n");
        printf("***************************************\n\n");
        js_destroy(zone);
        return JS_SUCCESS;
        }
    counter = 0;
    while(root_servers[counter].ip != 0xffffffff) {
        ret = -12;
        if(counter > 502)
            goto cleanup;
        ret = -13;
        if(add_closer_ipv4pair(zone,&(root_servers[counter]),APPEND)
           == JS_ERROR)
            goto cleanup;
        counter++;
        }

    js_destroy(zone);
    return JS_SUCCESS;

    cleanup:
        js_destroy(zone);
        return ret;

    }

/* Initialize the cache hash for general use
   Input: maximum number of elements the cache can have,
          maximum number of threads we are allowed to run
   Output: less than 0 on error, JS_SUCCESS on success
   Global variables used: max_cache, etc.
   Error return values:
    -1: Max cache elements is too big
    -2: We have already run init_cache
    -3: Failure to create a js_string object (shouldn't happen)
    -4: Failure to create a js_string object (shouldn't happen)
    -5: Failure to make a string a js_string object (shouldn't happen)
    -6: Failure to make a string a js_string object (shouldn't happen)
    -7: The root_servers["."] element does not exist
    -8: Failure to make a string a js_string object (shouldn't happen)
    -9: Failure to make "A." a binary hostname (shouldn't happen)
   -10: Failure to add a 16-bit integer to a js_string (shouldn't happen)
   -11: The elements of the root_servers["."] are invalid
   -12: Too many elements in the root_servers ACL (shouldn't happen)
   -13: Problem adding a root nameserver to the cache proper (shoudn't
        happen);
   -14: Both root servers and upstream servers are set

*/

int init_cache(int max_cache_elements, int max_threads, int max_glueless,
               int max_q_total, int timeout, int verbose_query_value) {
    js_string *rootns;
    js_string *rootns_zone;
    js_string *rzone;
    int bits = 1;
    int exp_counter = 1;
    int ret = 0;
    mhash *root_serv, *upstream_serv; /* A pointer to the hash with data
                                       * for root and upstream servers */
    int root_seen = 0;

    /* initialize the rra_data */
    if(rra_data == 0) {
            rra_data = init_ra_data();
    }

    /* Do we want verbose queries */
    verbose_query = verbose_query_value;

    /* Set the maximum glueless level */
    max_glueless_level = 10;
    if(max_glueless > 0)
        max_glueless_level = max_glueless;

    /* Set the maximum number of queries total */
    max_queries_total = 32;
    if(max_q_total > 0)
        max_queries_total = max_q_total;

    /* Set the maximum time to wait for another DNS server */
    timeout_seconds = 2;
    if(timeout > 0)
        timeout_seconds = timeout;

    /* Set the maximum number of threads */
    if(max_threads > 0)
        maximum_num_of_threads = max_threads;

    /* Determine the number of bits the hash will need */
    while(exp_counter <= max_cache_elements) {
        /* The enterprise version which can handle more than 2^30 elements
           will come when someone contracts me to write the code (the license
           has to be public domain or GPL) */
        if(bits > 30)
            return -1;
        bits++;
        exp_counter *= 2;
        }

    /* As a table:
       Max_cache_elements Bits
       0                  1 (hash size 1)
       1                  2 (size 3)
       2-3                3 (" "  7)
       4-7                4 (15)
       8-15               5 (31)
       16-31              6 (63)
       32-63              7 (127)
       64-127             8 (255)
       128-255            9 (511)
       256-511            10 (1023)
       512-1023           11 (2047)
       1024-2047          12 (4095)
       etc.
    */

    if(dnscache != 0) /* We can only init the DNS cache once */
        return -2;

    /* Initialize the dnscache object */
    dnscache = (mhash *)mhash_create(bits);

    /* Copy over max_cache_elements to a global variable, so that
       other routines can see this value */
    cache_max = max_cache_elements;

    /* Set up the root nameserver(s).  */

    if((rootns = js_create(256,1)) == 0) {
        return -3;
        }
    if((rootns_zone = js_create(256,1)) == 0) {
        js_destroy(rootns);
        return -4;
        }
    if((rzone = js_create(256,1)) == 0) {
        js_destroy(rootns);
        js_destroy(rzone);
        return -4;
        }

    root_serv = (mhash *)dvar_raw(dq_keyword2n("root_servers"));
    upstream_serv = (mhash *)dvar_raw(dq_keyword2n("upstream_servers"));

    /* Set the locale so it works with read_dvar */
    js_set_encode(rootns,MARA_LOCALE);
    js_set_encode(rootns_zone,MARA_LOCALE);
    /* Both "root_servers" and "upstream_servers" are not set, use
     * the default icann servers */
    if(root_serv == 0 && upstream_serv == 0) {
            /* Let them know they are using the default servers */
            printf("Using default ICANN root servers\n");
            if(js_qstr2js(rootns,ROOT_SERVERS) == JS_ERROR) {
                    goto cleanup;
                }
             if(js_qstr2js(rootns_zone,".") == JS_ERROR) {
                goto cleanup;
             }

            ret = add_hardcoded_ns(rootns,rootns_zone);
            goto cleanup;
    }
    /* Thou shalt not have both root servers and upstream servers */
    else if(root_serv != 0 && upstream_serv != 0) {
        ret = -14;
        goto cleanup;
    }

    /* At this point, one and only one of root_serv and upstream_serv
     * are set */

    /* If upstream_servers is set, set root_or_upstream to one
     * and treat upstream_serv just like root_serv */
    root_or_upstream = 0;
    if(upstream_serv != 0) {
        root_or_upstream = 1;
        root_serv = upstream_serv;
        }

    root_seen = 0;

    /* Now, let's walk through the root_serv hash */
    if(mhash_firstkey(root_serv,rzone) == JS_ERROR) {
        ret = JS_ERROR;
        goto cleanup;
        }
    do {
        js_string *nslist;
        nslist = mhash_get_js(root_serv,rzone);
        if(nslist == 0) { /* No elements in the root_servers/upstream_servers
                           * list */
             break;
        }
        printf("Adding root nameserver ");
        show_esc_stdout(nslist);
        printf(" for zone ");
        show_esc_stdout(rzone);
        printf("\n");
        if(js_qissame(".",rzone)) {
             root_seen = 1;
             }
        ret = add_hardcoded_ns(nslist,rzone);
        js_destroy(nslist);
        if(ret != JS_SUCCESS) {
             goto cleanup;
             }
        } while(mhash_nextkey(root_serv,rzone) != 0);

    if(root_seen == 0) {
        if(root_or_upstream == 1) {
                printf("FATAL ERROR: upstream_servers[\".\"] must be set when");
                printf("\nUsing upstream_servers\n");
                exit(1);
        }
        printf("Using ICANN nameservers for root_servers[\".\"]\n");
        if(js_qstr2js(rootns,ROOT_SERVERS) == JS_ERROR) {
                goto cleanup;
        }
        if(js_qstr2js(rootns_zone,".") == JS_ERROR) {
                goto cleanup;
        }
        ret = add_hardcoded_ns(rootns,rootns_zone);
        goto cleanup;

    }

    ret = JS_SUCCESS;
    cleanup:
        js_destroy(rootns);
        js_destroy(rootns_zone);
        js_destroy(rzone);
        return ret;
    }

/* BEGIN RNG USING CODE */
/* Initialize the secure psudo-random-number generator */
/* Input: pointer to string that has the filename with the PRNG seed
 * Whether we are re-keying the PRNG or not (rekey, 0 we aren't, 1 we are)
   Output: JS_SUCCESS.  On failure:
           -1 (JS_ERROR): Generalized error which means there is a bug in
                          MaraDNS
           -2: We could not open up the random seed file
           -3: We could not read 16 bytes from the random seed file
*/
int init_rng(js_string *seedfile, int rekey) {
    unsigned char prng_seed[34];
    static int desc;
    int counter, max;
    pid_t process_id;
    char path[MAXPATHLEN + 2];

    /* Initialize the input block and the "binSeed" (is this used?) */
    memset(r_inBlock,0,16);
    time((time_t *)&r_inBlock[0]);
    memset(r_binSeed,0,16);

    if(rekey == 0) {
        /* Read the key in from the file */
        if(js_js2str(seedfile,path,MAXPATHLEN) == JS_ERROR) {
            return JS_ERROR;
            }
        desc = open(path,O_RDONLY);
        if(desc == -1)
            return -2;
        } else {
            lseek(desc,0,SEEK_SET); /* Just in case this is a short
                                       ordinary file; we encrypt with
                                       the previously seeded material */
        }
    if(read(desc,prng_seed,16) != 16) /* 16 bytes: 128-bit seed */
        return -3;
    /*close(desc);*/ /* We keep it open for re-keying */

    /* In order to guarantee that two MaraDNS processes do not use the
       same prng seed, we exclusive-or the prng seed with the process-id
       of the maradns process */
    process_id = getpid();
    max = sizeof(pid_t);
    if(max > 15)
        max = 15;
    for(counter = 0; counter < max; counter++) {
        prng_seed[15 - counter] ^= process_id & 0xff;
        process_id >>= 8;
        }

    /* If we are re-keying instead of running this for the first time,
     * XOR the prng_seed with an encrypted block; this makes things
     * more secure when using an ordinary file for the RNG seed */
    if(rekey == 1) {
            blockEncrypt(&r_cipherInst,&r_seedInst,r_inBlock,128,r_outBlock);
            for(counter = 0; counter < 15; counter++) {
                    prng_seed[counter] ^= (r_outBlock[counter] & 0xff);
            }
    }

    /* Initialize the PRNG with the seed in question */
    if(makeKey(&r_seedInst, DIR_ENCRYPT, 128, (char *)prng_seed) != 1)
        return JS_ERROR;
    if(cipherInit(&r_cipherInst, MODE_ECB, NULL) != 1)
        return JS_ERROR;
    if(blockEncrypt(&r_cipherInst,&r_seedInst,r_inBlock,128,r_outBlock) != 128)
        return JS_ERROR;

    return JS_SUCCESS;
    }
/* END RNG USING CODE */

/* Set the level of logging of messages
   Input: Log level desired
   Output: js_success
   Global variables used: rlog_level
*/

int init_rlog_level(int verbose_level) {
    rlog_level = verbose_level;
    return JS_SUCCESS;
    }

/* Set the numer of times we try contacting all of the nameservers in the
 * process of resolving a name */
int init_retry_cycles(int in) {
    if(in >= 1 && in <= 31) {
         retry_cycles = in;
         }
    return JS_SUCCESS;
    }

