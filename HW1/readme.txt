1. In the while loop: we use select() to get the ready set. Because select() is destructive to its fd_set argument,
    we need to reset it every time.
2. If the ready-read file descripter is the server itself, accept a new client. Otherwise, use handle_read to deal the input.
3. We use READ_SERVER and ready_to_write to determine current state: 
    (1) read_server read (set read lock, and release after reading)
    (2) write_server first read  (set read lock, and write lock after reading)
    (3) write_server second read  (release lock after writing)
4. We implement two kinds of lock: internal and external:
    (1) internal lock: use an array to check status of each id to prevent the same id write in the same process at same time.
    (2) external lock: use fcntl() to check status of the file in the different process.