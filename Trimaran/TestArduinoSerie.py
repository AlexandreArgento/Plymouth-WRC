# -*- coding: utf-8 -*-
"""
Created on Sat Mar 30 18:28:48 2019

@author: catam
"""

import serial
import time

arduino = serial.Serial('/dev/ttyACM0', 115200)


for i in range(255):
    msg1 = 1060 + (i)%(120-60)
    msg2 = 2000 + (i)%(120-60)
    
    msg1 = str(msg1).encode()
    msg2 = str(msg2).encode()
    
    arduino.write(msg1)
    
    arduino.write(msg2)
    
    print((i)%(120-60))
    
    time.sleep(0.25)
