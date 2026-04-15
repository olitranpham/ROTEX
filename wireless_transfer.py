import network
import socket
import time

# Wi-Fi connection
SSID = "PicoHotspot"
PWD  = "pico1234"

# Laptop IP & port
DEST_IP = "172.20.10.6"
DEST_PORT = 5005

wlan = network.WLAN(network.STA_IF)
wlan.active(True)

print("Connecting to WiFi:", SSID)
wlan.connect(SSID, PWD)

# wait up to 10 seconds for WiFi
t0 = time.time()
while not wlan.isconnected() and (time.time() - t0) < 10:
    print(".", end="")
    time.sleep(0.5)

print() 

if not wlan.isconnected():
    print("❌ WiFi connect FAILED")
else:
    print("✅ Connected:", wlan.ifconfig())

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    t = 0.0
    while True:
        label = "imu1"
        w, x, y, z = 0.1, 0.2, 0.3, 0.4
        msg = f"{t},{label},{w},{x},{y},{z}\n"
        sock.sendto(msg.encode(), (DEST_IP, DEST_PORT))
        # debug print so you can see it sending
        print("sent:", msg.strip())
        t += 0.01
        time.sleep(0.01)
