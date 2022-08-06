#ifndef ENCRYPTION_HELPER_H
#define ENCRYPTION_HELPER_H

#include <QSettings>
#include <QIODevice>



int gcm_encrypt(const char *plaintext, int plaintext_len,
                const char *key,
                const char *iv, int iv_len,
                char *ciphertext);

int gcm_decrypt(const char *ciphertext, int ciphertext_len,
                const char *key,
                const char *iv, int iv_len,
                char *plaintext);


int gcm_encrypt(const unsigned char *plaintext, int plaintext_len,
                const unsigned char *key,
                const unsigned char *iv, int iv_len,
                unsigned char *ciphertext);

int gcm_decrypt(const unsigned char *ciphertext, int ciphertext_len,
                const unsigned char *key,
                const unsigned char *iv, int iv_len,
                unsigned char *plaintext);


#endif // ENCRYPTION_HELPER_H
