#ifndef THREADTIMES_H
#define THREADTIMES_H

/* class used for timing locks, busy waits, reads and writes */
class threadtimes 
{   
    public:
        long processTime;
        long lockTime;
        long busyWaitTime;
        threadtimes(): processTime(0), lockTime(0), busyWaitTime(0) {};
};

#endif