from matplotlib import pyplot as plt
import os
import time
import numpy as np
import re
import subprocess

TEST_PATH = "./"
TEST_LIST = ["21-create-many", "22-create-many-recursive",
             "23-create-many-once", "31-switch-many", "32-switch-many-join", "33-switch-many-cascade", "61-mutex", "62-mutex", "51-fibonacci" ]
TEST_ARGS_2 = [(1000,10),(1500,10),(2000,10),(2500,10)]
TEST_ARGS_1 = [1000,1500,2000,2500, 10000]
TEST_ARGS_11 = [100, 1000, 2500, 5000, 7500, 10000, 30000]
TEST_ARGS_22 = [(100,100),(100,1000),(100,2500),(100,5000)]
TEST_ARGS_23 = [(100,10),(1000,10),(2500,10),(5000,10)]
TEST_ARGS_33 = [5, 10, 20, 50, 100, 1000]
TEST_ARGS_FIBO = [3, 5, 10, 15, 20, 22, 23]
TEST_NUMBER = len(TEST_LIST)
LIB = "LD_LIBRARY_PATH=. "

def execute(test_name,ARGS):
    time_count = []
    for i in ARGS:
        begin_t1 = time.time()
        if(type(i) == tuple):
            os.system(LIB + TEST_PATH + test_name + " " + str(i[0]) + " " + str(i[1]))
            end_t1 = time.time()
            os.system(TEST_PATH + test_name + "-pthread" + " " + str(i[0]) + " " + str(i[1]))
            end_t2 = time.time()
        else:
            os.system(LIB + TEST_PATH + test_name + " " + str(i))
            end_t1 = time.time()
            os.system(TEST_PATH + test_name + "-pthread" + " " + str(i))
            end_t2 = time.time()

        time_count.append((end_t1 - begin_t1, end_t2 - end_t1))
    return time_count

def gen_graph(test_name, ARGS):
    time_pthread = []
    time_thread = []
    time = execute(test_name,ARGS)
    for i in time:
        time_pthread.append(i[1])
        time_thread.append(i[0])
    x = np.arange(len(ARGS))
    width = 0.35
    fig, ax = plt.subplots()
    rects1 = ax.bar(x - width/2, time_thread, width, label='thread')
    rects2 = ax.bar(x + width/2, time_pthread, width, label='pthread')
    ax.set_ylabel('Time (s)')
    ax.set_title('Comparaison de performance entre thread et pthread pour'+" "+test_name, fontsize=8)
    ax.set_xticks(x, ARGS)
    plt.xticks(range(len(ARGS)), ARGS)
    ax.legend()

    fig.tight_layout()
    plt.savefig(test_name+".png")
    plt.close()


def execute_test(test, args):
    time_pthread = []
    time_thread = []
    for i in args:
        if(type(i) == tuple):
            begin_t1 = time.time()
            os.system(LIB + TEST_PATH + test + " " + str(i[0]) + " " + str(i[1]))
            end_t1 = time.time()
            os.system(TEST_PATH + test + "-pthread" + " " + str(i[0]) + " " + str(i[1]))
            end_t2 = time.time()
        else:
            begin_t1 = time.time()
            os.system(LIB + TEST_PATH + test + " " + str(i))
            end_t1 = time.time()
            os.system(TEST_PATH + test + "-pthread" + " " + str(i))
            end_t2 = time.time()
        time_thread.append(end_t1 - begin_t1)
        time_pthread.append(end_t2 - end_t1)
    return time_thread, time_pthread

def graph_courbe(test, args):
    time_pthread = []
    time_thread = []
    arg = []
    if (type(args[0]) == tuple):
        for i in args:
            arg.append(i[1])
    else:
        arg = args
    (time_thread, time_pthread) = execute_test(test, args)
    plt.plot(arg, time_thread, label='thread')
    plt.plot(arg, time_pthread, label='pthread')
    plt.ylabel("Time (s)", size = 16,)
    if (type(args[0]) == tuple):
        plt.xlabel('Number of yields with {} threads'.format(args[0][0]), size = 16)
    elif (test == "51-fibonacci"):
        plt.xlabel('Fibo(n)', size = 16)
    else:
        plt.xlabel('Number of threads', size = 16)
    plt.legend()
    #plt.show()
    plt.savefig(test+".png")
    plt.close()


if __name__ == '__main__':
    for i in range(3):
        graph_courbe(TEST_LIST[i], TEST_ARGS_11)
    for i in range(3,6):
        graph_courbe(TEST_LIST[i], TEST_ARGS_22)
    for i in range(6,8):
        graph_courbe(TEST_LIST[i], TEST_ARGS_33)
    graph_courbe(TEST_LIST[-1], TEST_ARGS_FIBO)
    
    """for i in range(3):
        gen_graph(TEST_LIST[i], TEST_ARGS_1)
    for i in range(3,6):
        gen_graph(TEST_LIST[i], TEST_ARGS_2)
    gen_graph(TEST_LIST[-1], TEST_ARGS_1)
    """