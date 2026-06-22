#include "process_mgr.h"
#include "controller.h" // Kết nối với module của Chính để quản lý tiến trình ngầm
#include <iostream>
#include <fstream>
#include <string>

void executeCommand(const char* cmdName, char* argv[], bool isBackground) {
    STARTUPINFOA si;
    PROCESS_INFORMATION pi;

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    // CHUẨN HÓA THAM SỐ: Ghép mảng argv[] thành một chuỗi CommandLine duy nhất cho Windows API
    std::string fullCommandLine = "";
    if (argv != NULL && argv[0] != NULL) {
        int i = 0;
        while (argv[i] != NULL) {
            fullCommandLine += argv[i];
            fullCommandLine += " ";
            i++;
        }
    } else {
        // Nếu bộ Parser của Tuấn chưa truyền argv, tạm thời lấy cmdName làm dòng lệnh
        fullCommandLine = cmdName;
    }

    // Chuyển đổi chuỗi thành mảng char có thể chỉnh sửa (LPSTR) theo yêu cầu của CreateProcessA
    char* lpCommandLine = new char[fullCommandLine.size() + 1];
    strcpy_s(lpCommandLine, fullCommandLine.size() + 1, fullCommandLine.c_str());

    // Gọi Windows API để tạo tiến trình con
    if (!CreateProcessA(
        NULL,               // Ứng dụng thực thi trực tiếp từ CommandLine
        lpCommandLine,      // Toàn bộ dòng lệnh (bao gồm cả tham số)
        NULL,               // Thuộc tính bảo mật tiến trình
        NULL,               // Thuộc tính bảo mật luồng
        FALSE,              // Không kế thừa handle
        0,                  // Cờ tạo tiến trình (mặc định)
        NULL,               // Sử dụng môi trường của cha
        NULL,               // Sử dụng thư mục hiện hành của cha
        &si,                // Thông tin cấu hình cửa sổ ban đầu
        &pi                 // Nhận thông tin handle/PID trả về từ OS
    )) {
        std::cout << "Loi: Khong the thuc thi lenh. Ma loi Windows: " << GetLastError() << std::endl;
        delete[] lpCommandLine;
        return;
    }

    // XỬ LÝ CHẾ ĐỘ FOREGROUND / BACKGROUND THEO ĐỀ BÀI
    if (!isBackground) {
        // 1. Chế độ Foreground: TPCShell đứng đợi tiến trình con chạy xong
        const bool foregroundRegistered =
            setForegroundProcess(pi.dwProcessId, pi.hProcess);
        const DWORD waitResult = WaitForSingleObject(pi.hProcess, INFINITE);

        if (waitResult == WAIT_FAILED) {
            const DWORD errorCode = GetLastError();
            std::cerr << "[TPCShell] Canh bao: khong the cho tien trinh "
                      << "foreground PID " << pi.dwProcessId
                      << " ket thuc. Ma loi Windows: " << errorCode << '\n';
        }

        if (foregroundRegistered) {
            clearForegroundProcess();
        }
        
        // Giải phóng tài nguyên ngay sau khi tiến trình kết thúc
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } 
    else {
        // 2. Chế độ Background: Không chặn Shell, tiến trình chạy ngầm song song
        std::cout << "[TPCShell] Tiến trình chay ngam kich hoat thanh cong. PID: " << pi.dwProcessId << "\n";
        
        // Đẩy PID, Handle và tên lệnh sang cho Chính nạp vào danh sách quản lý
        addBackgroundProcess(pi.dwProcessId, pi.hProcess, cmdName);
        
        // Đóng handle luồng, giữ lại handle tiến trình (hProcess) để Chính có thể Kill/Stop sau này
        CloseHandle(pi.hThread);
    }

    // Giải phóng bộ nhớ đệm
    delete[] lpCommandLine;
}

void executeBatchFile(const char* filePath) {
    std::ifstream file(filePath);
    
    if (!file.is_open()) {
        std::cout << "Loi: Khong the mo file script '" << filePath << "'\n";
        return;
    }

    std::cout << "[TPCShell] Bat dau thuc thi file script: " << filePath << "\n";
    std::string line;
    
    // Đọc từng dòng trong file batch
    while (std::getline(file, line)) {
        // Bỏ qua dòng trống hoặc dòng chú thích
        if (line.empty() || line[0] == '#' || line.rfind("rem", 0) == 0) {
            continue;
        }

        std::cout << "-> Run: " << line << "\n";

        // Mặc định chạy các lệnh trong file script ở chế độ Foreground nối đuôi nhau
        executeCommand(line.c_str(), NULL, false); 
    }

    file.close();
    std::cout << "[TPCShell] Hoan thanh thuc thi file script.\n";
}