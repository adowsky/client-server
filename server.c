#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>
#include "message.h"

#define RESET "\033[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
/**
zaktualizować register o prześwietlanie tablicy jesli usr_count == 100;
*/
struct user{
  char name[60];
  char password[30];
  char properties[4][60];
  int type;
};

typedef struct message message;
typedef struct user user;

message *msg;
int smd, tmp1, tmp2;
unsigned int* s_area;
sem_t *sem_request, *sem_respond, *sem_clients;
user database[100];
int usr_count;
char *role[] = {"", "pacjent", "lekarz", "rejestracja"};
/**
Przetwarza żądanie i umieszcza je w pamięci współdzielonej.
*/
void process_request();
/**
Obsługuje żądanie help. Podaje wspierane funkcje.
*/
void commands();
/**
Obsługuje ządanie autentykacji.
*/
int authenticate();
/**
Obsługuje żądanie whoami. Podaje imię i nazwisko oraz rolę.
*/
void whoami();
/**
Obsluguje żądanie rejestracji. Doaje nowego użytkownika do bazy jeśli są spełnione odpowiednie warunki.
*/
void registr();
/**
Obssługuje żądanie details. Podaje szczegóły pacjenta podanego jako wartość w pamięci współdzielonej.
*/
void details();
/**
Obsługuje żądanie usunięcia użytkownika. Usuwa użytkownika jeśli wszystko jest ok.
*/
void delete();
/**
Obsługuje żądanie update. Aktualizuje dane pacjenta.
*/
void update();
/**
Obsługuje żądanie list. Podaje wszystkich użytkowniów w systemie.
*/
void list();
/**
Obsułguje żądania niewspieranie. Wypisuje komunikat o będzie.
*/
void unsupported();

void sigint_handler(int i){
  //finalization
  munmap(s_area, sizeof(message));
  close(smd);
  shm_unlink("/shm");
  sem_close( sem_clients );
  sem_unlink( "/sem_clients" );
  sem_close( sem_respond );
  sem_unlink( "/sem_respond" );
  sem_close( sem_request );
  sem_unlink( "/sem_request" );
  printf("\nSerwer wyłączony pomyślnie! \n");
  exit(0);
}

void create_basic_users(){
  printf("Tworzenie podstawowych użytkowników...\n");
  strcpy(database[usr_count].name, "lekarz\0");
  database[usr_count].type = 2;
  strcpy(database[usr_count++].password, "lek\0");
  strcpy(database[usr_count].name, "rejestracja\0");
  database[usr_count].type = 3;
  strcpy(database[usr_count++].password, "rej\0");

}
int main(){
  //init
  printf("Uruchamianie serwera... \n");
  signal(SIGINT, sigint_handler);
  printf("Inicjalizowanie struktur systemowych... ");;
  fflush(stdout);
  smd = shm_open("/shm", O_RDWR | O_CREAT, 0600);
  if(smd < 0){
    perror("SHM OPEN ERROR!");
    exit(-1);
  }
  int error = ftruncate(smd, sizeof(message));
  if(error < 0){
    perror("ftruncate error!");
    exit(-1);
  }
  s_area = (unsigned int*) mmap(0, sizeof(message), PROT_WRITE | PROT_READ, MAP_SHARED, smd, 0 );
  if(s_area == (void*)-1){
    perror("mmap error!");
    exit(-1);
  }
  msg = (void*)s_area;
  sem_request = sem_open("/sem_request", O_RDWR | O_CREAT, 0660, 0);
  if(sem_request == SEM_FAILED){
    perror("Request semafor error!");
    exit(-1);
  }
  sem_respond = sem_open("/sem_respond", O_RDWR | O_CREAT, 0660, 0);
  if(sem_respond == SEM_FAILED){
    perror("Respond semafor error!");
    exit(-1);
  }
  sem_clients = sem_open("/sem_clients", O_RDWR | O_CREAT, 0660, 1);
  if(sem_clients == SEM_FAILED){
    perror("Clients semafor error!");
    exit(-1);
  }
  usr_count = 0;
  create_basic_users();
  //end of initialization
  printf(KGRN "OK \n" RESET);
  printf("Serwer uruchomiony pomyślnie!\n");
  while(1){
    sem_wait(sem_request);
    printf("odebrano żądanie. \nPrzetwarzanie... \n");
    process_request();
    printf("wysyłanie...\n");
    sem_post(sem_respond);
    sleep(2);
  }

  return 0;
}

void process_request(){
  if(msg->command_type < 0 || msg->command_type > 14){
    msg->command_type = -1;
    strcpy(msg->message, "Nieprawidłowa funkcja!");
    return;
  }
  switch (msg->command_type) {
    case 0:
      commands();
      break;
    case 1:
      authenticate();
      break;
    case 3:
      whoami();
      break;
    case 4:
      registr();
      break;
    case 5:
      details();
      break;
    case 6:
      delete();
      break;
    case 8:
      update();
      break;
    case 9:
      list();
      break;
    default:
      unsupported();
      break;
  }
}

void commands(){
  strcpy(msg->message, "Dodatkowa funkcjonalność: update");
}
int authenticate(){
  char* password = strdup(msg->message);
  char* delim = ";";
  char* name = strsep(&password, delim); //password się utnie o name i zostanie pojedyńczy wpis.
  printf("Nazwa: %s\n Hasło: %s\n",name, password);
  int i;
  msg->user_id = -1;
  for(i=0; i<usr_count; i++){
    if(strcmp(name, database[i].name) == 0 && strcmp(password, database[i].password) == 0){
      printf("zautoryzowano\n");
      msg->user_id = i;
      if(database[i].type == 1)
        strcpy(msg->message, "pacjent");
      else if(database[i].type == 2)
        strcpy(msg->message, "lekarz");
      else if(database[i].type == 3){
        strcpy(msg->message, "rejestracja");
      }
      return 1;
    }
  }
    strcpy(msg->message, "autryzacja zakończona niepowodzeniem");
    return 0;
}
void whoami(){
  snprintf(msg->message, sizeof(msg->message), "%s %s",database[msg->user_id].name, role[database[msg->user_id].type]);
}
void registr(){
  if(database[msg->user_id].type != 3){
    strcpy(msg->message, "Brak uprawnień!");
    return;
  }
  int new = -1;
  if(usr_count >= 100){
    int i;
    for(i =0; i<100;i++){
      if(strcmp(database[i].name,"") == 0){
        new = i;
        break;
      }
    }
  }else{
    new = usr_count;
    usr_count++;
  }
  char* dup = strdup(msg->message);
  char* delim = ";";
  char* result = strsep(&dup, delim);
  strcpy(database[new].name, result);
  result = strsep(&dup, delim);
  strcpy(database[new].password, result);
  printf("Password: %s\n",result);
  result = strsep(&dup, delim);
  strcpy(database[new].properties[0], result);
  result = strsep(&dup, delim);
  printf("%s\n",result);
  strcpy(database[new].properties[1], result);
  time_t rawtime;
  struct tm * timeinfo;
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  strcpy(database[new].properties[2], asctime (timeinfo) );
  database[new].type = 1;
  strcpy(msg->message, "Rejestracja zakończona pomyślnie!");
}
void details(){
  if(database[msg->user_id].type == 2){ //lekarz
    int i;
    for(i=0;i<usr_count;i++){
      if(strcmp(msg->message,database[i].name) == 0){
        snprintf(msg->message, sizeof(msg->message),"%s %s %s %s", database[i].name, database[i].properties[0], database[i].properties[1], database[i].properties[2]);
        return;
      }
    }
  }else if(database[msg->user_id].type == 3){
    char* password = strdup(msg->message);
    char* delim = ";";
    char* name = strsep(&password, delim);
    int i;
    for(i=0;i<usr_count;i++){
      if(strcmp(name,database[i].name) == 0){
        if(strcmp(password,database[i].password) == 0){
        snprintf(msg->message,sizeof(msg->message),"%s %s %s %s", database[i].name, database[i].properties[0],database[i].properties[1], database[i].properties[2]);
        return;
      }else{
        strcpy(msg->message, "Niepopawne hasło!");
        msg->command_type = -1;
      }
    }
  }
  }else{
    strcpy(msg->message, "Niewystarczające uprawnienia!");
    msg->command_type = -1;
  }
}
void delete(){
  if(msg->user_id == 1){
    strcpy(msg->message, "Brak uprawnień!");
    return;
  }
  int i;
  int done = 0;
  for(i=0;i<usr_count;i++){
    if(strcmp(msg->message, database[i].name) == 0){
      memset(&database[i], 0, sizeof(user));       // zerowanie database[i]
      done = 1;
    }
  }
  if(done)
    strcpy(msg->message, "Usunieto pomyslnie");
  else
    strcpy(msg->message, "Nie znaleziono uzytkownika");
}
void update(){
  if(msg->user_id == 3){
  strcpy(msg->message,"Niewystarczające uprawnienia!");
  return;
  }
  char* dup = strdup(msg->message);
  char* delim = ";";
  char* result = strsep(&dup,delim);
  int i;
  int found = 0;
  for(i=0; i<usr_count;i++){
    if(strcmp(result, database[i].name) == 0){ // znaleziono usera
      result = strsep(&dup, delim);
    printf("%s\n",result);
      if(strcmp(result,"") != 0)  // hasło
        strcpy(database[i].password, result);
      result = strsep(&dup, delim);
      printf("%s\n",result);
      if(strcmp(result,"") != 0) //wiek
        strcpy(database[i].properties[0], result);
      //result = strsep(&dup, delim);
      //printf("%s\n",result);
      if(strcmp(dup,"") != 0)  //wstępne rozpoznanie
        strcpy(database[i].properties[1], result);
      found = 1;
    }
  }
  if(found)
    strcpy(msg->message, "Zaktualizowano pomyślnie!");
  else
    strcpy(msg->message, "Nie zaktualizowano");
}
void list(){
  int i;
    int field;
  if(msg->is_complete == -1){ // czy jest to kontynuacja wysyłania
    i = tmp1;
    field = tmp2;
  }
  else{
    i = 0;
    field = 0;
  }
  strcpy(msg->message, "");
  int size = 0;

  for(; i<usr_count; i++){
    if(field == 0){
      int len = strlen(database[i].name);
      if(size+len+1 >300){
        msg->is_complete = -1;
        tmp1 = i;
        tmp2 = field;
      }else{
        strcat(msg->message, database[i].name);
        strcat(msg->message, ":");
        size+=len+1;
        field++;
      }
    }else{
      int len = strlen(role[database[i].type]);
      if(size+len+1 >300){
        msg->is_complete = -1;
        tmp1 = i;
        tmp2 = field;
      }else{
        strcat(msg->message, role[database[i].type]);
        strcat(msg->message, ":");
        field = 0;
        size+=len+1;
      }
    }
}

}
void unsupported(){
  strcpy(msg->message, "Polecenie niewspierane");
}
