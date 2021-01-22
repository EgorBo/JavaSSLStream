package com.sslstream.javaclient;

import android.os.Bundle;
import androidx.appcompat.app.AppCompatActivity;
import java.net.Socket;

public class MainActivity extends AppCompatActivity {

    Socket _socket;
    SSLStream _sslStream;

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
                    _socket = new Socket("192.168.0.5", 8080);
                    _sslStream = new SSLStream(_socket.getOutputStream(), _socket.getInputStream());
                    _sslStream.AuthenticateAsClient("");
                } catch (Exception exc) {
                    exc.printStackTrace();
                }
            }
        }).start();
    }
}
