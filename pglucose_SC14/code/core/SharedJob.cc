#include "core/SharedJob.h"
#include <cstdio>
#include <unistd.h>

using namespace Glucose;
void _clean_(void *arg) {
  pthread_mutex_unlock((pthread_mutex_t*)arg); 
}

Shared::Shared(int threads_, int height_): 
    jobs           (new Job[1])
  , newjid         (0)
  , size           (1)
  , first          (0)
  , last           (0)
  , waiting        (0)
  , threads        (threads_)
  , height         (height_)
  , end            (false)
  , tids             (new pthread_t[threads_-1])
  
{
  pthread_mutex_init(&mutex, 0);
  pthread_cond_init(&cond, 0);
  mpz_init(result);
  mpz_set_ui(result, 0);
}

Shared::Shared(int threads_, int height_, unsigned int size_): 
    jobs           (new Job[size_])
  , newjid         (0)
  , size           (size_)
  , first          (0)
  , last           (0)
  , waiting        (0)
  , threads        (threads_)
  , height         (height_)
  , end            (false)
  , tids             (new pthread_t[threads_-1]) {
  pthread_mutex_init(&mutex, 0);
  pthread_cond_init(&cond, 0);
  mpz_init(result);
  mpz_set_ui(result, 0);
}
  
Shared::~Shared(){
  delete[] jobs;
  delete[] tids;
  pthread_mutex_destroy(&mutex);
  pthread_cond_destroy(&cond);
  mpz_clear(result);
}

void Shared::init(int threads_, 
		  int height_, 
		  unsigned int size_){

  delete[] jobs;
  pthread_mutex_init(&mutex, 0);
  pthread_cond_init(&cond, 0);
  mpz_set_ui(result, 0);
  jobs=new Job[size_];
  newjid =  0;
  size =size_;
  first = 0;
  last  = 0;
  waiting  = 0;
  threads = threads_;
  height = height_;
  end  = false;
}

void Shared::push(vec<TYPE>& in) {
  pthread_cleanup_push(_clean_, &mutex);
  pthread_mutex_lock(&mutex);

  if ((last+1) % size == first) {
    Job* tmp = new Job[size * SIZE_FACTOR];
    for(unsigned int i = 0; i < size; i++) {
      tmp[i] = jobs[(first+i)%size];
    }
    first = 0;
    last = size-1;
    size *= SIZE_FACTOR;
    delete[] jobs;
    jobs = tmp;
  }
  jobs[last].jid = newjid++;
  in.copyTo(jobs[last].assumps);
  last = (last+1) % size;
  pthread_cond_signal(&cond);

  pthread_mutex_unlock(&mutex);
  pthread_cleanup_pop(0);
}

void Shared::push(vec<TYPE>& in, int fid){
  pthread_cleanup_push(_clean_, &mutex);
  pthread_mutex_lock(&mutex);

  if ((last+1) % size == first) {
    Job* tmp = new Job[size * SIZE_FACTOR];
    for(unsigned int i = 0; i < size; i++) {
      tmp[i] = jobs[(first+i)%size];
    }
    first = 0;
    last = size-1;
    size *= SIZE_FACTOR;
    delete[] jobs;
    jobs = tmp;
  }
  jobs[last].fid = fid;
  jobs[last].jid = newjid++;
  in.copyTo(jobs[last].assumps);
  last = (last+1) % size;
  pthread_cond_signal(&cond);

  pthread_mutex_unlock(&mutex);
  pthread_cleanup_pop(0);
}


void Shared::endAll(){
  end =true;
  pthread_cond_broadcast(&cond);
 
  for (int i=0; i<threads-1; i++)
    if( pthread_self() != tids[i])
      pthread_cancel(tids[i]);
}

bool Shared::pop(vec<TYPE>& out) {
  bool ret = false;
 
  pthread_mutex_lock(&mutex);
  pthread_cleanup_push(_clean_, &mutex);
  
  //Detect the end of the solving: no more subproblem
  //and no thread is working  
  if (first == last && waiting == threads-1) {
     end = true;
    pthread_cond_broadcast(&cond);
  }
  
  //Wait for a new subproblem: at least 
  //one thread was working.
  while (!end && first == last) {
    waiting++;
    pthread_cond_wait(&cond, &mutex);
    waiting--;
  }

  //Pick a subproblem
  if(first != last) {
    jobs[first].assumps.copyTo(out);
    first = (first+1) % size;
    ret = true;
  }
 

 
  pthread_cleanup_pop(0);
  pthread_mutex_unlock(&mutex);
  return ret;
}

bool Shared::pop(Job& out){

  bool ret = false;

 
  pthread_mutex_lock(&mutex);
  pthread_cleanup_push(_clean_, &mutex);

  if (first == last && waiting == threads-1) {
    end = true;
    pthread_cond_broadcast(&cond);
  }

  while (!end && first == last) {
    waiting++;
    pthread_cond_wait(&cond, &mutex);
    waiting--;
  }

  if(first != last) {
    out = jobs[first];
    first = (first+1) % size;
    ret = true;
  }

  pthread_cleanup_pop(0);
  pthread_mutex_unlock(&mutex);

  return ret;
}

void Shared::putResult(mpz_t r) {
  
  pthread_mutex_lock(&mutex);
  pthread_cleanup_push(_clean_, &mutex);
  mpz_add(result,result,r);
  pthread_cleanup_pop(0);
  pthread_mutex_unlock(&mutex);

}

void Shared::getResult(mpz_t& out) {
  mpz_set(out, result);
}


void Shared::IFinish() {
 
  pthread_mutex_lock(&mutex);
  pthread_cleanup_push(_clean_, &mutex);
  waiting++;
  pthread_cleanup_pop(0);
  pthread_mutex_unlock(&mutex);


}
