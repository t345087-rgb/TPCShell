# Dự án Nguyên lý Hệ điều hành: TPCShell (Windows API)

## 👥 Thành viên nhóm & Phân chia công việc

1. **Tuấn Anh / Tuấn (Leader)**
   - Quản lý chung dự án và điều phối tiến độ.
   - Thiết kế kiến trúc chương trình và quản lý tích hợp hệ thống.
   - Cài đặt REPL chính: quản lý vòng lặp `TPCShell>`.
   - **Parser:** Tách chuỗi lệnh, bóc tách tham số (`arguments`), nhận biết chế độ chạy.
   - **Built-in commands cơ bản:** `help`, `exit`, `date`, `time`, `dir`, `cd`.
   - **Quản lý biến môi trường:** `path`, `addpath`, `delpath`.

2. **Phương**
   - **Core Process Execution:** Nghiên cứu và hiện thực hóa cấu trúc gọi tiến trình.
   - Cài đặt chạy chương trình bên ngoài bằng Windows API `CreateProcess`.
   - **Foreground mode:** Đồng bộ hệ thống, ép shell đợi tiến trình con kết thúc.
   - **Background mode:** Xử lý bất đồng bộ, cho shell chạy song song với tiến trình con.
   - Lưu thông tin và chuyển giao dữ liệu tiến trình ngầm sau khi khởi tạo thành công.
   - Hỗ trợ đọc và thực thi chuỗi lệnh tự động từ file kịch bản `.bat`.

3. **Chính**
   - **Process Controller:** Quản lý và giám sát vòng đời các tiến trình ngầm.
   - Cài đặt lệnh `list`: Truy vấn và hiển thị danh sách các background processes hiện hành.
   - Cài đặt lệnh `kill`: Kết thúc một background process từ xa thông qua PID.
   - Cài đặt lệnh `stop`: Tạm dừng (đóng băng) luồng hoạt động của background process.
   - Cài đặt lệnh `resume`: Kích hoạt cho background process tiếp tục chạy.
   - **Signal Handler:** Xử lý cô lập tín hiệu `Ctrl+C` để hủy foreground process mà không làm tắt shell.

---

## 🛠️ Môi trường phát triển

- **Ngôn ngữ:** C/C++
- **Công cụ:** Visual Studio / Visual Studio Code
- **Nền tảng:** Windows OS
- **Thư viện / API chính:** Windows API / Windows SDK (`<windows.h>`)

---

## ✅ Tiến độ chức năng cần cài đặt (Checklist)

### 1. Core Shell & Giao diện (Tuấn)
- [ ] Hiển thị prompt `TPCShell>` liên tục qua vòng lặp REPL
- [ ] Đọc dữ liệu chuỗi lệnh đầu vào từ bàn phím
- [ ] Tách chuỗi lệnh thành tên ứng dụng và mảng tham số (`argv`)
- [ ] Nhận biết ký tự đặc biệt hoặc cờ chạy ngầm `&`

### 2. Built-in Commands & Môi trường (Tuấn)
- [ ] `help`, `exit`
- [ ] `date`, `time`
- [ ] `dir`, `cd`
- [ ] `path`, `addpath`, `delpath`

### 3. Thực thi tiến trình (Phương)
- [x] Kích hoạt ứng dụng ngoài bằng `CreateProcessA`
- [x] **Foreground mode:** Chặn dòng lệnh bằng `WaitForSingleObject`
- [x] **Background mode:** Tạo tiến trình song song không chặn dòng lệnh
- [x] Đọc file cấu trúc kịch bản và chạy nối đuôi tuần tự file `*.bat`

### 4. Quản lý tiến trình ngầm (Chính)
- [ ] Thiết lập cấu trúc dữ liệu lưu trữ danh sách Background Process
- [ ] Lệnh `list` in bảng danh sách chi tiết (PID, Name, Status)
- [ ] Lệnh `kill <PID>` sử dụng `TerminateProcess`
- [ ] Lệnh `stop <PID>` và `resume <PID>` bằng cơ chế đóng băng/nhả luồng

### 5. Xử lý Tín hiệu (Chính)
- [ ] Đăng ký hàm bắt sự kiện bằng `SetConsoleCtrlHandler`
- [ ] Cô lập `Ctrl+C` chỉ tác động đến tiến trình Foreground, giữ an toàn cho Shell