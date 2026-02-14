import socket
import struct
import os
import time

# 設定伺服器監聽的 IP 和 Port
# '0.0.0.0' 表示監聽所有網卡 (Wi-Fi, Ethernet 等)
HOST = '0.0.0.0'
PORT = 3333

# 要傳送的檔案名稱
FILE_CONTROL = 'control.dat'
FILE_FRAME = 'frame.dat'

def get_file_data(filename):
    if not os.path.exists(filename):
        print(f"錯誤: 找不到檔案 {filename}")
        # 如果檔案不存在，建立一個假的測試檔案
        with open(filename, 'wb') as f:
            f.write(b'This is a test data for ' + filename.encode())
        print(f"已自動建立測試檔案: {filename}")
    
    with open(filename, 'rb') as f:
        return f.read()

def start_server():
    # 建立 TCP Socket
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    
    # 允許 Port 重複使用 (避免重啟時出現 Address already in use)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    
    try:
        server_socket.bind((HOST, PORT))
        server_socket.listen(5) # 允許等待的連線數
        
        # 顯示本機 IP 供參考
        hostname = socket.gethostname()
        local_ip = socket.gethostbyname(hostname)
        print(f"========================================")
        print(f"TCP Server 啟動中...")
        print(f"監聽 Port: {PORT}")
        print(f"本機 IP (參考用): {local_ip}")
        print(f"請確認 ESP32 的 TCP_SERVER_IP 設定為你的電腦 IP")
        print(f"========================================")

        while True:
            print("等待 ESP32 連線...")
            client_sock, addr = server_socket.accept()
            print(f"連線成功! 來自: {addr}")

            try:
                # 1. 接收 Player ID (ESP32 發送類似 "1\n")
                # 設定接收緩衝區，簡單讀取
                player_id_data = client_sock.recv(1024)
                if not player_id_data:
                    print("未收到數據，斷開連線")
                    client_sock.close()
                    continue
                
                player_id = player_id_data.decode('utf-8').strip()
                print(f"收到 Player ID: {player_id}")

                # 準備要發送的檔案數據
                control_data = get_file_data(FILE_CONTROL)
                frame_data = get_file_data(FILE_FRAME)

                # 2. 發送 control.dat
                # 格式: [4 bytes Size (Big Endian)] + [Data]
                print(f"正在發送 {FILE_CONTROL} ({len(control_data)} bytes)...")
                
                # pack('>I') 表示 Big-Endian Unsigned Int (4 bytes)，對應 ESP32 的 ntohl
                size_header = struct.pack('>I', len(control_data))
                client_sock.sendall(size_header)
                client_sock.sendall(control_data)
                
                # 稍微延遲一下確保 ESP32 處理完 (非必要，但測試時有助穩定)
                time.sleep(0.1)

                # 3. 發送 frame.dat
                print(f"正在發送 {FILE_FRAME} ({len(frame_data)} bytes)...")
                size_header = struct.pack('>I', len(frame_data))
                client_sock.sendall(size_header)
                client_sock.sendall(frame_data)

                print("發送完成，關閉連線")

            except Exception as e:
                print(f"傳輸過程發生錯誤: {e}")
            
            finally:
                client_sock.close()
                print("----------------------------------------")

    except Exception as e:
        print(f"Server 啟動失敗: {e}")
    finally:
        server_socket.close()

if __name__ == '__main__':
    start_server()