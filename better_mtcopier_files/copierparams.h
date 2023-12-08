#ifndef COPIER_H
#define COPIER_H

/* 
* params for the copier threads 
* used to know where in the input file to read from
* and how many bytes to read
*/
class copierparams
{
    public:
        const long id;
        const char* infileName;
        const char* outfileName;
        const long position;
        const long bytes;
        copierparams(long i, const char* ifile, const char* ofile, long p, long b) :
            id(i), infileName(ifile), outfileName(ofile), position(p), bytes(b) {};
};

#endif