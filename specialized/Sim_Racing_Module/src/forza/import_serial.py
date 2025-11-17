import serial
import pyautogui
Arduino_Serial = serial.Serial('COM7', 9600) # Arduino Uno is connected on COM7

handbrakeStatus = 1 # 1 = on, 0 = off

while 1:
    incoming_data = str(Arduino_Serial.readline())
    print(incoming_data)
    if 'pressK' in incoming_data:
        pyautogui.keyDown('k')
        handbrakeStatus = 1
    else:
        if handbrakeStatus == 1:
            pyautogui.keyUp('k')
            handbrakeStatus = 0
    incoming_data = ""