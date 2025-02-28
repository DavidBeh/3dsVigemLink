using System;
using System.Net;
using System.Net.Sockets;
using System.Numerics;
using System.Text;
using Nefarius.ViGEm.Client;
using Nefarius.ViGEm.Client.Targets;
using Nefarius.ViGEm.Client.Targets.Xbox360;
using VigemFeederFor3DS;

class Program
{
    
    public static double InverseLerp(int start, int end, int value, bool clamp = true)
    {
        if (clamp)
        {
            value = Math.Clamp(value, start, end);
        }
        return (double)(value - start) / (end - start);
    }

    // Converts a short which goes from -140 to 140 to the full short range, then clamps it between short.MinValue and short.MaxValue
    static void ConvertValue(ref short value)
    {
        var s1 = Math.Clamp(value, (short)-140, (short)140);
        var s2 = (short)(s1 * short.MaxValue / 140);
        value = s2;
    }

    static void Main(string[] args)
    {
        
        // Define the port and IP address to listen on
        
        int port = 12346;
        if (args.Length >= 1)
            int.TryParse(args[0], out port);
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
                IPEndPoint remoteEndPoint = new(ipAddress, port);
                byte[] receiveBytes = udpClient.Receive(ref remoteEndPoint);

                if (controller == null)
                {
                    controller = vigem.CreateXbox360Controller();
                    controller.Connect();
                    controller.AutoSubmitReport = false;
                }


                // Process received data
                if (receiveBytes.Length == 20) // Ensure the buffer size matches the expected size
                {
                    uint net_kDown = BitConverter.ToUInt32(receiveBytes, 0);
                    short net_pos_dx = BitConverter.ToInt16(receiveBytes, 4);
                    short net_pos_dy = BitConverter.ToInt16(receiveBytes, 6);
                    short net_cstick_dx = BitConverter.ToInt16(receiveBytes, 8);
                    short net_cstick_dy = BitConverter.ToInt16(receiveBytes, 10);
                    short net_touch_x = BitConverter.ToInt16(receiveBytes, 12);
                    short net_touch_y = BitConverter.ToInt16(receiveBytes, 14);
                    uint net_sequence = BitConverter.ToUInt32(receiveBytes, 16);


                    // Convert from network byte order to host byte order
                    NKey kDown = (NKey) IPAddress.NetworkToHostOrder((int)net_kDown);
                    short pos_dx = IPAddress.NetworkToHostOrder(net_pos_dx);
                    short pos_dy = IPAddress.NetworkToHostOrder(net_pos_dy);
                    short cstick_dx = IPAddress.NetworkToHostOrder(net_cstick_dx);
                    short cstick_dy = IPAddress.NetworkToHostOrder(net_cstick_dy);
                    short touch_x = IPAddress.NetworkToHostOrder(net_touch_x);
                    short touch_y = IPAddress.NetworkToHostOrder(net_touch_y);
                    uint sequence = (uint)IPAddress.NetworkToHostOrder((int)net_sequence);
                    ushort mask = 0;

                    double touch_x_norm = InverseLerp(0, 320, touch_x);
                    double touch_y_norm = InverseLerp(0, 240, touch_y);

                    controller.LeftTrigger = 0;
                    controller.RightTrigger = 0;

                    
                    foreach (Xbox360Property xbox360Property in ButtonHelper.GetProperties(kDown))
                    {
                        switch (xbox360Property)
                        {
                            case Xbox360Button button:
                                mask += button.Value;
                                break;
                            case Xbox360Slider slider:
                                controller.SetSliderValue(slider, byte.MaxValue);
                                break;
                        }
                    }
                    Console.WriteLine($"kDown: {kDown}");
                    Console.WriteLine($"pos_dx: {pos_dx}, pos_dy: {pos_dy}");
                    Console.WriteLine($"cstick_dx: {cstick_dx}, cstick_dy: {cstick_dy}");
                    Console.WriteLine($"touch_x: {touch_x}, touch_y: {touch_y}");
                    Console.WriteLine($"Sequence: {sequence}, MAsk: {mask}");


                    controller.SetButtonsFull(mask);
                    // Set the thumbstick-buttons based on the side of the screen the touch is on
                    if (kDown.HasFlag(NKey.KEY_TOUCH))
                    {
                        if (touch_x_norm < 0.7)
                        {
                            controller.SetButtonState(Xbox360Button.LeftThumb, true);
                        }
                        if (touch_x_norm > 0.3)
                        {
                            controller.SetButtonState(Xbox360Button.RightThumb, true);
                        }
                    }
                    
                    ConvertValue(ref pos_dx);
                    ConvertValue(ref pos_dy);
                    ConvertValue(ref cstick_dx);
                    ConvertValue(ref cstick_dy);

                    controller.SetAxisValue(Xbox360Axis.LeftThumbX, pos_dx);
                    controller.SetAxisValue(Xbox360Axis.LeftThumbY, pos_dy);
                    controller.SetAxisValue(Xbox360Axis.RightThumbX, cstick_dx);
                    controller.SetAxisValue(Xbox360Axis.RightThumbY, cstick_dy);
                    
                    controller.SubmitReport();
                }

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