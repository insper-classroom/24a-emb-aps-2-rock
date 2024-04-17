import serial
from pynput.keyboard import Controller, Key
import uinput
import sys
import time

ser = serial.Serial('/dev/ttyACM0', 115200)
keyboard = Controller()

device = uinput.Device([
    uinput.BTN_LEFT,
    uinput.BTN_RIGHT,
    uinput.REL_X,
    uinput.REL_Y,
    uinput.KEY_A,
    uinput.KEY_S,
    uinput.KEY_J,
    uinput.KEY_K,
    uinput.KEY_L,
])



def move_mouse(axis, value):
    if axis == 0:    # X-axis
        if (value > 40) and (value <60):
            value = 0
        else:
            if value < 50:
                value = -1* (50 - value)
            else:
                value = value - 50
        device.emit(uinput.REL_X, value)
    elif axis == 1:  # Y-axis
        if (value > 40) and (value <60):
                value = 0
        else:
            if value < 50:
                value = -1* (50 - value)
            else:
                value = value - 50
        device.emit(uinput.REL_Y, value)

botao_letra = {
    8: 'a',   # Verde
    16: 's',  # Vermelho
    2: 'j',   # Amarelo
    1: 'k',   # Azul
    4: 'l'    # Laranja
}

cases_press = {
            8: {'a'},
            16: {'s'},
            2: {'j'},
            1: {'k'},
            4: {'l'},
            24: {'a', 's'},
            10: {'a', 'j'},
            9: {'a', 'k'},
            12: {'a', 'l'},
            18: {'s', 'j'},
            17: {'s', 'k'},
            20: {'s', 'l'},
            3: {'j', 'k'},
            6: {'j', 'l'},
            5: {'k', 'l'},
            30: {'a', 's', 'j'},
            28: {'a', 's', 'k'},
            22: {'a', 'j', 'k'},
            14: {'a', 'j', 'l'},
            13: {'a', 'k', 'l'},
            7: {'s', 'j', 'k'},
            11: {'s', 'j', 'l'},
            19: {'s', 'k', 'l'},
            15: {'j', 'k', 'l'},
            31: {'a', 's', 'j', 'k'},
            29: {'a', 's', 'j', 'l'},
            27: {'a', 's', 'k', 'l'},
            23: {'a', 'j', 'k', 'l'},
            26: {'s', 'j', 'k', 'l'},
            25: {'a', 's', 'j', 'k', 'l'},
        }
cases_release = {
            8: {'s','j','k','l'},
            16: {'a','j','k','l'},
            2: {'a','s','k','l'},
            1: {'a','s','j','l'},
            4: {'a','s','j','k'},
            24: {'j','k','l'},
            10: {'s','k','l'},
            9: {'s','j','l'},
            12: {'s','j','k'},
            18: {'a','k','l'},
            17: {'a','j','l'},
            20: {'a','j','k'},
            3: {'a','s','l'},
            6: {'a','s','j'},
            5: {'a','s','k'},
            30: {'s'},
            28: {'j'},
            22: {'k'},
            14: {'l'},
            13: {'a'},
            7: {'s','j','k','l'},
            11: {'a','j','k','l'},
            19: {'a','s','k','l'},
            15: {'a','s','j','l'},
            31: set(),  # Não há necessidade de liberar nenhuma tecla
            29: set(),  # Não há necessidade de liberar nenhuma tecla
            27: set(),  # Não há necessidade de liberar nenhuma tecla
            23: set(),  # Não há necessidade de liberar nenhuma tecla
            26: set(),  # Não há necessidade de liberar nenhuma tecla
            25: set(),  # Não há necessidade de liberar nenhuma tecla
        }

def levantar_tecla(button_index):
    if button_index in cases_release:
        for key in cases_release[button_index]:
                keyboard.release(key)    
    else:
        pass
def press_key(button_index):
    if button_index in cases_press: 
        for key in cases_press[button_index]:
            keyboard.press(key)
        levantar_tecla(button_index)
    else:
        pass


botao_anterior = 0
try:
    while True:
        # Lê os dados da porta serial
        #data = ser.read()
        print('Waiting for sync package...')
        i = 0
        while (i==0):
            data = ser.read(1)
            if data == b'\xff':
                i = 1

        print(int.from_bytes(data, byteorder=sys.byteorder))
        data = ser.read(1)
        axis = int.from_bytes(data, byteorder=sys.byteorder)
        print("Axis: ", axis)
        data = ser.read(1)
        valor_mouse = int.from_bytes(data, byteorder=sys.byteorder)
        print("Valor Mouse: ", valor_mouse)
        data = ser.read(1)
        data_button = int.from_bytes(data, byteorder=sys.byteorder)
        print("Button: ", data_button)
        data = ser.read(1)
        print(int.from_bytes(data, byteorder=sys.byteorder))
        print("\n\n")
        move_mouse(axis, valor_mouse)

        if data_button:
            #passando data_bubtton pra binario
            if data_button != botao_anterior:
                levantar_tecla(botao_anterior)
                botao_anterior = data_button
            press_key(data_button)

except KeyboardInterrupt:
    print("Program terminated by user")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    ser.close()
