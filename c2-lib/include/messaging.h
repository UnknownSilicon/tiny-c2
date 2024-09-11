#ifndef MESSAGING_H
#define MESSAGING_H 1

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "aes.h"
#include "util.h"


bool write_encrypted_padded(int sock, struct AES_ctx* ctx, void* data, size_t data_len);
#endif