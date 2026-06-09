import serial
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import struct
import time

# --- CAU HINH THONG SO ---
COM_PORT = 'COM7'     # Ban hay thay doi COM_PORT nay cho dung voi tren may ban
BAUD_RATE = 115200
SAMPLES_PER_FRAME = 1000
SAMPLE_RATE_HZ = 40000  # Khóa cứng ở 40 kHz để đồng bộ tuyệt đối với mạch
BYTES_PER_FRAME = SAMPLES_PER_FRAME * 2

try:
    ser = serial.Serial(COM_PORT, BAUD_RATE, timeout=1)
    print(f"Da mo thanh cong cong {COM_PORT}")
except Exception as e:
    print(f"Loi: Khong the mo cong {COM_PORT}. Vui long kiem tra lai.")
    exit()

# Khoi tao do thi
fig, ax = plt.subplots(figsize=(10, 5))
line, = ax.plot([], [], lw=2, color='blue')
display_samples = SAMPLES_PER_FRAME
ax.set_xlim(0, display_samples)
ax.set_ylim(0, 4095) # ADC 12-bit co gia tri tu 0 den 4095
ax.set_title("Oscilloscope - Realtime Waveform")
ax.set_xlabel("Sample Index")
ax.set_ylabel("ADC Value (0-4095)")
ax.grid(True)

# Text de hien thi tan so
freq_text = ax.text(0.02, 0.95, '', transform=ax.transAxes, fontsize=12,
                    verticalalignment='top', bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))

def calculate_frequency(data, fs):
    # Loai bo nhieu neu tin hieu qua phang (vd: song vuong o khung hinh nho)
    if np.max(data) - np.min(data) < 200:
        return 0.0
        
    # Tinh tan so dua tren thuat toan Zero-Crossing voi bo loc trung binh (Moving Average)
    window = np.ones(5) / 5
    smooth_data = np.convolve(data, window, mode='valid')
    mean_val = np.mean(smooth_data)
    
    # Tim cac vi tri ma tin hieu cat qua duong trung binh
    crossings = np.where(np.diff(np.sign(smooth_data - mean_val)))[0]
    
    # Loc bo cac lan cat qua sat nhau (nhieu gai)
    valid_crossings = []
    for c in crossings:
        # Khoang cach giua 2 lan cat phai lon hon 0.1ms
        if len(valid_crossings) == 0 or (c - valid_crossings[-1]) > (fs / 10000): 
            valid_crossings.append(c)
            
    if len(valid_crossings) >= 2:
        num_cycles = (len(valid_crossings) - 1) / 2.0
        time_span = (valid_crossings[-1] - valid_crossings[0]) / fs
        if time_span > 0:
            return num_cycles / time_span
    return 0.0

def read_frame():
    # Doi Marker 0xFFFF (2 byte 0xFF)
    while True:
        b1 = ser.read(1)
        if b1 == b'\xff':
            b2 = ser.read(1)
            if b2 == b'\xff':
                break # Da tim thay marker 0xFFFF

    # Doc 4 bytes sample rate do STM32 gui len
    sr_data = ser.read(4)
    if len(sr_data) < 4:
        return None, None
    sample_rate = struct.unpack('<I', sr_data)[0]

    # Doc 2000 bytes du lieu
    raw_data = ser.read(BYTES_PER_FRAME)
    
    if len(raw_data) == BYTES_PER_FRAME:
        # Chuyen doi raw bytes thanh mang uint16
        data = np.frombuffer(raw_data, dtype=np.uint16)
        return data, sample_rate
    return None, None

def update(frame):
    # Doc 1 frame du lieu tu STM32
    data, sample_rate = read_frame()
    if data is not None and sample_rate > 0:
        # Cap nhat do thi
        line.set_data(range(SAMPLES_PER_FRAME), data)
        
        # Tinh va cap nhat tan so
        freq = calculate_frequency(data, sample_rate)
        freq_text.set_text(f"Frequency: {freq:.2f} Hz\nSample Rate: {sample_rate} Hz")
        
        return line, freq_text
    return line,

print("Dang doc du lieu tu STM32... Tat cua so de thoat.")
print("HUONG DAN: Cuon chuot hoac Click chuot (Trai/Phai) de Zoom (Time/Div)")

# --- THEM TINH NANG ZOOM (TIME/DIV) ---
def zoom(factor):
    global display_samples
    display_samples = int(display_samples * factor)
    
    # Gioi han zoom toi da (10 mau) va toi thieu (1000 mau)
    if display_samples < 10:
        display_samples = 10
    if display_samples > SAMPLES_PER_FRAME:
        display_samples = SAMPLES_PER_FRAME
        
    ax.set_xlim(0, display_samples)
    fig.canvas.draw_idle()

def on_scroll(event):
    if event.button == 'up':
        zoom(0.8) # Cuon len: Phong to (hien thi it mau di)
    elif event.button == 'down':
        zoom(1.25) # Cuon xuong: Thu nho (hien thi nhieu mau hon)

def on_click(event):
    if event.inaxes != ax: return
    if event.button == 1: # Chuot trai
        zoom(0.8)
    elif event.button == 3: # Chuot phai
        zoom(1.25)

fig.canvas.mpl_connect('scroll_event', on_scroll)
fig.canvas.mpl_connect('button_press_event', on_click)

# Chay animation de cap nhat do thi lien tuc
ani = animation.FuncAnimation(fig, update, interval=10, blit=False, cache_frame_data=False)
plt.show()

ser.close()
