using System;
using System.Net.Security;
using System.Net.Sockets;
using System.Security.Authentication;
using System.Text;
using System.Security.Cryptography.X509Certificates;
using System.IO;

public class SslTcpClient
{
    public static bool ValidateServerCertificate(
            object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors)
    {
        if (sslPolicyErrors == SslPolicyErrors.None)
            return true;

        Console.WriteLine("Certificate error: {0}", sslPolicyErrors);
        // Ignore errors (our random server's cert is too random :\)
        return true;
    }

    public static void RunClient(string machineName, string serverName)
    {
        TcpClient client = new TcpClient(machineName, 8080);
        Console.WriteLine("Client connected.");
        SslStream sslStream = new SslStream(new LoggableStream(client.GetStream()), false,
            new RemoteCertificateValidationCallback(ValidateServerCertificate), null);
        try
        {
            // TLS1.2: (doesn't include TCP handshake)
            //
            // Send     ClientHello  153  bytes
            // Receive  ServerHello  5    bytes
            // Receive  Certificate  1248 bytes
            // Send     ...          158  bytes
            // Receive  ...          51   bytes
            
            // NOTE: serverName here is useless?
            // NOTE: I use TSL1.2 here because my Galaxy S8+ doesn't support TLS1.3
            sslStream.AuthenticateAsClient(serverName, null, SslProtocols.Tls12, false);
            Console.WriteLine("----------------------- [auth completed] -------------------");
        }
        catch (AuthenticationException e)
        {
            Console.WriteLine("Exception: {0}", e.Message);
            if (e.InnerException != null)
            {
                Console.WriteLine("Inner exception: {0}", e.InnerException.Message);
            }
            Console.WriteLine("Authentication failed - closing the connection.");
            client.Close();
            return;
        }
        byte[] messsage = Encoding.UTF8.GetBytes("Hello from the client.<EOF>");
        sslStream.Write(messsage);
        sslStream.Flush();
        string serverMessage = ReadMessage(sslStream);
        Console.WriteLine("Server says: {0}", serverMessage);
        client.Close();
        Console.WriteLine("Client closed.");
    }

    static string ReadMessage(SslStream sslStream)
    {
        byte[] buffer = new byte[2048];
        StringBuilder messageData = new StringBuilder();
        int bytes = -1;
        do
        {
            bytes = sslStream.Read(buffer, 0, buffer.Length);
            Decoder decoder = Encoding.UTF8.GetDecoder();
            char[] chars = new char[decoder.GetCharCount(buffer, 0, bytes)];
            decoder.GetChars(buffer, 0, bytes, chars, 0);
            messageData.Append(chars);
            if (messageData.ToString().IndexOf("<EOF>") != -1)
            {
                break;
            }
        } while (bytes != 0);

        return messageData.ToString();
    }

    public static void Main()
    {
        RunClient("192.168.0.5", "192.168.0.5");
    }
}

// just a stream wrapper, logs reads and writes
public class LoggableStream : Stream
{
    Stream _stream;

    public LoggableStream(Stream stream)
    {
        _stream = stream;
    }

    public override bool CanRead => _stream.CanRead;

    public override bool CanSeek => _stream.CanSeek;

    public override bool CanWrite => _stream.CanWrite;

    public override long Length => _stream.Length;

    public override long Position { get => _stream.Position; set => _stream.Position = value; }

    public override void Flush()
    {
        _stream.Flush();
    }

    public override int Read(byte[] buffer, int offset, int count)
    {
        Console.WriteLine($"[] Read offset={offset}, count={count}");
        var read = _stream.Read(buffer, offset, count);
        return read;
    }

    public override long Seek(long offset, SeekOrigin origin)
    {
        var rz = _stream.Seek(offset, origin);
        return rz;
    }

    public override void SetLength(long value)
    {
        _stream.SetLength(value);
    }

    public override void Write(byte[] buffer, int offset, int count)
    {
        Console.WriteLine($"[] Write offset={offset}, count={count}");
        _stream.Write(buffer, offset, count);
    }
}
