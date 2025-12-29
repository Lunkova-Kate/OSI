#include "mythread.h"
#include <stdio.h>
#include <unistd.h>

void *my_thread_function(void *arg) {
  char *message = (char *)arg;

  printf("Мой Поток [TID: %ld] запущен. Получено сообщение: %s \n",
         (long)mythread_self(), message);

  sleep(2);

  printf(" Поток [TID: %ld] завершает работу.\n", (long)mythread_self());

  return (void *)"Поток успешно завершен";
}

int main() {
  mythread_t thread_id;
  char *thread_message = "Hello from main process!";
  int result;
  void *thread_result;

  printf("Главный поток [PID: %d] запускает дочерний поток.\n", (int)getpid());

  result =
      mythread_create(&thread_id, my_thread_function, (void *)thread_message);

  if (result == SUCCESS) {
    printf("Поток успешно создан. TID нового потока: %d\n", thread_id->tid);

    printf("Ожидаем завершения потока...\n");
    result = mythread_join(thread_id, &thread_result);

    if (result == SUCCESS) {
      printf("Поток завершился с результатом: %s\n", (char *)thread_result);
    } else {
      fprintf(stderr, "Ошибка ожидания потока: %s\n", strerror(errno));
    }

  } else {
    fprintf(stderr, "Ошибка создания потока. Код: %d\n", result);
    return 1;
  }

  // printf("Главный поток завершает работу.\n");
  return 0;
}
