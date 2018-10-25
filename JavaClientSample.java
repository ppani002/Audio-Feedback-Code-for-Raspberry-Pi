import java.io.*;
import java.net.*;

public class JavaClientSample
{
	public static void main(String[] args) throws UnknownHostException, IOException, InterruptedException, SocketException
	{
		int portnumber = 3024;
		String hostname = "127.0.0.1"; //loop back address. Or 127.0.0.1

		System.out.println("Setting up");
		
		//Exception required here
		Socket clientSocket = new Socket(hostname, portnumber);

		System.out.println("Connected to server");

		//Exception required here
		OutputStream outputstream = clientSocket.getOutputStream();

		byte sequence = 0x00;

		while(true)
		{
			//Exception required here
			outputstream.write(sequence);
			sequence ^= 0x01; //toggle between hit and miss
			System.out.println("Sent sequence");

			//Exception required here
			Thread.sleep(1500); //switch every 1.5 seconds
		}
	}
}