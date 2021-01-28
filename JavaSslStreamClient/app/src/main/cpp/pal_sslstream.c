#include "pal_sslstream.h"

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

SSLStream* AndroidCrypto_CreateSSLStreamAndStartHandshake(int tlsVersion)
{
    JNIEnv* env = GetJNIEnv();
    /*
    SSLContext sslContext = SSLContext.getInstance("TLSv1.2");
    sslContext.init(null, new TrustManager[]{trustAllCerts}, null);
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
    sslStream->sslEngine  = ToGRef(env, (*env)->CallObjectMethod(env, sslStream->sslContext, g_SSLContextCreateSSLEngineMethod));

    checkHandshakeStatus(env, sslStream, -1);
    return NULL;
}


int AndroidCrypto_SSLStreamRead(SSLStream* sslStream, uint8_t* buffer, int offset, int length)
{
    JNIEnv* env = GetJNIEnv();
    return 0;
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
    ReleaseGRef(env, sslStream->appOutBuffer);
    ReleaseGRef(env, sslStream->netOutBuffer);
    ReleaseGRef(env, sslStream->netInBuffer);
    ReleaseGRef(env, sslStream->appInBuffer);
    free(sslStream);
}
