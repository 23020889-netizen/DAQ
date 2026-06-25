import matplotlib.pyplot as plt
import csv
import sys
import os

# Cấu hình encoding cho terminal để tránh lỗi in tiếng Việt trên Windows
sys.stdout.reconfigure(encoding='utf-8')

# Đường dẫn mặc định đến file data.csv (nằm cùng thư mục)
file_path = 'data.csv'

if not os.path.exists(file_path):
    print(f"Lỗi: Không tìm thấy file '{file_path}'.")
    print("Vui lòng bật chế độ SAVE: ON trên STM32 và chạy oscilloscope.py trước để tạo dữ liệu.")
    sys.exit()

times = []
voltages = []

print(f"Đang đọc dữ liệu từ {file_path}...")
with open(file_path, 'r') as csvfile:
    reader = csv.reader(csvfile)
    header = next(reader) # Bỏ qua dòng tiêu đề
    
    for row in reader:
        if len(row) >= 2:
            try:
                t = float(row[0])
                v = float(row[1])
                times.append(t)
                voltages.append(v)
            except ValueError:
                continue

if not times:
    print("File CSV trống hoặc không có dữ liệu hợp lệ.")
    sys.exit()

print(f"Đã nạp thành công {len(times)} mẫu dữ liệu.")

max_v = max(voltages)
min_v = min(voltages)
print(f"Điện áp lớn nhất: {max_v:.2f} V")
print(f"Điện áp nhỏ nhất: {min_v:.2f} V")

# Vẽ đồ thị
fig, ax = plt.subplots(figsize=(12, 6))
ax.plot(times, voltages, label='Voltage', color='blue', linewidth=1.5)

ax.set_title('Dữ liệu thu thập từ Voltmeter (Analog Input)', fontsize=14)
ax.set_xlabel('Thời gian (s)', fontsize=12)
ax.set_ylabel('Điện áp (V)', fontsize=12)

# Giới hạn trục Y từ 0 đến 3.5V để dễ nhìn
ax.set_ylim(0, 3.5)

ax.grid(True, linestyle='--', alpha=0.7)
ax.legend()

# --- TÍNH NĂNG ZOOM TIME/DIV BẰNG CHUỘT ---
def zoom(factor, event):
    xlim = ax.get_xlim()
    # Nếu chuột không nằm trong đồ thị, lấy điểm giữa trục X làm tâm zoom
    if getattr(event, 'xdata', None) is not None:
        mouse_x = event.xdata
    else:
        mouse_x = (xlim[0] + xlim[1]) / 2

    left_dist = mouse_x - xlim[0]
    right_dist = xlim[1] - mouse_x

    new_left = mouse_x - left_dist * factor
    new_right = mouse_x + right_dist * factor
    
    # Không zoom ra xa hơn dữ liệu gốc
    if new_left < times[0]: new_left = times[0]
    if new_right > times[-1]: new_right = times[-1]
    
    # Chống zoom quá nhỏ (gây crash hoặc lỗi scale)
    if new_right - new_left < 1e-6:
        return

    ax.set_xlim(new_left, new_right)
    fig.canvas.draw_idle()

def on_scroll(event):
    if event.button == 'up':
        zoom(0.8, event) # Cuộn lên: Phóng to
    elif event.button == 'down':
        zoom(1.25, event) # Cuộn xuống: Thu nhỏ

def on_click(event):
    if event.inaxes != ax: return
    if event.button == 1: # Chuột trái
        zoom(0.8, event)
    elif event.button == 3: # Chuột phải
        zoom(1.25, event)

fig.canvas.mpl_connect('scroll_event', on_scroll)
fig.canvas.mpl_connect('button_press_event', on_click)

print("Đang vẽ đồ thị... Hãy cuộn chuột (hoặc Click Trái/Phải) để Zoom ngang (Time/Div) quanh vị trí trỏ chuột.")
plt.show()
