import os

# 產生 5MB 的隨機數據 (5 * 1024 * 1024 bytes)
size = 5 * 1024 * 1024 
filename = "frame_5mb.dat"

with open(filename, "wb") as f:
    f.write(os.urandom(size))

print(f"已生成 {filename}, 大小: {size/1024/1024} MB")