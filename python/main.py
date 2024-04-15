import serial
from pynput.keyboard import Controller, Key

ser = serial.Serial('/dev/ttyACM0', 115200)
keyboard = Controller()

def parse_button_states(data):
    button_states = []
    inside_brackets = False
    for byte in data:
        if byte == 91:  # '['
            inside_brackets = True
        elif byte == 93:  # ']'
            inside_brackets = False
            break
        elif inside_brackets:
            button_states.append(byte)
    return button_states

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
        data = ser.read_until(b']')
        if data:
            button_data = parse_button_states(data)
            print(f"Button States: {button_data}")
            
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
