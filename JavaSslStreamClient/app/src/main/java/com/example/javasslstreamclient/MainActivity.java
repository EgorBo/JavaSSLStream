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
                    boolean javaImpl = false; // set to true to use JAVA impl instead of JNI

                    String serverResponse = "";
                    if (javaImpl) {
                        SSLStream sslStream = new SSLStream(socket.getOutputStream(), socket.getInputStream());
                        sslStream.AuthenticateAsClient(socket.getSendBufferSize(), socket.getReceiveBufferSize());
                        sslStream.send("EgorBo<EOF>".getBytes("UTF-8"));
                        byte[] serverResponseUtf8 = sslStream.receive();
                        serverResponse = new String(serverResponseUtf8, "UTF-8");

                    } else {
                        // Same, but via JNI
                        serverResponse = SSLStreamNative(socket.getSendBufferSize(), socket.getReceiveBufferSize(), socket.getInputStream(), socket.getOutputStream());
                    }

                    Log.i("EGOR", "server: " + serverResponse + ", using JNI? " + !javaImpl);
                    // print server response on UI
                    final String serverMsg = serverResponse;
                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            TextView tv = findViewById(R.id.sample_text);
                            tv.setText(serverMsg);
                        }
                    });
                } catch (Exception exc) {
                    exc.printStackTrace();
                }
            }
        }).start();
    }

    public native String SSLStreamNative(int sendBufferSize, int receiveBufferSize, InputStream inputStream, OutputStream outputStream);
}