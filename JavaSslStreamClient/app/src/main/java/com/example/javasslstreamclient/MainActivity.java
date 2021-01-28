package com.example.javasslstreamclient;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;
import android.os.Bundle;
import android.util.Log;
import androidx.appcompat.app.AppCompatActivity;

import java.io.InputStream;
import java.io.OutputStream;
import java.net.Socket;

import javax.net.ssl.SSLEngine;

public class MainActivity extends AppCompatActivity {

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        // Socket has to be opened in a background thread:
        new Thread(new Runnable() {
            @Override
            public void run() {
                try {
                    // NOTE: works only on a real device for me (connected to the same network/wi-fi as the C# server)
                    Socket socket = new Socket("192.168.0.5", 8080);

                    boolean javaImpl = true;

                    if (javaImpl) {
                        SSLStream sslStream = new SSLStream(socket.getOutputStream(), socket.getInputStream());
                        sslStream.AuthenticateAsClient(socket.getSendBufferSize(), socket.getReceiveBufferSize());

                        // now let's send & wait some app data
                        sslStream.send("EgorBo<EOF>".getBytes("UTF-8"));
                        byte[] serverResponseUtf8 = sslStream.receive();
                        final String serverMessage = new String(serverResponseUtf8, "UTF-8");

                        runOnUiThread(new Runnable() {
                            @Override
                            public void run() {
                                TextView tv = findViewById(R.id.sample_text);
                                tv.setText(serverMessage);
                            }
                        });
                        Log.i("EGOR", "server: " + serverMessage);
                    } else {
                        // Same, but via JNI (not finished yet)
                        Object object = SSLStreamNative(socket.getInputStream(), socket.getOutputStream());
                        Log.i("EGOR", "server: " + object);
                    }
                } catch (Exception exc) {
                    exc.printStackTrace();
                }
            }
        }).start();
    }

    public native Object SSLStreamNative(InputStream inputStream, OutputStream outputStream);
}