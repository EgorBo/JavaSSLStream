#include "pal_sslstream.h"


int getHandshakeStatus(JNIEnv* env, SSLStream* sslStream)
{
    jobject obj = (*env)->CallObjectMethod(env, sslStream->sslEngine, g_SSLEngineGetHandshakeStatusMethod);
    return 0;
}

void checkHandshakeStatus(JNIEnv* env, SSLStream* sslStream, int status)
{
}

void doWrap(JNIEnv* env, SSLStream* sslStream)
{
}

void doUnwrap(JNIEnv* env, SSLStream* sslStream)
{
}

void flush(JNIEnv* env, SSLStream* sslStream)
{
}

jobject ensureRemaining(JNIEnv* env, SSLStream* sslStream, jobject oldBuffer, int newRemaining)
{
    return NULL;
}

SSLStream* AndroidCrypto_CreateSSLStreamAndStartHandshake(int tlsVersion, int appOutBufferSize, int appInBufferSize)
{
    JNIEnv* env = GetJNIEnv();
    /*
        SSLContext sslContext = SSLContext.getInstance("TLSv1.2");
        sslContext.init(null, new TrustManage r[]{trustAllCerts}, null);
        this.sslEngine = sslContext.createSSLEngine();
        this.sslEngine.setUseClientMode(true);
        SSLSession sslSession = sslEngine.getSession();
        final int applicationBufferSize = sslSession.getApplicationBufferSize();
        final int packetBufferSize = sslSession.getPacketBufferSize();
        this.appOutBuffer = ByteBuffer.allocate(appOutBufferSize);
        this.netOutBuffer = ByteBuffer.allocate(packetBufferSize);
        this.netInBuffer =  ByteBuffer.allocate(packetBufferSize);
        this.appInBuffer =  ByteBuffer.allocate(Math.max(applicationBufferSize, appInBufferSize));
        sslEngine.beginHandshake();
    */

    SSLStream* sslStream = malloc(sizeof(SSLStream));

    jobject tlsVerStr = NULL;
    if (tlsVersion == 11)
        tlsVerStr = JSTRING("TLSv1.1");
    else if (tlsVersion == 12)
        tlsVerStr = JSTRING("TLSv1.2");
    else if (tlsVersion == 13)
        tlsVerStr = JSTRING("TLSv1.3");
    else
        assert(0 && "unknown tlsVersion");

    sslStream->sslContext = ToGRef(env, (*env)->CallStaticObjectMethod(env, g_SSLContext, g_SSLContextGetInstanceMethod, tlsVerStr));

    // TODO: set TrustManager[] argument.
    (*env)->CallVoidMethod(env, sslStream->sslContext, g_SSLContextInitMethod, NULL, NULL, NULL);
    sslStream->sslEngine  = ToGRef(env, (*env)->CallObjectMethod(env, sslStream->sslContext, g_SSLContextCreateSSLEngineMethod));
    sslStream->sslSession = ToGRef(env, (*env)->CallObjectMethod(env, sslStream->sslEngine, g_SSLEngineGetSessionMethod));

    int applicationBufferSize = (*env)->CallIntMethod(env, sslStream->sslSession, g_SSLSessionGetApplicationBufferSizeMethod);
    int packetBufferSize = (*env)->CallIntMethod(env, sslStream->sslSession, g_SSLSessionGetPacketBufferSizeMethod);

    (*env)->CallVoidMethod(env, sslStream->sslEngine, g_SSLEngineSetUseClientModeMethod, true);

    sslStream->appOutBuffer = ToGRef(env, (*env)->CallStaticObjectMethod(env, g_ByteBuffer, g_ByteBufferAllocateMethod, appOutBufferSize));
    sslStream->netOutBuffer = ToGRef(env, (*env)->CallStaticObjectMethod(env, g_ByteBuffer, g_ByteBufferAllocateMethod, packetBufferSize));
    sslStream->appInBuffer = ToGRef(env, (*env)->CallStaticObjectMethod(env, g_ByteBuffer, g_ByteBufferAllocateMethod, applicationBufferSize > appInBufferSize ? applicationBufferSize : appInBufferSize));
    sslStream->netInBuffer = ToGRef(env, (*env)->CallStaticObjectMethod(env, g_ByteBuffer, g_ByteBufferAllocateMethod, packetBufferSize));

    (*env)->CallVoidMethod(env, sslStream->sslEngine, g_SSLEngineBeginHandshakeMethod);

    checkHandshakeStatus(env, sslStream, getHandshakeStatus(env, sslStream));
    return sslStream;
}


int AndroidCrypto_SSLStreamRead(SSLStream* sslStream, uint8_t* buffer, int offset, int length)
{
    JNIEnv* env = GetJNIEnv();
    return -1;
}

void AndroidCrypto_SSLStreamWrite(SSLStream* sslStream, uint8_t* buffer, int offset, int length)
{
    JNIEnv* env = GetJNIEnv();
    return;
}

void AndroidCrypto_Dispose(SSLStream* sslStream)
{
    JNIEnv* env = GetJNIEnv();
    ReleaseGRef(env, sslStream->sslContext);
    ReleaseGRef(env, sslStream->sslEngine);
    ReleaseGRef(env, sslStream->sslSession);
    ReleaseGRef(env, sslStream->appOutBuffer);
    ReleaseGRef(env, sslStream->netOutBuffer);
    ReleaseGRef(env, sslStream->netInBuffer);
    ReleaseGRef(env, sslStream->appInBuffer);
    free(sslStream);
}
