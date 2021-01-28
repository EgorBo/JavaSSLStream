#include <jni.h>
#include "pal_jni.h"
#include "pal_sslstream.h"

jobject inputStreamGref;
jobject outputStreamGref;

void onInnerStreamWrite(uint8_t* data, uint32_t offset, uint32_t length)
{
    // TODO: write to outputStreamGref
}

int onInnerStreamRead(uint8_t* data, uint32_t offset, uint32_t length)
{
    // TODO: read from inputStreamGref
    return -1;
}

JNIEXPORT jobject JNICALL
Java_com_example_javasslstreamclient_MainActivity_SSLStreamNative(
        JNIEnv* env,
        jobject thisObj,
        jobject inputStream,
        jobject outputStream) {

    inputStreamGref = ToGRef(env, inputStream);
    outputStreamGref = ToGRef(env, outputStream);

    SSLStream* sslStream = AndroidCrypto_CreateSSLStreamAndStartHandshake(&onInnerStreamRead, &onInnerStreamWrite, 12, 2048, 2048);
    return sslStream->sslEngine;
}