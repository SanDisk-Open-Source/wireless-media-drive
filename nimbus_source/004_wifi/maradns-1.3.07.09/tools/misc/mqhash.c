/* Copyright (c) 2002-2006 Sam Trenholme
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

/* This code is based on the aeshash.pdf file, written by Bram Cohen and
   Ben Laurie.  The code currently uses the 128-bit key and block size of
   MaraDNS's hasher, making a 128-bit hash */

/* Note that you need to change the cipher we call to change these
   constants */
#define HASH_BITS 128
#define HASH_BYTES (HASH_BITS / 8)

#include "../../rng/rng-api-fst.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

/* Routine that runs the compression funciton of our hash.
   Input: Plaintext we wish to compress (which is HASH_BYTES long),
          current state of the hash
   Output: The current state of the hash is modified
           1 on success, -1 on fail
*/

int hash_compress(char *input, char *state) {
    MARA_BYTE r_inBlock[HASH_BYTES + 1], r_outBlock[HASH_BYTES + 1],
         r_binKey[HASH_BYTES + 1];
    keyInstance r_keyInst;
    cipherInstance r_cipherInst;
    int counter;

#ifdef DEBUG
    for(counter = 0; counter < HASH_BYTES; counter++) {
        if(input[counter] >= 32 && input[counter] < 128)
            printf("%c",input[counter] & 0xff);
        else
            printf("");
        }
    printf("\n");
#endif /* DEBUG */

    /* Copy over the input of the hash to the key for the cipher */
    for(counter = 0; counter < HASH_BYTES; counter++) {
        r_binKey[counter] = input[counter];
        }

    /* Copy over the state and make it the "plaintext" of the cipher */
    for(counter = 0; counter < HASH_BYTES; counter++) {
        r_inBlock[counter] = state[counter];
        }

    /* Prepare the encryption */
    if(makeKey(&r_keyInst, DIR_ENCRYPT, HASH_BITS, r_binKey) != 1) {
        return -1;
        }

    if(cipherInit(&r_cipherInst, MODE_ECB, NULL) != 1) {
        return -1;
        }

    /* Perform the encryption */
    if(blockEncrypt(&r_cipherInst, &r_keyInst, r_inBlock, HASH_BITS,
                    r_outBlock) != HASH_BITS) {
        return -1;
        }

    /* XOR the ciphertext with the current hash state */
    for(counter = 0; counter < HASH_BYTES; counter++) {
        r_outBlock[counter] ^= state[counter];
        }

    /* Make the modified ciphertext the new state */
    for(counter = 0; counter < HASH_BYTES; counter++) {
        state[counter] = r_outBlock[counter];
        }

    return 1;
    }

/* The main routine.  This reads a file specified on the command line,
   then makes a hash out of that file. */

main(int argc, char **argv) {
    char state[HASH_BYTES + 1], input[HASH_BYTES + 1];

    int readed, counter;
    FILE *sh;
    char copy[HASH_BYTES * 43];
    int n = 0;

    unsigned int len = 0;

    /* Check the command line argument */
    if(argc != 3 && argc != 5) {
        if(argc >= 1) {
            printf("Usage: %s [-n #] [-s] [-u] {data to hash}\n",argv[0]);
            exit(1);
            }
        else {
            printf("Usage: <this program> [-n #] [-s] [-u] {data to hash}\n");
            exit(2);
            }
        }

    /* Initialize the state */
    for(counter = 0; counter < HASH_BYTES; counter++) {
        state[counter] = 0xff;
        }

    if(argc == 5 && (*(argv[1]) != '-' || *(argv[1] + 1) != 'n')) {
            printf("Usage: <this program> [-n #] [-s] [-u] {data to hash}\n");
            exit(3);
            }
    else if(argc == 5) {
            n = atoi(argv[2]);
            if(n < 2 || n > 9) {
                printf("n must be between 2 and 9\n");
                exit(4);
                }
            }
    /* Open up what we prepend to the hash if -s is in argv */
    if((argc == 3 && *(argv[1]) == '-' && *(argv[1] + 1) == 's') ||
       (argc == 5 && *(argv[3]) == '-' && *(argv[3] + 1) == 's')) {
            char fp[100];
            int zork;

            if(strncpy(fp,getenv("HOME"),50) == NULL) {
                    perror("Problem copying string");
                    exit(35);
            }
            if(strcat(fp,"/.mhash_prefix") == NULL) {
                    perror("Problem making string");
                    exit(36);
            }
            if((sh = fopen(fp,"rb")) == NULL) {
                perror("Could not open file ~/.mhash_prefix");
                exit(5);
                }
            /* Get only one line from this file: A string we put at
             * the beginning of the hash we will make */
            for(counter = 0; counter < 85; counter++) {
                copy[counter] = 0;
                }
            fgets(copy,79,sh);
            fclose(sh);
            counter = strnlen(copy,85);
            counter--;
            /* Remove :, which is a metacharacter */
            for(zork = 0; zork <= counter; zork++) {
                if(copy[zork] == ':') {
                    copy[zork] = '@';
                    }
                }
            copy[counter] = ':';
            counter++;
            }
    else if((argc == 3 && *(argv[1]) == '-' && *(argv[1] + 1) == 'u') ||
       (argc == 5 && *(argv[3]) == '-' && *(argv[3] + 1) == 'u')) {
       counter = 0;
       }
    else {
       printf("Usage: <this program> [-n #] [-s] [-u] {data to hash}\n");
       exit(6);
       }

    if(strnlen(argv[argc - 1],HASH_BYTES * 35) >= HASH_BYTES * 31) {
         printf("Hash input is too long!\n");
         }

    if(n >= 1 && n <= 9) {
        copy[counter] = '0' + n;
        counter++;
        copy[counter] = ':';
        counter++;
        }
    readed = counter;
    for(;counter < HASH_BYTES * 42; counter++)
        copy[counter] = 0;
    counter = readed;
    for(;counter < HASH_BYTES * 41; counter++) {
        /* The ':' is always a metacharacter */
        if(argv[argc - 1][counter - readed] == ':')
            argv[argc - 1][counter - readed] = '@';
        if(argv[argc - 1][counter - readed] == '\0')
            break;
        copy[counter] = argv[argc - 1][counter - readed];
        }

    /* Initialize the state */
    for(counter = 0; counter < HASH_BYTES; counter++) {
        state[counter] = 0xff;
        }

    for(counter = 0;counter < 128;counter+=16) {
        for(readed = 0; readed < 16; readed++) {
            input[readed] = copy[counter + readed];
            if(input[readed] == '\0')
                break;
            }
        if(input[readed] == '\0' && readed < 16)
            break;
        hash_compress(input,state);
        }

    len = readed + counter;
#ifdef DEBUG
    printf("%d\n",len);
#endif

    /* Pad the final block */
#ifdef ONE_PAD
    if(readed == 0) {
        readed = 1;
        input[0] = 1;
        }
    else if(readed <= (HASH_BYTES / 2) - 1) {
        input[readed] = 1;
        readed++;
        }
    else if(readed < HASH_BYTES) {
        input[readed] = 1;
        readed++;
#else
    if(readed >= HASH_BYTES / 2) {
#endif
        while(readed < HASH_BYTES) {
            input[readed] = 0;
            readed++;
            }
        hash_compress(input,state);
        readed = 0;
        }
    else if(readed == HASH_BYTES) {
        hash_compress(input,state);
#ifdef ONE_PAD
        readed = 1;
        input[0] = 1;
#else
        readed = 0;
#endif
        }

    while(readed < HASH_BYTES - 4) {
        input[readed] = 0;
        readed++;
        }

    /* We count bits, not bytes (as specified in the aeshash pdf document),
       so multiply len times eight */
    len *= 8;

    /* Pad the length to the hash input */
    for(counter = 3; counter >= 0; counter--) {
        input[readed] = (len >> (counter * 8)) & 0xff;
        readed++;
        }
    hash_compress(input,state);
    hash_compress(state,state);

    for(counter = 0; counter < HASH_BYTES; counter++) {
        printf("%02x",state[counter] & 0xff);
        if(counter % 4 == 3) {printf(" ");}
        }
    printf("\n");
    }

