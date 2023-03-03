# DESCRIPTION: PIPEs/FIFOs
## SOLUTION IDEA
### Với file related_pipe.c: PIPEs
* Từ main process, create a pipe by using system call pipe();
* Từ main process, tạo ra child process bằng system call fork(). Khi đó child process và parent process là các related process.

* Ở child process:
1. Đóng vai trò là consumer. Đầu tiên, close write() by using system call close(fds[1]). 
2. Đọc dữ liệu từ pipe bằng system call read() và lưu vào 1 buffer.
3. In ra các trạng thái của return value system call read(). Nếu > 0 là số lượng byte đọc được, = 0 là write end of pipe, < 0 là on error.

* ở parent process:
1. Đầu tiên, handle SIGCHLD signal to avoid creating zombie process.
2. Đóng vai trò là producer. Đầu tiên, close read function by using close(fds[0]).
3. Ghi dữ liệu lên pipe bằng system call write().
4. Tạo ra trường hợp write end of pipe bằng cách close write function by using close(fds[1]).

### Với file named_pipe.c và producer.c: FIFOs
Đối với file named_pipe.c plays a role of consumer - reader.
* Tạo file FIFOs với specific pathname và mode by using mkfifo().
* Sử dụng loop while(1) để thao tác FIFOs file by using system call open(), read(), close().
1. Open file with O_RDONLY mode by using system call open(), return value to fd - int variable is FIFOs file description.
2. Read data of file by using system call read().
3. Close file by using system call close().

Đối với file producer.c plays a role of producer - writer.
Do FIFOs file đã được tạo bởi consumer nên bỏ qua bước tạo file.
* Using loop while(1) để thao tác với FIFOs file by using system call open(), write(), close().
1. Open FIFOs file with O_WRONLY mode by using system call open(), return value to fd - int variable is FIFOs file description.
2. Write data to FIFOs file by using system call write().
3. Close file by using system call close().

# THEORY
## PIPEs
### Detail des: https://man7.org/linux/man-pages/man7/pipe.7.html
### Creating a pipe by following this ref: https://man7.org/linux/man-pages/man2/pipe.2.html

## FIFOs
### Detail des : https://man7.org/linux/man-pages/man7/fifo.7.html

# REF:
    https://man7.org/linux/man-pages/man2/pipe.2.html
    https://man7.org/linux/man-pages/man2/read.2.html
    https://man7.org/linux/man-pages/man2/close.2.html
    https://man7.org/linux/man-pages/man2/write.2.html
    https://man7.org/linux/man-pages/man3/mkfifo.3.html