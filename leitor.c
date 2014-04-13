#include <stdio.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <sys/shm.h>
#include <sys/sem.h>


struct sembuf	g_lock_sembuf[1];
struct sembuf	g_unlock_sembuf[1];

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

void show_to_user(int pid)
{ 
	MsgStruct msg;	
	int status;
	int msg_id;
	char fim[] = "FIM\n";

	msg.type = 1;

	// ligar à fila de mensagens. A key deve ser hexadecimal.
	msg_id = msgget ( 0x002, 0600 | IPC_CREAT );
	
	while(1) {	
		status = msgrcv( msg_id, &msg, sizeof(msg.text), 1, 0);

		printf ("%s", msg.text);

		if(strcmp(msg.text, fim) == 0)
		{
			break;
		}
	}
}		

void get_from_shm(int sem_id)
{ 
	int msg_id;
	int status;
	int count;
	int last_position_readed = 0;
	char fim[] = "FIM\n";
	MsgStruct msg;
	ArrayMsg *messages;	

	// Creates the shared memory and the queue of messages.
	shm_id = shmget(0x4321, sizeof(ArrayMsg), IPC_CREAT | 0666);
	msg_id = msgget ( 0x002, 0600 | IPC_CREAT );

	// Pointing to the shared memory. 
	messages = (ArrayMsg *) shmat(shm_id, NULL, 0);

	while(1)
	{

		// Entra na região crítica, travando o mutex.
		semop(sem_id, g_lock_sembuf, 1);
	
		count = (*messages).position;

		while (last_position_readed <= count) 
		{
			msg = (*messages).msgs[last_position_readed];	
			status = msgsnd( msg_id, &msg, sizeof(msg.text), 0);
			last_position_readed++;
		}
		// Sai da região crítica, liberando o mutex
		semop(sem_id, g_unlock_sembuf, 1);
		
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

	// Cria o semáforo
	sem_id = semget(SEM_KEY, 1, IPC_CREAT | 0666);
	
	pid = fork();
	if(pid > 0)
	{
		show_to_user(pid);
		kill(pid, 9);
	}
	else if (pid == 0)
	{
		printf("aqui");
		get_from_shm(sem_id);
	} else {
		printf("Erro no Fork!\n");	
	}

	shmctl(shm_id, IPC_RMID, NULL);
	
	return 0;
}

