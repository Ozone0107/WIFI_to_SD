import socket
import time

# --- 設定區 ---
# 請替換成你 ESP32 剛連上 WiFi 後，Serial Monitor 印出來的 IP
ESP_IP = "192.168.0.132" 
PORT = 3333              # 對應你 C 程式碼中的 CONFIG_EXAMPLE_PORT

# 要發送的訊息
MESSAGE = b"Hello ESP32 RingBuffer!" 
# ----------------

print(f"開始發送 UDP 封包到 {ESP_IP}:{PORT} ...")

# 建立 UDP Socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

try:
    count = 0
    while True:
        # 發送資料
        msg = f"Packet {count}: ".encode() + MESSAGE
        sock.sendto(msg, (ESP_IP, PORT))
        
        print(f"已發送: {msg}")
        
        count += 1
        time.sleep(0.5) # 每 0.5 秒發送一次 (你可以調快來測試 Buffer 極限)

except KeyboardInterrupt:
    print("\n停止發送。")
    sock.close()