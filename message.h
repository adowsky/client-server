/**
Struktura używana do komunikacji między klientem, a serwerem. Reprezentuje zarówno żądanie jak i odpowiedź.
*/
struct message{
  int command_type;
  int user_id;
  char message[300];
  int is_complete;
};

/*! \mainpage Strona główna projektu SZPITAL
 *
 * \section Wprowadzenie
 *
 * Ta strona jest dokumentacją projektu szpital.
 * Znajdziesz tu opis funkcji i sturktur użytych w kodzie.
 *
 * \section Użytkowanie
 *
 * \subsection Uruchamianie
 * Przed uruchomieniem aplikacji klienta musi zostać uruchomiony serwer.
 *
 * \subsection Korzystanie
 *
 * Klient po uruchomieniu poprosi cię o dane do logowania.
 * Po pomyślnej autentykacji dostęp do poszeczególnych funkcji będzie dostępny prez podanie nazwy funkcji.
 */
