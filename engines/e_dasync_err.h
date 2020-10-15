/*
 * Generated by util/mkerr.pl DO NOT EDIT
 * Copyright 1995-2019 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#ifndef OSSL_ENGINES_E_DASYNC_ERR_H
# define OSSL_ENGINES_E_DASYNC_ERR_H

# include <openssl/opensslconf.h>
# include <openssl/symhacks.h>


# define DASYNCerr(f, r) ERR_DASYNC_error(0, (r), OPENSSL_FILE, OPENSSL_LINE)


/*
 * DASYNC function codes.
 */
# ifndef OPENSSL_NO_DEPRECATED_3_0
#  define DASYNC_F_BIND_DASYNC                             0
#  define DASYNC_F_CIPHER_AES_128_CBC_CODE                 0
#  define DASYNC_F_DASYNC_AES128_CBC_HMAC_SHA1_INIT_KEY    0
#  define DASYNC_F_DASYNC_AES128_INIT_KEY                  0
#  define DASYNC_F_DASYNC_BN_MOD_EXP                       0
#  define DASYNC_F_DASYNC_CIPHER_INIT_KEY_HELPER           0
#  define DASYNC_F_DASYNC_MOD_EXP                          0
#  define DASYNC_F_DASYNC_PRIVATE_DECRYPT                  0
#  define DASYNC_F_DASYNC_PRIVATE_ENCRYPT                  0
#  define DASYNC_F_DASYNC_PUBLIC_DECRYPT                   0
#  define DASYNC_F_DASYNC_PUBLIC_ENCRYPT                   0
# endif

/*
 * DASYNC reason codes.
 */
# define DASYNC_R_INIT_FAILED                             100

#endif
