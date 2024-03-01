import os, sys, time

sys.path.append('')
sys.path.append('.')

# chdir to "/sd" or "/flash"
devices = os.listdir("/")
if "sd" in devices:
    os.chdir("/sd")
    sys.path.append('/sd')
else:
    os.chdir("/flash")
sys.path.append('/flash')
del devices

print("[CanMV] init end") # for IDE
for i in range(200):
    time.sleep_ms(1) # wait for key interrupt(for canmv ide)
del i

# check IDE mode
ide_mode_conf = "/flash/ide_mode.conf"
ide = True
try:
    f = open(ide_mode_conf)
    f.close()
    del f
except Exception:
    ide = False

if ide:
    os.remove(ide_mode_conf)
    from machine import UART
    import lcd
    lcd.init(color=lcd.PINK)
    repl = UART.repl_uart()
    repl.init(1500000, 8, None, 1, read_buf_len=2048, ide=True, from_ide=False)
    sys.exit()
del ide, ide_mode_conf

# detect boot.py
main_py = '''
try:
    import gc, lcd, image
    gc.collect()
    lcd.init()
    loading = image.Image(size=(lcd.width(), lcd.height()))
    loading.draw_rectangle((0, 0, lcd.width(), lcd.height()), fill=True, color=(0, 81, 137))
    info = "Welcome to CanMV"
    loading.draw_string(int(lcd.width()//2 - len(info) * 5), (lcd.height())//4, info, color=(255, 255, 255), scale=2, mono_space=0)
    v = sys.implementation.version
    vers = 'V{}.{}.{} : canaan-creative.com'.format(v[0],v[1],v[2])
    loading.draw_string(int(lcd.width()//2 - len(info) * 6), (lcd.height())//3 + 20, vers, color=(255, 255, 255), scale=1, mono_space=1)
    lcd.display(loading)
    del loading, v, info, vers
    gc.collect()
finally:
    gc.collect()
'''

flash_ls = os.listdir()
if not "main.py" in flash_ls:
    f = open("main.py", "wb")
    f.write(main_py)
    f.close()
    del f
del main_py

flash_ls = os.listdir("/flash")
try:
    sd_ls = os.listdir("/sd")
except Exception:
    sd_ls = []
if "cover.boot.py" in sd_ls:
    code0 = ""
    if "boot.py" in flash_ls:
        with open("/flash/boot.py") as f:
            code0 = f.read()
    with open("/sd/cover.boot.py") as f:
        code=f.read()
    if code0 != code:
        with open("/flash/boot.py", "w") as f:
            f.write(code)
        import machine
        machine.reset()

if "cover.main.py" in sd_ls:
    code0 = ""
    if "main.py" in flash_ls:
        with open("/flash/main.py") as f:
            code0 = f.read()
    with open("/sd/cover.main.py") as f:
        code = f.read()
    if code0 != code:
        with open("/flash/main.py", "w") as f:
            f.write(code)
        import machine
        machine.reset()

try:
    del flash_ls
    del sd_ls
    del code0
    del code
except Exception:
    pass

banner = '''
  _____            __  ____      __
 / ____|          |  \/  \ \    / /
| |     __ _ _ __ | \  / |\ \  / / 
| |    / _` | '_ \| |\/| | \ \/ /  
| |___| (_| | | | | |  | |  \  /   
 \_____\__,_|_| |_|_|  |_|   \/    

Official Site : https://canaan-creative.com
'''
print(banner)
del banner

import json

config = {
"type": "ALIENTEK",
"board_name": "DNK210",
"lcd": {
    "rst" : 37,
    "dcx" : 38,
    "ss" : 36,
    "clk" : 39,
    "height": 240,
    "width": 320,
    "invert": 1,
    "offset_x1": 0,
    "offset_y1": 0,
    "offset_x2": 0,
    "offset_y2": 0,
    "dir": 96
},
"freq_cpu": 416000000,
"freq_pll1": 400000000,
"kpu_div": 1,
"sdcard":{
    "sclk":27,
    "mosi":28,
    "miso":26,
    "cs":29
},
"board_info": {
    # Core
    "BOOT_KEY":     16,
    # LCD
    "LCD_BL":       35,
    # Button
    "KEY0":         18,
    "KEY1":         19,
    "KEY2":         16,
    # LED
    "LEDR":         24,
    "LEDB":         25,
    # IMU
    "IMU_SCL":      22,
    "IMU_SDA":      23,
    "IMU_INT":      20,
    # Buzzer
    "BEEP":         17,
    # Speaker
    "SPK_WS":       33,
    "SPK_SCLK":     32,
    "SPK_SDOUT":    31,
    "SPK_CTRL":     21,
    # Microphone
    "MIC_WS":       33,
    "MIC_SCLK":     32,
    "MIC_SDIN":     30,
    # Extended UART
    "EX_UART1_TX":  7,
    "EX_UART1_RX":  9,
    "EX_UART2_TX":  6,
    "EX_UART2_RX":  8,
    # Extended I/O
    "EX_IO0":       14,
    "EX_IO1":       15,
    "EX_IO2":       12,
    "EX_IO3":       13,
    "EX_IO4":       10,
    "EX_IO5":       11,
}
}

cfg = json.dumps(config)
print(cfg)

try:
    with open('/flash/config.json', 'rb') as f:
        tmp = json.loads(f.read())
    # print(tmp)
    if tmp["type"] != config["type"]:
        raise Exception('config.json no exist')
except Exception as e:
    with open('/flash/config.json', "w") as f:
        f.write(cfg)
    import machine
    machine.reset()
