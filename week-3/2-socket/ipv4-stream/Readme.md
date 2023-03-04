# DESCPIPTION: IPv4 Stream Socket.
## TOPIC: Handling muiltiple clients on server.
## SOLUTION IDEA
Basic: Trong mô hình client/server, một server có thể phục vụ đồng thời cho nhiều clients. Bỏ qua cách xử lý tuần tự và không phù hợp với việc nhiều clients request cùng một lúc, ta có thể sử dụng system call fork() để tạo các child processes hoạt động độc lập so với parent process (tương ứng với server). Mỗi child process chịu trách nhiệm phục vụ 1 clients theo cách riêng của nó.

## QUES: Tại sao lại dùng multi-process thay vì multi-thread trong bài toán client/server trong UNIX và LINUX?
According to a topic on Geekforgeeks (https://www.geeksforgeeks.org/socket-programming-in-cc-handling-multiple-clients-on-server-without-multi-threading/), this method (multi-thread) is strongly not recommended because of various disadvantages, namely:
* Threads are difficult to code, debug and sometimes they have unpredictable results.
* Overhead switching of context
* Not scalable for large number of clients
* Deadlocks can occur

# THEORY:

