#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void
putChar(char c)
{
  write(1, &c, 1);
}

void
putString(char *string)
{
  while (*string) {
    putChar(*string);
    string++;
  }
}

void
putNumber(int n)
{
  if (n == 0)
    putChar('0');
  while (n) {
    char c = '0' + (n % 10);
    putChar(c);
    n /= 10;
  }
}

void
printArgs(int argc, char *argv[], int m)
{
  int pid = getpid();
  for (int i = 0; i < argc; i++) {
    if (mutex_lock(m) < 0) {
      printf("Ошибка захвата мьютекса\n");
      exit(1);
    }

    putNumber(pid);
    putString(": arg ");
    putNumber(i);
    putString(", char '");
    for (int j = 0; argv[i][j] != '\0'; j++) {
      putChar(argv[i][j]);
    }
    putString("'\n");

    mutex_unlock(m);
  }
}

int
main(int argc, char *argv[])
{
  printf("\n--- Синхронизированный вывод ---\n");
  printf("Родитель pid=%d запущен\n\n", getpid());

  int m = mutex();
  if (m < 0) {
    printf("Не удалось создать mutex\n");
    exit(1);
  }

  int pid = fork();
  if (pid < 0) {
    printf("Ошибка fork\n");
    exit(1);
  }

  if (pid == 0) {
    printArgs(argc, argv, m);
    exit(0);
  }

  printArgs(argc, argv, m);

  wait(0);
  close(m);

  printf("\n--- Конец ---\n");
  exit(0);
}