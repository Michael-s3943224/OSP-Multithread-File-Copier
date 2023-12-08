/*
This is the single-threaded copier which uses a queue
*/

#include <stdexcept>
#include <queue>
#include <string>
#include <iostream>
#include <fstream>
#include <chrono>
#include <functional>

/*----CONSTANTS----*/
/* cmd args position for infile */
#define INFILE_INDX 1
/* cmd args position for outfile */
#define OUTFILE_INDX 2
/* cmd args position for timer flag*/
#define TIMER_INDEX 3
/* min number of cmd args */
#define MIN_NUM_ARGS 3
/* max number of cmd args*/
#define MAX_NUM_ARGS 4
/* num bytes read at a time */
#define READ_CHUNK 32768
/* convert nano seconds to ms*/
#define NS_PER_MS 1000000

/* used to time functions */
std::chrono::nanoseconds timeFunction(const std::function<void()>& func) {
    auto start = std::chrono::high_resolution_clock::now();
    func();
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    return duration;
}

/* read file contents into a queue */
void fileIntoQueue(const std::string& infileName, std::queue<std::string>& queue) {
    /* current bytes from file */
    char buffer[READ_CHUNK];

    /* open the file in binary mode */
    std::ifstream file (infileName, std::ifstream::binary);

    /* throw an error if we can't find the file */
    if (!file) {
        const std::string fileNotFoundMsg = "fileIntoQueue: cannot find infile";
        throw std::runtime_error(fileNotFoundMsg);
    }

    /* keep reading chunk by chunk until we get to the end*/
    while (!file.eof()) {
        file.read(buffer, READ_CHUNK);
        /* last chunk might not be the same size as the rest*/
        std::streamsize len = file.gcount();
        queue.push(std::string(buffer, len));
    }

    /* don't forget to close the file! */
    file.close();
}

/* write into file from queue */
void fileFromQueue(const std::string& outfileName, std::queue<std::string>& queue) {
    /* open the file in binary mode and clear it*/
    std::ofstream file(outfileName, std::ofstream::binary | std::ofstream::trunc);

    if (!file) {
        const std::string fileNotFoundMsg = "fileFromQueue: cannot find outfile";
        throw std::runtime_error(fileNotFoundMsg);
    }

    /* keep writing to file from queue until its empty */
    while (!queue.empty()) {
        file << queue.front();
        queue.pop();
    }

    /* don't forget to close the file! */
    file.close();
}

int main(int argc, char** argv) {

    const std::string cmdErrorMessage = "main: the correct command is: ./copier <infile> <outfile> <optional -t>";
    const std::string timerFlag = "-t";

    /* check number of cmd args */
    if (argc < MIN_NUM_ARGS || argc > MAX_NUM_ARGS) {

        throw std::runtime_error(cmdErrorMessage);
    }

    /* the cmd args */
    std::string infileName = argv[INFILE_INDX];
    std::string outfileName = argv[OUTFILE_INDX];
    bool showTime = false;

    /* check if the command has the timer flag*/
    if (argc == MAX_NUM_ARGS) {
        if (argv[TIMER_INDEX] == timerFlag) {
            showTime = true;
        } else {
            throw std::runtime_error(cmdErrorMessage);
        }
    }

    /* the queue to store the file chunks*/
    std::queue<std::string> queue;

    /* copy the file */
    long readTime = timeFunction([&infileName, &queue] { 
        fileIntoQueue(infileName, queue); }).count();
    long writeTime = timeFunction([&outfileName, &queue] { 
        fileFromQueue(outfileName, queue); }).count();
    long totalTime = readTime + writeTime;

    /* display the time taken */
    if (showTime) {
        std::cout << "----COPYING STATS----" << std::endl;
        std::cout << "read time: " << readTime / NS_PER_MS << " ms" << std::endl;
        std::cout << "write time: " << writeTime / NS_PER_MS << " ms" << std::endl;
        std::cout << "total time: " << totalTime / NS_PER_MS << " ms" << std::endl;
    }
}