using Nefarius.ViGEm.Client.Targets.Xbox360;

namespace VigemFeederFor3DS;

using System;

[Flags]
public enum NKey
{
    KEY_A = 1 << 0, // A
    KEY_B = 1 << 1, // B
    KEY_SELECT = 1 << 2, // Select
    KEY_START = 1 << 3, // Start
    KEY_DRIGHT = 1 << 4, // D-Pad Right
    KEY_DLEFT = 1 << 5, // D-Pad Left
    KEY_DUP = 1 << 6, // D-Pad Up
    KEY_DDOWN = 1 << 7, // D-Pad Down
    KEY_R = 1 << 8, // R
    KEY_L = 1 << 9, // L
    KEY_X = 1 << 10, // X
    KEY_Y = 1 << 11, // Y
    KEY_ZL = 1 << 14, // ZL (New 3DS only)
    KEY_ZR = 1 << 15, // ZR (New 3DS only)
    KEY_TOUCH = 1 << 20, // Touch (Not actually provided by HID)
    KEY_CSTICK_RIGHT = 1 << 24, // C-Stick Right (New 3DS only)
    KEY_CSTICK_LEFT = 1 << 25, // C-Stick Left (New 3DS only)
    KEY_CSTICK_UP = 1 << 26, // C-Stick Up (New 3DS only)
    KEY_CSTICK_DOWN = 1 << 27, // C-Stick Down (New 3DS only)
    KEY_CPAD_RIGHT = 1 << 28, // Circle Pad Right
    KEY_CPAD_LEFT = 1 << 29, // Circle Pad Left
    KEY_CPAD_UP = 1 << 30, // Circle Pad Up
    KEY_CPAD_DOWN = 1 << 31, // Circle Pad Down

    // Generic catch-all directions
    KEY_UP = KEY_DUP | KEY_CPAD_UP, // D-Pad Up or Circle Pad Up
    KEY_DOWN = KEY_DDOWN | KEY_CPAD_DOWN, // D-Pad Down or Circle Pad Down
    KEY_LEFT = KEY_DLEFT | KEY_CPAD_LEFT, // D-Pad Left or Circle Pad Left
    KEY_RIGHT = KEY_DRIGHT | KEY_CPAD_RIGHT // D-Pad Right or Circle Pad Right
}

static class ButtonHelper
{
    
    public static IEnumerable<Xbox360Property> GetProperties(NKey keys)
    {
        foreach (NKey key in Enum.GetValues<NKey>())
        {
            if (keys.HasFlag(key))
            {
                Xbox360Property? property = GetProperty(key);
                if (property != null)
                {
                    yield return property;
                }
            }
        }
    }
    
    
    
    public static Xbox360Property? GetProperty(NKey key)
    {
        return key switch
        {
            NKey.KEY_A => Xbox360Button.A,
            NKey.KEY_B => Xbox360Button.B,
            NKey.KEY_X => Xbox360Button.X,
            NKey.KEY_Y => Xbox360Button.Y,
            NKey.KEY_START => Xbox360Button.Start,
            NKey.KEY_SELECT => Xbox360Button.Back,
            NKey.KEY_DRIGHT => Xbox360Button.Right,
            NKey.KEY_DLEFT => Xbox360Button.Left,
            NKey.KEY_DUP => Xbox360Button.Up,
            NKey.KEY_DDOWN => Xbox360Button.Down,
            NKey.KEY_R => Xbox360Button.RightShoulder,
            NKey.KEY_L => Xbox360Button.LeftShoulder,
            NKey.KEY_ZL => Xbox360Slider.LeftTrigger,
            NKey.KEY_ZR => Xbox360Slider.RightTrigger,
            _ => null
        };

        // returns the Xbox360Property that corresponds to the given NKey
    }
}