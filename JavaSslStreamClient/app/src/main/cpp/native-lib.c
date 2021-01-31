#include <jni.h>
#include "pal_jni.h"
#include "pal_sslstream.h"

jobject inputStreamGref;
jobject outputStreamGref;

/*
 * onInnerStreamWrite and onInnerStreamRead callbacks are supposed to be implemented in C#
 * these two are just examples, I implemented them using Socket.InputStream & Socket.OutputStream
 * */

void onInnerStreamWrite(uint8_t* data, uint32_t offset, uint32_t length)
{
    LOG_INFO("onInnerStreamWrite: offset=%d, length=%d", offset, length);
    JNIEnv* env = GetJNIEnv();
    AssertOnJNIExceptions(env);
    jbyteArray tmpArray = (*env)->NewByteArray(env, length);
    (*env)->SetByteArrayRegion(env, tmpArray, 0, length, (jbyte*)(data + offset));
    (*env)->CallVoidMethod(env, outputStreamGref, g_OutputStreamWriteMethod, tmpArray);
    (*env)->DeleteLocalRef(env, tmpArray);
}

int onInnerStreamRead(uint8_t* data, uint32_t offset, uint32_t length)
{
    LOG_INFO("onInnerStreamRead: offset=%d, length=%d", offset, length);
    JNIEnv* env = GetJNIEnv();
    AssertOnJNIExceptions(env);
    jbyteArray tmpArray = (*env)->NewByteArray(env, length);
    int read = (*env)->CallIntMethod(env, inputStreamGref, g_InputStreamReadMethod, tmpArray);
    LOG_INFO("onInnerStreamRead: read=%d", read);
    (*env)->GetByteArrayRegion(env, tmpArray, 0, read, (jbyte*) (data + offset));
    (*env)->DeleteLocalRef(env, tmpArray);
    return read;
}

JNIEXPORT jstring JNICALL
Java_com_example_javasslstreamclient_MainActivity_SSLStreamNative(
        JNIEnv* env,
        jobject thisObj,
        int sendBufferSize,
        int receiveBufferSize,
        jobject inputStream,
        jobject outputStream) {

    inputStreamGref = ToGRef(env, inputStream);
    outputStreamGref = ToGRef(env, outputStream);

    // 1. Init & handshake
    SSLStream* sslStream = AndroidCrypto_CreateSSLStreamAndStartHandshake(&onInnerStreamRead, &onInnerStreamWrite, 12, sendBufferSize, receiveBufferSize);

    // 2. Write some app data, our C# server expects <EOF> as an end mark.
    const char* clientMessage = "EGOR<EOF>";
    AndroidCrypto_SSLStreamWrite(sslStream, (uint8_t*)clientMessage, 0, 9);

    // 3. Read some app data from the C# server:
    char serverResponse[1024];
    int read = AndroidCrypto_SSLStreamRead(sslStream, (uint8_t*)serverResponse, 0, 1024);
    assert(read > 0 && read <= 1024);
    serverResponse[read] = '\0';

    // 4. Close
    AndroidCrypto_Dispose(sslStream);

    // 5. convert server response (UTF8) to JString
    return (*env)->NewStringUTF(env, serverResponse);
}