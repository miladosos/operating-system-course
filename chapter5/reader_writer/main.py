import random
from threading import current_thread, Thread, Lock
from typing import List

current_readers = 0  # Number of readers currently accessing resource

resource_access = Lock()  # Control Read/Write Access to resource
read_access = Lock()  # Syncing Changes to shared variable current_readers
service_queue = Lock()  # Fairness: preserves ordering of requests (signaling must be fifo)

shared_value = 0  # Shared Value (Threads may change this value)


def writer():
    global shared_value

    with service_queue:
        resource_access.acquire()

    c_thread = current_thread()
    shared_value = c_thread.ident
    print("{name} writes {value}".format(name=c_thread.name, value=shared_value))

    resource_access.release()


def reader():
    global current_readers
    global shared_value

    with service_queue:
        with read_access:
            if current_readers == 0:
                resource_access.acquire()
            current_readers += 1

    c_thread = current_thread()
    print("{name} reads {value}".format(name=c_thread.name, value=shared_value))

    with read_access:
        current_readers -= 1
        if current_readers == 0:
            resource_access.release()


if __name__ == '__main__':
    """
        !!! Important Note !!!
        
        Based on Python implementation of Threads, thread identifiers may be recycled 
            when a thread exits and another thread is created.
        
        for more info check: https://docs.python.org/3.7/library/threading.html#threading.Thread.ident
    """

    writer_threads: List[Thread] = []
    reader_threads: List[Thread] = []

    for i in range(2):
        name = "writer-{}".format(i + 1)
        writer_threads.append(Thread(name=name, target=writer))

    for i in range(4):
        name = "reader-{}".format(i + 1)
        reader_threads.append(Thread(name=name, target=reader))

    threads = writer_threads + reader_threads
    random.shuffle(threads)  # Shuffle Created Threads
    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()
