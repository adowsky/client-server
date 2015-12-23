/**
Struktura używana do komunikacji między klientem, a serwerem. Reprezentuje zarówno żądanie jak i odpowiedź.
*/
struct message{
  int command_type;
  int user_id;
  char message[300];
  int is_complete;
};
