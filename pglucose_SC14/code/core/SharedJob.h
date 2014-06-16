#ifndef Minisat_SharedJob_h
#define Minisat_SharedJob_h

#include <semaphore.h>
#include <gmp.h>
#include <pthread.h>
#include "mtl/Vec.h"
#include "core/SolverTypes.h"

#define SIZE_FACTOR 2
#define TYPE Lit

namespace Glucose {

  //This is the class that represents 
  //a subproblem : the vector assumps 
  //stores a partial assignment
  class Job {
  public:
    vec<TYPE> assumps;
    int jid;
    int fid;
    Job& operator=(const Job& j) {
      fid = j.fid;
      jid = j.jid;
      j.assumps.copyTo(assumps);
	return *this;
    }  
  };
  
  // This class represents 
  // the shared queue of subproblems
  // to be resolved
  class Shared {
  private:
    Job            *jobs;
    unsigned int    newjid;
    unsigned int    size;
    unsigned int    first;
    unsigned int    last;
    mpz_t           result;
    
    int             waiting;
    int             threads;
    int             height;
    
    // Synchro variables
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
    bool            end;

  public:
    pthread_t      *tids;

    //Constructor:
    Shared(int threads_, int height_);
    Shared(int threads_, int height_, 
	   unsigned int sizein);
    void init(int threads_, 
	      int height_, 
	      unsigned int size_);
    ~Shared();
    
    //Old Interface:
    void push(vec<TYPE>& in);
    //Returns 1 if success to pop, 
    //otherwise 0 queue is empty
    bool pop(vec<TYPE>& out);
  
    void push(vec<TYPE>& in, int fid);
    bool pop(Job& j);

    void putResult(mpz_t r);

    void IFinish();
    
    //Accessors:
    void getResult(mpz_t& out);
    int getJobLeft() const  {
      if(last >= first) return  last - first;
      else return size - first + last;
    }
    int getHeight() const { return height; }
    int getThreads() const { return threads; }
    int getNbJobs() const { return newjid; }

    void endAll();
  };
}

#endif
