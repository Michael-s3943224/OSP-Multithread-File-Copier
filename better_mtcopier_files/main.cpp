/*
This aims to copy files with multithreading without locks and waiting
this is from the result of this implementation not needing a queue

PLEASE NOTE THAT THIS ONLY WORKS ON LINUX
FOR SOME REASON FSEEK AND SEEKP DON'T WORK
AND I DON'T KNOW WHY :C

Again, I'm sorry with the macros
but these timing functions really slow everything down
*/

#include <pthread.h>
#include <iostream>
#include <stdexcept>
#include <fcntl.h> 
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <chrono>
#include <functional>

#include "copierparams.h"
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
/* file open error*/
#define FILE_OPEN_ERR -1
/* convert nano seconds to ms*/
#define NANO_PER_MS 1000000
/* read write access */
#define READ_WRITE_ACCESS 0644

/* whether to show the time for each thread */
//#define SHOW_EACH_THREAD_TIME
/* wehter to show other times */
//#define SHOW_OTHER_TIMES

/*----GLOBAL VARIABLES-----*/
/* whether we should show the time */
bool showTime = false;
/* keeps track of thread times*/
threadtimes* threadTimes;

/* used to time functions */
std::chrono::nanoseconds timeFunction(const std::function<void()>& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return duration;
}

/* function which is used to hopefully get a file's size */
long getFileSize(const char* fileName) {
    struct stat st;
    if (stat(fileName, &st) == 0) {
        return st.st_size;
    }
    return -1;
}

/* function which is used to clear a file */
void clearFile(const char* fileName) {
    close(open(fileName, O_WRONLY | O_TRUNC));
}

/* used to find the slowest thread */
int slowestThread(threadtimes* threadTimes, int len) {
    if (len <= 0 || threadTimes == nullptr) {
        return -1;
    }

    int indx = 0;
    for (int i = 0; i < len; ++i) {
        if (threadTimes[i].totalTime > threadTimes[indx].totalTime) {
            indx = i;
        }
    }

    return indx;
}

/* runner for each thread to copy a file's contents */
void* copierThread(void* arg) {
    #ifdef SHOW_OTHER_TIMES
    /* this is just for timing stuff */
    long totalReadTime = 0;
    long totalWriteTime = 0;
    #endif

    /* cast the arg pointer so we can now have an easier time accessing stuff*/
    copierparams* params = (copierparams*) arg;

    /* the buffer to store the characters */
    char buffer[READ_CHUNK];

    /* open the infile in read only */
    int infile = open(params->infileName, O_RDONLY);

    /* check if we actually opened infile */
    if (infile == FILE_OPEN_ERR) {
        const std::string errMsg = "Could not open infile!";
        throw std::runtime_error(errMsg);
    }

    /* open the outfile with write permisions and create it if it doesn't exist */
    int outfile = open(params->outfileName, O_WRONLY|O_CREAT, READ_WRITE_ACCESS);

    /* check if we actually opened outfile */
    if (infile == FILE_OPEN_ERR) {
        const std::string errMsg = "Could not open outfile!";
        throw std::runtime_error(errMsg);
    }

    /* 
    * go to starting position for both files
    * where the chunk of text is going to be read and written from
    */
    lseek(infile, params->position, SEEK_SET);
    lseek(outfile, params->position, SEEK_SET);
    
    /* loop through the bytes and perform the copy */
    for (long b = 0; b < params->bytes; b += READ_CHUNK) {
        /* read a chunk from the infile and put it in the buffer */
        #ifdef SHOW_OTHER_TIMES
        totalReadTime += timeFunction([infile, &buffer]{
        #endif
            std::ignore = read(infile, buffer, READ_CHUNK); 
        #ifdef SHOW_OTHER_TIMES
        }).count();
        #endif
        /* 
        * make sure a we don't take the entire last chunk
        * just in case if a bit of the last chunk is assigned for another thread
        */
        long length = READ_CHUNK;
        if (b + READ_CHUNK > params->bytes) {
            length = params->bytes - b;
        }
        /* write to the output file */
        #ifdef SHOW_OTHER_TIMES
        totalWriteTime += timeFunction([outfile, length, &buffer]{
        #endif
            std::ignore = write(outfile, buffer, length); 
        #ifdef SHOW_OTHER_TIMES
        }).count();
        #endif
    }

    #ifdef SHOW_OTHER_TIMES
    long totalTime = totalReadTime + totalWriteTime;

    /* set the time for the corresponding thread */
    threadTimes[params->id].readTime = totalReadTime;
    threadTimes[params->id].writeTime = totalWriteTime;
    threadTimes[params->id].totalTime = totalTime;
    #endif

    /* don't forget to close the files */
    close(infile);
    close(outfile);
    /* don't forget to clear memory*/
    delete params;
    /* exit */
    return nullptr;
}

/* start the copier threads */
void startCopierThreads(int numThreads, const char* infileName, const char* outfileName)
{
    const std::string threadCreateErrMsg = "could not create thread";
    const std::string threadJoinErrMsg = "could not join thread";

    /* copier threads */
    pthread_t copiers[numThreads];

    /* get the file size for in file */
    long infileSize = getFileSize(infileName);
    /* clear the output file */
    clearFile(outfileName);
    /* the minimum number of bytes processed for each thread */
    long bytesPerThread = infileSize / numThreads;

    for (int i = 0; i < numThreads; ++i)
    {
        long position = i * bytesPerThread;
        long bytes = (i == numThreads - 1) ? infileSize - position : bytesPerThread;
        copierparams* cParams = new copierparams(i, infileName, outfileName, position, bytes);

        if (pthread_create(&copiers[i], nullptr, &copierThread, cParams) != THREAD_SUCCESS) {
            throw std::runtime_error(threadCreateErrMsg);
        }
    }

    for (int i = 0; i < numThreads; ++i)
    {
        if (pthread_join(copiers[i], nullptr) != THREAD_SUCCESS) {
            throw std::runtime_error(threadJoinErrMsg);
        }
    }
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
    /* initialise the thread time arrays*/
    threadTimes = new threadtimes[numThreads];
    #endif

    /* start the threads */
    long totalActualTime = timeFunction([numThreads, &infileName, &outfileName]{
        startCopierThreads(numThreads, infileName, outfileName);
    }).count();

    /* display the times */
    if (showTime) { 
        #if defined(SHOW_EACH_THREAD_TIME) && defined(SHOW_OTHER_TIMES)
        for (int i = 0; i < numThreads; ++i) {
            std::cout << "---THREAD " << i << " STATS---" << std::endl;
            std::cout << "read time*: " << threadTimes[i].readTime / NANO_PER_MS << " ms" << std::endl;
            std::cout << "write time*: " << threadTimes[i].writeTime / NANO_PER_MS << " ms" << std::endl;
            std::cout << "total time*: " << threadTimes[i].totalTime / NANO_PER_MS << " ms" << std::endl;
        }
        #endif

        std::cout << "===FINAL STATS===" << std::endl;
        #ifdef SHOW_OTHER_TIMES
        int slowestThreadIndx = slowestThread(threadTimes, numThreads);
        std::cout << "SLOWEST THREAD TOTAL READ: " << threadTimes[slowestThreadIndx].readTime / NANO_PER_MS << " ms" << std::endl;
        std::cout << "SLOWEST THREAD TOTAL WRITE: " << threadTimes[slowestThreadIndx].writeTime / NANO_PER_MS << " ms" << std::endl;
        std::cout << "SLOWEST THREAD TOTAL TIME (READ + WRITE): " << threadTimes[slowestThreadIndx].totalTime / NANO_PER_MS << " ms" << std::endl; 
        #endif
        std::cout << "TOTAL ACTUAL TIME: " << totalActualTime / NANO_PER_MS << " ms" << std::endl;
    }

    #ifdef SHOW_OTHER_TIMES
    /* clean up the arrays for thread times */
    delete[] threadTimes;
    #endif

    return EXIT_SUCCESS;
}

  