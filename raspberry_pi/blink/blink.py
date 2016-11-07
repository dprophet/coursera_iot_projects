import Tkinter as tk
import RPi.GPIO as GPIO
import threading as t
import time
import sys
GPIO.setmode(GPIO.BOARD)
GPIO.setup(13, GPIO.OUT)
myButtonDown = False
myWindowClosed = False
myroot = tk.Tk()
myroot.geometry('800x600')

def button_down(event):
    global myButtonDown
    myButtonDown = True
    print("Button dowb called")

def button_up(event):
    global myButtonDown
    myButtonDown = False
    print("Button up called")

def window_close():
    global myWindowClosed
    print("Window Closing myWindowClosed=", myWindowClosed)
    myWindowClosed = True
    myroot.destroy()

myroot.title("Coursera Blink Button")
B = tk.Button(myroot, text="Push and keep down for solid on LED")
B.pack()
B.bind("<ButtonPress-1>", button_down)
B.bind("<ButtonRelease-1>", button_up)

def blink_loop():
    global myButtonDown
    global myWindowClosed
    while myWindowClosed == False:
        if myWindowClosed == True:
           print("Exiting Program")
           return

        GPIO.output(13, True)
        time.sleep(0.5)
        if myButtonDown == False:
            GPIO.output(13, False)
            time.sleep(0.5)

myt = t.Thread(target = blink_loop, args= ())
myt.start()
myroot.protocol("WM_DELETE_WINDOW", window_close)
myroot.mainloop()
print("After mainloop")
sys.exit()
print("After system exit")
