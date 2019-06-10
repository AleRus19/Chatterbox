
#include "config.h"

/**
 * @file  hash.h
 * @author Russo Alessio 545373
 *      Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *      originale dell'autore  
 * @brief Contiene le funzioni che implementano la tabella hash
 *        per la gestione degli utenti registrati
 *      
 */

/**
  *@struct _nodo
  *@brief Struttura utente registrato
  *@var next puntatore al prossimo elemento
  *@var value nome dell'utente registrato
  *
  */
typedef struct _nodo{
  struct _nodo *next;
  char value[MAX_NAME_LENGTH+1];
}item;


/**
 * @function InitHash
 * @brief Inizializza la tabella Hash 
 *
 * @return codice di errore in caso di fallimento
 *        
 */
void InitHash();
 

/**
 * @function FindUserHash
 * @brief ricerca l'elemento nella tabella hash 
 * 
 * @param nick nome dell'utente da ricercare
 *
 * @return 0 se nick non è presente, 
 *         1 altrimenti
 *        
 */
int FindUserHash(char *nick);

/**
 * @function InsertUserHash
 * @brief Inserisce l'elemento nella tabella Hash 
 * 
 * @param x nome dell'utente da inserire
 *
 * @return 0 se nick non è presente ed è stato registato, 
 *         1 se l'utente già esiste
 *        
 */
int InsertUserHash(char * x);
/**
 * @function Elimina
 * @brief Elimina l'elemento dalla tabella Hash 
 * 
 * @param nick nome dell'utente da rimuovere
 *
 *        
 */
void DeleteUserHash(char *nick);

/**
 * @function Destroy
 * @brief Dealloca l'intera tabella Hash al momento della terminazione del server 
 * 
 */
void Destroy();


/**
 * @function hash_Register
 * @brief calcolo chiave hash 
 * 
 * @param key chiave su cui applicare la funzione di hash
 *
 * @return posizione nella tabella hash
 *        
 */
unsigned int hash_Register(void* key);
