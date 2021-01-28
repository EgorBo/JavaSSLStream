
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

SSLStream* AndroidCrypto_CreateSSLStreamAndStartHandshake(int tlsVersion, int appOutBufferSize, int appInBufferSize);

int AndroidCrypto_SSLStreamRead(SSLStream* sslStream, uint8_t* buffer, int offset, int length);

void AndroidCrypto_SSLStreamWrite(SSLStream* sslStream, uint8_t* buffer, int offset, int length);

void AndroidCrypto_Dispose(SSLStream* sslStream);