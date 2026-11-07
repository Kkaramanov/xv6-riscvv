// проверка мьютексов (проверка отладочного вывода)
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

static void
check(const char *name, int got, int expected)
{
  if (got == expected)
    printf("OK   %s -> %d\n", name, got);
  else
    printf("FAIL %s -> %d (expected %d)\n", name, got, expected);
}

int
main(void)
{
  int m;
  char c = 'x';
  struct stat st;

  printf("mutexcheck start (pid=%d) ===\n", getpid());

  // Создание мьютекса
  m = mutex();
  printf("mutex() -> fd=%d\n", m);
  if (m < 0) {
    printf("FAIL: mutex() returned %d\n", m);
    exit(1);
  }

  // read/write/fstat 
  check("read(m)",  read(m, &c, 1),  -1);
  check("write(m)", write(m, &c, 1), -1);
  check("fstat(m)", fstat(m, &st),   -1);

  // lock/unlock
  check("mutex_lock(m)",   mutex_lock(m),   0);
  check("mutex_unlock(m)", mutex_unlock(m), 0);

  // Повторный lock/unlock 
  check("mutex_lock(m) #2",   mutex_lock(m),   0);
  check("mutex_unlock(m) #2", mutex_unlock(m), 0);

  // fork: ребёнок наследует дескриптор, ref становится 2
  int pid = fork();
  if (pid < 0) {
    printf("FAIL: fork() -> %d\n", pid);
    close(m);
    exit(1);
  }

  if (pid == 0) {

    printf("[child pid=%d] lock\n", getpid());
    check("child mutex_lock(m)", mutex_lock(m), 0);
    printf("[child pid=%d] unlock\n", getpid());
    check("child mutex_unlock(m)", mutex_unlock(m), 0);
    printf("[child pid=%d] exit (fileclose -> ref--)\n", getpid());
    exit(0);
  }

  printf("[parent pid=%d] wait child\n", getpid());
  wait(0);
  printf("[parent pid=%d] child done\n", getpid());

  // close: последняя ссылка -> mutexclose -> kfree
  printf("[parent pid=%d] close(m)\n", getpid());
  check("close(m)", close(m), 0);

  // Повторный close 
  check("close(m) #2", close(m), -1);

  printf("mutexcheck done\n");
  exit(0);
}