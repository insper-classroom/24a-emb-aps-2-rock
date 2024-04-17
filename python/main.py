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


def levantar_tecla(button_index):
    # Convertendo o índice do botão para binário
    bin_button = bin(button_index)[2:].zfill(5)  # Garante que tenhamos 5 dígitos binários

    # Iterando sobre cada bit para verificar e levantar as teclas necessárias
    for i, bit in enumerate(bin_button[::-1]):  # Inverte a string binária para começar do bit menos significativo
        if bit == '1':
            keyboard.release(botao_letra[i])
            


def press_key(binary_number):
    for index, bit in enumerate(binary_number):
        if bit == '1':
            if index == 0:
                keyboard.press('s')
            elif index == 1:
                keyboard.press('a')
            elif index == 2:
                keyboard.press('l')
            elif index == 3:
                keyboard.press('j')
            elif index == 4:
                keyboard.press('k')
        elif bit == '0':
            if index == 0:
                keyboard.release('s')
            elif index == 1:
                keyboard.release('a')
            elif index == 2:
                keyboard.release('l')
            elif index == 3:
                keyboard.release('j')
            elif index == 4:
                keyboard.release('k')

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

        if data_button is not None:
            # transofrmando data_button em um binario que necessarioamente tem que ter 5 bits
            d = bin(data_button)[2:].zfill(5)
            print(d)
            press_key(d)

except KeyboardInterrupt:
    print("Program terminated by user")
except Exception as e:
    print(f"An error occurred: {e}")
finally:
    ser.close()

