#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>
#include "message.h"

/**
@file
@brief Plik klienta.
*/

/** \cond */
typedef struct message message;

message* msg;
message tmp;
int smd, csem;
long my_usr_id;
char user_type[11];
unsigned int* s_area;
sem_t* sem_clients;
/** \endcond */

void request_details(int* cmd, int* auth);
void copy_request();
/*! Czyta wejście wraz z białymi znakami do wystąpienia znaku nowej linii lub przepełnienia bufora. */
int read_input(int count, char* dir){
  if(count == 1){
    *dir = getchar();
    char ch;
    while((ch = getchar())!='\n'  && ch != EOF  );
    return 1;
  }
  char ch;
  fgets(dir, count, stdin);
  if (strlen(dir) == count )
    while((ch = getchar())!='\n'  && ch != EOF  );
  else
    dir[strlen(dir)-1] = 0;
  }
  /**
  * Czyści bufory używane przy komunikacji z serwerem.
  */
  void clear_buffer(){
    for(int i=0;i<300;i++){
      msg->message[i] = 0;
      tmp.message[i] = 0;
    }
  }
  /**
  * Czyści bufor wejścia.
  */
void flushin(){
  char ch;
  while((ch = getchar())!='\n'  && ch != EOF  );
}
/**
Zamyka otwarte zasoby i wyłącza klienta.
*/
void sigint_handler(int i){
  //finalization
  munmap(s_area, sizeof(message));
  close(smd);
  if(csem)
    sem_post(sem_clients);
  printf("\nKlient zamknięty pomyślnie! \n");
  exit(0);
}
/**
\cond
*/
int main(){
  printf("Ładowanie klienta...\n");
  signal(SIGINT, sigint_handler);
  smd = shm_open("/shm", O_RDWR, 0600);
  if(smd < 0){
    perror("SHM OPEN ERROR!");
    exit(-1);
  }
  s_area = (unsigned int*) mmap(0, sizeof(message),
      PROT_WRITE | PROT_READ, MAP_SHARED, smd, 0 );
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
  sem_clients = sem_open("/sem_clients", O_RDWR);
  if(sem_clients == SEM_FAILED){
    perror("Clients semafor error!");
    exit(-1);
  }
  //end of initialization
  msg = (void*) s_area;
  csem = 0;
  my_usr_id = -1;
  printf("Welcome in Hospital client!\n To get command list type 0 \n");
  int auth = 0;

  while(1){
    int make = make_request(&auth);
    sem_wait(sem_clients);
    csem = 1;
    if(make == 0 ){
      //clear_buffer();
      copy_request();
      sem_post(sem_request);
      sem_wait(sem_respond);
      process_respond(&auth);
      while(msg->is_complete < 0){
        sem_wait(sem_respond);
        process_respond(&auth);
      }
  }
    sem_post(sem_clients);
    csem = 0;
  }
  return 0;
}
/** \endcond */

/**
* Tworzy żądanie, prosząc o odpowienie wpisy użytkownika.
* Zwraca -1 jeśli nastąpił błąd lub 0 - jeśli proces przebiegł pomyślnie.
*/
int make_request(int *auth){
  if(!*auth){  // autoryzuje do skutku
    char name[60], password[30];
    printf("Imię i nazwisko: ");
    char c;
    read_input(60, name);
    printf("Hasło(jeśli puste wpisz \"\")[max. 30 zn]: ");
    read_input(30, password);
    if(strcmp(password,"\"\"") == 0)
      strcpy(password,"");
    strcpy(tmp.message, name);
    strcat(tmp.message, ";");
    strcat(tmp.message, password);
    tmp.command_type = 1;
    return 0;
  }
  printf("Command: ");
  int cmd = -1;
  char tab[40];
  read_input(40,tab);
  cmd = parse_comand(tab);
  request_details(&cmd,auth);  // tworzymy żądanie
  if(cmd < 0 || cmd > 14){
    printf("Wrong command!\n");
    return -1;
  }
    tmp.command_type = cmd;
  return 0;
}
/**
Przetwarza ciąg znaków podany przez użytkownika na numer funkcji.
*/
int parse_comand(char* tab){
  if(strcmp(tab, "help") == 0)
    return 0;
  else if(strcmp(tab, "authorize") == 0)
    return 1;
  else if(strcmp(tab, "whoami") == 0)
    return 3;
  else if(strcmp(tab, "register") == 0)
    return 4;
  else if(strcmp(tab, "details") == 0)
    return 5;
  else if(strcmp(tab, "update") == 0)
    return 8;
  else if(strcmp(tab, "list") == 0)
    return 9;
  return -1;
}
/**
* Uściśla żądanie jeśli takie uśliślenie jest wymagane. Zwraca -1 jeśli żądanie jest nieobsługiwane
* lub 0 - jeśli proces przebiegł pomyślnie.
*/
void request_details(int* cmd, int* auth){
  switch (*cmd) {
    case 1:
      my_usr_id = -1;
      tmp.user_id = -1;
      printf("Ta funkcja spowodowała twoje wylogowanie!\n");
      *auth = 0;
      char tab[61];
      printf("Imię i nazwisko(max. 60 zn): ");
      read_input(61,tab);
      strcpy(tmp.message, tab);
      printf("Hasło(max. 60 zn): ");
      read_input(61,tab);
      strcat(tmp.message, ";");
      strcat(tmp.message, tab);
      printf("%s\n", tmp.message);
      tmp.command_type = 1;
      break;
    case 0:
    case 3:
    case 9: // help, whoami, list
      strcpy(tmp.message, "");
      break;
    case 4: // rejestracja
      printf("Imię i nazwisko: ");
      char name[60];
      read_input(60,name);
      strcat(tmp.message,name);
      strcat(tmp.message,";");
      char conf;
      printf("Hasło? (t\\N) ");
      read_input(1,&conf);
      if(conf == 't'){
      printf("Podaj hasło: ");
      read_input(60,tab);
      strcat(tmp.message,tab);
      }
      strcat(tmp.message,";");
      printf("Wiek? (t\\N) ");
      read_input(1,&conf);
      if(conf == 't'){
      printf("Podaj wiek: ");
      read_input(60,tab);
      strcat(tmp.message,tab);
      }
      strcat(tmp.message,";");
      printf("Wstępne rozpoznanie? (t\\N) ");
      read_input(1,&conf);
      if(conf == 't'){
        char rozp[100];
        printf("Podaj rozpoznanie(max 100 zn.): ");
        read_input(100,rozp);
      strcat(tmp.message, rozp);
    }
      break;
    case 5: // details
      if(strcmp(user_type, "lekarz") == 0){ // wysyła lekarz
        printf("Imię i nazwisko: ");
        read_input(60,tmp.message);
      }else if(strcmp(user_type, "rejestracja") == 0){  // wysyła rejestracja
        printf("Imię i nazwisko: ");
        char tab[60];
        read_input(60,tab);
        strcat(tmp.message, tab);
        strcat(tmp.message, ";");
        printf("Hasło: ");
        read_input(60,tab);
        strcat(tmp.message, tab);
      }else{  // wysyła pacjent
        printf("Nie masz uprawnień!\n");
        *cmd = -1;
      }
      break;
    case 6: //delete
      printf("Imię i nazwisko: ");
      read_input(60,tmp.message);
      break;
    case 7:
    case 10 ... 14:
      break;
    case 8: //update
      if(strcmp(user_type, "lekarz") == 0){ // wysyła lekarz
        printf("Imię: ");
        char tab[100];
        read_input(25,tmp.message);
        strcat(tmp.message, ";");
        printf("nazwisko: ");
        read_input(35,tab);
        strcat(tmp.message, tab);
        strcat(tmp.message, ";");
        printf("rozpoznanie: ");
        read_input(100,tab);
        strcat(tmp.message, tab);
        strcat(tmp.message, ";");
        printf("Zarejestrowany: ");
        read_input(3,tab);
        strcat(tmp.message, tab);

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
  printf("%d\n",*auth);
  if(! *auth){ // Autoryzuje do skutku
    my_usr_id = msg->user_id;
    if(my_usr_id != -1){
      *auth = 1;
      strcpy(user_type, msg->message);
      printf("Zalogowano jako: %s\n",msg->message);
    }else{
      printf("SERVER: %s \n", msg->message);
    }
    clear_buffer();
    return;
  }
  switch (msg->command_type) {
    case -1:  // Błąd
      printf("Problem z przetworzeniem żądania!\nSERVER: %s\n", msg->message);
      break;
    case 0:
    case 2 ... 14:
      printf("SERVER: %s\n",msg->message);
      if(msg->is_complete == 0)
        printf("\n");
      break;
    default:  //jakieś inne bugi
      printf("Wystapil problem z żądaniem lub żądanie nieobsugiwane! \n");
      break;
  }
  clear_buffer();
}
/*! Kopiuje żądanie do pamięci współdzielonej. */
void copy_request(){
  msg->user_id = my_usr_id;
  msg->command_type = tmp.command_type;
  msg->is_complete = tmp.is_complete;
  strcpy(msg->message,tmp.message);
}
