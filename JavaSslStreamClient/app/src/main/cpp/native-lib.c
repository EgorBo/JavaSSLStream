#include <jni.h>
#include "pal_jni.h"
#include "pal_sslstream.h"

jobject inputStreamGref;
jobject outputStreamGref;

void onInnerStreamWrite(uint8_t* data, uint32_t offset, uint32_t length)
{
    JNIEnv* env = GetJNIEnv();
    jbyteArray tmpArray = (*env)->NewByteArray(env, length);
    (*env)->SetByteArrayRegion(env, tmpArray, 0, length, (jbyte*)(data + offset));
    (*env)->CallVoidMethod(env, outputStreamGref, g_OutputStreamWriteMethod, tmpArray);
    (*env)->DeleteLocalRef(env, tmpArray);
}

int onInnerStreamRead(uint8_t* data, uint32_t offset, uint32_t length)
{
    JNIEnv* env = GetJNIEnv();
    jbyteArray tmpArray = (*env)->NewByteArray(env, length);
    int read = (*env)->CallIntMethod(env, inputStreamGref, g_InputStreamReadMethod, tmpArray);
    (*env)->GetByteArrayRegion(env, tmpArray, 0, read, (jbyte*) (data + offset));
    (*env)->DeleteLocalRef(env, tmpArray);
    return read;
}

JNIEXPORT jobject JNICALL
Java_com_example_javasslstreamclient_MainActivity_SSLStreamNative(
        JNIEnv* env,
        jobject thisObj,
        jobject inputStream,
        jobject outputStream) {

    inputStreamGref = ToGRef(env, inputStream);
    outputStreamGref = ToGRef(env, outputStream);

    SSLStream* sslStream = AndroidCrypto_CreateSSLStreamAndStartHandshake(&onInnerStreamRead, &onInnerStreamWrite, 12, 1024 * 8, 1024 * 8);
    return sslStream->sslEngine;
}