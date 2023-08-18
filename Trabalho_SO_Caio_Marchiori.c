
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <semaphore.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/shm.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/msg.h>
#include <sys/wait.h>
#include <string.h>

#define MIN (0.2f) //sleep minimo em segundos
#define MAX (2.0f) //sleep maximo em segundos

int *var_shared,*var_shared2; // variaveis da shared memory

pthread_mutex_t mutex_thread; // mutex de exclusao mutua das threads A e B

sem_t *mutex; // mutex de exclusao mutua dos processos 1 , 2 e 3

//funcao que gera valores aleatorios entre 0.2 e 2 segundos
float rand_time(double min, double max){
    return min + (rand() / (RAND_MAX / (max - min) ) );
}

//estrutura da mensagem
struct message{
    long mtype; //tipo da memsagem
    char mtext[100]; //quantidade de caracter maximo por mensagem
}msg;

void threads_function(char *nome){
    int var_local;
    int i = 0;
    while(i < 30){
        i++;
    
        //shmemn begin

        sem_wait(mutex);
        pthread_mutex_lock(&mutex_thread); 
        
        
        var_local = *(var_shared);
        var_local = var_local - 1; //decrementa o valor
        double t = (rand_time( MIN, MAX ));
        sleep(t);
    
        *(var_shared) = var_local;
        *(var_shared2) = *(var_shared2) + 1; //incrementa o valor

        //shmen end

        sem_post(mutex);

        printf("%s: TID - %ld - i: %d - var_shared: %d - var_shared2: %d\n", nome , pthread_self() , i , *(var_shared) , *(var_shared2));
    
        pthread_mutex_unlock(&mutex_thread);

        t = (rand_time(MIN,MAX));
        sleep(t);
    }

    printf("%s de TID: %ld finalizada\n", nome, pthread_self()); //avisa o seu término

    pthread_exit(NULL);
}


int main(){
    
    int msqid = msgget(IPC_PRIVATE, IPC_CREAT | 0600); //identificador da fila de mensagem
    msg.mtype = 1;

    if(msqid == -1){
        perror("Erro msgget()");
        exit(1);
    }

    int lg;            //tamanho da mensagem recebida
    long type = 1;     //tipo de mensagem buscado
    int size_msg = 22; //tamanho maximo do texto a ser recuperado
    
    mutex = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    
    //atribuimos valor inicial 1 para o mutex
    if(sem_init(mutex , 1 , 1) < 0){
        perror("error");
        return 0;
    }

    //variaveis da shared memory
    var_shared = mmap(NULL, (sizeof(int)), PROT_READ | PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS, -1, 0);
    var_shared2 = mmap(NULL, (sizeof(int)), PROT_READ | PROT_WRITE,MAP_SHARED|MAP_ANONYMOUS, -1, 0);

    *(var_shared) = 120;
    *(var_shared2) = 0;

    pid_t pid = fork();
    
    if(pid != 0){ //pai de todos

        pid_t pid2 = fork();

        if(pid2 != 0){

            pid_t pid3 = fork();

            if(pid3 != 0){

                for(int i=0;i<3;i++){
                    lg = msgrcv(msqid,&msg,size_msg,type,0);
                    printf("PAI: mensagem recebida: %s\n",msg.mtext);
                }
                
                 //destroi a fila de mensagem ao final do processo
                if (msgctl(msqid, IPC_RMID, NULL) == -1) {
                    perror("msgctl");
                    return EXIT_FAILURE;
                }

                printf("\nPai informa que o programa sera finalizado...\n");
                sleep(3);
                printf("var_shared: %d e var_shared2: %d",*(var_shared),*(var_shared2));
                exit(0);
            }
            
            else{ // manipular shmen

                int var_local;
                int i = 0;
                while(i < 30){
                    i++;
                    sem_wait(mutex);

                    //begin shmen
                    
                    var_local = *(var_shared);
                    var_local = var_local - 1; //decrementa o valor
                    double t = (rand_time(MIN,MAX));
                    sleep(t);
    
                    *(var_shared) = var_local;
                    *(var_shared2) = *(var_shared2) + 1; //incrementa o valor

                    //end shmen

                    sem_post(mutex);

                    printf("Filho 3 de PID: %d - i: %d - var_shared: %d - var_shared2: %d\n", getpid() , i , *(var_shared), *(var_shared2));
    
                    t = (rand_time(MIN,MAX));
                    sleep(t);
                }

                //escreve o texto da mensagem
                sprintf(msg.mtext,"Olá pai,filho3 acabou");

                //envia a mensagem a fila
                if(msgsnd(msqid,&msg,strlen(msg.mtext),IPC_NOWAIT) == -1){
                    perror("Envio de mensagem impossivel");
                    exit(-1);
                }

                exit(0); //finalizar o processo
            }
        }
        
        else{ // manipular shmen

            int var_local;
            int i = 0;
            while(i < 30){
                i++;
                sem_wait(mutex);

                //begin shmen
                
                var_local = *(var_shared);
                var_local = var_local - 1; //decrementa o valor
                double t = (rand_time(MIN,MAX));
                sleep(t);
    
                *(var_shared) = var_local;
                *(var_shared2) = *(var_shared2) + 1; //incrementa o valor
                
                //end shmen

                sem_post(mutex);

                printf("Filho 2 de PID: %d - i: %d - var_shared: %d - var_shared2: %d\n", getpid(), i , *(var_shared), *(var_shared2));

                t = (rand_time(MIN,MAX));
                sleep(t);
            }

            //escreve o texto da mensagem 
            sprintf(msg.mtext,"Olá pai,filho2 acabou");

            //envia a mensagem a fila
            if(msgsnd(msqid,&msg,strlen(msg.mtext),IPC_NOWAIT) == -1){
                perror("Envio de mensagem impossivel") ;
                exit(-1) ;
            }

            exit(0);
        
        }

    }
    
    else{ // processo que cria as threads A e B

        printf("Filho 1 criando as Threads\n");
        
        pthread_t thread1, thread2;
    
        pthread_mutex_init(&mutex_thread,NULL);

        printf("Threads A e B criadas com sucesso\n");
        pthread_create(&thread1, NULL, (void*) threads_function,"Thread A");
        pthread_create(&thread2, NULL, (void*) threads_function,"Thread B");


        pthread_join(thread1,NULL);
        pthread_join(thread2,NULL);
    
        //escreve o texto da mensagem
        sprintf(msg.mtext,"Olá pai,filho1 acabou");

        //envia a mensagem a fila
        if(msgsnd(msqid,&msg,strlen(msg.mtext),IPC_NOWAIT) == -1){
            perror("Envio de mensagem impossivel");
            exit(-1) ;
        }

        exit(0);
    }

    return 0;
}
