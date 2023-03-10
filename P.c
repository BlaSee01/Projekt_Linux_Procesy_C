#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>

#define LOG(FORMAT, ...) \
fprintf(stdout,"\n[%d] " FORMAT "\n",getpid(), ##__VA_ARGS__);

#define SLEEP 1000
#define READ 0
#define WRITE 1
#define S1 20		

#define PATH_FILE_SRC "/dev/urandom"

typedef struct sembuf sembuf;
typedef struct

#define LOG(FORMAT, ...) \
fprintf(stdout,"\n[%d] " FORMAT "\n",getpid(), ##__VA_ARGS__);

#define SLEEP 1000
#define READ 0
#define WRITE 1
#define S1 20		

#define PATH_FILE_SRC "/dev/urandom"

typedef struct sembuf sembuf;
typedef struct

{
	long type;
	char text[31];
}msgbuf;

key_t MSGKEY, SEMKEY, SHMKEY;
int MSGID, SEMID, SHMID;
struct sembuf SEMBUF;
msgbuf MSGBUF;
char* SHMBUF;


int PID;
int CH[3];

char buf[16];
char hex[31];

int mode = 1;
char* path_file_src = PATH_FILE_SRC;
char* path_file_k2 = "K2";

FILE* src, *f;
int node[2], fdes;

void podnies(int semid, int semnum){
	SEMBUF.sem_num = semnum;
	SEMBUF.sem_op = 1;
	SEMBUF.sem_flg = 0;
	if (semop(SEMID, &SEMBUF, 1) == -1)
	{
		perror("semop"); 
	}
}

void opusc(int semid, int semnum){
	SEMBUF.sem_num = semnum;
	SEMBUF.sem_op = -1;
	SEMBUF.sem_flg = 0;
	if (semop(SEMID, &SEMBUF, 1) == -1){
		perror("semop"); 
	}
}


void print(char* str, size_t size){
	size_t i = 0;
	while (i < size)
	{
		printf("%.2s ", str + i);
		i += 2;
	}
	printf("\n");
}

void clear_hex(char* str, size_t size){
	size_t i;
	for (i = 0; i < size; i++) str[i] = 0;
}

void move(char* des, const char* src, const size_t size){
	if (!des || !src) return;
	size_t i = 0;
	while (i < size) { des[i] = src[i]; i++; };
}


void get(char* buf, size_t size){
	size_t i = 0;
	printf("Podaj komunikat: ");
	while (i < size){
		fscanf(stdin, "%c", &buf[i]);
		if (buf[i] == ' ') continue;
		if (buf[i] == '\n') { buf[i] = '\0';  break; }
		else i++;
	}
}


void handler_exit(int sig){
	LOG("SIGNAL: %d", sig); 
	fflush(stdout);
	
	size_t i;
	
	for (i = 0; i < 3; i++) kill(CH[i], SIGKILL);	//	unicestwienie dzieci
	semctl(SEMID, 0, IPC_RMID);	
	semctl(SEMID, 1, IPC_RMID);	
	semctl(SEMID, 2, IPC_RMID);	

	unlink(path_file_k2);

	fprintf(stdout, "WATEK "); 
	fflush(stdout);	
	kill(getpid(), SIGKILL);	//	unicestwienie
}


int main(int argc, char* argv[]){
	
	printf("[PROJEKT]\n\n");
	
	
	sigset_t set1;
	size_t i;

	if (argc > 1) sscanf(argv[1], "%d", &mode);

	if (mode < 1 || mode > 3) mode = 1;

	if (mode == 2){
		if (argc == 3) path_file_src = argv[2];	//	czy podalem 2 argumenty
	}

	if ((SEMKEY = ftok(".", '!')) == -1)
	{
		perror("SEMKEY = ftok"); exit(1);
	}
	
	if ((SEMID = semget(SEMKEY, 3, IPC_CREAT | 0600)) == -1)
	{
		perror("SEMID = semget"); exit(1);
	}
	
	if (semctl(SEMID, 0, SETVAL, (int)1) == -1)
	{
		perror("semctl"); exit(1);
	}
		
	if (semctl(SEMID, 1, SETVAL, (int)0) == -1)
	{
		perror("semctl"); exit(1);
	}
	
	if (semctl(SEMID, 2, SETVAL, (int)0) == -1)
	{
		perror("semctl"); exit(1);
	}

	pipe(node);	// utworzenie mechanizmu pipe


	if ((CH[0] = fork()) == 0){	// PROCES 1
	
		sigfillset(&set1);
		sigprocmask(SIG_SETMASK, &set1, NULL);

		close(node[READ]);

		if (mode == 2 || mode == 3){
			if ((src = fopen(path_file_src, "r")) == NULL){
				LOG("ERROR: %s", path_file_src);
				kill(getppid(), S1);
			}
		}

		while (1){
			usleep(SLEEP);
			clear_hex(buf, sizeof(buf));

			opusc(SEMID, 0);

			if (mode == 2 || mode == 3){
				if (fread(buf, 1, 15, src) == 0){
					LOG("Koniec pliku. Wszystko odczytane.");
					kill(getppid(), S1);
					while (1);
				}
			}
			
			if (mode == 1){
				get(buf, 15);
			}

			write(node[WRITE], buf, sizeof(buf));

			podnies(SEMID, 1);
		}
	}
		
	if ((CH[1] = fork()) == 0){	// PROCES 2
		sigfillset(&set1);
		sigprocmask(SIG_SETMASK, &set1, NULL);

		close(node[WRITE]);

		while (1){
			usleep(SLEEP);
			clear_hex(buf, sizeof(buf));
			clear_hex(hex, sizeof(hex));

			opusc(SEMID, 1);

			read(node[READ], buf, sizeof(buf));

			for (i = 0; i < sizeof(buf) - 1; i++){
				if ((mode == 1) && (buf[i] == '\0')){		
					move(hex + 2 * i, (const char[2]) { '\0', '\0' }, 2);
					continue;
				}	//	maskowanie "zer" w wyĹ›wietlaniu
				sprintf(hex + 2 * i, "%02hhX", buf[i]);
			}


			if ((fdes = open(path_file_k2, O_WRONLY | O_CREAT, 0666)) == -1){
				perror("open");
				LOG("ERROR: %s", path_file_k2);
				kill(getppid(), S1);
			}
			write(fdes, hex, sizeof(hex));
			close(fdes);

			podnies(SEMID, 2);
		}
	}
		
	if ((CH[2] = fork()) == 0){	// PROCES 3
		sigfillset(&set1);
		sigprocmask(SIG_SETMASK, &set1, NULL);
		close(node[READ]);
		close(node[WRITE]);

		while (1){
			usleep(SLEEP);
			clear_hex(hex, sizeof(hex));	// cleat tabeli hex przed odczytaniem

			opusc(SEMID, 2);

			if ((fdes = open(path_file_k2, O_RDONLY, 0666)) == -1)
			{
				LOG("ERROR: %s", path_file_k2);
				kill(getppid(), S1);
			}
			read(fdes, hex, sizeof(hex));
			close(fdes);
			print(hex, 30);

			podnies(SEMID, 0);
		}
	}else{
		sigfillset(&set1);	//	maskowanie sygnalow
		sigdelset(&set1, S1);	//	wyjatek dla sygnalu 20
		sigprocmask(SIG_SETMASK, &set1, NULL);
		signal(S1, handler_exit);
	
		close(node[READ]);
		close(node[WRITE]);

		PID = getpid();
		
		fprintf(stdout, "[MACIERZYSTY]: %d\n", PID);
		for (i = 0; i < 3; i++) fprintf(stdout, "[WATEK %d]: %d\n", i + 1, CH[i]);
		fprintf(stdout, "\n");

		while (1) pause();
	}

	return 0;
}

