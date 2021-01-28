
#pragma once

#include "pal_jni.h"

typedef struct SSLStream
{
    jobject sslContext;
    jobject sslEngine;
    jobject sslSession;
    jobject appOutBuffer;
    jobject netOutBuffer;
    jobject appInBuffer;
    jobject netInBuffer;
} SSLStream;

#define TLS11 11
#define TLS12 12
#define TLS13 13

#define HANDSHAKE_STATUS__NOT_HANDSHAKING 0
#define HANDSHAKE_STATUS__FINISHED 1
#define HANDSHAKE_STATUS__NEED_TASK 2
#define HANDSHAKE_STATUS__NEED_WRAP 3
#define HANDSHAKE_STATUS__NEED_UNWRAP 4

SSLStream* AndroidCrypto_CreateSSLStreamAndStartHandshake(int tlsVersion, int appOutBufferSize, int appInBufferSize);

int AndroidCrypto_SSLStreamRead(SSLStream* sslStream, uint8_t* buffer, int offset, int length);

void AndroidCrypto_SSLStreamWrite(SSLStream* sslStream, uint8_t* buffer, int offset, int length);

void AndroidCrypto_Dispose(SSLStream* sslStream);