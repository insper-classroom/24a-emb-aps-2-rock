import serial
from pynput.keyboard import Controller, Key
import uinput
import sys
import time

ser = serial.Serial('/dev/rfcomm0', 9600, timeout=10)
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


def move_mouse(x, y, select):
    if (x > 40) and (x <60):
            x = 0
    else:
        if x < 50:
            x = -1* (50 - x)
        else:
            x = x - 50
    device.emit(uinput.REL_X, x)
    if (y > 40) and (y <60):
            y = 0
    else:
        if y < 50:
            y = -1* (50 - y)
        else:
            y = y - 50
    device.emit(uinput.REL_Y, y)

    if select == 0:
        device.emit(uinput.BTN_LEFT, 1)
        device.emit(uinput.BTN_LEFT, 0)

botao_letra = {
    2: 'k',   # Verde
    4: 'l',  # Vermelho
    8: 'j',   # Amarelo
    1: 'a',   # Azul
    16: 's'    # Laranja
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
                device.emit(uinput.BTN_LEFT, 1)
                device.emit(uinput.BTN_LEFT, 0)

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
            print(data)
            if data == b'\xff':
                i = 1

        print(int.from_bytes(data, byteorder=sys.byteorder))
        data = ser.read(1)
        data_x = int.from_bytes(data, byteorder=sys.byteorder)
        print("Valor de X: ", data_x)
        data = ser.read(1)
        data_y = int.from_bytes(data, byteorder=sys.byteorder)
        print("Valor de Y: ", data_y)
        data = ser.read(1)
        data_button = int.from_bytes(data, byteorder=sys.byteorder)
        print("Button: ", data_button)
        data = ser.read(1)
        data_select = int.from_bytes(data, byteorder=sys.byteorder)
        print("Select: ", data_select)
        move_mouse(data_x, data_y, data_select)

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

