/*
 * Copyright 1995-2016 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the OpenSSL license (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

/* ====================================================================
 * Copyright 2002 Sun Microsystems, Inc. ALL RIGHTS RESERVED.
 *
 * Portions of the attached software ("Contribution") are developed by
 * SUN MICROSYSTEMS, INC., and are contributed to the OpenSSL project.
 *
 * The Contribution is licensed pursuant to the OpenSSL open source
 * license provided above.
 *
 * The ECDH and ECDSA speed test software is originally written by
 * Sumit Gupta of Sun Microsystems Laboratories.
 *
 */

// Modified by Arne Schwabe to give a simple openssl evp speed java api

#include <jni.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/times.h>
#include <linux/if.h>
#include <android/log.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include "jniglue.h"
#include <android/log.h>

#include <openssl/crypto.h>
#include <openssl/rand.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/objects.h>
#include <openssl/async.h>
#include <openssl/provider.h>


/* This file just contains code thrown together until it works */


#undef SECONDS
#define SECONDS                 3
#define PRIME_SECONDS   10
#define RSA_SECONDS             10
#define DSA_SECONDS             10
#define ECDSA_SECONDS   10
#define ECDH_SECONDS    10


typedef struct loopargs_st {
    unsigned char *buf;
    unsigned char *buf2;
    unsigned char *buf_malloc;
    unsigned char *buf2_malloc;
    unsigned int siglen;
    EVP_CIPHER_CTX *ctx;
    HMAC_CTX *hctx;
} loopargs_t;

#undef BUFSIZE
#define BUFSIZE (1024*16+1)
#define MAX_MISALIGNMENT 63


#define MAX_BLOCK_SIZE 128
static unsigned char iv[2 * MAX_BLOCK_SIZE / 8];

#define SIZE_NUM        7
static const int lengths[SIZE_NUM] = {
    16, 64, 256, 1024, 1500, 8 * 1024, 16 * 1024
};

static int testnum;

# define COND(unused_cond) (run && count<0x7fffffff)

static volatile int run = 0;

#ifdef SIGALRM
# if defined(__STDC__) || defined(sgi) || defined(_AIX)
#  define SIGRETTYPE void
# else
#  define SIGRETTYPE int
# endif


#define START   0
#define STOP    1
#define TM_START        0
#define TM_STOP         1

static int usertime = 1;

double app_tminterval(int stop, int usertime)
{
    double ret = 0;
    struct tms rus;
    clock_t now = times(&rus);
    static clock_t tmstart;

    if (usertime)
        now = rus.tms_utime;

    if (stop == TM_START)
        tmstart = now;
    else {
        long int tck = sysconf(_SC_CLK_TCK);
        ret = (now - tmstart) / (double)tck;
    }

    return (ret);
}




static double Time_F(int s)
{
    double ret = app_tminterval(s, usertime);
    if (s == STOP)
        alarm(0);
    return ret;
}

#endif


static long save_count = 0;
static int decrypt = 0;
static int EVP_Update_loop(void *args)
{
    loopargs_t *tempargs = *(loopargs_t **)args;
    unsigned char *buf = tempargs->buf;
    EVP_CIPHER_CTX *ctx = tempargs->ctx;
    int outl, count;

    if (decrypt)
        for (count = 0; COND(nb_iter); count++)
            EVP_DecryptUpdate(ctx, buf, &outl, buf, lengths[testnum]);
    else
        for (count = 0; COND(nb_iter); count++)
            EVP_EncryptUpdate(ctx, buf, &outl, buf, lengths[testnum]);
    if (decrypt)
        EVP_DecryptFinal_ex(ctx, buf, &outl);
    else
        EVP_EncryptFinal_ex(ctx, buf, &outl);
    return count;
}

static const EVP_MD *evp_md = NULL;
static int EVP_Digest_loop(void *args)
{
    loopargs_t *tempargs = *(loopargs_t **)args;
    unsigned char *buf = tempargs->buf;
    unsigned char md[EVP_MAX_MD_SIZE];
    int count;

    for (count = 0; COND(nb_iter); count++) {
        if (!EVP_Digest(buf, lengths[testnum], md, NULL, evp_md, NULL))
            return -1;
    }
    return count;
}


static int run_benchmark(int async_jobs,
                         int (*loop_function)(void *), loopargs_t *loopargs)
{
    int job_op_count = 0;
    int total_op_count = 0;
    int num_inprogress = 0;
    int error = 0, i = 0, ret = 0;
    OSSL_ASYNC_FD job_fd = 0;
    size_t num_job_fds = 0;

    run = 1;

    if (async_jobs == 0) {
        return loop_function((void *)&loopargs);
    }
    return 1234567;
}


static void* stop_run(void* arg)
{
    __android_log_write(ANDROID_LOG_DEBUG,"openvpn", "stop run thread started");
    sleep(3);
    run=0;
    __android_log_write(ANDROID_LOG_DEBUG,"openvpn", "stop run thread stopped");
    return NULL;
}

jdoubleArray Java_de_blinkt_openvpn_core_NativeUtils_getOpenSSLSpeed(JNIEnv* env, jclass thiz, jstring algorithm, jint testnumber)
{

    OSSL_PROVIDER *legacy;
    OSSL_PROVIDER *deflt;

    OSSL_LIB_CTX *lib_ctx = OSSL_LIB_CTX_new();

    /* Load Multiple providers into the default (NULL) library context */
    legacy = OSSL_PROVIDER_load(lib_ctx, "legacy");
    if (legacy == NULL) {
        __android_log_write(ANDROID_LOG_DEBUG,"openvpn", "Failed to load Legacy provider\n");
        return NULL;
    }
    deflt = OSSL_PROVIDER_load(lib_ctx, "default");
    if (deflt == NULL) {
        __android_log_write(ANDROID_LOG_DEBUG,"openvpn", "Failed to load Default provider\n");
        OSSL_PROVIDER_unload(legacy);
        return NULL;
    }

    static const unsigned char key16[16] = {
        0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
        0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0, 0x12
    };
    const EVP_CIPHER *evp_cipher = NULL;

    const char* alg = (*env)->GetStringUTFChars( env, algorithm , NULL ) ;

    evp_cipher = EVP_CIPHER_fetch(lib_ctx, alg, NULL);
    if (evp_cipher == NULL)
        evp_md = EVP_MD_fetch(lib_ctx, alg, NULL);
    if (evp_cipher == NULL && evp_md == NULL) {
        //        BIO_printf(bio_err, "%s: %s is an unknown cipher or digest\n", prog, opt_arg());
        //jniThrowException(env, "java/security/NoSuchAlgorithmException", "Algorithm not found");
        __android_log_write(ANDROID_LOG_DEBUG,"openvpn", "Algorithm not found");
        return NULL;
    }

    loopargs_t *loopargs = NULL;
    int loopargs_len = 1;
    int async_jobs = 0;
    loopargs = malloc(loopargs_len * sizeof(loopargs_t));
    memset(loopargs, 0, loopargs_len * sizeof(loopargs_t));


    jdoubleArray ret = (*env)->NewDoubleArray(env, 3);

    if (testnum < 0 || testnum >= SIZE_NUM)
        goto error;

    testnum = testnumber;


    for (int i = 0; i < loopargs_len; i++) {
        int misalign=0;
        loopargs[i].buf_malloc = malloc((int)BUFSIZE + MAX_MISALIGNMENT + 1);
        loopargs[i].buf2_malloc = malloc((int)BUFSIZE + MAX_MISALIGNMENT + 1);
        /* Align the start of buffers on a 64 byte boundary */
        loopargs[i].buf = loopargs[i].buf_malloc + misalign;
        loopargs[i].buf2 = loopargs[i].buf2_malloc + misalign;
    }


    int count;
    double d;
    if (evp_cipher) {
        /*
         * -O3 -fschedule-insns messes up an optimization here!
         * names[D_EVP] somehow becomes NULL
         */


        for (int k = 0; k < loopargs_len; k++) {
            loopargs[k].ctx = EVP_CIPHER_CTX_new();
            if (decrypt)
                EVP_DecryptInit_ex(loopargs[k].ctx, evp_cipher, NULL, key16, iv);
            else
                EVP_EncryptInit_ex(loopargs[k].ctx, evp_cipher, NULL, key16, iv);
            EVP_CIPHER_CTX_set_padding(loopargs[k].ctx, 0);
        }

        Time_F(START);
        pthread_t timer_thread;

        if (pthread_create(&timer_thread, NULL, stop_run, NULL))
            goto error;

        count = run_benchmark(async_jobs, EVP_Update_loop, loopargs);
        d = Time_F(STOP);
        for (int k = 0; k < loopargs_len; k++) {
            EVP_CIPHER_CTX_free(loopargs[k].ctx);
        }
    }
    if (evp_md) {
        pthread_t timer_thread;
        if (pthread_create(&timer_thread, NULL, stop_run, NULL))
        {
            __android_log_write(ANDROID_LOG_DEBUG,"openvpn", "creating thread failed");
            goto error;
        }

        Time_F(START);
        count = run_benchmark(async_jobs, EVP_Digest_loop, loopargs);
        d = Time_F(STOP);
    }

    // Save results in hacky way
    double results[] = {(double) lengths[testnum], (double) count, d};


    (*env)->SetDoubleArrayRegion(env, ret, 0, 3, results);
    //        print_result(D_EVP, testnum, count, d);

    OSSL_LIB_CTX_free(lib_ctx);
    free(loopargs);
    return ret;
error:
    for (int k = 0; k < loopargs_len; k++) {
      EVP_CIPHER_CTX_free(loopargs[k].ctx);
    }
    free(loopargs);

    OSSL_LIB_CTX_free(lib_ctx);
  return NULL;
}
