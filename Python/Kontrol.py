```python
##############################################################################
# Project Name : Gamepad Controller System
#
# Course       : Electrical and Electronics Engineering (EEE)
#
# Students:
#   Mustafa Haki Karaca
#   Student ID : 61230001
#   Department : EEE
#
#   Kerim Güler
#   Student ID : 61230005
#   Department : EEE
#
# Description:
# This program receives controller data from a serial port, maps it to a
# virtual Xbox controller, and controls rumble feedback.
##############################################################################

import serial
import vgamepad as vg
import time

# Serial communication settings
SERIAL_PORT = 'COM14'
BAUD_RATE = 9600

ser = None

# Axis inversion settings
INVERT_J1_X = True
INVERT_J1_Y = True

INVERT_J2_X = False
INVERT_J2_Y = True

# Mapping between received button names and Xbox buttons
BUTTON_MAP = {
    "X": vg.XUSB_BUTTON.XUSB_GAMEPAD_X,
    "Y": vg.XUSB_BUTTON.XUSB_GAMEPAD_Y,
    "B": vg.XUSB_BUTTON.XUSB_GAMEPAD_B,
    "A": vg.XUSB_BUTTON.XUSB_GAMEPAD_A,
    "RB": vg.XUSB_BUTTON.XUSB_GAMEPAD_RIGHT_SHOULDER,
    "LB": vg.XUSB_BUTTON.XUSB_GAMEPAD_LEFT_SHOULDER,
    "Yukari": vg.XUSB_BUTTON.XUSB_GAMEPAD_DPAD_UP,
    "Asagi": vg.XUSB_BUTTON.XUSB_GAMEPAD_DPAD_DOWN,
    "Sag": vg.XUSB_BUTTON.XUSB_GAMEPAD_DPAD_RIGHT,
    "Sol": vg.XUSB_BUTTON.XUSB_GAMEPAD_DPAD_LEFT
}

# Handle rumble feedback from the virtual controller
def rumble_callback(client, target, large_motor, small_motor, led_number, user_data):
    global ser
    try:
        if large_motor > 0 or small_motor > 0:
            if ser and ser.is_open: ser.write(b'M')
        else:
            if ser and ser.is_open: ser.write(b'S')
    except Exception:
        pass

# Convert joystick ADC values to Xbox joystick range
def map_analog_custom(val, center=512, min_val=0, max_val=750, deadzone=30):
    if val > max_val: val = max_val
    if val < min_val: val = min_val

    if abs(val - center) <= deadzone:
        return 0

    if val > center:
        return int(((val - (center + deadzone)) / (max_val - (center + deadzone))) * 32767)
    else:
        return int(((val - min_val) / ((center - deadzone) - min_val)) * 32768 - 32768)

# Convert trigger ADC values to Xbox trigger range
def map_trigger_custom(val, deadzone=80, max_val=750):
    if val < deadzone:
        return 0
    if val > max_val:
        return 255

    return int(((val - deadzone) / (max_val - deadzone)) * 255)

# Main program
def main():
    global ser

    # Create virtual Xbox controller
    gamepad = vg.VX360Gamepad()
    gamepad.register_notification(callback_function=rumble_callback)

    print("Virtual Xbox Controller Launched.")

    # Open serial connection
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=0.1)
    except Exception as e:
        print(f"Port Hatası: {e}")
        return

    while True:
        try:
            # Read one line from serial port
            line = ser.readline().decode('utf-8').strip()
            if not line:
                continue

            line = line.replace("T:", "T: ")
            gamepad.reset()

            parts = line.split()
            active_buttons = []

            # Parse incoming controller data
            for part in parts:

                # Process left joystick
                if part.startswith("J1:"):
                    x, y = map(int, part[3:].split(','))
                    j1_x = map_analog_custom(x)
                    j1_y = map_analog_custom(y)
                    if INVERT_J1_X: j1_x = -j1_x
                    if INVERT_J1_Y: j1_y = -j1_y
                    gamepad.left_joystick(x_value=j1_x, y_value=j1_y)

                # Process right joystick
                elif part.startswith("J2:"):
                    x, y = map(int, part[3:].split(','))
                    j2_x = map_analog_custom(x)
                    j2_y = map_analog_custom(y)
                    if INVERT_J2_X: j2_x = -j2_x
                    if INVERT_J2_Y: j2_y = -j2_y
                    gamepad.right_joystick(x_value=j2_x, y_value=j2_y)

                # Process left trigger
                elif part.startswith("L2:"):
                    val = int(part[3:])
                    gamepad.left_trigger(value=map_trigger_custom(val))

                # Process right trigger
                elif part.startswith("R2:"):
                    val = int(part[3:])
                    gamepad.right_trigger(value=map_trigger_custom(val))

                # Store active buttons
                elif part in BUTTON_MAP:
                    active_buttons.append(part)

            # Update button states
            for btn_name, btn_code in BUTTON_MAP.items():
                if btn_name in active_buttons:
                    gamepad.press_button(button=btn_code)
                else:
                    gamepad.release_button(button=btn_code)

            # Send updated state to virtual controller
            gamepad.update()

        except KeyboardInterrupt:
            break
        except Exception:
            pass

# Program entry point
if __name__ == "__main__":
    main()
```
