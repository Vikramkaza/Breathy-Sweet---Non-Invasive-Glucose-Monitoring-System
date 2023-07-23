# Extracting readings from serial monitor

import glucose
import glucose1
import serial

ser = serial.Serial('COM5', 115200, timeout=1)
while(True):
    
    line =  ser.readline()

    #line = b'128\r\n'

    string = line.decode()
    stripped_string = string.strip()
    print(stripped_string)
    if not stripped_string:
        print("None")
    else:
        # lst = int(stripped_string)
        lst=stripped_string.split(',')
        lst=[eval(i) for i in lst]
        
        
        x=lst[0]
        y=lst[2]
        lst1=glucose.prediction(x)
        lst2=glucose1.prediction(y)
        value=str((lst1[0]+lst2[0])/2)
        print(value)
        ser.write(value.encode())

ser.close()
#1028,1207,376,0