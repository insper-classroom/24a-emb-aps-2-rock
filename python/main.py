import serial
from pynput.keyboard import Controller, Key
import uinput

ser = serial.Serial('/dev/ttyACM0', 115200)
keyboard = Controller()

device = uinput.Device([
    uinput.BTN_LEFT,
    uinput.BTN_RIGHT,
    uinput.REL_X,
    uinput.REL_Y,
])
def parse_mouse_data(data):
    #corta os 5 primeiros bytes e só considera os ultimos
    #data = data[5:]
    #print(f"Data Joy: {data}")
    axis = data[0]  # 0 for X, 1 for Y
    value = int.from_bytes(data[1:3], byteorder='little', signed=True)
    print(f"Received data: {data}")
    print(f"axis: {axis}, value: {value}")
    return axis, value
    
def parse_button_states(data):
    button_states = []
    inside_brackets = False
    #print(data)
    for byte in data:
        if byte == 91:  # '['
            inside_brackets = True
        elif byte == 93:  # ']'
            inside_brackets = False
            break
        elif inside_brackets:
            button_states.append(byte)
    return button_states

def move_mouse(axis, value):
    if axis == 0:    # X-axis
        device.emit(uinput.REL_X, value)
    elif axis == 1:  # Y-axis
        device.emit(uinput.REL_Y, value)

def press_key(button_index):
    if button_index == 6:  # Button 1 corresponds to the 'A' key
        keyboard.press('a')
    elif button_index == 8:  # Button 2 corresponds to the 'S' key
        keyboard.press('s')
    elif button_index == 2:  # Button 3 corresponds to the 'D' key
        keyboard.press('j')
    elif button_index == 0:  # Button 4 corresponds to the 'F' key
        keyboard.press('k')
    elif button_index == 4:  # Button 5 corresponds to the 'G' key
        keyboard.press('l')
def release_key(button_index):
    if button_index == 6:  # Button 1 corresponds to the 'A' key
        keyboard.release('a')
    elif button_index == 8:  # Button 2 corresponds to the 'S' key
        keyboard.release('s')
    elif button_index == 2:  # Button 3 corresponds to the 'D' key
        keyboard.release('j')
    elif button_index == 0:  # Button 4 corresponds to the 'F' key
        keyboard.release('k')
    elif button_index == 4:  # Button 5 corresponds to the 'G' key
        keyboard.release('l')
try:
    while True:
        # Lê os dados da porta serial
        #data = ser.read()
        print('Waiting for sync package...')
        while True:
            data = ser.read(1)
            if data == b'\xff':
                break
        data_joy = ser.read_until(b'B')
        data_mouse = b''
        for i in data_joy:
            # print(i)
            if i != 66:
                print(i)
                data_mouse += i.to_bytes(1, byteorder='big')
        print(f"Data Mouse: {data_mouse}")
        data = ser.read_until(b']')
        print(f"Data: {data}")
        axis, value = parse_mouse_data( b'\x01\x00\x01\x00\x00\xff')
        move_mouse(axis, value)

        #print(f"Data Joy: {data_joy}")
        if data:
            button_data = parse_button_states(data)
            #print(f"Button States: {button_data}")
            
            # Verifica se algum botão está sendo pressionado
            if button_data:
                for index, button in enumerate(button_data):
                    if (button != 32) and (button != 49):  # Ignora o valor 32 (espaço)
                        press_key(index)
                    if  (button == 49):
                        release_key(index)
            else:
                # Se nenhum botão está sendo pressionado, libera todas as teclas
                keyboard.release('a')
                keyboard.release('s')
                keyboard.release('d')
                keyboard.release('f')
                keyboard.release('g')

except KeyboardInterrupt:
    print("Program terminated by user")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    ser.close()
