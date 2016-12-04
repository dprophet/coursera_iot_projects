import RPi.GPIO as GPIO
import time
import traceback
import sys

my_pin = 12   #GPIO 10


try:

    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(my_pin,GPIO.OUT)

    pwm = GPIO.PWM(my_pin, 60)

    pwm.start(0)

    for i in range(10):
        print "Duty ", i
        pwm.ChangeDutyCycle(i)
        time.sleep(1)


except Exception as ex:
    print "Exception = ", ex, " Tracekack:", traceback.format_exc()
    sys.exit(1)

