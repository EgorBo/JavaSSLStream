package com.sslstream.javaclient;

import android.util.Log;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.ByteBuffer;
import java.security.KeyManagementException;
import java.security.NoSuchAlgorithmException;
import java.security.cert.X509Certificate;
import javax.net.ssl.SSLContext;
import javax.net.ssl.SSLEngine;
import javax.net.ssl.SSLEngineResult;
import javax.net.ssl.SSLSession;
import javax.net.ssl.TrustManager;
import javax.net.ssl.X509TrustManager;

public class SSLStream {

    private SSLEngine _sslEngine;
    private ByteBuffer _sendBufferSrc = null;
    private ByteBuffer _sendBufferDst = null;
    private ByteBuffer _recvBufferSrc = null;
    private ByteBuffer _recvBufferDst = null;
    private OutputStream _outputStream;
    private InputStream _inputStream;

    public SSLStream(OutputStream outputStream, InputStream inputStream) {
        _outputStream = outputStream;
        _inputStream = inputStream;
    }

    public void AuthenticateAsClient(String targetHost) throws NoSuchAlgorithmException, KeyManagementException, IOException {
        SSLContext sslContext = null;

        // EGOR: my server cert has to pass client validations so trust all of them
        TrustManager trustAllCerts = new X509TrustManager() {
            public java.security.cert.X509Certificate[] getAcceptedIssuers() {
                return new X509Certificate[0];
            }

            public void checkClientTrusted(X509Certificate[] certs, String authType) {
            }

            public void checkServerTrusted(X509Certificate[] certs, String authType) {
            }
        };

        sslContext = SSLContext.getInstance("TLSv1.2");
        sslContext.init(null, new TrustManager[]{trustAllCerts}, null);

        _sslEngine = sslContext.createSSLEngine();
        _sslEngine.setUseClientMode(true);

        SSLSession session = _sslEngine.getSession();

        _sendBufferSrc = ByteBuffer.allocate(session.getPacketBufferSize());
        _recvBufferSrc = ByteBuffer.allocate(session.getPacketBufferSize());
        _sendBufferDst = ByteBuffer.allocate(session.getPacketBufferSize());
        _recvBufferDst = ByteBuffer.allocate(session.getPacketBufferSize());

        // TLS1.2 handshake: (doesn't include TCP handshake)
        //
        // Send     ClientHello  153  bytes
        // Receive  ServerHello  5    bytes
        // Receive  Certificate  1248 bytes
        // Send     ...          158  bytes
        // Receive  ...          5    bytes
        // Receive  ...          1    bytes
        // Receive  ...          5    bytes
        // Receive  ...          40   bytes
        _sslEngine.beginHandshake();
        SSLEngineResult wrapResult = _sslEngine.wrap(_sendBufferSrc, _sendBufferDst);

        writeBuffer(_sendBufferDst, _outputStream);

        byte[] data = new byte[2048];
        int read = _inputStream.read(data);
        // ok so now "data" contains "serverHello" + "certificate"!
        SSLEngineResult unwrapResult = _sslEngine.unwrap(ByteBuffer.wrap(data, 0, read), _recvBufferDst);

        // at this point SSLEngine should process that certificate and validate it.

        // TODO: finish the hand-shake here (1 write, 1 read);

        // Now we should be able to send any data to/from the server
        Log.i("EGOR", "handshake finished");

    }

    public void writeBuffer(ByteBuffer buffer, OutputStream stream) {
        int pos = buffer.position();
        buffer.rewind();
        byte[] byteArray = new byte[pos];
        buffer.get(byteArray, 0, pos);
        try {
            stream.write(byteArray, 0, pos);
            stream.flush();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }

    public void Write(byte[] data, int offset, int count) {
    }

    public int Read(byte[] data) {
        return 0;
    }
}