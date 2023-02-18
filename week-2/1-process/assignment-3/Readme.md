# DESCRIPTION: Segmentation Fault (Core Dumped) Program

## TOPIC: Segmentation Fault (Core Dumped), GDB FIX BUG

## THEORY:

* Segmentation fault: Chương trình chỉ được phép truy cập đến vùng nhớ thuộc quyền quản lý của nó mà thôi. Bất cứ truy cập vào vùng nhớ nào nằm phạm vi không cho phép của chương trình sẽ dẫn đến lỗi “Segmentation fault”.

* Có 5 lỗi phổ biến dẫn đến lỗi "segmentation fault":
1. Deferencing NULL pointer
2. Dereferencing con trỏ chưa được khởi tạo
3. Dereferencing con trỏ đã bị free hoặc delete
4. Ghi giá trị vượt quá giới hạn của mảng
5. Hàm đệ quy sử dụng hết vùng bộ dành cho stack – còn gọi là “stack overflow”
 
* Core Dump: Trên Linux, bất cứ khi nào một ứng dụng bị crash (thông thường nhất là gây ra bởi “Segmentation fault”), nó có tùy chọn tạo ra một file lưu vết lỗi gọi là “core dump”.
Tóm lại, Core dump là một file lưu lại trạng thái của chương trình tại thời điểm mà nó chết. Nó cũng là bản sao lưu lại tất cả các vùng bộ nhớ ảo đã được truy cập bởi chương trình.

* Bật tính năng tạo file "core dump" khi app crash:
Để bật tính năng tự động tạo file core dump, chúng ta cần phải cho Linux biết kích thước cho phép của file core dump là bao nhiêu. Sử dụng lệnh ulimit để thiết lập:

    `$ ulimit -c unlimited`

Theo mặc định, giá trị này là 0, đó là lý do tại file core dump không được tạo ra theo mặc định. Việc chạy dòng lệnh ulimit trong một Terminal sẽ cho phép tạo file core dump cho phiên Terminal đó. Tham số unlimited có nghĩa là không hạn chế kích thước của file core dump. Bây giờ, nếu có chương trình bị tèo, bạn hãy chạy ứng dụng đó trong phiên Terminal này và chờ nó tèo.

* Để tìm ra nguyên nhân gây ra lỗi "core dumped"
1. Chạy lệnh *ulimit ->*

    `$ ulimit -c unlimited`

2. *Install gbd*

    `$ sudo apt-get install gdb -y`

3. Build lại chương trình ở mode debug

    `$ gcc main.cpp -o test -g`

-g là option dùng để bật chế độ debug với gdb.

4. Chạy lại chương trình 
    `$ ./test`
Chương trình vẫn tèo giống lúc đầu nhưng bây giờ sẽ có thêm file tên là “core” được tạo ra và nằm trong directory hiện tại của terminal.
**Regular user dumps caught by Apport write to:**
    `/var/lib/apport/coredump/`

5. Chạy lại chương trình phát nữa sử dụng gdb kết hợp với file “core”
    `$ gdb test core`
Tuy nhiên, nếu chắc chắn nguyên nhân gốc nằm ở code logic của chương trình. Trong trường hợp này hãy dùng lệnh “backtrace” của gdb để xem thêm các frame khác trong callstack:
    `(gdb) backtrace`

Trong callstack thì các frame được thực thi trước sẽ ở bên dưới và ngược lại. Vì vậy hãy nhìn từ dưới lên trên để xem frame cuối cùng thuộc phạm vi source code của mình (chưa đi vào hàm trong thư viện) là frame nào.

#References:
    https://askubuntu.com/questions/966407/where-do-i-find-the-core-dump-in-ubuntu-16-04lts
    https://cppdeveloper.com/c-nang-cao/debug-loi-khi-chuong-trinh-bi-segmentation-fault-tren-linux/