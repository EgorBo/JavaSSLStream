package com.example.javasslstreamclient;

import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLEngine;
import javax.net.ssl.SSLEngineResult;
import javax.net.ssl.SSLException;
import javax.net.ssl.SSLSession;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.nio.channels.Channels;
import java.nio.channels.ReadableByteChannel;
import java.nio.channels.WritableByteChannel;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.X509Certificate;

public final class SSLStream {
    private SSLEngine sslEngine;
    private ByteBuffer appOutBuffer;
    private ByteBuffer netOutBuffer;
    private ByteBuffer netInBuffer;
    private ByteBuffer appInBuffer;
    private OutputStream outputStream;
    private InputStream inputStream;

    public SSLStream(OutputStream outputStream, InputStream inputStream)
    {
        this.outputStream = outputStream;
        this.inputStream = inputStream;
    }

    public void AuthenticateAsClient(int appOutBufferSize, int appInBufferSize)
            throws IOException, NoSuchAlgorithmException, KeyManagementException {


        TrustManager trustAllCerts = new X509TrustManager()
        {
            public java.security.cert.X509Certificate[] getAcceptedIssuers()
            {
                return new X509Certificate[0];
            }
            public void checkClientTrusted(X509Certificate[] certs, String authType)
            {
            }
            public void checkServerTrusted(X509Certificate[] certs, String authType)
            {
            }
        };

        SSLContext sslContext = SSLContext.getInstance("TLSv1.2");
        sslContext.init(null, new TrustManager[]{trustAllCerts}, null);

        this.sslEngine = sslContext.createSSLEngine();
        this.sslEngine.setUseClientMode(true);
        SSLSession sslSession = sslEngine.getSession();
        final int applicationBufferSize = sslSession.getApplicationBufferSize();
        final int packetBufferSize = sslSession.getPacketBufferSize();
        this.appOutBuffer = ByteBuffer.allocate(appOutBufferSize);
        this.netOutBuffer = ByteBuffer.allocate(packetBufferSize);
        this.netInBuffer = ByteBuffer.allocate(packetBufferSize);
        this.appInBuffer = ByteBuffer.allocate(Math.max(applicationBufferSize, appInBufferSize));
        sslEngine.beginHandshake();
        checkHandshakeStatus();
    }

    public void send(byte[] message) throws IOException {
        appOutBuffer.put(message);
        doWrap();
    }

    public byte[] receive() throws IOException {
        while (true) {
            appInBuffer.flip();
            try {
                if (appInBuffer.remaining() > 0) {
                    byte[] data = new byte[appInBuffer.remaining()];
                    appInBuffer.get(data);
                    return data;
                }
            } finally {
                appInBuffer.compact();
            }
            doUnwrap();
        }
    }

    private void checkHandshakeStatus() throws IOException {
        checkHandshakeStatus(sslEngine.getHandshakeStatus());
    }

    private void checkHandshakeStatus(SSLEngineResult.HandshakeStatus handshakeStatus) throws IOException {
        switch (handshakeStatus) {
            case NOT_HANDSHAKING:
                return;
            case FINISHED:
                return;
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
    }

    private void doWrap() throws IOException {
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
    }

    private void flush() throws IOException {
        netOutBuffer.flip();
        byte[] data = new byte[netOutBuffer.limit()];
        netOutBuffer.get(data);
        outputStream.write(data, 0, data.length);
        netOutBuffer.compact();
    }

    private void doUnwrap() throws IOException {
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
    }

    private void handleEndOfStream() throws IOException {
        try {
            sslEngine.closeInbound();
            close();
        } catch (SSLException e) {
        }
    }

    public void close() throws IOException {
        sslEngine.closeOutbound();
        checkHandshakeStatus();
    }

    private ByteBuffer ensureRemaining(ByteBuffer oldBuffer, int newRemaining) {
        if (oldBuffer.remaining() < newRemaining) {
            oldBuffer.flip();
            final ByteBuffer newBuffer = ByteBuffer.allocate(oldBuffer.remaining() + newRemaining);
            newBuffer.put(oldBuffer);
            return newBuffer;
        } else {
            return oldBuffer;
        }
    }


}