# ANSWERS:

## 1.2 Điều gì xảy ra khi chúng ta sử dụng họ hàm execute (execl/exevp/exepv)? Giải thích và viết một chương trình để xác minh câu trả lời. Sau đó hãy cho biết bản chất khi lệnh system() hoạt động như thế nào.

Answer:

* Họ hàm execute: 
Trong nhiều trường hợp, bạn đang có một process A đang thực thi và muốn chạy 1 program B từ process A đang thực thi đó, hoặc con của nó. Điều này hoàn toàn thực hiện được thông qua function call exec.
Phổ biến hay sử dụng:
    Under the header file: unistd.h

    int execl(const char *pathname, const char *arg, ...);

The program is pointed to by pathname. Pathname must be either a binary executable (BIN file), or a script starting with a line of the form: #! interpreter [optional-arg]

* Function call: The exec() family of functions replaces the current process image with a new process image

Khi process A đang thực thi và sử dụng họ hàm execute để gọi tới program B, sau khi gọi xong toàn bộ process image của A sẽ được thay thế bởi process image của B. Tuy nhiên, PID vẫn giữ nguyên.

PID of the process is not changed but the data, code, stack, heap, etc. of the process are changed and are replaced with those of newly loaded process. The new process is executed from the entry point (According to references)

Nếu process A đang thực thi có nhiều threads, các threads đó sẽ bị terminated và process image của B sẽ được loaded vào và thực thi. 

* Advanced Expansion: Combining fork() and exec() system calls

* Bản chất khi lệnh system() hoạt động:
    Under the header file: stdlib.h

    int system(const char *command);

Đầu tiên, nó sử dụng system call fork() để tạo 1 child process. Sau đó, thực thi command được chỉ định (chính là biến const char *command) sử dụng execl():

    execl("/bin/sh", "sh", "-c", command, (char *) NULL);

system() returns sau khi command được hoàn thành.

Trong quá trình thực thi command, SIGCHLD bị blocked, SIGINT và SIGQUIT bị ignored trong process that calls system().

## 1.3 Debug là một công việc quan trọng trong việc lập trình do đó hãy tìm hiểu về segmentation fault, core dumped và cho biết chúng là gì? Viết một chương trình tái hiện lại lỗi. Sau khi tái hiện thành công, tìm hiểu về gdb và trình bày các bước fix cho lỗi này.

Answer:
* Segmentation fault: Chương trình chỉ được phép truy cập đến vùng nhớ thuộc quyền quản lý của nó mà thôi. Bất cứ truy cập vào vùng nhớ nào nằm phạm vi không cho phép của chương trình sẽ dẫn đến lỗi “Segmentation fault”.

* Có 5 lỗi phổ biến dẫn đến lỗi "segmentation fault":
    Deferencing NULL pointer
    Dereferencing con trỏ chưa được khởi tạo
    Dereferencing con trỏ đã bị free hoặc delete
    Ghi giá trị vượt quá giới hạn của mảng
    Hàm đệ quy sử dụng hết vùng bộ dành cho stack – còn gọi là “stack overflow”
 
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
5. Chạy lại chương trình phát nữa sử dụng gdb kết hợp với file “core”
    `$ gdb test core`
Tuy nhiên, nếu chắc chắn nguyên nhân gốc nằm ở code logic của chương trình. Trong trường hợp này hãy dùng lệnh “backtrace” của gdb để xem thêm các frame khác trong callstack:
    `(gdb) backtrace`

Trong callstack thì các frame được thực thi trước sẽ ở bên dưới và ngược lại. Vì vậy hãy nhìn từ dưới lên trên để xem frame cuối cùng thuộc phạm vi source code của mình (chưa đi vào hàm trong thư viện) là frame nào.

## 3.1 Giả sử rằng một parent process đã thiết lập một handler cho SIGCHLD và cũng block tín hiệu này. Sau đó, một trong các child process của nó thoát ra và parent process sau đó thực hiện wait() để thu thập trạng thái của child process. Điều gì xảy ra khi parent process bỏ chặn SIGCHLD?  Viết một chương trình để xác minh câu trả lời. 

Answer: Theo em, lúc đầu parrent process đang block tín hiệu SIGCHLD. Sau khi child process exit, nó sẽ gửi tín hiệu SIGCHLD tới parrent process để xóa tiến trình Zombie Process (clear hẳn data còn lại của child process). Tuy nhiên, do parrent process đang block tín hiệu SIGCHLD nên tín hiệu SIGCHLD sẽ được kernel giữ vào hàng chờ xử lý (PENDING). Tín hiệu SIGCHLD chỉ được gửi tới parrent process sau khi parrent process unblocked SIGCHLD. Khi đó, hàm hanlder tương ứng với tín hiệu SIGCHLD mới được thực thi.

Chương trình để xác minh ở dir: week-2/1-process/assignment-1






# REFERENCES:
(Question 1.2):
    execl(3) - Linux man page: https://linux.die.net/man/3/execl
    https://linuxhint.com/linux-exec-system-call/

