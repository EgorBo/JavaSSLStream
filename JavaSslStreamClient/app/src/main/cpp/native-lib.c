#include <jni.h>
#include "pal_jni.h"
#include "pal_sslstream.h"

JNIEXPORT jstring JNICALL
Java_com_example_javasslstreamclient_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject thisObj) {

    SSLStream* sslStream = AndroidCrypto_CreateSSLStreamAndStartHandshake(12, 2048, 2048);

    return NULL;
}