﻿using System;
using System.Net;
using System.Net.Sockets;
using System.Net.Security;
using System.Security.Authentication;
using System.Text;
using System.Security.Cryptography.X509Certificates;

public sealed class SslTcpServer
{
    private static X509Certificate _serverCertificate = null;

    private static void RunServer(string certificate = null)
    {
        if (certificate == null)
        {
            // YOLO: use some random existing certificate 
            X509Store store = new X509Store(StoreLocation.CurrentUser);
            store.Open(OpenFlags.OpenExistingOnly);
            _serverCertificate = store.Certificates[0];
        }
        else
        {
            // TODO: load certificate from file
        }

        TcpListener listener = new TcpListener(IPAddress.Any, 8080);
        listener.Start();
        while (true)
        {
            Console.WriteLine("Waiting for a client to connect...");
            TcpClient client = listener.AcceptTcpClient();
            Console.WriteLine("Accepted a client!");
            ProcessClient(client);
        }
    }
    private static void ProcessClient(TcpClient client)
    {
        SslStream sslStream = new SslStream(client.GetStream(), false, CertValid);

        // Authenticate the server but don't require the client to authenticate.
        try
        {
            sslStream.AuthenticateAsServer(_serverCertificate, 
                clientCertificateRequired: false, 
                checkCertificateRevocation: true,
                enabledSslProtocols: SslProtocols.Tls12);

            // Display the properties and settings for the authenticated stream.
            DisplaySecurityLevel(sslStream);
            DisplaySecurityServices(sslStream);
            DisplayCertificateInformation(sslStream);
            DisplayStreamProperties(sslStream);

            // Set timeouts for the read and write to 5 seconds.
            sslStream.ReadTimeout = 5000;
            sslStream.WriteTimeout = 5000;

            // Read a message from the client.
            Console.WriteLine("Waiting for client message...");
            string messageData = ReadMessage(sslStream);
            Console.WriteLine("Received: {0}", messageData);

            // Write a message to the client.
            byte[] message = Encoding.UTF8.GetBytes("Hello from the server.<EOF>");
            Console.WriteLine($"Sending hello message: {BitConverter.ToString(message)}");
            sslStream.Write(message);
        }
        catch (AuthenticationException e)
        {
            Console.WriteLine("Exception: {0}", e.Message);
            if (e.InnerException != null)
            {
                Console.WriteLine("Inner exception: {0}", e.InnerException.Message);
            }
            Console.WriteLine("Authentication failed - closing the connection.");
            sslStream.Close();
            client.Close();
            return;
        }
        catch (Exception e)
        {
            Console.WriteLine(e);
        }
        finally
        {
            sslStream.Close();
            client.Close();
        }
    }

    private static bool CertValid(object sender, X509Certificate certificate, X509Chain chain, SslPolicyErrors sslPolicyErrors)
    {
        return true;
    }

    private static string ReadMessage(SslStream sslStream)
    {
        // Read the  message sent by the client.
        // The client signals the end of the message using the
        // "<EOF>" marker.
        byte[] buffer = new byte[2048];
        StringBuilder messageData = new StringBuilder();
        int bytes = -1;
        do
        {
            // Read the client's test message.
            bytes = sslStream.Read(buffer, 0, buffer.Length);

            // Use Decoder class to convert from bytes to UTF8
            // in case a character spans two buffers.
            Decoder decoder = Encoding.UTF8.GetDecoder();
            char[] chars = new char[decoder.GetCharCount(buffer, 0, bytes)];
            decoder.GetChars(buffer, 0, bytes, chars, 0);
            messageData.Append(chars);
            // Check for EOF or an empty message.
            if (messageData.ToString().IndexOf("<EOF>") != -1)
            {
                break;
            }
        } while (bytes != 0);

        return messageData.ToString();
    }

    private static void DisplaySecurityLevel(SslStream stream)
    {
        Console.WriteLine("Cipher: {0} strength {1}", stream.CipherAlgorithm, stream.CipherStrength);
        Console.WriteLine("Hash: {0} strength {1}", stream.HashAlgorithm, stream.HashStrength);
        Console.WriteLine("Key exchange: {0} strength {1}", stream.KeyExchangeAlgorithm, stream.KeyExchangeStrength);
        Console.WriteLine("Protocol: {0}", stream.SslProtocol);
    }

    private static void DisplaySecurityServices(SslStream stream)
    {
        Console.WriteLine("Is authenticated: {0} as server? {1}", stream.IsAuthenticated, stream.IsServer);
        Console.WriteLine("IsSigned: {0}", stream.IsSigned);
        Console.WriteLine("Is Encrypted: {0}", stream.IsEncrypted);
    }

    private static void DisplayStreamProperties(SslStream stream)
    {
        Console.WriteLine("Can read: {0}, write {1}", stream.CanRead, stream.CanWrite);
        Console.WriteLine("Can timeout: {0}", stream.CanTimeout);
    }

    private static void DisplayCertificateInformation(SslStream stream)
    {
        Console.WriteLine("Certificate revocation list checked: {0}", stream.CheckCertRevocationStatus);

        X509Certificate localCertificate = stream.LocalCertificate;
        if (stream.LocalCertificate != null)
        {
            Console.WriteLine("Local cert was issued to {0} and is valid from {1} until {2}.",
                localCertificate.Subject,
                localCertificate.GetEffectiveDateString(),
                localCertificate.GetExpirationDateString());
        }
        else
        {
            Console.WriteLine("Local certificate is null.");
        }
        // Display the properties of the client's certificate.
        X509Certificate remoteCertificate = stream.RemoteCertificate;
        if (stream.RemoteCertificate != null)
        {
            Console.WriteLine("Remote cert was issued to {0} and is valid from {1} until {2}.",
                remoteCertificate.Subject,
                remoteCertificate.GetEffectiveDateString(),
                remoteCertificate.GetExpirationDateString());
        }
        else
        {
            Console.WriteLine("Remote certificate is null.");
        }
    }

    public static void Main()
    {
        RunServer();
    }
}
