# DESCRIPTION: 
## Viết một chương trình để chứng minh rằng các thread khác nhau trong cùng một process có thể có các tập hợp signal đang chờ xử lý (set of pending signal) khác nhau, được trả về bằng sigpending(). Bạn có thể làm điều này bằng cách sử dụng pthread_kill() để gửi các tín hiệu khác nhau đến hai thread khác nhau đã bị block các tín hiệu này, và sau với mỗi thread gọi sigpending() hãy hiển thị thông tin về các tín hiệu đang chờ xử lý.

## SOLUTION IDEA:

* Main thread: set signal mask để chặn tín hiệu SIGTERM bằng cách sử dụng hàm pthread_sigmask()

* Từ main thread tạo ra 2 thread mới với id tương ứng thread_id1 và thread_id2 bằng cách sử dụng hàm pthread_create()
**Khi đó, 2 thread mới được tạo sẽ kế thừa signal mask và disposition của main thread**

* Ở các hàm start_routine của 2 thread mới, set signal mask khác nhau:
Cụ thể, với thread_id1: thêm block SIGUSR1 bằng cách sử dụng hàm sigaddset()
Cụ thể, với thread_id2: thêm block SIGUSR2 bằng cách sử dụng hàm sigaddset()

Trong vòng lặp while(true) của các thread, kiểm tra các signal pending bằng cách sử dụng sigpending();

* Quay lai main thread, sau khi tạo thành công 2 thread, sử dụng hàm pthread_kill() để send signal tới 2 thread vừa tạo ra.

# THEORY:

## Thread and POSIX Thread, Management:

Khi chạy multi – thread, nếu một trong các thread gọi tới hàm exit() trong khi các thread khác vẫn đang hoạt động thì dẫn tới lỗi “Memory leak”, cụ thể là trạng thái possibly lost do Valgrind report.

* REF: 
    https://vimentor.com/en/lesson/gioi-thieu-ve-thread
    https://vimentor.com/en/lesson/quan-ly-posix-thread-1

## Process's signal disposition

### process's signal disposition: similar to the signal's handler
* The signal disposition can be changed by calling signal() or sigaction

* REF: 

    https://csresources.github.io/SystemProgrammingWiki/SystemProgramming/Signals-Part-2:-Pending-Signals-and-Signal-Masks/

    https://csresources.github.io/SystemProgrammingWiki/SystemProgramming/Signals,-Part-3:-Raising-signals/

### Pending state

* The most common reason for a signal to be pending is that the process (or thread) has currently blocked that particular signal.

* **Signals can be blocked (meaning they will stay in the pending state) by setting the process signal mask or, when you are writing a multi-threaded program, the thread signal mask.**

## Children's signal disposition

### What happens during fork ?

* The child process inherits a copy of the parent process's signal disposition and a copy of the parent's signal mask.

* Pending signals however are not inherited by the child.

## Thread's signal disposition

### What happens when creating a new thread?

* The new thread inherits a copy of the calling thread's mask

### Block signals in a multi-threaded program
* **Blocking signals is similar in multi-threaded programs to single-threaded programs: Use pthread_sigmask instead of sigprocmask Block a signal in all threads to prevent its asynchronous delivery.**

### How are pending signals delivered in a multi-threaded program?

* A signal is delivered to any signal thread that is not blocking that signal.

* If the two or more threads can receive the signal then which thread will be interrupted is arbitrary!

## sigpending():

* returns a set up of signals that are pending for delivery to the call thread (i.e., the signals which have been raised while blocked).  The mask of pending signals is returned in set.
* RETURN VALUE: 0 on sucess, -1 on failure

* REF:
    https://man7.org/linux/man-pages/man2/sigpending.2.html
    https://www.ibm.com/docs/en/zos/2.2.0?topic=functions-sigpending-examine-pending-signals
