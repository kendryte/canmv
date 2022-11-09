from maix import FPIOA
from fpioa_manager import fm

fpioa = FPIOA()
fpioa.set_function(0, fm.fpioa.GPIOHS0)
pin = fpioa.get_Pin_num(fm.fpioa.GPIOHS0)
if pin == 0:
    print("set function ok")
print("Done.")
