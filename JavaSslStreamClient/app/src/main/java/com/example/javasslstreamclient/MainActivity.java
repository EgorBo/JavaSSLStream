package com.example.javasslstreamclient;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.widget.TextView;
import android.os.Bundle;
import android.util.Log;
import androidx.appcompat.app.AppCompatActivity;
import java.net.Socket;

public class MainActivity extends AppCompatActivity {

    Socket _socket;
    SSLStream _sslStream;

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
                    _socket = new Socket("192.168.0.5", 8080);
                    _sslStream = new SSLStream(_socket.getOutputStream(), _socket.getInputStream());
                    _sslStream.AuthenticateAsClient(_socket.getSendBufferSize(), _socket.getReceiveBufferSize());

                    // now let's send & wait some app data
                    _sslStream.send("EgorBo<EOF>".getBytes("UTF-8"));
                    byte[] serverResponseUtf8 = _sslStream.receive();
                    final String serverMessage = new String(serverResponseUtf8, "UTF-8");

                    runOnUiThread(new Runnable() {
                        @Override
                        public void run() {
                            TextView tv = findViewById(R.id.sample_text);
                            tv.setText(serverMessage);
                        }
                    });

                    Log.i("EGOR", "server: " + serverMessage);
                } catch (Exception exc) {
                    exc.printStackTrace();
                }
            }
        }).start();
    }

    public native String stringFromJNI();
}