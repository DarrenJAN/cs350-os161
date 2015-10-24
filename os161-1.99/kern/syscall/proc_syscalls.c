#include <types.h>
#include <kern/errno.h>
#include <kern/unistd.h>
#include <kern/wait.h>
#include <lib.h>
#include <syscall.h>
#include <current.h>
#include <proc.h>
#include <thread.h>
#include <addrspace.h>
#include <copyinout.h>
  /* this implementation of sys__exit does not do anything with the exit code */
  /* this needs to be fixed to get exit() and waitpid() working properly */

void sys__exit(int exitcode) {

  struct addrspace *as;
  struct proc *p = curproc;
  /* for now, just include this to keep the compiler from complaining about
     an unused variable */
  (void)exitcode;

  DEBUG(DB_SYSCALL,"Syscall: _exit(%d)\n",exitcode);

  KASSERT(curproc->p_addrspace != NULL);
  as_deactivate();
  /*
   * clear p_addrspace before calling as_destroy. Otherwise if
   * as_destroy sleeps (which is quite possible) when we
   * come back we'll be calling as_activate on a
   * half-destroyed address space. This tends to be
   * messily fatal.
   */
  as = curproc_setas(NULL);
  as_destroy(as);

  /* detach this thread from its process */
  /* note: curproc cannot be used after this call */
  proc_remthread(curthread);

  /* if this is the last user process in the system, proc_destroy()
     will wake up the kernel menu thread */
  proc_destroy(p);
  
  thread_exit();
  /* thread_exit() does not return, so we should never get here */
  panic("return from thread_exit in sys_exit\n");
}

/*
Fork system call. 
ctf-> current trap frame
retval -> child proc PID
child Prod returns value via enter_forked_process
*/

int sys_fork(struct trapframe *ctf, pid_t *retval) {
  struct proc *curProc = curproc;
  struct proc *newProc = proc_create_runprogram(curProc->p_name);

  if(newProc == NULL) {
    DEBUG(DB_SYSCALL, "sys_fork_error: Wasn't able to make new process.\n");
    return ENPROC;
  }
  DEBUG(DB_SYSCALL, "sys_fork: New process created.\n");

  as_copy(curproc_getas(), &(newProc->p_addrspace));

  if(newProc->p_addrspace == NULL) {
    DEBUG(DB_SYSCALL, "sys_fork_error: Couldn't make addrspace for new process.\n");
    proc_destroy(newProc);
    return ENOMEM;
  }

  DEBUG(DB_SYSCALL, "New addrspace created.\n");

  struct trapframe *ntf = kmalloc(sizeof(struct trapframe));

  if (ntf == NULL)
  {
    DEBUG(DB_SYSCALL, "sys_fork_error: Couldn't create trapframe for new process.\n");
    proc_destroy(newProc);
    return ENOMEM;
  }

  memcpy(ntf,ctf, sizeof(trapframe));
  DEBUG(DB_SYSCALL, "sys_fork: Created new trap frame\n");
  //still to add the child to parent

  *retval = newProc->pid;
  return 0;
}
/* stub handler for getpid() system call                */
int
sys_getpid(pid_t *retval)
{
  *retval = curproc->pid;
  return(0);
}

/* stub handler for waitpid() system call                */

int
sys_waitpid(pid_t pid,
	    userptr_t status,
	    int options,
	    pid_t *retval)
{
  int exitstatus;
  int result;

  /* this is just a stub implementation that always reports an
     exit status of 0, regardless of the actual exit status of
     the specified process.   
     In fact, this will return 0 even if the specified process
     is still running, and even if it never existed in the first place.

     Fix this!
  */

  if (options != 0) {
    return(EINVAL);
  }
  /* for now, just pretend the exitstatus is 0 */
  exitstatus = 0;
  result = copyout((void *)&exitstatus,status,sizeof(int));
  if (result) {
    return(result);
  }
  *retval = pid;
  return(0);
}

