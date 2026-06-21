# ESP32 烧录常见问题

## 1. 串口被占用 (Resource busy)

### 错误信息
```
A fatal error occurred: Could not open /dev/tty.usbmodem1301, the port is busy or doesn't exist.
([Errno 16] could not open port /dev/tty.usbmodem1301: [Errno 16] Resource busy: '/dev/tty.usbmodem1301')
```

### 原因
`idf.py monitor` 或其他串口监控工具正在运行，占用了串口设备。

### 解决方法
```bash
# 查找占用串口的进程
lsof /dev/tty.usbmodem1301

# 或者查找 monitor 进程
ps aux | grep "idf.py monitor" | grep -v grep

# 杀掉相关进程
pkill -f "idf.py monitor"
# 或指定 PID
kill <PID>
```

### 预防
- 烧录前先关闭 monitor 终端窗口
- 使用 `idf.py flash monitor` 可以自动完成烧录后监控，避免手动切换

---

## 2. Python 环境不一致

### 错误信息
```
WARNING: The IDF_PYTHON_ENV_PATH is missing in environmental variables!
'/Users/xweng/.espressif/tools/python/v5.3.5/venv/bin/python3' is currently active in the environment while the project was configured with '/Users/xweng/.espressif/python_env/idf5.3_py3.14_env/bin/python'.
```

### 原因
- 构建时使用的 Python 环境与烧录时不一致
- 缺少 `IDF_PYTHON_ENV_PATH` 环境变量

### 解决方法
在 `~/.zshrc` 中添加 ESP-IDF 环境变量：
```bash
# ESP-IDF Environment Variables
export IDF_PATH="$HOME/.espressif/v5.3.5/esp-idf"
export IDF_PYTHON_ENV_PATH="$HOME/.espressif/python_env/idf5.3_py3.14_env"
export PATH="$IDF_PYTHON_ENV_PATH/bin:$PATH"
```

然后执行 `source ~/.zshrc` 使配置生效。

---

## 3. 烧录文件路径错误

### 错误信息
```
esptool write_flash: error: argument <address> <filename>: [Errno 2] No such file or directory: 'bootloader/bootloader.bin'
```

### 原因
手动执行 esptool 时，工作目录不在 `build` 目录下，导致相对路径找不到文件。

### 解决方法
```bash
# 确保在 build 目录下执行
cd build
esptool.py --chip esp32s3 -p /dev/tty.usbmodem1301 write_flash ...

# 或者使用 idf.py（推荐，自动处理路径）
idf.py -p /dev/tty.usbmodem1301 flash
```
