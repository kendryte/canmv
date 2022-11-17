import time
from machine import Timer

def on_timer(timer):
    print("on timer cb")

arg = 1

tim = Timer(Timer.TIMER0, Timer.CHANNEL0, mode=Timer.MODE_PERIODIC, period=500, unit=Timer.UNIT_MS, callback=on_timer, arg=arg, start=False, priority=1, div=0)
print("period:",tim.period())
tim.start()
time.sleep(1)
tim.stop()
time.sleep(1)
tim.restart()
time.sleep(1)
tim.stop()
del tim

print("Done.")
