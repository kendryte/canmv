import sensor, image, lcd

lcd.init()                          # 初始化屏幕显示
lcd.clear(lcd.RED)                  # 将屏幕清空，使用红色填充

sensor.reset()                      # 复位并初始化摄像头
sensor.set_pixformat(sensor.RGB565) # 设置摄像头输出格式为 RGB565（也可以是GRAYSCALE）
sensor.set_framesize(sensor.QVGA)   # 设置摄像头输出大小为 QVGA (320x240)
sensor.skip_frames(time = 2000)     # 跳过2000帧

img = sensor.snapshot()         # 拍照，获取一张图像
lcd.display(img)                # 在屏幕上显示图像
print("Done.")