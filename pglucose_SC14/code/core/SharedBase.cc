#include "core/SharedBase.h"
#include <iostream>

using namespace Glucose;

SharedBase::SharedBase(int threads_) : 
  threads (threads_){
  Lists = new ListLearn[threads_];
  for(int i=0; i<threads_; i++ ) {
    Lists[i].head = 0;
    Lists[i].tail = 0;
    Lists[i].sidx = new SelfishIdx[threads];
    Lists[i].rmfrqc = 0;
    Lists[i].nba = 0;
    Lists[i].nbs = 0;
    Lists[i].nbptr=0;
  }

  /*RAJOUT STAGE*/

  nb_updates = 0;

  /*FIN RAJOUT STAGE*/
}

SharedBase::~SharedBase()  {

  for(int i=0; i<threads; i++){
 
    elearn *cur = Lists[i].head;

    while(cur != 0){
      elearn *nxt= cur->next;
      delete cur; 
      cur=nxt;
    }
    
    delete[] Lists[i].sidx;
  }

  delete[] Lists;
}

// List Primitive 
void SharedBase::append(elearn *e, ListLearn *l)
{ 
  if (l->tail) 
    l->tail->next = e;
  else 
    l->head = e;
  l->tail = e;
}

/*
tete de la liste de partage
suppr toutes les clauses déjà partagées entre tous les autres threads.
*/
void SharedBase::clean(ListLearn *l) {
  
  elearn *cur = l->head;
 
  if (!cur) return;

  while(cur != 0 && cur->cref == 0){
    elearn *nxt= cur->next;
    delete cur;
    l->nbs++;
    cur=nxt;
  }

  if(!cur)
    l->tail=0;

  l->head = cur;
}



/*
sharedBase->push(learnt_clause, nblevels,this);
Solveur->id indique la file où rajouter le nouveau élément. 


*/
void SharedBase::push(vec<Lit>& learn, int nblev, pSolver *solver) {

  ListLearn &l = Lists[solver->id];
  elearn *e = new elearn(learn,nblev,threads-2);
  append(e, &l);
  l.nba++;  

  /* the reduce of the DB */
  if(solver->conflicts >=
     (uint64_t)solver->curRestart* 
     solver->nbclausesbeforereduce)
    clean(&Lists[solver->id]);
}



void SharedBase::update(pSolver * solver)
{
  //S'identifier
  int id = solver->id;
  elearn* j,*nxt;

  for (int i=1; i<threads; i++) {
    //Ne pas gerer sa propre base de données.
    if (id == i)
       continue;
      
    //Se positionner sur une des Bases de Données partagés
    ListLearn& l = Lists[i];

    //Si le pointeur d'une des listes pointe sur NULL, le reposoitionner à "head"
    if (!l.sidx[id].ptr) 
      j= l.sidx[id].ptr = l.head;
    else {
      //Le faire avancer d'un cran sinon (donc, possibilité de ponter sur NULL ici je suppose, d'où le precedent if)
      j = l.sidx[id].ptr->next;
      //si pointe sur du non vide, soustraire atomiquement 1 à l'indice de référence de la case précédente.
      if(j)
	__sync_sub_and_fetch(&(l.sidx[id].ptr->cref),1);
    }

    //j est une elearn, donc possède un ensemble de littéraux (= UNE CLAUSE), un nb de niveaux (?), pointeur sur suivant, et une référence vers ...
    while(j) { 
      //Tant que j ne pointe pas vers NULL, pas vide
      //se placer sur le vecteur des littéraux de j
      vec<Lit>& learn = j->learn;

      //S'il ne reste plus qu'un littéral, alors
      if (learn.size() == 1){
        //si valeur == 2, donc non défini, donc si tous les deux sont indéfinis à la base (normalement?)...les indéfinis appartiennent à NOUS
        //pas à la clause/littéral partagée.
	if (solver->value(learn[0]) == l_Undef)
	  solver->uncheckedEnqueue(learn[0]);
    //Prend en queue d'attente un littéral => enregistrer son signe (vrai/faux), enregistrer son niveau + sa raison, l'enregistrer dans trail.
    //On enregistre le littéral restnt qui est affecté chez le thread voisin, mais pas chez nous.
      }
      else {
      // S'il y a plusieurs littéraux, donc la clause partagée n'est pas unitaire, c'est là qu'on la gèle eventuellement!
      // Pour chaque nouvelle clause, si geler on la doit, on le fait.
	CRef cr = solver->ca.alloc(learn, true);
	solver->ca[cr].setLBD(j->nblevels); 
	solver->learnts.push(cr);
	solver->attachClause(cr);
	solver->claBumpActivity(solver->ca[cr]);

  /*RAJOUT STAGE*/

  // Indique si oui ou non, on doit geler la clause nouvellement apprise.
  // 

  solver->frozen_clauses.push(solver->mkFreeze(0, solver->to_freeze(cr)));
  //solver->frozen_clauses.push(solver->to_freeze(cr));

  /*FIN RAJOUT STAGE*/

      } 
      solver->varDecayActivity();
      solver->claDecayActivity();
      
      //Avancer au litteral suivant.
      l.sidx[id].ptr = j;
      nxt=j->next;

    //Si nxt != NULL, s'il reste des littéraux, soustraire atomiquement 1 à l'indice de référence de la case précédente (j).  
      if(nxt) 
	__sync_sub_and_fetch(&(j->cref),1);
      
      j=nxt;     
      
   }	
 } 
}
