#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>
#include "message.h"

typedef struct message message;

message msg;
int smd;
char user_type[11];

int make_request();
void process_respond();



void sigint_handler(int i){
  //finalization
  munmap(s_area, sizeof(message));
  close(smd);
  printf("\nClient closed successfully! \n");
  exit(0);
}


int main(){
  signal(SIGINT, sigint_handler);
  smd = shm_open("/shm", O_RDWR, 0600);
  if(smd < 0){
    perror("SHM OPEN ERROR!");
    exit(-1);
  }
  unsigned int* s_area = (unsigned int*) mmap(0, sizeof(message), PROT_WRITE | PROT_READ, MAP_SHARED, smd, 0 );
  if(s_area == (void*)-1){
    perror("mmap error!");
    exit(-1);
  }
  sem_t* sem_request = sem_open("/sem_request", O_RDWR);
  if(sem_request == SEM_FAILED){
    perror("Request semafor error!");
    exit(-1);
  }
  sem_t* sem_respond = sem_open("/sem_respond", O_RDWR);
  if(sem_respond == SEM_FAILED){
    perror("Respond semafor error!");
    exit(-1);
  }
  sem_t* sem_clients = sem_open("/sem_clients", O_RDWR);
  if(sem_clients == SEM_FAILED){
    perror("Clients semafor error!");
    exit(-1);
  }
  //end of initialization
  msg = ((message*)s_area)[0];
  printf("Welcome in Hospital client!\n To get command list type 0 \n");
  int auth = 0;
  while(1){
    sem_wait(sem_clients);
    if(make_request(auth) == 0 ){
      sem_post(sem_request);
      while(msg.is_complete < 0){
        sem_wait(sem_respond);
        process_respond(&auth);
      }
  }
    sem_post(sem_clients);
  }

printf("%s\n",msg.name);

  return 0;
}
/**
* Tworzy żądanie, prosząc o odpowienie wpisy użytkownika.
* Zwraca -1 jeśli nastąpił błąd lub 0 - jeśli proces przebiegł pomyślnie.
*/
int make_request(int *auth){
  if(!auth){  // autoryzuje do skutku
    char name[60], password[30];
    printf("Name: ");
    scanf("%s", &name);
    printf("Password: ");
    scanf("%s", &password);
    strcpy(msg.message, name);
    strcat(msg.message, password);
    msg.command_type = 1;
    return -1;
  }
  printf("Command number: ");
  int cmd = -1;
  scanf("%d", &cmd);
  request_details(&cmd);  // tworzymy żądanie
  if(cmd < 0 || cmd > 14){
    printf("Wrong command!");
    return -1;
  }
    msg.command_type = cmd;
  return 0;
}
/**
* Uściśla żądanie jeśli takie uśliślenie jest wymagane. Zwraca -1 jeśli żądanie jest nieobsługiwane
* lub 0 - jeśli proces przebiegł pomyślnie.
*/
void request_details(int* cmd){
  switch (*cmd) {
    case 0 || 3 || 9: // help, whoami, list
      msg.message = "";
      break;
    case 4: // rejestracja
      printf("Typ użytkownika: ");
      char tab[60];
      scanf("%s",tab);
      strcpy(msg.message,tab);
      strcat(msg.message,";");
      printf("Imię i nazwisko: ");
      scanf("%s",tab);
      strcat(msg.message,tab);
      strcat(msg.message,";");
      printf("Hasło: ");
      scanf("%s",tab);
      strcat(msg.message, tab);
      break;
    case 5: // details
      if(strcmp(user_type, "lekarz") == 0){ // wysyła lekarz
        printf("Imię i nazwisko: ");
        scanf("%s", msg.message);
      }else if(strcmp(user_type, "rejestracja") == 0){  // wysyła rejestracja
        printf("Imię i nazwisko: ");
        char tab[60];
        scanf("%s", tab);
        strcmp(msg.message, tab);
        strcat(msg.message, ";");
        printf("Hasło: ");
        scanf("%s", tab);
        strcat(msg.message, tab);
      }else{  // wysyła pacjent
        printf("Nie masz uprawnień!\n");
        *cmd = -1;
      }
      break;
    case 6: //delete
      printf("Imię i nazwisko: ");
      scanf("%s", msg.message);
      break;
    case 7 || 10 || 11 ||12 ||13 || 14: //niewspierane
      break;
    case 8: //update
      if(strcmp(user_type, "lekarz") == 0){ // wysyła lekarz
        printf("Imię i nazwisko: ");
        char tab[60];
        scanf("%s", tab);
        strcpy(msg.message, tab);
        strcat(msg.message, ";");
        printf("Choroba: ");
        scanf("%s", tab);
        strcat(msg.message, tab);
      }else{  // wysyła rejestracja lub pacjent
        printf("Nie masz wystarczajacych uprawnień! \n");
        *cmd = -1;
      }
      break;
    default:{ // jakaś inna wartość funkcji - błąd
      *cmd = -1;
      break;
    }

  }
}
/**
* Przetwarza otrzymaną odpowiedź serwera.
*/
void process_respond(int* auth){
  if(!*auth){ // Autoryzuje do skutku
    if(msg.user_id != -1){
      auth = 1;
      strcpy(user_type, msg.message);
    }
    return;
  }
  switch (msg.command_type) {
    case -1:  // Błąd
      printf("Problem z przetworzeniem żądania! \n");
      break;
    case 2 || 3 || 5 || 7 || 9 || 10 || 11 || 12 ||13 ||14: //niewspierane i przesyłane jako tekst.
      printf("%s",msg.message);
      if(msg.is_complete == 0)
        printf("\n");
      break;
    case 4 || 6 || 8: // nic wykonało się po stronie serwera, ale nie ma wiadomości.
      break;

    default:  //jakieś inne bugi
      printf("Wystapil problem z żądaniem lub żądanie nieobsugiwane! \n");
      break;
  }
}
