#ifndef THREADTIMES_H
#define THREADTIMES_H

/* class used for timing reads and writes */
class threadtimes 
{   
    public:
        long readTime;
        long writeTime;
        long totalTime;
        threadtimes(): readTime(0), writeTime(0), totalTime(0) {};
};

#endif