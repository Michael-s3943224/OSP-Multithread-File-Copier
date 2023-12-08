/*
This is the mtcopier but using the <fcntl.h> header
to do I/O operations with files
*/

#include <pthread.h>
#include <iostream>
#include <stdexcept>
#include <chrono>
#include <functional>
#include <sstream>
#include <deque>
#include <fstream>
#include <vector>
#include <numeric>
#include <iterator>
#include <fcntl.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "threadtimes.h"

/*----CONSTANTS----*/
/* cmd args position for number of threads*/
#define NUM_THREADS_INDX 1
/* cmd args position for infile */
#define INFILE_INDX 2
/* cmd args position for outfile */
#define OUTFILE_INDX 3
/* cmd args position for timer flag*/
#define TIMER_INDEX 4
/* min number of cmd args */
#define MIN_NUM_ARGS 4
/* max number of cmd args*/
#define MAX_NUM_ARGS 5
/* num bytes read at a time */
#define READ_CHUNK 32768
/* value for successful thread create or join*/
#define THREAD_SUCCESS 0
/* max size for the queue */
#define QUEUE_MAX_SIZE 1024
/* file open error*/
#define FILE_OPEN_ERR -1
/* convert nano seconds to ms*/
#define NS_PER_MS 1000000

/* whether to show the time for each thread */
//#define SHOW_EACH_THREAD_TIME
/* whether to only actually time the functions */
//#define SHOW_OTHER_TIMES
/* whether to record the */
//#define SHOW_HIGHEST_QUEUE_SIZE

/*----GLOBAL VARIABLES----*/
/* the input file*/
int infile;
/* the ouput file*/
int outfile;
/* the queue to hold the file chunks */
std::deque<std::string> queue;
/* whether the reader threads are still reading */
bool reading = true;
/* wether the writer threads are still writing */
bool writing = true;
/* mutex for the file chunk queue */
pthread_mutex_t queueMutex;
/* mutex for infile */
pthread_mutex_t infileMutex;
/* mutex for outfile */
pthread_mutex_t outfileMutex;
/* conditional to check if queue has elements*/
pthread_cond_t queueFullCond;
/* conditional to check if the queue has space*/
pthread_cond_t queueEmptyCond;

/* whether to show the time*/
bool showTime = false;

#ifdef SHOW_HIGHEST_QUEUE_SIZE
/* track the highest queue size achieved */
unsigned int highestQueueSize = 0;
#endif

#ifdef SHOW_OTHER_TIMES
/* record the times for reader threads */
threadtimes* readerTimes;
/* record the times for writer threads */
threadtimes* writerTimes;
#endif

/* used to time functions */
std::chrono::nanoseconds timeFunction(const std::function<void()>& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return duration;
}

/* reader thread */
void* reader(void* arg)
{
    /* buffer to read file chunks at a time */
    char buffer[READ_CHUNK];

    /* the parameters */
    int* params = (int*) arg;

    #ifdef SHOW_OTHER_TIMES
    /* get the thread id */
    int index = *params;

    /* store the total times */
    long totalReadTime = 0;
    long totalReadLockWaitTime = 0;
    long totalReadBusyWaitTime = 0;
    #endif

    while (reading) {
        /* lock the infile mutex */
        #ifdef SHOW_OTHER_TIMES
        totalReadLockWaitTime += timeFunction([] {
        #endif
            pthread_mutex_lock(&infileMutex);
        #ifdef SHOW_OTHER_TIMES
        }).count();
        #endif

        /* get the number of bytes actually read */
        ssize_t len;

        /* read the from the file */
        #ifdef SHOW_OTHER_TIMES
        totalReadTime += timeFunction([&buffer, &len] {
        #endif
            len = read(infile, buffer, READ_CHUNK);
        #ifdef SHOW_OTHER_TIMES
        }).count();
        #endif


        /* the string */
        std::string item(buffer, len);

        /* lock the queue mutex */
        #ifdef SHOW_OTHER_TIMES
        totalReadLockWaitTime += timeFunction([] {
        #endif
            pthread_mutex_lock(&queueMutex);
        #ifdef SHOW_OTHER_TIMES
        }).count();
        #endif


        /* keep waiting until the queue has space */
        #ifdef SHOW_OTHER_TIMES
        totalReadBusyWaitTime += timeFunction([] {
        #endif
            while (queue.size() >= QUEUE_MAX_SIZE && reading) {
                pthread_cond_wait(&queueEmptyCond, &queueMutex);
            }
        #ifdef SHOW_OTHER_TIMES
        }).count();
        #endif

        /* push the file chunk to the queue */
        if (reading) {
            /* push read chunk to queue */
            queue.push_back(item);

            #ifdef SHOW_HIGHEST_QUEUE_SIZE
            /* keep track of the highest queue size*/
            if (queue.size() > highestQueueSize) {
                highestQueueSize = queue.size();
            }
            #endif

            /* stop the loop when the end of file is reached*/
            if (len == 0) {
                reading = false;
                pthread_cond_broadcast(&queueEmptyCond);
            }
        }

        /* unlock the queue mutex */
        pthread_mutex_unlock(&queueMutex);

        /* broadcast that there is new element in the queue */
        pthread_cond_broadcast(&queueFullCond);

        /* unlock the infile mutex */
        pthread_mutex_unlock(&infileMutex);
    }

    #ifdef SHOW_OTHER_TIMES
    /* set the times */
    readerTimes[index].lockTime = totalReadLockWaitTime;
    readerTimes[index].busyWaitTime = totalReadBusyWaitTime;
    readerTimes[index].processTime = totalReadTime;

    #endif

    /* clean up */
    delete params;

    /* exit thread */
    return nullptr; 
}

/* writer thread */
void* writer(void* arg)
{
    /* get the paremters */
    int* params = (int*) arg;

    #ifdef SHOW_OTHER_TIMES
    /* get the thread id */
    int index = *params;

    /* store the total times */
    long totalWriteTime = 0;
    long totalLockTime = 0;
    long totalBusyWaitTime = 0;
    #endif

    while (writing) {
        std::string item;

        #ifdef SHOW_OTHER_TIMES
        totalLockTime += timeFunction([]{
        #endif
            /* lock the queue mutex */
            pthread_mutex_lock(&outfileMutex);
        #ifdef SHOW_OTHER_TIMES
        }).count();
        #endif

        #ifdef SHOW_OTHER_TIMES
        totalLockTime += timeFunction([]{
        #endif
            /* lock the queue mutex */
            pthread_mutex_lock(&queueMutex);
        #ifdef SHOW_OTHER_TIMES
        }).count();
        #endif
        
        #ifdef SHOW_OTHER_TIMES
        totalBusyWaitTime += timeFunction([] {
        #endif
            /* keep waiting until theres an element in the queue */
            while (queue.empty() && writing) {
                pthread_cond_wait(&queueFullCond, &queueMutex);
            }
        #ifdef SHOW_OTHER_TIMES
        }).count();
        #endif

        /* get the front element of the queue */
        if (writing) {
            item = queue.front();
            queue.pop_front();

            /* stop the loop when both the queue is empty and all readers have stopped */
            if (queue.empty() && !reading) {
                writing = false;
                pthread_cond_broadcast(&queueFullCond);
            }
        }

        /* unlock the queue mutex */
        pthread_mutex_unlock(&queueMutex);

        /* broadcast that there is empty space in the queue */
        pthread_cond_broadcast(&queueEmptyCond);

        #ifdef SHOW_OTHER_TIMES
        totalWriteTime += timeFunction([&item]{
        #endif
            /* write the element to the file */
            std::ignore = write(outfile, item.c_str(), item.length());
        #ifdef SHOW_OTHER_TIMES
        }).count();
        #endif

        /* unlock the outfile mutex */
        pthread_mutex_unlock(&outfileMutex);
    }

    #ifdef SHOW_OTHER_TIMES
    /* set the times for the writer threads */
    writerTimes[index].busyWaitTime = totalBusyWaitTime;
    writerTimes[index].lockTime = totalLockTime;
    writerTimes[index].processTime = totalWriteTime;

    #endif
    
    /* cleanup */
    delete params;

    /* exit the thread */
    return nullptr;
}

/* starting the copying threads */
void startCopierThreads(int numThreads, const char* infileName, const char* outfileName)
{  
    queue.resize(QUEUE_MAX_SIZE);

    /* set reading flag to true */
    reading = true;
    writing = true;

    /* reader threads */
    pthread_t readers[numThreads];
    pthread_t writers[numThreads];

    /* initialise mutexes */
    pthread_mutex_init(&queueMutex, nullptr);
    pthread_mutex_init(&infileMutex, nullptr);
    pthread_mutex_init(&outfileMutex, nullptr);

    /* initialise conditionals */
    pthread_cond_init(&queueFullCond, nullptr);
    pthread_cond_init(&queueEmptyCond, nullptr);

    /* open the infile */
    infile = open(infileName, O_RDONLY);
    /* check if the infile exists */
    if (infile == FILE_OPEN_ERR){
        const std::string errMsg = "Could not find infile";
        throw std::runtime_error(errMsg);
    }

    /* open the outfile */
    outfile = open(outfileName, O_WRONLY|O_CREAT|O_TRUNC, 0644);
    /* check if the outfile exists */
    if (outfile == FILE_OPEN_ERR) {
        const std::string errMsg = "Could not find outfile";
        throw std::runtime_error(errMsg);
    }

    /* create reader and writer threads */
    for (int i = 0; i < numThreads; ++i) {
        int* index1 = new int(i);
        if (pthread_create(&readers[i], nullptr, &reader, index1) != THREAD_SUCCESS) {
            const std::string errMsg = "Failed to create reader thread";
            throw std::runtime_error(errMsg);
        }
        int* index2 = new int(i);
        if (pthread_create(&writers[i], nullptr, &writer, index2) != THREAD_SUCCESS) {
            const std::string errMsg = "Failed to create writer thread";
            throw std::runtime_error(errMsg);
        }
    }

    /* join reader and writer threads */
    for (int i = 0; i < numThreads; ++i) {
        if (pthread_join(readers[i], nullptr) != THREAD_SUCCESS) {
            const std::string errMsg = "Failed to join reader thread";
            throw std::runtime_error(errMsg);
        }
        if (pthread_join(writers[i], nullptr) != THREAD_SUCCESS) {
            const std::string errMsg = "Failed to join writer thread";
            throw std::runtime_error(errMsg);
        }
    }

    /* destroy mutexes */
    pthread_mutex_destroy(&queueMutex);
    pthread_mutex_destroy(&infileMutex);
    pthread_mutex_destroy(&outfileMutex);

    /* destroy conditionals */
    pthread_cond_destroy(&queueFullCond);
    pthread_cond_destroy(&queueEmptyCond);
}

int main(int argc, char** argv) {

    const std::string cmdErrorMessage = "main: the correct command is: ./better_mtcopier <#threads> <infile> <outfile> <optional -t>";
    const std::string timerFlag = "-t";

    /* check number of cmd args */
    if (argc < MIN_NUM_ARGS || argc > MAX_NUM_ARGS) {

        throw std::runtime_error(cmdErrorMessage);
    }

    /* the cmd args */
    char* infileName = argv[INFILE_INDX];
    char* outfileName = argv[OUTFILE_INDX];
    int numThreads;

    /* try parse the number of threads */
    try {
        numThreads = std::stoi(argv[NUM_THREADS_INDX]);
    }
    catch(const std::exception& e) {
        throw std::runtime_error("main: invalid thread command argument format");
    }
    if (numThreads < 1) {
        throw std::runtime_error("main: thread command argument cannot be below 1");
    }

    /* check if the command has the timer flag*/
    if (argc == MAX_NUM_ARGS) {
        if (argv[TIMER_INDEX] == timerFlag) {
            showTime = true;
        } else {
            throw std::runtime_error(cmdErrorMessage);
        }
    }

    #ifdef SHOW_OTHER_TIMES
    /* set up the arrays to store the times for writer and reader threads */
    readerTimes = new threadtimes[numThreads];
    writerTimes = new threadtimes[numThreads];
    #endif

    /* start the threads */
    long totalActualTime = timeFunction([numThreads, &infileName, &outfileName]{
        startCopierThreads(numThreads, infileName, outfileName);
    }).count();

    /* display time */
    if (showTime) { 
        
        #if defined(SHOW_EACH_THREAD_TIME) && defined(SHOW_OTHER_TIMES)
            /* display times for each thread */
            for (int i = 0; i < numThreads; ++i) {
                std::cout << "---READER THREAD " << i << "---" << std::endl;
                std::cout << "busy wait time: " << readerTimes[i].busyWaitTime / NS_PER_MS << " ms" << std::endl;
                std::cout << "lock time: " << readerTimes[i].lockTime / NS_PER_MS << " ms" << std::endl;
                std::cout << "read time: " << readerTimes[i].processTime / NS_PER_MS << " ms" << std::endl;

                std::cout << "---WRITER THREAD " << i << "---" << std::endl;
                std::cout << "busy wait time: " << writerTimes[i].busyWaitTime / NS_PER_MS << " ms" << std::endl;
                std::cout << "lock time: " << writerTimes[i].lockTime / NS_PER_MS << " ms" << std::endl;
                std::cout << "write time: " << writerTimes[i].processTime / NS_PER_MS << " ms" << std::endl;
            }
        #endif

        #ifdef SHOW_OTHER_TIMES
        /* calculate the total times */
        long totalReadBusyWaitTime = std::accumulate(readerTimes, readerTimes + numThreads, 0L, 
            [](long s, const threadtimes& t){ return s + t.busyWaitTime; });
        long totalReadLockTime = std::accumulate(readerTimes, readerTimes + numThreads, 0L, 
            [](long s, const threadtimes& t){ return s + t.lockTime; });
        long totalReadTime = std::accumulate(readerTimes, readerTimes + numThreads, 0L, 
            [](long s, const threadtimes& t){ return s + t.processTime; });

        long totalWriteBusyWaitTime = std::accumulate(writerTimes, writerTimes + numThreads, 0L, 
            [](long s, const threadtimes& t){ return s + t.busyWaitTime; });
        long totalWriteLockTime = std::accumulate(writerTimes, writerTimes + numThreads, 0L, 
            [](long s, const threadtimes& t){ return s + t.lockTime; });
        long totalWriteTime = std::accumulate(writerTimes, writerTimes + numThreads, 0L, 
            [](long s, const threadtimes& t){ return s + t.processTime; });

        long totalBusyWaitTime = totalWriteBusyWaitTime + totalReadBusyWaitTime;
        long totalLockTime = totalReadLockTime + totalWriteLockTime;
        #endif

        /* show the end results */
        std::cout << "===FINAL STATS===" << std::endl;
        #ifdef SHOW_OTHER_TIMES
        std::cout << "READ BUSY WAIT TIME TOTAL: " << totalReadBusyWaitTime / NS_PER_MS << " ms" << std::endl;
        std::cout << "WRITE BUSY WAIT TIME TOTAL: " << totalWriteBusyWaitTime / NS_PER_MS << " ms" << std::endl;
        std::cout << "BUST WAIT TIME TOTAL" << totalBusyWaitTime / NS_PER_MS << " ms" << std::endl;
        std::cout << "READ LOCK TIME TOTAL: " << totalReadLockTime / NS_PER_MS << " ms" << std::endl;
        std::cout << "WRITE LOCK TIME TOTAL: " << totalWriteLockTime / NS_PER_MS << " ms" << std::endl;
        std::cout << "LOCK TIME TOTAL" << totalLockTime / NS_PER_MS << " ms" << std::endl;
        std::cout << "READ TIME TOTAL: " << totalReadTime / NS_PER_MS << " ms" << std::endl;
        std::cout << "WRITE TIME TOTAL: " << totalWriteTime / NS_PER_MS << " ms" << std::endl;
        #endif
        std::cout << "TOTAL ACTUAL TIME: " << totalActualTime / NS_PER_MS << " ms" << std::endl;
        #ifdef SHOW_HIGHEST_QUEUE_SIZE
        std::cout << "HIGHEST QUEUE SIZE: " << highestQueueSize << std::endl;
        #endif
    }

    #ifdef SHOW_OTHER_TIMES
    /* clean up */
    delete[] writerTimes;
    delete[] readerTimes;
    #endif

    return EXIT_SUCCESS;
}

  
