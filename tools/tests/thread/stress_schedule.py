# This test ensures that the scheduler doesn't trigger any assertions
# while dealing with concurrent access from multiple threads.

import _thread
import utime
import micropython
import gc

try:
    micropython.schedule
except AttributeError:
    print("SKIP")
    raise SystemExit

gc.disable()

_NUM_TASKS = 10000
_TIMEOUT_MS = 10000

n = 0  # How many times the task successfully ran.
t = None  # Start time of test, assigned here to preallocate entry in globals dict.


def task(x):
    global n
    n += 1


def thread():
    while True:
        try:
            micropython.schedule(task, None)
        except RuntimeError:
            # Queue full, back off.
            utime.sleep_ms(10)


for i in range(8):
    _thread.start_new_thread(thread, ())

# Wait up to 10 seconds for 10000 tasks to be scheduled.
t = utime.ticks_ms()
while n < _NUM_TASKS and utime.ticks_diff(utime.ticks_ms(), t) < _TIMEOUT_MS:
    pass

if n < _NUM_TASKS:
    # Not all the tasks were scheduled, likely the scheduler stopped working.
    print(n)
else:
    print("PASS")
