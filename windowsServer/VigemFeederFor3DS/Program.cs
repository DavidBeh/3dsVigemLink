using System;
using System.Net;
using System.Net.Sockets;
using System.Text;
using Nefarius.ViGEm.Client;
using Nefarius.ViGEm.Client.Targets;
using Nefarius.ViGEm.Client.Targets.Xbox360;
using VigemFeederFor3DS;

class Program
{
    static void Main()
    {
        // Define the port and IP address to listen on
        int port = 12346;
        IPAddress ipAddress = IPAddress.Any;
        var vigem = new ViGEmClient(); 
        IXbox360Controller? controller = null;
        
        
        // Create a UDP client
        UdpClient udpClient = new UdpClient(port);

        Console.WriteLine("Listening for data...");

        try
        {
            while (true)
            {
                // Receive data
                IPEndPoint remoteEndPoint = new IPEndPoint(ipAddress, port);
                byte[] receiveBytes = udpClient.Receive(ref remoteEndPoint);
                
                if (controller == null)
                {
                    controller = vigem.CreateXbox360Controller();
                    controller.Connect();                    
                    //controller.AutoSubmitReport = true;

                }
                
                

                
                // Process received data
                if (receiveBytes.Length == 24) // Ensure the buffer size matches the expected size
                {
                    uint net_kDown = BitConverter.ToUInt32(receiveBytes, 0);
                    uint net_kHeld = BitConverter.ToUInt32(receiveBytes, 4);
                    uint net_kUp = BitConverter.ToUInt32(receiveBytes, 8);
                    short net_pos_dx = BitConverter.ToInt16(receiveBytes, 12);
                    short net_pos_dy = BitConverter.ToInt16(receiveBytes, 14);
                    short net_cstick_dx = BitConverter.ToInt16(receiveBytes, 16);
                    short net_cstick_dy = BitConverter.ToInt16(receiveBytes, 18);
                    uint net_sequence = BitConverter.ToUInt32(receiveBytes, 20);

                    // Convert from network byte order to host byte order
                    uint kDown = (uint)IPAddress.NetworkToHostOrder((int)net_kDown);
                    uint kHeld = (uint)IPAddress.NetworkToHostOrder((int)net_kHeld);
                    uint kUp = (uint)IPAddress.NetworkToHostOrder((int)net_kUp);
                    short pos_dx = IPAddress.NetworkToHostOrder(net_pos_dx);
                    short pos_dy = IPAddress.NetworkToHostOrder(net_pos_dy);
                    short cstick_dx = IPAddress.NetworkToHostOrder(net_cstick_dx);
                    short cstick_dy = IPAddress.NetworkToHostOrder(net_cstick_dy);
                    uint sequence = (uint)IPAddress.NetworkToHostOrder((int)net_sequence);
                    ushort mask = 0;
                    foreach (Xbox360Property xbox360Property in ButtonHelper.GetProperties((NKey)kHeld))
                    {
                        mask += xbox360Property switch
                        {
                            Xbox360Button button => button.Value,
                            Xbox360Axis axis => 0,
                            Xbox360Slider slider => 0,
                            _ => 0
                        };
                    }
                    Console.WriteLine($"kDown: {kDown}, kHeld: {kHeld}, kUp: {kUp}");
                    Console.WriteLine($"pos_dx: {pos_dx}, pos_dy: {pos_dy}");
                    Console.WriteLine($"cstick_dx: {cstick_dx}, cstick_dy: {cstick_dy}");
                    Console.WriteLine($"Sequence: {sequence}, MAsk: {mask}");
                    

                    controller.SetButtonsFull(mask);
                    controller.SubmitReport();
                }
                // Print the values

                else
                {
                    Console.WriteLine("Received data of unexpected length: " + receiveBytes.Length);
                }
            }
        }
        catch (Exception e)
        {

            Console.WriteLine(e.ToString());
        }
        finally
        {
            controller?.Disconnect();
            udpClient.Close();
        }
    }
}