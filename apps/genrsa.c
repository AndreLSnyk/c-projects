/*
 * Copyright 1995-2020 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#include <openssl/opensslconf.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "apps.h"
#include "progs.h"
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/bn.h>
#include <openssl/rsa.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <openssl/pem.h>
#include <openssl/rand.h>

#define DEFBITS 2048
#define DEFPRIMES 2

static int verbose = 0;

static int genrsa_cb(EVP_PKEY_CTX *ctx);

typedef enum OPTION_choice {
    OPT_ERR = -1, OPT_EOF = 0, OPT_HELP,
    OPT_3, OPT_F4, OPT_ENGINE,
    OPT_OUT, OPT_PASSOUT, OPT_CIPHER, OPT_PRIMES, OPT_VERBOSE,
    OPT_R_ENUM, OPT_PROV_ENUM
} OPTION_CHOICE;

const OPTIONS genrsa_options[] = {
    {OPT_HELP_STR, 1, '-', "Usage: %s [options] numbits\n"},

    OPT_SECTION("General"),
    {"help", OPT_HELP, '-', "Display this summary"},
#ifndef OPENSSL_NO_ENGINE
    {"engine", OPT_ENGINE, 's', "Use engine, possibly a hardware device"},
#endif

    OPT_SECTION("Input"),
    {"3", OPT_3, '-', "Use 3 for the E value"},
    {"F4", OPT_F4, '-', "Use F4 (0x10001) for the E value"},
    {"f4", OPT_F4, '-', "Use F4 (0x10001) for the E value"},

    OPT_SECTION("Output"),
    {"out", OPT_OUT, '>', "Output the key to specified file"},
    {"passout", OPT_PASSOUT, 's', "Output file pass phrase source"},
    {"primes", OPT_PRIMES, 'p', "Specify number of primes"},
    {"verbose", OPT_VERBOSE, '-', "Verbose output"},
    {"", OPT_CIPHER, '-', "Encrypt the output with any supported cipher"},

    OPT_R_OPTIONS,
    OPT_PROV_OPTIONS,

    OPT_PARAMETERS(),
    {"numbits", 0, 0, "Size of key in bits"},
    {NULL}
};

int genrsa_main(int argc, char **argv)
{
    BN_GENCB *cb = BN_GENCB_new();
    ENGINE *eng = NULL;
    BIGNUM *bn = BN_new();
    RSA *rsa;
    BIO *out = NULL;
    const BIGNUM *e;
    EVP_PKEY *pkey = NULL;
    EVP_PKEY_CTX *ctx = NULL;
    const EVP_CIPHER *enc = NULL;
    int ret = 1, num = DEFBITS, private = 0, primes = DEFPRIMES;
    unsigned long f4 = RSA_F4;
    char *outfile = NULL, *passoutarg = NULL, *passout = NULL;
    char *prog, *hexe, *dece;
    OPTION_CHOICE o;
    unsigned char *ebuf = NULL;

    if (bn == NULL || cb == NULL)
        goto end;

    prog = opt_init(argc, argv, genrsa_options);
    while ((o = opt_next()) != OPT_EOF) {
        switch (o) {
        case OPT_EOF:
        case OPT_ERR:
opthelp:
            BIO_printf(bio_err, "%s: Use -help for summary.\n", prog);
            goto end;
        case OPT_HELP:
            ret = 0;
            opt_help(genrsa_options);
            goto end;
        case OPT_3:
            f4 = RSA_3;
            break;
        case OPT_F4:
            f4 = RSA_F4;
            break;
        case OPT_OUT:
            outfile = opt_arg();
            break;
        case OPT_ENGINE:
            eng = setup_engine(opt_arg(), 0);
            break;
        case OPT_R_CASES:
            if (!opt_rand(o))
                goto end;
            break;
        case OPT_PROV_CASES:
            if (!opt_provider(o))
                goto end;
            break;
        case OPT_PASSOUT:
            passoutarg = opt_arg();
            break;
        case OPT_CIPHER:
            if (!opt_cipher(opt_unknown(), &enc))
                goto end;
            break;
        case OPT_PRIMES:
            if (!opt_int(opt_arg(), &primes))
                goto end;
            break;
        case OPT_VERBOSE:
            verbose = 1;
            break;
        }
    }
    argc = opt_num_rest();
    argv = opt_rest();

    if (argc == 1) {
        if (!opt_int(argv[0], &num) || num <= 0)
            goto end;
        if (num > OPENSSL_RSA_MAX_MODULUS_BITS)
            BIO_printf(bio_err,
                       "Warning: It is not recommended to use more than %d bit for RSA keys.\n"
                       "         Your key size is %d! Larger key size may behave not as expected.\n",
                       OPENSSL_RSA_MAX_MODULUS_BITS, num);
    } else if (argc > 0) {
        BIO_printf(bio_err, "Extra arguments given.\n");
        goto opthelp;
    }

    private = 1;
    if (!app_passwd(NULL, passoutarg, NULL, &passout)) {
        BIO_printf(bio_err, "Error getting password\n");
        goto end;
    }

    out = bio_open_owner(outfile, FORMAT_PEM, private);
    if (out == NULL)
        goto end;

    if (!init_gen_str(&ctx, "RSA", eng, 0))
        goto end;

    EVP_PKEY_CTX_set_cb(ctx, genrsa_cb);
    EVP_PKEY_CTX_set_app_data(ctx, bio_err);

    if (EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, num) <= 0) {
        BIO_printf(bio_err, "Error setting RSA length\n");
        goto end;
    }
    if (!BN_set_word(bn, f4)) {
        BIO_printf(bio_err, "Error allocating RSA public exponent\n");
        goto end;
    }
    if (EVP_PKEY_CTX_set_rsa_keygen_pubexp(ctx, bn) <= 0) {
        BIO_printf(bio_err, "Error setting RSA public exponent\n");
        goto end;
    }
    if (EVP_PKEY_CTX_set_rsa_keygen_primes(ctx, primes) <= 0) {
        BIO_printf(bio_err, "Error setting number of primes\n");
        goto end;
    }
    if (verbose)
        BIO_printf(bio_err, "Generating RSA private key, %d bit long modulus (%d primes)\n",
                   num, primes);
    if (!EVP_PKEY_keygen(ctx, &pkey)) {
        BIO_printf(bio_err, "Error generating RSA key\n");
        goto end;
    }

    if (verbose) {
        if ((rsa = EVP_PKEY_get0_RSA(pkey)) != NULL) {
            RSA_get0_key(rsa, NULL, &e, NULL);
        } else {
            BIO_printf(bio_err, "Error cannot access RSA e\n");
            goto end;
        }
        hexe = BN_bn2hex(e);
        dece = BN_bn2dec(e);
        if (hexe && dece) {
            BIO_printf(bio_err, "e is %s (0x%s)\n", dece, hexe);
        }
        OPENSSL_free(hexe);
        OPENSSL_free(dece);
    }
    if (!PEM_write_bio_PrivateKey(out, pkey, enc, NULL, 0, NULL, passout))
        goto end;

    ret = 0;
 end:
    BN_free(bn);
    BN_GENCB_free(cb);
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    BIO_free_all(out);
    release_engine(eng);
    OPENSSL_free(passout);
    OPENSSL_free(ebuf);
    if (ret != 0)
        ERR_print_errors(bio_err);
    return ret;
}

static int genrsa_cb(EVP_PKEY_CTX *ctx)
{
    char c = '*';
    BIO *b = EVP_PKEY_CTX_get_app_data(ctx);
    int p = EVP_PKEY_CTX_get_keygen_info(ctx, 0);

    if (!verbose)
        return 1;

    if (p == 0)
        c = '.';
    if (p == 1)
        c = '+';
    if (p == 2)
        c = '*';
    if (p == 3)
        c = '\n';
    BIO_write(b, &c, 1);
    (void)BIO_flush(b);
    return 1;
}