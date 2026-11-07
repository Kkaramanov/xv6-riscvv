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
printArgs(int argc, char *argv[])
{
  int pid = getpid();
  for (int i = 0; i < argc; i++) {
    putNumber(pid);
    putString(": arg ");
    putNumber(i);
    putString(", char '");
    for (int j = 0; argv[i][j] != '\0'; j++) {
      putChar(argv[i][j]);
    }
    putString("'\n");
  }
}

int
main(int argc, char *argv[])
{
  printf("\n--- Несинхронизированный вывод ---\n");
  printf("Родитель pid=%d запущен\n\n", getpid());

  int pid = fork();
  if (pid < 0) {
    printf("Ошибка fork\n");
    exit(1);
  }

  if (pid == 0) {
    printArgs(argc, argv);
    exit(0);
  }

  printArgs(argc, argv);

  wait(0);
  printf("\n--- Конец ---\n");
  exit(0);
}