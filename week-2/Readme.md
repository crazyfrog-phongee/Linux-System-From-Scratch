# DESCRIPTION: THEORY EXERCISES WEEK 2-3

## 1.2 Điều gì xảy ra khi chúng ta sử dụng họ hàm execute (execl/exevp/exepv)? Giải thích và viết một chương trình để xác minh câu trả lời. Sau đó hãy cho biết bản chất khi lệnh system() hoạt động như thế nào.

Answer:

### Khái niệm:

* Họ hàm execute: 
Trong nhiều trường hợp, bạn đang có một process A đang thực thi và muốn chạy 1 program B từ process A đang thực thi đó, hoặc con của nó. Điều này hoàn toàn thực hiện được thông qua function call exec.
Phổ biến hay sử dụng:
    Under the header file: unistd.h

    int execl(const char *pathname, const char *arg, ...);

The program is pointed to by pathname. Pathname must be either a binary executable (BIN file), or a script starting with a line of the form: #! interpreter [optional-arg]

* Function call: **The exec() family of functions replaces the current process image with a new process image**

Khi process A đang thực thi và sử dụng họ hàm execute để gọi tới program B, sau khi gọi xong toàn bộ process image của A sẽ được thay thế bởi process image của B. Tuy nhiên, PID vẫn giữ nguyên.

PID of the process is not changed but the data, code, stack, heap, etc. of the process are changed and are replaced with those of newly loaded process. The new process is executed from the entry point (According to references)

Nếu process A đang thực thi có nhiều threads, các threads đó sẽ bị terminated và process image của B sẽ được loaded vào và thực thi. 

* Advanced Expansion: Combining fork() and exec() system calls

### Bản chất khi lệnh system() hoạt động:
    Under the header file: stdlib.h

    int system(const char *command);

Đầu tiên, nó sử dụng system call fork() để tạo 1 child process. Sau đó, thực thi command được chỉ định (chính là biến const char *command) sử dụng execl():

    execl("/bin/sh", "sh", "-c", command, (char *) NULL);

system() returns sau khi command được hoàn thành.

Trong quá trình thực thi command, SIGCHLD bị blocked, SIGINT và SIGQUIT bị ignored trong process that calls system().

### REF: 
    execl(3) - Linux man page: https://linux.die.net/man/3/execl
    https://linuxhint.com/linux-exec-system-call/

## 1.3 Debug là một công việc quan trọng trong việc lập trình do đó hãy tìm hiểu về segmentation fault, core dumped và cho biết chúng là gì? Viết một chương trình tái hiện lại lỗi. Sau khi tái hiện thành công, tìm hiểu về gdb và trình bày các bước fix cho lỗi này.

Answer:

### Khái niệm: 
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

### Các bước fix lỗi "core dumped"
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

### REF:
    https://cppdeveloper.com/c-nang-cao/debug-loi-khi-chuong-trinh-bi-segmentation-fault-tren-linux/
    https://askubuntu.com/questions/966407/where-do-i-find-the-core-dump-in-ubuntu-16-04lts

## 3.1 Giả sử rằng một parent process đã thiết lập một handler cho SIGCHLD và cũng block tín hiệu này. Sau đó, một trong các child process của nó thoát ra và parent process sau đó thực hiện wait() để thu thập trạng thái của child process. Điều gì xảy ra khi parent process bỏ chặn SIGCHLD?  Viết một chương trình để xác minh câu trả lời. 

Answer: Theo em, lúc đầu parrent process đang block tín hiệu SIGCHLD. Sau khi child process exit, nó sẽ gửi tín hiệu SIGCHLD tới parrent process để xóa tiến trình Zombie Process (clear hẳn data còn lại của child process). Tuy nhiên, do parrent process đang block tín hiệu SIGCHLD nên tín hiệu SIGCHLD sẽ được kernel giữ vào hàng chờ xử lý (PENDING). Tín hiệu SIGCHLD chỉ được gửi tới parrent process sau khi parrent process unblocked SIGCHLD. Khi đó, hàm hanlder tương ứng với tín hiệu SIGCHLD mới được thực thi.

Chương trình để xác minh ở dir: week-2/1-process/assignment-1

## 3.2 Realtime signal và standard signal là gì? Phân biệt sự khác nhau giữa chúng.

### Khái niệm: 
* Bản chất của signals: là 1 software interrupt, là cơ chế xử lý các sự kiện bất đồng bộ
(Signals are notifications delivered asynchronously to a process by the kernel)
* Signal được chia thành 2 nhóm: Standard Signals and Real-time Signals

### So sánh giữa Standard và Real-time signals:
Các đặc tính của POSIX real-time signals:
* Linux Kernel supports real-time signals range defined by macros SIGRTMIN và SIGRTMAX
* Không giống như standard signals, real-time signals không có xác định tên một cách riêng lẻ (Apps can identify the real-time signals by using an expression like (SIGRTMIN + n) or (SIGRTMAX - n))
* Default action: terminate the receiving process
* Real-time signals are queued to the receiving process. In contrast, standard signals are in a pending state (not queued)
* 1 real-time signal is received multiple tịmes. In contrast, nếu 1 standard signal bị block và nhiều phiên bản của nó được delivered tới process, chỉ 1 phiên bản được pending và còn lại bị loại bỏ.
* Nếu multiple real-time signals are queued to a process, they are delivered in the ascending (tăng dần) order of their signal numbers, tức là lower real-time signal first. Ngược lại, với standard signals, sự phân phối (delivered) không xác định.
* Nếu cả multiple real-time and standard signals are queued to a process, the standard signals are delivered first (phù hợp với concept that the lower numbered signals are delivered first)
* Về khía cạnh sending signals:
    standard signals: using system call kill()
    real-time signals: using sigqueue()

**sigqueue() khác với kill() ở chỗ: a value or a pointer can be sent along with the signal. The value or the pointer can be retrieved by the receiving process from the second para of the real-time signal handler (the pointer to siginfo_t). The value or the pointer is stored in si_value or si_ptr members respectively.**

* Về khía cạnh change the disposition (signal's handler)
    A process can change the disposition of a signal using sigaction(2) or signal(2).  (The latter is less portable when establishing a signal handler

    Common: 
        `kill() --- signal()`
        `sigqueue() --- sigaction()`

### REF: 
    https://www.softprayog.in/programming/posix-real-time-signals-in-linux
    https://learning.oreilly.com/library/view/understanding-the-linux/0596000022/0596000022_ch09-21982.html

## 1. Program, Process and Thread
* Program (chương trình): là nhóm các câu lệnh thực thi một nhiệm vụ cụ thể, được thể hiển bằng các file thực thi và nằm trên ổ cứng máy tính
* Process (tiến trình): là một chương trình đang được thực thi và sử dụng tài nguyên của hệ thống
* Thread (luồng): ở tầng app, được gọi là user thread/lightweight thread, được quản lý độc lập bởi một bộ lập lịch (Schedular). Nó được kiểm soát ở user-space.

* Ref: 
    https://viblo.asia/p/019-lightweight-thread-va-threading-model-naQZRQ1PKvx 

## 2. Context Switching

### Khái niệm: Context switch (đôi khi được gọi là process switch hoặc task switch) là quá trình lưu trữ trạng thái của CPU hoặc của một thread để có thể tiếp tục thực thi sau đó. Việc lưu trữ này cho phép nhiều tiến trình có thể cùng thực thi trên một CPU vật lý và là chức năng quan trọng của các hệ điều hành đa nhiệm.

Cụ thể, quá trình đó như thế nào? Theo em tìm hiểu, quá trình đó xảy ra như sau:

### Đối với Thread Switching Context:
* Đối với Linux Kernel, context switch thread liên quan tới các thanh ghi (registers), con trỏ ngăn xếp (stack pointer) và con trỏ chương trình. 
* Khi luân chuyển thread, CPU phải làm công việc luân chuyển dữ liệu của thread trước nạp vào thanh ghi ra bộ nhớ đệm, và nạp dữ liệu của thread mới vào thanh ghi. Việc luân chuyển dữ liệu tham số vào và ra khỏi thanh ghi và bộ đệm chính là Context Switch
* The cost of thread-to-thread switching is about the same as the cost of entering and exiting the kernel.
* Nếu context switch xảy ra giữa 2 thread thuộc 2 tiến trình (process) khác nhau sẽ phức tạp và tốn thời gian hơn.
* Ngoài chi phí cho việc lưu trữ/phục hồi trạng thái của các thread, hệ điều hành cũng phải tốn chi phí cho bộ lập lịch (task scheduler) để chọn lựa thread tiếp theo được đưa vào xử lý.
* Context switching xảy ra ở application, dễ dàng kiểm soát hơn, cost dành cho nó cũng giảm đi nhiều so với OS thread và process.
* Thread context switching (chuyển đổi ngữ cảnh thread) sẽ nhẹ nhàng hơn process context switch (chuyển đổi ngữ cảnh processs) bởi các threads trong cùng một ứng dụng cùng chia sẻ vùng nhớ cho phép thread có thể đọc và ghi cùng một cấu trúc dữ liệu và biến với thread anh em.

### Đối với Thread Switching Context:   
* a type of context switching where we switch one process with another process
* switching of all the process resources with those needed by a new process, meaning switching the memory address space. This includes memory addresses, page tables, and kernel resources, caches in the processor.

### REF:
    https://tldp.org/LDP/LG/issue23/flower/context.html 
    https://codelearn.io/sharing/da-luong-nhanh-hay-cham 
    https://techmaster.vn/posts/35265/target=%22_blank%22 

## 3. So sánh một vài khía cạnh Thread và Process

### Khía cạnh Context Switching:
* Thread: 
    Switching PC, registers and SP 

**Thread nhanh hơn so với Process vì Thread bản chất là 1 lightweight process, nhẹ hơn process ban đầu.**

* Process:
    Switching the memory address space. This includes memory addresses, page tables, and kernel resources, caches in the processor.

### Khía cạnh Shared Memory
* Thread: 
    Chung không gian bộ nhớ toàn cục (không gian bộ nhớ của process)
* Process: 
    Mỗi process nằm trên không gian bộ nhớ riêng biệt (virtual memory address) dẫn đến chia sẻ dữ liệu giữa các process là khó khăn hơn. Thông qua cơ chế giao tiếp IPC

### Khía cạnh ID
* Thread: 
    ThreadID là một cấu trúc dữ liệu struct (pthread_t) dẫn đến in ra TID khó hơn. 
    **ThreadID là duy nhất trong một process.**

* Process:
    PID là một số nguyên – int (pid_t) dẫn đến in ra PID dễ hơn.
    **PID là duy nhất trên toàn hệ thống.**

### Khía cạnh Blocked
* Thread:
    Nếu 1 thread bị block, các thread khác vẫn hoạt động bình thường.
* Process:
    Nếu 1 process bị block, các process khác vẫn hoạt động bình thường.

### Khía cạnh Crashed:
* Thread: 
    Nếu một thread bị crashed, các threads khác chấm dứt ngay lập tức.
* Process:
    Nếu một process bị crashed, process khác vẫn thực thi bình thường.

### Khía cạnh State:
* Thread:

    Trạng thái mặc định là joinable, tức là khi thread kết thúc thì một thread khác có thể thu được giá trị trả về của thread đó thôn qua pthread_join().
    Khi thread kết thúc, nó chuyển qua trạng thái thread zombie (xử lý tương tự zombie process). Nếu số lượng thread zombie ngày càng lớn, sẽ không thể tạo thêm thread được nữa.
    Trạng thái detached, ta không thể dùng pthread_join() để thu được trạng thái kết thúc của thread, và thread không thể trở về trạng thái joinable.

* Process:

    1. Running or Runnable (R)
    2. Uninterruptible Sleep (D)
    3. Interruptable Sleep (S)
    4. Stopped (T)
    5. Zombie (Z)

### Khía cạnh Terminate:
* Thread:
    Bất cứ một thread nào gọi hàm exit(), hoặc main thread kết thúc thì tất cả các thread còn lại kết thúc ngay lập tức.

* Process:
    Bất kỳ process cha hay con gọi hàm exit(), các process khác vẫn sẽ hoạt động bình thường. Tùy thuộc vào parent process hay child process kết thúc trước mà rơi vào các trường hợp Orphane hoặc Zombie

### Khía cạnh Sending Signals:
* Thread:
    pthread_kill();

* Process:
    systemcall kill();

## 4. Management

### ID:
* Thread:

    int pthread_self(void);
    int pthread_equal(pthread_t tid1, pthread_t tid2);

* Process:

    int getpid(void);
    int getppid(void);

### Create:
* Thread:

    int pthread_create(pthread_t *restrict thread, const pthread_attr_t *restrict attr, void *( *start_routine)(void *), void *restrict arg ))

* Process:

    system call fork();

### Terminate:
* Thread:

    int pthread_exit(void *retval);
    Đối số là giá trị trả về từ thread đang gọi hàm này.

    int pthread_cancel(pthread_t thread);
    Bất cứ một thread nào gọi hàm exit(), hoặc main thread kết thúc thì tất cả các thread còn lại kết thúc ngay lập tức.

* Process:
    Kết thúc bình thường: `system call _exit();` `void exit(int status);`
        0: on success
        ≠ 0: on failure

    Kết thúc bất thường: `kill command`

### Get exit value
* Thread:

    int pthread_join(pthread_t thread, void **retval);
    Truy cập bởi thread cha đang đợi thread này kết thúc và có thể được truy cập bởi một thread khác. Tại thời điểm được gọi, bị block
    Free dữ liệu còn lại của trạng thái zombie

* Process: 
    
    system call wait();
    Tiến trình cha có thể thu được trạng thái kết thúc của tiến trình con.

    system call waitpid();
    Giải quyết vấn đề multi children, theo dõi child process cụ thể. Tại thời điểm được gọi, bị block

### Detaching:
* Thread: 

    int pthread_detached(pthread_t thread);

Dùng trong trường hợp không quan tâm đến trạng thái kết thúc của thread mà chỉ cần hệ thống tự động clean and remove.

## 5. Synchronous and Asynchronous
Một trong những điểm mạnh của thread là chia sẻ dữ liệu với nhau thông qua các biến global -> 	Dẫn đến vấn đề đồng bộ (Thread Synchronization)

### Các khái niệm quan trọng trong Thread Synchoronization:
* Shared resource: Tài nguyên được chia sẻ giữa các thread.

* Atomic/Nonatomic:

    Atomic: Tại một thời điểm chỉ có một thread duy nhất được truy cập vào tài nguyên được chia sẻ (shared resource) -> An toàn

    Nonatomic: Nhiều threads có thể truy cập vào shared resource cùng một thời điểm -> Không an toàn

* Critical Section: đoạn code truy cập vào vùng tài nguyên được chia sẻ giữa (shared resource) giữa các threads và **việc thực thi của nó nằm trong bối cảnh atomic**. Tức là thời điểm đoạn code được thực thi sẽ không bị gián đoạn bởi bất cứ một thread nào truy cập đồng thời vào shared resource đó.

### Xử lý vấn đề bất đồng bộ:
* Sử dụng kỹ thuật Mutex

    Khái niệm: Mutex (mutual exclusion) là một kĩ thuật được sử dụng để đảm bảo rằng tại một thời điểm chỉ có 1 thread mới có quyền truy cập vào các tài nguyên dùng chung (shared resources).

    Triển khai mutex:
    
        1. Khởi tạo khóa mutex
        2. Thực hiện khóa mutex cho các shared resource trước khi vào critical section. Thực hiện truy cập vào shared resources
        3. Mở khóa mutex

* Sử dụng kỹ thuật Condition Variables

    Trên Linux có 2 loại waiting events: Busy Waiting/Sleep Waiting: 
    
        Đối với BW: polling – thăm dò (ví dụ như khoảng thời gian t ra kiểm tra hộp thư 1 lần).
        Đối với SW: (khi có thư, người đưa thư tự đưa đến tận răng)
    
    Khái niệm: Một condition variable được sử dụng để thông báo tới một thead khác về sự thay đổi của một shared variable và cho phép một thread khác block cho tới khi nhận được thông báo.

    Triển khai CV:
    
        1. Allocated
        2. Signaling: `pthread_cond_signal()`
        3. Waiting: `pthread_cond_waiting()`

    **CV thường giải quyết bài toán producer – consumer, một tình huống thường xuyên gặp trong lập trình multi-thread, giúp cho các thread giao tiếp và sử dụng tài nguyên CPU hiệu quả hơn.**

    REF: 
        https://vimentor.com/vi/lesson/thread-synchronization-bien-dieu-kien 