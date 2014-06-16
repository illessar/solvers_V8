namespace Glucose {
  class SharedBase;
}

#ifndef Minisat_SharedBase_h
#define Minisat_SharedBase_h

/*RAJOUT STAGE*/

#define NB_UPDATES 100

/*FIN RAJOUT STAGE*/

#include "core/SolverTypes.h"
#include "core/pSolver.h"
#include <memory>

namespace Glucose {

  class SharedBase {
  private:
    struct elearn;    
    struct elearn {
      vec<Lit>    learn;
      int nblevels;
      int cref;
      elearn *next;
        
      elearn(vec<Lit>& l,int nbl,int th) : 
	nblevels(nbl),cref(th),next (0) { 
	l.copyTo(learn);	 
      }
    };

    struct SelfishIdx{
      elearn* ptr;
      SelfishIdx(): ptr (0) {}
       
    };

    struct ListLearn{
      elearn *head;
      elearn *tail;
      SelfishIdx* sidx;
      int rmfrqc;
      int nba;
      int nbs;
      int nbptr;
    };

    /*RAJOUT STAGE*/

    // Nombre de MAJ de la base de données depuis le dernier "freeze"
    int nb_updates;

    /*FIN RAJOUT STAGE*/

    //Attributs
    int threads;
    ListLearn *Lists;

    // Primitive for List
    void append(elearn *e, ListLearn *l);
    void clean(ListLearn *l);    
    //void tryCleanDB(Solver *solver);

  public:
    SharedBase(int threads);
    ~SharedBase();
    
    //Main interface
    void push(vec<Lit>& learn, int nbl,pSolver *solver);
    void update(pSolver *solver);
    int getTotalDB(int i) const { return Lists[i].nba ;}
    int getTotalSDB(int i) const { return Lists[i].nbs ;}
    int getNbPtr(int i) const { return Lists[i].nbptr ;}

  };
  
}
#endif
