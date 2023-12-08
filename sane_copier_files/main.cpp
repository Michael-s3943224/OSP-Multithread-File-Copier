/*
This is the direct copier
This represent a more typical way someone would actually copy files
*/

#include <stdexcept>
#include <queue>
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

/* 
* copy the contents of a file into another 
* why put it in a queue when you can straight up put it in the output file?
*/
void copyFile(const std::string& infileName, const std::string& outfileName) {
    // this was actually not as fast as I expected, huh?

    /* open the infile for reading */
    std::ifstream infile(infileName, std::ifstream::binary);

    /* throw an error if we can't find the infile */
    if (!infile) {
        const std::string fileNotFoundMsg = "copyFile: cannot find infile";
        throw std::runtime_error(fileNotFoundMsg);
    }

    /* open the outfile for writing while also clearing its contents*/
    std::ofstream outfile(outfileName, std::ofstream::binary | std::ofstream::trunc);

    /* throw an error if we can't find the outfile*/
    if (!outfile) {
        const std::string fileNotFoundMsg = "copyFile: cannot find outfile";
        throw std::runtime_error(fileNotFoundMsg);
    }

    /* read from the infile and write directly to the outfile */
    char buffer[READ_CHUNK];
    while (!infile.eof())
    {
        infile.read(buffer, READ_CHUNK);
        std::streamsize len = infile.gcount();
        outfile.write(buffer, len);
    }

    /* don't forget to close the files! */
    infile.close();
    outfile.close();
}

int main(int argc, char** argv) {

    const std::string cmdErrorMessage = "main: the correct command is: ./sane_copier <infile> <outfile> <optional -t>";
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

    /* copy the file */
    long totalTime = timeFunction([&infileName, &outfileName] { copyFile(infileName, outfileName); }).count();

    /* display the time taken */
    if (showTime) {
        std::cout << "----COPYING STATS----" << std::endl;
        std::cout << "total time: " << totalTime / NS_PER_MS << " ms" << std::endl;
    }
}