#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <unistd.h>        

struct sembuf g_lock_sembuf[1];
struct sembuf g_unlock_sembuf[1];

#define SEM_KEY 0x1111

int shm_id;

typedef struct
{
	long int type;
	char text[250];
} MsgStruct;

typedef struct
{
	MsgStruct msgs[9999];
	int position;  
} ArrayMsg;

void read_from_user(int pid)
{
	MsgStruct msg;	
	int status;
	int msg_id;
	char fim[] = "FIM\n";

	msg.type = 1;
	
	// ligar à fila de mensagens. A key deve ser hexadecimal.
	msg_id = msgget ( 0x001, 0600 | IPC_CREAT );

	while(1)
	{
		printf("Digite a mensagem: ");	
		fgets(msg.text, 250, stdin);
		printf ("Mensagem enviada!\n");
		status = msgsnd( msg_id, &msg, sizeof(msg.text), 0);
		
		if(strcmp(msg.text, fim)==0)
		{
			break;
		}	
	}
}

void send_to_reader(int sem_id)
{
	int msg_id;
	int status;
	int count = 1;
	MsgStruct msg;
	ArrayMsg *messages;	
	char fim[] = "FIM\n";

	// Cria a memória compartilhada e a fila de mensages	
	shm_id = shmget(0x4321, sizeof(ArrayMsg), IPC_CREAT | 0666);
	msg_id = msgget ( 0x001, 0600 | IPC_CREAT );

	// Aponta para a memória comppatilhada
	messages = (ArrayMsg *) shmat(shm_id, NULL, 0);

	while(1)
	{
		// receber uma mensagem (bloqueia se não houver)
		status = msgrcv( msg_id, &msg, sizeof(msg.text), 1, 0);
	
		// Entra na região crítica, travando o mutex.
		semop (sem_id, g_lock_sembuf, 1);	
		
		(*messages).msgs[count] = msg;
		(*messages).position = count;
		
		// Sai da região crítica, liberando o mutex
		semop (sem_id, g_unlock_sembuf, 1);

		count++;

		if(strcmp(msg.text, fim)==0)
		{
			break;
		}
		
	}
}

int main()
{
	int pid;
	int sem_id;

	// Inicializa estrutras de controle do semaforo
	g_lock_sembuf[0].sem_num = 0; 
	g_lock_sembuf[0].sem_op = -1;
	g_lock_sembuf[0].sem_flg = 0;

	g_unlock_sembuf[0].sem_num = 0; 
	g_unlock_sembuf[0].sem_op = 1; 
	g_unlock_sembuf[0].sem_flg = 0;

	// Cria e Inicializa o semáforo. O mutex comeca aberto. 
	sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
	semop (sem_id, g_unlock_sembuf, 1);
	
	pid = fork();
	if (pid > 0)
	{
		read_from_user(pid);
	}
	else if (pid == 0)
	{
		send_to_reader(sem_id);
	} 
	else 
	{
		printf("Erro no fork!");
	}

	shmctl(shm_id, IPC_RMID, NULL);
	
	return 0;
}

