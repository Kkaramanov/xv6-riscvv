#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "fs.h"
#include "sleeplock.h"
#include "file.h"

int
mutexalloc(struct file **fp)
{
  struct file *f;
  struct sleeplock *m;

  *fp = 0;
  f = filealloc();
  if (f == 0) {
    return -1;
  }

  f->type = FD_NONE;
  m = (struct sleeplock *)kalloc();
  if (m == 0) {
    fileclose(f);
    return -1;
  }

  MTRACE("mutex: pid=%d m=%p kalloc\n", myproc()->pid, m);

  initsleeplock(m, "mutex");
  f->mutex = m;
  f->readable = 0;
  f->writable = 0;
  f->type = FD_MUTEX;

  *fp = f;
  MTRACE("mutex: pid=%d m=%p mutexalloc\n", myproc()->pid, m);
  return 0;
}

void
mutexclose(struct sleeplock *m)
{
  // Called only after the last file reference has disappeared.
  // A process waiting for this mutex still owns a file reference.
  acquire(&m->lk);
  if (m->locked)
    panic("mutexclose: locked");
  release(&m->lk);

  MTRACE("mutex: pid=%d m=%p mutexclose\n", myproc()->pid, m);
  MTRACE("mutex: pid=%d m=%p kfree\n", myproc()->pid, m);
  kfree((void *)m);
}

int
mutexlock(struct sleeplock *m)
{
  struct proc *p = myproc();

  acquire(&m->lk);
  // Recursive acquisition would otherwise put the owner to sleep forever.
  if (m->locked && m->pid == p->pid) {
    release(&m->lk);
    return -1;
  }

  for (;;) {
    if (killed(p)) {
      release(&m->lk);
      return -1;
    }
    if (!m->locked)
      break;

    // Register before releasing the state lock to avoid lost wakeups.
    sleep_prepare(m);

    release(&m->lk);

    MTRACE("mutex: pid=%d m=%p wait\n", p->pid, m);
    sleep();
    acquire(&m->lk);
  }

  m->locked = 1;
  m->pid = p->pid;
  release(&m->lk);

  MTRACE("mutex: pid=%d m=%p mutex_lock\n", p->pid, m);
  return 0;
}

int
mutexunlock(struct sleeplock *m)
{
  struct proc *p = myproc();
  acquire(&m->lk);
  if (!m->locked || m->pid != p->pid) {
    release(&m->lk);
    return -1;
  }

  m->locked = 0;
  m->pid = 0;
  wakeup(m);
  release(&m->lk);

  MTRACE("mutex: pid=%d m=%p mutex_unlock\n", p->pid, m);
  return 0;
}
