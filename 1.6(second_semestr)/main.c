#include "mythread.h" 
#include <unistd.h>   



void* my_thread_function(void* arg) {
    char* message = (char*)arg;

    printf("--> Мой Поток [TID: %d] запущен. Получено сообщение: \"%s\"\n", 
           (int)mythread_self(), message);

    sleep(2); 

    printf("--> Поток [TID: %d] завершает работу.\n", (int)mythread_self());

    return NULL; 
}

int main() {
    mythread_t thread_id;
    char* thread_message = "Hello from main process!";
    int result;

    printf("Главный поток [PID: %d] запускает дочерний поток.\n", (int)getpid());

    result = mythread_create(
        &thread_id,            
        my_thread_function,     
        (void*)thread_message   
    );

    if (result == SUCCESS) {
        printf("Поток успешно создан. TID нового потока: %d\n", (int)thread_id);

    } else {
        fprintf(stderr, "Ошибка создания потока. Код: %d\n", result);
        return 0;
    }

    //  утечка памяти

    return 0;
}