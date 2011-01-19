#ifndef PARALLELPP_TASK_H
#define PARALLELPP_TASK_H

#include "utilspp/mach_types.h"
#include <mpc.h>

class MPC;

class TaskInfo {

protected:

    int_type taskid;
    MPC *mpcpointer;
    bool aff_reduction;
    
    // ArrayT<ReducMessage> buf_reduc_int;
    
public:

    virtual ~TaskInfo() { }

    TaskInfo() {
	aff_reduction = true;
    }
    
  //   void ReductionCollector(array_u_int_type buf, char *mess);
//     void ReductionCollectorAffiche();
    
    
    bool AffReduction() const {
	return aff_reduction;
    }
    
    void SetAffReduction(bool a) {
	aff_reduction = a;
    }
    
    virtual void SetTaskID(c_int_type_p pid) {
	taskid = *pid;
    }

    virtual int_type GetTaskID() const {
	return taskid;
    }

    virtual bool Ihavetoprint() const {
	return ( GetTaskID() == 0 );
    }

    virtual void SetMPCpointer( MPC *mpcp ) {
	mpcpointer = mpcp;
    }

    virtual MPC *GetMPCpointer() const {
	return mpcpointer;
    }

};

class TaskInfo_MT: public TaskInfo {
protected:
    mpc_thread_key_t thread_key;
    struct internal_taskinfo_s{
      int_type taskid;
      MPC *mpcpointer;
      bool aff_reduction;
    } internal_taskinfo_t;
        

public:
    TaskInfo_MT(){
      static mpc_thread_mutex_t lock = MPC_THREAD_MUTEX_INITIALIZER;
      static volatile int done = 0;
      mpc_thread_mutex_lock(&lock);
      if(done == 0){
	mpc_thread_key_create(&thread_key,NULL);
	done = 1; 
      }
      mpc_thread_mutex_unlock(&lock);
    }
    void InitTaskInfo(){
      struct internal_taskinfo_s* tab;
      tab = (struct internal_taskinfo_s*)malloc(sizeof(struct internal_taskinfo_s));
      tab->aff_reduction = true;
      mpc_thread_setspecific(thread_key,(void*)(tab));
    }

    bool AffReduction() const {
      struct internal_taskinfo_s* tab;
      tab = (struct internal_taskinfo_s*)mpc_thread_getspecific(thread_key);
      return tab->aff_reduction;
    }
    
    void SetAffReduction(bool a) {
      struct internal_taskinfo_s* tab;
      tab = (struct internal_taskinfo_s*)mpc_thread_getspecific(thread_key);
      tab->aff_reduction = a;
    }
    
    virtual void SetTaskID(c_int_type_p pid) {
      struct internal_taskinfo_s* tab;
      tab = (struct internal_taskinfo_s*)mpc_thread_getspecific(thread_key);
      tab->taskid = *pid;
    }

    virtual int_type GetTaskID() const {
      struct internal_taskinfo_s* tab;
      tab = (struct internal_taskinfo_s*)mpc_thread_getspecific(thread_key);
      return tab->taskid;
    }

    virtual bool Ihavetoprint() const {
      return ( GetTaskID() == 0 );
    }

    virtual void SetMPCpointer( MPC *mpcp ) {
      struct internal_taskinfo_s* tab;
      tab = (struct internal_taskinfo_s*)mpc_thread_getspecific(thread_key);
      tab->mpcpointer = mpcp;
    }

    virtual MPC *GetMPCpointer() const {
      struct internal_taskinfo_s* tab;
      tab = (struct internal_taskinfo_s*)mpc_thread_getspecific(thread_key);
      return tab->mpcpointer;
    }

    virtual ~TaskInfo_MT() {
    }
} ;

extern TaskInfo *taskinfo;

#endif
