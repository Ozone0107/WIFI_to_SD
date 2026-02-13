import socket
import struct
import os
import time

# --- 設定區 ---
ESP_IP = "192.168.0.238"  # 請確認 ESP32 的 IP
ESP_PORT = 3333        # TCP Port (需與 ESP32 一致)
CONTROL_FILE = "control.dat"
FRAME_FILE = "frame.dat"

# 協定定義 (跟原本一樣)
PACKET_TYPE_CONTROL = 0x01
PACKET_TYPE_FRAME   = 0x02

class TcpSender:
    def __init__(self, ip, port):
        self.ip = ip
        self.port = port
        self.sock = None

    def connect(self):
        """建立 TCP 連線"""
        print(f"正在連線到 TCP Server {self.ip}:{self.port} ...")
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.sock.settimeout(5) # 設定 5 秒連線超時
        try:
            self.sock.connect((self.ip, self.port))
            print("連線成功！")
            return True
        except Exception as e:
            print(f"連線失敗: {e}")
            return False

    def send_file(self, file_path, packet_type, skip_bytes=0):
        """發送檔案：先送 Header 告訴長度，再送內容"""
        if not os.path.exists(file_path):
            print(f"錯誤：找不到檔案 {file_path}")
            return

        # 1. 取得檔案大小
        file_size = os.path.getsize(file_path) - skip_bytes
        if file_size < 0: file_size = 0

        print(f"準備發送 {file_path} (Type: {packet_type}, Size: {file_size} bytes)...")

        try:
            # 2. 發送檔頭 (Header)
            # 格式: [Type (1 byte)] + [Size (4 bytes, Big-Endian)]
            # Big-Endian (>) 是網路傳輸標準
            header = struct.pack('>BI', packet_type, file_size)
            self.sock.sendall(header)

            # 3. 發送檔案內容 (Body)
            with open(file_path, 'rb') as f:
                f.seek(skip_bytes) # 跳過檔頭 (如果有的話)
                
                sent_total = 0
                buffer_size = 4096 # 每次讀 4KB (TCP 會自動分包，這裡只是讀檔緩衝)
                
                while True:
                    data = f.read(buffer_size)
                    if not data:
                        break
                    self.sock.sendall(data) # sendall 保證資料送完
                    sent_total += len(data)
                    # 這裡可以印進度條，但不需要 sleep

            print(f"發送完成：{file_path} (共 {sent_total} bytes)")

        except Exception as e:
            print(f"發送過程中發生錯誤: {e}")

    def close(self):
        if self.sock:
            self.sock.close()
            print("連線已關閉。")

if __name__ == "__main__":
    sender = TcpSender(ESP_IP, ESP_PORT)

    if sender.connect():
        # 1. 傳送 control.dat
        # 這裡假設 control.dat 不需要跳過檔頭
        sender.send_file(CONTROL_FILE, PACKET_TYPE_CONTROL, skip_bytes=0)

        # 稍微等一下，確保 ESP32 處理完上一個檔案的邏輯 (非必要，但保險)
        time.sleep(0.5)

        # 2. 傳送 frame.dat
        # 這裡保留你之前的設定：如果需要跳過 2 bytes 版本號，把 0 改成 2
        sender.send_file(FRAME_FILE, PACKET_TYPE_FRAME, skip_bytes=0)
        
        sender.close()