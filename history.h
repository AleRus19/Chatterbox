/**
 * @file  history.h
 * @author Russo Alessio 545373
 *      Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *      originale dell'autore.
 * @brief Struttura dati che permette di gestire la history di ogni utente registrato.
 *        Contine informazioni : nome utente, messaggi con i rispettivi mittenti
 *        l'inizio dei messaggi ancora da leggere.
 */

/**
 * @struct _nodo1 
 * @brief struttura history
 * @var *next puntatore al prossimo elemento
 * @var user nome utente
 * @var buf array circolare per messaggi
 * @var sender array circolare corrispondente ai messaggi per mantenere traccia dei mittenti
 * @var type array circolare corrispondete ai messaggi per mantenere il tipo dei messaggi
 * @var letto array circolare corrispondente ai messaggi per aggiornare le statistiche
 * @var tot numero totale di messaggi conservati
 * @var start inizio dei messaggi non ancora letti
 * @var end inizio messaggi non ancora letti
*/

typedef struct _nodo1{
  struct _nodo1 *next;
  char *user;
  char **buf;
  char **sender;
  char **type;
  int *letto;
  int tot;
  int start;
  int end;
}hist;


/**
 * @function InitHistory
 * @brief Inizializza la tabella hash per la history
 *
 * @return codice di errore in caso di fallimento
 *        
 */
void InitHistory();


/**
 * @function InsertHistory
 * @brief Registra e inizializza un nuovo utente nella history 
 * 
 * @param nick nome dell'utente da inserire
 *
 * @return 1 se l'utente è stato regstrato, altrimenti operazione fallita
 *        
 */
int InsertHistory(char *nick);



/**
 * @function InsertMsgHistory
 * @brief Inserisce un nuovo messaggio nella history dell'utente 
 * 
 * @param nick nome dell'utente destinatario
 * @param msg messaggio da inserire
 * @param sender nome dell'utente mittente
 * @param type messaggio testuale o file
 *
 * @return 1 se il messaggio è stato inserito, altrimenti messaggio non inserito
 *        
 */
int InsertMsgHistory(char *nick,char *msg,char *sender,char *type);

/**
 * @function hash_History
 * @brief calcolo chiave hash 
 * 
 * @param key chiave su cui applicare la funzione di hash
 *
 * @return posizione nella tabella hash
 *        
 */
unsigned int hash_History(void* key);

/**
 * @function FreeHistory
 * @brief Dealloca l'intera History al momento della terminazione del server 
 * 
 */
void FreeHistory();


/**
 * @function DeleteUserHistory
 * @brief Elimina un utente e tutte le sue informazioni 
 * 
 * @param nick nome dell'utente da rimuovere
 *
 * @return 1 in caso di successo
 *        
 */
int DeleteUserHistory(char *nick);



/**
 * @function InsertMsgHistoryBroadcast
 * @brief Inserisce un nuovo messaggio nella history di ogni utente registrato
 * 
 * @param msg messaggio da inserire
 * @param sender nome dell'utente mittente
 * @param type messaggio testuale o file
 *
 * @return numero di messaggi testuali inseriti
 *        
 */
int InsertMsgHistoryBroadcast(char *msg,char *sender,char *type);


/**
 * @function GetHistory

 * @brief Estrae e invia la history all'utente che ne ha fatto richiesta
 * 
 * @param fd descrittore di connessione 
 * @param req richiesta inviata dall'utente
 * @param mex contatore messaggi di testo inviati
 * @param mex contatore file inviati
 *
 * @return 1 in caso di successo, messaggio di errore in caso di fallimento
 *        
 */
int GetHistory(long fd,message_t *req,int *mex,int *file);
