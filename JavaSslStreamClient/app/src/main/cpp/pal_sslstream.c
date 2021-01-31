#include "pal_sslstream.h"

void checkHandshakeStatus(JNIEnv* env, SSLStream* sslStream, int handshakeStatus);

int getHandshakeStatus(JNIEnv* env, SSLStream* sslStream, jobject engineResult)
{
    if (engineResult)
        return GetEnumAsInt(env, (*env)->CallObjectMethod(env, engineResult, g_SSLEngineResultGetHandshakeStatusMethod), true);
    else
        return GetEnumAsInt(env, (*env)->CallObjectMethod(env, sslStream->sslEngine, g_SSLEngineGetHandshakeStatusMethod), true);
}

void close(JNIEnv* env, SSLStream* sslStream) {
    /*
        sslEngine.closeOutbound();
        checkHandshakeStatus();
    */
    (*env)->CallVoidMethod(env, sslStream->sslEngine, g_SSLEngineCloseOutboundMethod);
    checkHandshakeStatus(env, sslStream, getHandshakeStatus(env, sslStream, NULL));
}

void handleEndOfStream(JNIEnv* env, SSLStream* sslStream)  {
    /*
        sslEngine.closeInbound();
        close();
    */
    (*env)->CallVoidMethod(env, sslStream->sslEngine, g_SSLEngineCloseInboundMethod);
    close(env, sslStream);
}

void flush(JNIEnv* env, SSLStream* sslStream)
{
    /*
        netOutBuffer.flip();
        byte[] data = new byte[netOutBuffer.limit()];
        netOutBuffer.get(data);
        outputStream.write(data, 0, data.length); // in JNI we write to "streamWriter" function pointer.
        netOutBuffer.compact();
    */

    // DeleteLocalRef because we don't need the return value (Buffer)
    (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->netOutBuffer, g_ByteBufferFlipMethod));
    int bufferLimit = (*env)->CallIntMethod(env, sslStream->netOutBuffer, g_ByteBufferLimitMethod);
    jbyteArray data = (*env)->NewByteArray(env, bufferLimit);

    // DeleteLocalRef because we don't need the return value (Buffer)
    (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->netOutBuffer, g_ByteBufferGetMethod, data));

    uint8_t* dataPtr = malloc(bufferLimit);
    (*env)->GetByteArrayRegion(env, data, 0, bufferLimit, (jbyte*) dataPtr);
    sslStream->streamWriter(dataPtr, 0, bufferLimit);
    free(dataPtr);
    // DeleteLocalRef because we don't need the return value (Buffer)
    (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->netOutBuffer, g_ByteBufferCompactMethod));
}

jobject ensureRemaining(JNIEnv* env, SSLStream* sslStream, jobject oldBuffer, int newRemaining)
{
    /*
        if (oldBuffer.remaining() < newRemaining) {
            oldBuffer.flip();
            final ByteBuffer newBuffer = ByteBuffer.allocate(oldBuffer.remaining() + newRemaining);
            newBuffer.put(oldBuffer);
            return newBuffer;
        } else {
            return oldBuffer;
        }
    */

    int oldRemaining = (*env)->CallIntMethod(env, oldBuffer, g_ByteBufferRemainingMethod);
    if (oldRemaining < newRemaining)
    {
        // DeleteLocalRef because we don't need the return value (Buffer)
        (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->netOutBuffer, g_ByteBufferFlipMethod));
        jobject newBuffer = ToGRef(env, (*env)->CallStaticObjectMethod(env, g_ByteBuffer, g_ByteBufferAllocateMethod, oldRemaining + newRemaining));
        // DeleteLocalRef because we don't need the return value (Buffer)
        (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, newBuffer, g_ByteBufferPutBufferMethod, oldBuffer));
        ReleaseGRef(env, oldBuffer);
        return newBuffer;
    }
    else
    {
        return oldBuffer;
    }
}

void doWrap(JNIEnv* env, SSLStream* sslStream)
{
    /*
        appOutBuffer.flip();
        final SSLEngineResult result;
        try {
            result = sslEngine.wrap(appOutBuffer, netOutBuffer);
        } catch (SSLException e) {
            return;
        }
        appOutBuffer.compact();

        final SSLEngineResult.Status status = result.getStatus();
        switch (status) {
            case OK:
                flush();
                checkHandshakeStatus(result.getHandshakeStatus());
                if (appOutBuffer.position() > 0) doWrap();
                break;
            case CLOSED:
                flush();
                checkHandshakeStatus(result.getHandshakeStatus());
                close();
                break;
            case BUFFER_OVERFLOW:
                netOutBuffer = ensureRemaining(netOutBuffer, sslEngine.getSession().getPacketBufferSize());
                doWrap();
                break;
        }
    */

    (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->appOutBuffer, g_ByteBufferFlipMethod));
    jobject sslEngineResult = (*env)->CallObjectMethod(env, sslStream->sslEngine, g_SSLEngineWrapMethod, sslStream->appOutBuffer, sslStream->netOutBuffer);
    (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->appOutBuffer, g_ByteBufferCompactMethod));

    int status = GetEnumAsInt(env, (*env)->CallObjectMethod(env, sslEngineResult, g_SSLEngineResultGetStatusMethod), true);
    switch (status)
    {
        case STATUS__OK:
            flush(env, sslStream);
            checkHandshakeStatus(env, sslStream, getHandshakeStatus(env, sslStream, sslEngineResult));
            if ((*env)->CallIntMethod(env, sslStream->appOutBuffer, g_ByteBufferPositionMethod) > 0)
                doWrap(env, sslStream);
            break;
        case STATUS__CLOSED:
            flush(env, sslStream);
            checkHandshakeStatus(env, sslStream, getHandshakeStatus(env, sslStream, sslEngineResult));
            close(env, sslStream);
            break;
        case STATUS__BUFFER_OVERFLOW:
            sslStream->netOutBuffer = ensureRemaining(env, sslStream, sslStream->netOutBuffer, (*env)->CallIntMethod(env, sslStream->sslSession, g_SSLSessionGetPacketBufferSizeMethod));
            doWrap(env, sslStream);
            break;
    }
}

void doUnwrap(JNIEnv* env, SSLStream* sslStream)
{
    /*
        if (netInBuffer.position() == 0)
        {
            byte[] tmp = new byte[netInBuffer.limit()];
            int count = inputStream.read(tmp);
            if (count == -1) {
                handleEndOfStream();
                return;
            }
            netInBuffer.put(tmp, 0, count);
        }

        netInBuffer.flip();
        final SSLEngineResult result;
        try {
            result = sslEngine.unwrap(netInBuffer, appInBuffer);
        } catch (SSLException e) {
            return;
        }
        netInBuffer.compact();
        final SSLEngineResult.Status status = result.getStatus();
        switch (status) {
            case OK:
                checkHandshakeStatus(result.getHandshakeStatus());
                break;
            case CLOSED:
                checkHandshakeStatus(result.getHandshakeStatus());
                close();
                break;
            case BUFFER_UNDERFLOW:
                netInBuffer = ensureRemaining(netInBuffer, sslEngine.getSession().getPacketBufferSize());
                doUnwrap();
                break;
            case BUFFER_OVERFLOW:
                appInBuffer = ensureRemaining(appInBuffer, sslEngine.getSession().getApplicationBufferSize());
                doUnwrap();
                break;
        }
    */

    if ((*env)->CallIntMethod(env, sslStream->netInBuffer, g_ByteBufferPositionMethod) == 0)
    {
        int netInBufferLimit = (*env)->CallIntMethod(env, sslStream->netInBuffer, g_ByteBufferLimitMethod);
        jbyteArray tmp = (*env)->NewByteArray(env, netInBufferLimit);
        uint8_t* tmpNative = malloc(netInBufferLimit);
        int count = sslStream->streamReader(tmpNative, 0, netInBufferLimit);
        if (count == -1)
        {
            handleEndOfStream(env, sslStream);
            return;
        }
        (*env)->SetByteArrayRegion(env, tmp, 0, count, (jbyte*)(tmpNative));
        (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->appOutBuffer, g_ByteBufferPut3Method, tmp, 0, count));
        free(tmpNative);
        (*env)->DeleteLocalRef(env, tmp);
    }

    (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->netInBuffer, g_ByteBufferFlipMethod));
    jobject sslEngineResult = (*env)->CallObjectMethod(env, sslStream->sslEngine, g_SSLEngineUnwrapMethod, sslStream->netInBuffer, sslStream->appInBuffer);
    (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->netInBuffer, g_ByteBufferCompactMethod));

    int status = GetEnumAsInt(env, (*env)->CallObjectMethod(env, sslEngineResult, g_SSLEngineResultGetStatusMethod), true);
    switch (status)
    {
        case STATUS__OK:
            checkHandshakeStatus(env, sslStream, getHandshakeStatus(env, sslStream, sslEngineResult));
            break;
        case STATUS__CLOSED:
            checkHandshakeStatus(env, sslStream, getHandshakeStatus(env, sslStream, sslEngineResult));
            close(env, sslStream);
            break;
        case STATUS__BUFFER_UNDERFLOW:
            sslStream->netInBuffer = ensureRemaining(env, sslStream, sslStream->netInBuffer, (*env)->CallIntMethod(env, sslStream->sslSession, g_SSLSessionGetPacketBufferSizeMethod));
            doUnwrap(env, sslStream);
            break;
        case STATUS__BUFFER_OVERFLOW:
            sslStream->appInBuffer = ensureRemaining(env, sslStream, sslStream->appInBuffer, (*env)->CallIntMethod(env, sslStream->sslSession, g_SSLSessionGetApplicationBufferSizeMethod));
            doUnwrap(env, sslStream);
            break;
    }
}

void checkHandshakeStatus(JNIEnv* env, SSLStream* sslStream, int handshakeStatus)
{
    /*
        switch (handshakeStatus) {
            case NEED_WRAP:
                doWrap();
                break;
            case NEED_UNWRAP:
                doUnwrap();
                break;
            case NEED_TASK:
                Runnable task;
                while ((task = sslEngine.getDelegatedTask()) != null) task.run();
                checkHandshakeStatus();
                break;
        }
    */

    switch (handshakeStatus)
    {
        case HANDSHAKE_STATUS__NEED_WRAP:
            doWrap(env, sslStream);
            break;
        case HANDSHAKE_STATUS__NEED_UNWRAP:
            doUnwrap(env, sslStream);
            break;
        case HANDSHAKE_STATUS__NEED_TASK:
            assert(0 && "unexpected NEED_TASK handshake status");
    }
}

SSLStream* AndroidCrypto_CreateSSLStreamAndStartHandshake(STREAM_READER streamReader, STREAM_WRITER streamWriter, int tlsVersion, int appOutBufferSize, int appInBufferSize)
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

    jobject trustAllCertsObj = (*env)->NewObject(env, g_TrustAllCerts, g_TrustAllCertsCtor);
    jobject certManagersArray = (*env)->NewObjectArray(env, 1, g_TrustManager, trustAllCertsObj);

    // TODO: set TrustManager[] argument.
    (*env)->CallVoidMethod(env, sslStream->sslContext, g_SSLContextInitMethod, NULL, certManagersArray, NULL);
    sslStream->sslEngine  = ToGRef(env, (*env)->CallObjectMethod(env, sslStream->sslContext, g_SSLContextCreateSSLEngineMethod));
    sslStream->sslSession = ToGRef(env, (*env)->CallObjectMethod(env, sslStream->sslEngine, g_SSLEngineGetSessionMethod));

    int applicationBufferSize = (*env)->CallIntMethod(env, sslStream->sslSession, g_SSLSessionGetApplicationBufferSizeMethod);
    int packetBufferSize = (*env)->CallIntMethod(env, sslStream->sslSession, g_SSLSessionGetPacketBufferSizeMethod);

    (*env)->CallVoidMethod(env, sslStream->sslEngine, g_SSLEngineSetUseClientModeMethod, true);

    sslStream->appOutBuffer = ToGRef(env, (*env)->CallStaticObjectMethod(env, g_ByteBuffer, g_ByteBufferAllocateMethod, appOutBufferSize));
    sslStream->netOutBuffer = ToGRef(env, (*env)->CallStaticObjectMethod(env, g_ByteBuffer, g_ByteBufferAllocateMethod, packetBufferSize));
    sslStream->appInBuffer = ToGRef(env, (*env)->CallStaticObjectMethod(env, g_ByteBuffer, g_ByteBufferAllocateMethod,
            applicationBufferSize > appInBufferSize ? applicationBufferSize : appInBufferSize));
    sslStream->netInBuffer = ToGRef(env, (*env)->CallStaticObjectMethod(env, g_ByteBuffer, g_ByteBufferAllocateMethod, packetBufferSize));
    sslStream->streamReader = streamReader;
    sslStream->streamWriter = streamWriter;

    (*env)->CallVoidMethod(env, sslStream->sslEngine, g_SSLEngineBeginHandshakeMethod);

    checkHandshakeStatus(env, sslStream, getHandshakeStatus(env, sslStream, NULL));

    AssertOnJNIExceptions(env);

    (*env)->DeleteLocalRef(env, tlsVerStr);
    return sslStream;
}

int AndroidCrypto_SSLStreamRead(SSLStream* sslStream, uint8_t* buffer, int offset, int length)
{
    JNIEnv* env = GetJNIEnv();

    // appInBuffer.flip();
    // int rem = appInBuffer.remaining();
    // if (rem> 0) {
    //     byte[] data = new byte[rem];
    //     appInBuffer.get(data);
    //     appInBuffer.compact();
    // } else {
    //     appInBuffer.compact();
    //     doUnwrap();
    // }

    (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->netOutBuffer, g_ByteBufferFlipMethod));
    int rem = (*env)->CallIntMethod(env, sslStream->appInBuffer, g_ByteBufferRemainingMethod);
    if (rem > 0)
    {
        jbyteArray data = (*env)->NewByteArray(env, rem);
        (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->appOutBuffer, g_ByteBufferGetMethod, data));
        (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->netOutBuffer, g_ByteBufferCompactMethod));

        //TODO: (*env)->GetByteArrayRegion(env, data, 0, bufferLimit, (jbyte*) dataPtr);
        return rem;
    }
    else
    {
        (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->netOutBuffer, g_ByteBufferCompactMethod));
        doUnwrap(env, sslStream);
        return AndroidCrypto_SSLStreamRead(sslStream, buffer, offset, length);
    }
}

void AndroidCrypto_SSLStreamWrite(SSLStream* sslStream, uint8_t* buffer, int offset, int length)
{
    /*
        appOutBuffer.put(message);
        doWrap();
    */
    JNIEnv* env = GetJNIEnv();
    jbyteArray data = (*env)->NewByteArray(env, length);
    (*env)->SetByteArrayRegion(env, data, 0, length, (jbyte*)(buffer + offset));
    (*env)->DeleteLocalRef(env, (*env)->CallObjectMethod(env, sslStream->appOutBuffer, g_ByteBufferPut2Method, data));
    (*env)->DeleteLocalRef(env, data);
    doWrap(env, sslStream);
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
