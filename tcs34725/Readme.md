# TCS34725 Kernel Driver for Linux
Mai Minh Đức -      22146296
Nguyễn Hoàng Danh - 22146281
Mai Thanh Bình -    22146272

## 📝 1.Giới thiệu và ứng dụng của TCS34725

Đây là một Linux kernel driver dành cho cảm biến phát hiện màu sắc **TCS34725**, giao tiếp qua phương thức **I2C**. Sau khi nạp driver, thiết bị `/dev/tcs34725` sẽ được tạo ra, cho phép người dùng đọc giá trị màu **Clear**, **Red**, **Green** và **Blue** thông qua các lệnh `ioctl`.

- Dùng để nhận diện, nhận dạng màu sắc trong các hệ thống sản phẩm
- Robot tránh vạch hoặc nhận diện các vạch màu
- Điều chỉnh ánh sáng tự đông dựa vào ánh sáng môi trường xung quanh
- Cảm biến môi trường đối với các thiết bị IoT
---

## 🔧 2. Cài đặt cho TCS34725

### 2.1. Chuẩn bị hệ thống
- Raspberry Pi 5 model B 
- Hệ điều hành: Raspberry Pi OS hoặc Linux hỗ trợ I2C
- Đã bật I2C (trên RPi: dùng  câu lệnh `sudo raspi-config`)
- Cảm biến màu sắc TCS34725
- Dây kết nối TCS34725 với Raspberry

    Dùng câu lệnh sau để cập nhật Raspberry Pi 5
    ```bash
   `sudo apt update
    sudo apt install build-essential raspberrypi-kernel-headers i2c-tools`

### 2.2. Hướng dẫn kết nối với chân (sử dụng bộ I2C1)
|    TCS34725   |     Pi 5      |   Physical Pin   |
|---------------|---------------|------------------|
|      VIN      |      3.3V     |  Pin 1 / Pin 17  |
|      GND      |      0V       |  Pin 6 / Pin 9   |
|      SDA      |      SDA 1    |      Pin 3       |
|      SCL      |      SCL 1    |      Pin 5       |

- Đảm bảo hệ thống nhận diện cảm biến:
    Kiểm tra bằng câu lệnh ` i2cdetect -y 1 `
    Kết quả sẽ hiển thị địa chỉ 0x29 nếu TCS34725 được kết nối đúng.

### 2.3. Cập nhật Device Tree cho file bcm2712-rpi-5-b.dtb (chỉ đối với bản Raspberry Pi5 bản lưu hành thị trường)
1. Mở terminal và truy cập vào thư mục boot hoặc thư mục boot/firmware (dùng câu lệnh `cd /boot` hoặc `cd /boot/firmware`)
2. Dùng câu lệnh `sudo dtc -I dtb -O dts -o bcm2712-rpi-5-b.dts bcm2712-rpi-5-b.dtb` để chuyển đổi file bcm2712-rpi-5-b.dtb sang đuôi .dts
3. Truy cập vào file .dts để chỉnh sửa bằng câu lệnh `sudo nano bcm2712-rpi-5-b.dts`
4. Sử dụng tổ hợp phím `Ctrl + W` tìm kiếm aliases
5. Kiếm đến dòng chứa cụm i2c1 (ví dụ: i2c1@74000) dùng `Ctrl + Shift + C` để copy cụm từ và tìm kiếm 1 lần nữa
6. Ở dòng cuối cùng của hàm i2c1@174000, thêm những dòng sau:
            tcs34725@29 {
                compatible = "taos,tcs34725";
                reg = <0x29>;
            };
7. Lưu lại file bằng cách `Ctrl + O và Enter` và Thoát ra bằng `Ctrl + X`
8. Chuyển đổi lại file bcm2712-rpi-5-b.dts về  bcm2712-rpi-5-b.dtb sử dụng câu lệnh `sudo dtc -I dts -O dtb -o bcm2712-rpi-5-b.dtb bcm2712-rpi-5-b.dts`
9. Sử dụng câu lệnh reboot để  khởi động lại Raspberry Pi và đợi cho máy kết nối lại

### 2.4. Hướng dẫn cài đặt driver Kernel TCS34725 cho Raspberry Pi
1. Đảm bảo kết nối giữa cảm biến TCS34725 với Raspberry Pi 5
2. Tạo 1 Makeflie với nội dung như sau:
        obj-m += tcs34725_ioctl.o
        KDIR = /lib/modules/$(shell uname -r)/build

        all:
            make -C $(KDIR) M=$(shell pwd) modules
        clean: 
            make -C $(KDIR) M=$(shell pwd) clean
3. Tại thư mục chứa file driver và Makefile vừa tạo, sử dụng câu lệnh `make`
4. Tiếp theo cài đặt driver bằng câu lệnh `sudo insmod tcs34725_ioctl.ko`
    Kiểm tra:
        dmesg | grep TCS34725

        Nếu thành công, bạn sẽ thấy dòng như:

        TCS34725: Driver loaded, device /dev/tcs34725 created

5. Khi không sử dụng driver nữa sử dụng câu lệnh ` sudo rmmod tcs34725_ioctl`, sau đó dùng câu lệnh `make clean` để  xóa các file đã được biên dịch từ `make`

## 3. Hướng dẫn sử dụng các câu lệnh trong driver
### 3.1.  ⚙️ Sử dụng câu lệnh

    Thiết bị /dev/tcs34725 có thể được truy cập thông qua các lệnh ioctl sau:
    Lệnh IOCTL	                    Mã số	                     Mô tả
    TCS34725_IOCTL_READ_CLEAR	_IOR('t', 1, int)	    Đọc giá trị ánh sáng tổng (Clear)
    TCS34725_IOCTL_READ_RED	    _IOR('t', 2, int)	    Đọc giá trị màu Đỏ
    TCS34725_IOCTL_READ_GREEN	_IOR('t', 3, int)	    Đọc giá trị màu Xanh lá
    TCS34725_IOCTL_READ_BLUE	_IOR('t', 4, int)	    Đọc giá trị màu Xanh dương

### 3.2. Hướng dẫn tạo code để kiểm tra và đọc giá trị trả về của cảm biến TCS34725
1. Nạp vào các thư viện cần thiết như :
    #include <stdio.h>          // printf, perror, fflush
    #include <fcntl.h>          // open, O_RDONLY
    #include <unistd.h>         // close, usleep
    #include <sys/ioctl.h>      // ioctl
    #include <signal.h>         // signal, sig_atomic_t
    #include <stdlib.h>         // exit
2. Định nghĩa các macro ioctl 
    #define TCS34725_IOCTL_MAGIC 't'
    #define TCS34725_IOCTL_READ_CLEAR _IOR(TCS34725_IOCTL_MAGIC, 1, int)
    #define TCS34725_IOCTL_READ_RED   _IOR(TCS34725_IOCTL_MAGIC, 2, int)
    #define TCS34725_IOCTL_READ_GREEN _IOR(TCS34725_IOCTL_MAGIC, 3, int)
    #define TCS34725_IOCTL_READ_BLUE  _IOR(TCS34725_IOCTL_MAGIC, 4, int)

TCS34725_IOCTL_MAGIC là mã định danh dành riêng cho thiết bị này (thường là một ký tự).
_IOR() là macro định nghĩa lệnh ioctl để đọc dữ liệu từ kernel driver vào user-space

3. Xử lý tín hiệu ngắt
    Dùng để dừng vòng lặp khi người dùng nhấn `Ctrl + C`
        volatile sig_atomic_t keep_running = 1;
    Hàm xử  lý tín hiệu ngắt 
        void handle_sigint(int sig) {
        keep_running = 0;
    }
    Khi nhấn Ctrl+C, hệ điều hành gửi tín hiệu SIGINT. Hàm này sẽ gán keep_running = 0 để thoát vòng lặp chính
4. Hàm main()
    -------------------------------------------
    int fd = open("/dev/tcs34725", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open device");
        return 1;
    }
    Mở file thiết bị /dev/tcs34725 để đọc dữ liệu từ driver đã cài đặt
    Khi không mở được thiết bị sẽ có thông báo lỗi
    -------------------------------------------
    signal(SIGINT, handle_sigint);  // Ctrl+C cleanup
    Gắn hàm xử lý  SIGINT với handle_sigint()
    -------------------------------------------
    int clear, red, green, blue;
    Lưu các giá trị cảm biến
    ------------------------------------------
    ### Vòng lặp để đọc dữ liệu:
    while (keep_running) {
        if (ioctl(fd, TCS34725_IOCTL_READ_CLEAR, &clear) < 0 ||
            ioctl(fd, TCS34725_IOCTL_READ_RED, &red) < 0 ||
            ioctl(fd, TCS34725_IOCTL_READ_GREEN, &green) < 0 ||
            ioctl(fd, TCS34725_IOCTL_READ_BLUE, &blue) < 0) {
            perror("Failed to read from device");
            break;
        }
        Gọi ioctl() để yêu cầu driver trả về các giá trị đo được từ cảm biến và lưu vào biến tương ứng.
        Nếu bất kỳ lệnh nào thất bại, in lỗi và thoát vòng lặp.
        -------------------------------------------------------------------
        printf("\rClear: %-5d | Red: %-5d | Green: %-5d | Blue: %-5d", clear, red, green, blue);
        fflush(stdout);
        usleep(200000);  // 200ms delay

        In kết quả ra cùng một dòng (\r) để cập nhật liên tục.
        fflush(stdout) để đảm bảo in ngay lập tức.
        Delay giữa các lần đo để giảm tải cảm biến và CPU
        -------------------------------------------------------------------
    }
    printf("\nExiting.\n");
    close(fd);
    return 0;
    Dọn dẹp khi thoát khỏi chương trình
## 4. Các lỗi thường gặp
Lỗi                                                         Nguyên nhân                                 Giải pháp 
Failed to open device                           Thiếu quyền hoặc thiết bị chưa tồn tại         Dùng sudo, kiểm tra /dev/tcs34725
Unexpected ID                                   Cảm biến không phản hồi đúng                   Kiểm tra kết nối, dây I2C   
No such file or directory: /dev/tcs34725        Driver chưa được nạp thành công                Kiểm tra dmesg, đảm bảo insmod thành công

## 5. 📚 Tài liệu tham khảo

    Datasheet cảm biến: ams.com/tcs34725

    man ioctl, i2c_smbus_* (API trong Linux kernel)

    i2c-tools

👤 Tác giả

    Tên: Mai Minh Duc, Mai Thanh Binh, Nguyen Hoang Danh

    License: GPL v2

    Mô tả: Driver I2C kernel cho cảm biến TCS34725 tích hợp giao diện ký tự + ioctl