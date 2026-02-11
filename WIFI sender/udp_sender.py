import socket
import struct
import os
import time

PACKET_TYPE_CONTROL = 0x01  
PACKET_TYPE_FRAME   = 0x02  

class PatternSender:
    def __init__(self, target_ip, target_port):
        self.target_ip = target_ip
        self.target_port = target_port
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        

        self.of_num = 0
        self.led_num = 0
        self.frame_size = 0

    def parse_control_config(self, control_path): #解析 control.dat 顯示資訊 (確認用)
        if not os.path.exists(control_path):
            print(f"錯誤: 找不到檔案 {control_path}")
            return

        with open(control_path, "rb") as f:
            data = f.read()
        
        # 確保資料長度足夠解析 Header
        if len(data) < 50:
            print("警告: control.dat 檔案過小，無法解析完整 Header")
            return

        # 解析版本 (Offset 0, 1)
        major, minor = struct.unpack("<BB", data[0:2])
        
        # 計算 OF 數
        of_channels = struct.unpack("<" + "B"*40, data[2:42])
        self.of_num = sum(of_channels)
        
        # 計算總 LED 數
        strip_led_counts = struct.unpack("<" + "B"*8, data[42:50])
        self.led_num = sum(strip_led_counts)
        
        # 根據規格計算 frame_size
        self.frame_size = 4 + 1 + (self.of_num * 3) + (self.led_num * 3) + 4
        
        print(f"--- 配置解析 ---")
        print(f"版本: {major}.{minor}")
        print(f"OF 數量: {self.of_num}")
        print(f"LED 總數: {self.led_num}")
        print(f"單幀大小: {self.frame_size} bytes")
        print(f"----------------")

    def _send_packet_chunk(self, packet_type, payload):
        """
        底層發送函式：封裝 [Type] + [Payload] 並發送
        """
        # 封包結構: [1 byte Type] + [N bytes Data]
        # pack("B") 把整數轉為 1 byte unsigned char
        header = struct.pack("B", packet_type)
        packet = header + payload
        
        self.sock.sendto(packet, (self.target_ip, self.target_port))

    def send_file_in_chunks(self, filepath, packet_type):
        """
        通用檔案傳送函式：負責將檔案切片並發送
        """
        # 設定每個 UDP 封包的資料酬載大小 (Payload Size)
        CHUNK_SIZE = 1024 
        
        if not os.path.exists(filepath):
            print(f"錯誤: 找不到檔案 {filepath}")
            return

        file_size = os.path.getsize(filepath)
        sent_bytes = 0
        
        print(f"開始傳送 {filepath} (Type: 0x{packet_type:02x})...")

        with open(filepath, "rb") as f:

            while True:
                chunk = f.read(CHUNK_SIZE)
                if not chunk:
                    break
                
                # 發送切片
                self._send_packet_chunk(packet_type, chunk)
                sent_bytes += len(chunk)

                # 重要：流量控制
                time.sleep(0.002) 

        print(f"\n{filepath} 傳送完成！共發送 {sent_bytes} bytes 資料")

    def send_pattern(self, control_path, frame_path):
        """
        主流程：依序傳送 control 和 frame
        """
        # 1. 傳送 control.dat
        # Packet Type: 0x01 (PACKET_TYPE_CONTROL)
        self.send_file_in_chunks(control_path, PACKET_TYPE_CONTROL)
        
        # 稍微等待，確保 ESP32 寫入完成並關閉 control.dat
        time.sleep(0.5)

        # 2. 傳送 frame.dat
        # Packet Type: 0x02 (PACKET_TYPE_FRAME)
        self.send_file_in_chunks(frame_path, PACKET_TYPE_FRAME)

if __name__ == "__main__":
    # 設定 ESP32 的 IP 與 Port
    TARGET_IP = "192.168.0.128" # 請確認你的 ESP32 IP
    TARGET_PORT = 3333

    sender = PatternSender(TARGET_IP, TARGET_PORT)
    
    try:
        # 1. 解析並顯示資訊 (選用)
        sender.parse_control_config("control.dat")
        
        # 2. 開始傳送流程
        # 確保當前目錄有這兩個檔案
        sender.send_pattern("control.dat", "frame.dat")
        
    except KeyboardInterrupt:
        print("\n傳送已手動中止")
    except Exception as e:
        print(f"\n發生錯誤: {e}")