import RPi.GPIO as GPIO
import time
import traceback
import sys
import signal

my_pin = 12   #GPIO 10

def signal_handler(signal,frame):
	print("\nProgram exiting gracefully.")
	
	pwm.stop()
	print("Stopped the PWM output")
	
	GPIO.cleanup()
	print("GPIO cleaned up.")
	
	sys.exit(0)

signal.signal(signal.SIGINT, signal_handler)
print("\nPress ctrl-c to terminate program.")

try:

    GPIO.setmode(GPIO.BOARD)
    GPIO.setup(my_pin,GPIO.OUT)

    pwm = GPIO.PWM(my_pin, 60)

    pwm.start(0)

    for i in range(10):
        print "Duty ", i
        pwm.ChangeDutyCycle(i)
        time.sleep(1)

    for i in range(10, 0, -1):
        print "Duty ", i
        pwm.ChangeDutyCycle(i)
        time.sleep(1)



except Exception as ex:
    print "Exception = ", ex, " Tracekack:", traceback.format_exc()
    sys.exit(1)

GPIO.cleanup()
