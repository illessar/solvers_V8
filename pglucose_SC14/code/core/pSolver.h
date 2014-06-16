
namespace Glucose
{
  class pSolver;
}


#ifndef Glucose_pSolver_h
#define Glucose_pSolver_h

#include "core/Solver.h"
#include "core/SharedJob.h"
#include "core/SharedBase.h"
#include <pthread.h>
#include "mtl/Queue.h"
#include <gmp.h>

#define MAX_JOBS 1024

namespace Glucose {

  class pSolver : public Solver{
    
  protected :
    static int nbworkers;
    static double const percentage = 0.6;
    static double const pfstopDB = 0.05;
    static double const pfstopNB = 0.75;
    static double const restart_base = 10000;
    static double const distance = 0;
    static pSolver ** solvers;
    static int folio;
    static Queue<Lit> vars;

    int          id;
    double       thtime;      
    unsigned int maxsizeshared;
    SharedBase   *sharedBase;
    Shared       *sharedJob;
    unsigned int nbjobs;
 
    // Create and start a thread
    pthread_t create_worker(void *arg);
 
    //Replay an assumption 
    virtual lbool playAssumptions();
    //Associate a job to a thread 
    static void* bootstrap(void * arg);

    // The solving algorithems (DPLL based)
    lbool search(int nof_conflicts);
    lbool search(Lit);
    void getPromisingOrder();

    //Pricise The work to do by a thread
    virtual  void* thread_work();
    // Create Jobs with respect to a height h
    virtual bool createJobs(unsigned int);
    //Main loop for a thread: pick a subproblem resolve it 
    //and passe to a new one if any.
    virtual void searchSubProblem();    
    bool grestart(pSolver* []);
    int getbpick(pSolver* []);
    void getfirstLits();
      
  
  public :
   
    friend class SharedBase;

    pSolver():
      id(0),
      thtime(0),    
      maxsizeshared(0),
      sharedBase(0),
      sharedJob(0),
      nbjobs(0){}
    
    virtual void solveMultiThreaded(int h, int t, pSolver *datas[]);
    void printStats();

    double gettime() const {return thtime;}
    int getNbjobs()const{return nbjobs;}
    void setId(int id){this->id =id;}
    void setSdb(SharedBase *sb){sharedBase=sb;}
    void setSqueue(Shared *sq){sharedJob=sq;}
    void setMsshared(int mss){maxsizeshared=mss;}
    bool stop(mpz_t ret){
      return (mpz_cmp_si(ret, -1)==0 ||
	      mpz_cmp_si(ret, -1)<0  ||
	      mpz_cmp_si(ret, 1)==0  ||
	      mpz_cmp_si(ret, 1)>0);
    }
   
  };
}
#endif
