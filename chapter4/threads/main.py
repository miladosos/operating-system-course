import math
import threading


def average():
    global numbers_average, numbers
    numbers_average = sum(numbers) / len(numbers)


def maximum():
    global numbers_maximum, numbers

    numbers_maximum = max(numbers)


def minimum():
    global numbers, numbers_minimum

    numbers_minimum = min(numbers)


def standard_deviation():
    global numbers, numbers_sd
    data = numbers

    n = len(data)

    if n <= 1:
        return 0.0

    mean, sd = avg_calc(data), 0.0

    # calculate stan. dev.
    for el in data:
        sd += (float(el) - mean) ** 2
    sd = math.sqrt(sd / float(n - 1))

    numbers_sd = sd


def avg_calc(ls):
    n, mean = len(ls), 0.0

    if n <= 1:
        return ls[0]

    # calculate average
    for el in ls:
        mean = mean + float(el)
    mean = mean / float(n)

    return mean


numbers = []

numbers_average = None
numbers_minimum = None
numbers_maximum = None
numbers_sd = None

if __name__ == '__main__':
    numbers = [int(item) for item in input().split()]

    threads = list()

    threads.append(threading.Thread(target=average))
    threads.append(threading.Thread(target=maximum))
    threads.append(threading.Thread(target=minimum))
    threads.append(threading.Thread(target=standard_deviation))

    for thread in threads:
        thread.start()

    for thread in threads:
        thread.join()

    print("Numbers: {}".format(numbers))
    print("Average: {}".format(numbers_average))
    print("Maximum: {}".format(numbers_maximum))
    print("Minimum: {}".format(numbers_minimum))
    print("Standard Deviation: {}".format(numbers_sd))
