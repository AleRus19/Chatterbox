#include "config.h"
#include "message.h"

/**
 * @file  Group.h
 * @author Russo Alessio 545373
 *      Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *      originale dell'autore.
 * @brief Struttura dati:hash
 *        Permette la gestione dei gruppi
 */


/**
 * @struct _nodog
 * @brief struttura gruppi
 * @var next puntatore al prossimo elemento
 * @var name nome del gruppo
 * @var nome del creatore del gruppo 
 * @var GroupList lista membri gruppo 
 * @var n_membri   
 */

typedef struct _nodog{
  struct _nodog *next;
  char name[MAX_NAME_LENGTH+1];
  char creator[MAX_NAME_LENGTH+1];
  char **GroupList;
  int n_membri;
}group;

/**
 * @function InitGroup
 * @brief Inizializza la tabella hash dei Gruppi
 *
 * @return codice di errore in caso di fallimento
 *        
 */
void InitGroup();

/**
 * @function FreeGroup
 * @brief Dealloca l'intera tabella hash Gruppi al momento della terminazione del server 
 * 
 */
void FreeGroup();

/**
 * @function hash_pj
 * @brief calcolo chiave hash 
 * 
 * @param key chiave su cui applicare la funzione di hash
 *
 * @return posizione nella tabella hash
 *        
 */
unsigned int hash_Gruppi(void* key);

/**
 * @function InsertGroup
 * @brief Registra e inizializza un nuovo gruppo nella tabella gruppi e il suo creatore
 * 
 * @param req messaggio richiesta che contiene le informazioni per registrare un gruppo
 *
 * @return 1 se il gruppo è stato registrato correttamene,-1 se esite già, altrimenti messaggio di errore 
 *        
 */
int InsertGroup(message_t *req);


/**
 * @function IsGroup
 * @brief Inserisce messaggi inviati al gruppo nella history di tutti i membri che ne fanno parte
 * 
 * @param req messaggio che contiene i parametri per soddisfare la richiesta 
 * @param tmp mantiene traccia delle persone appartenenti a quel gruppo 
 * @param cont cardinalità di tmp 
 * @param type tipo messaggio inviato: testuale o file 
 * @return  -2 utente non può inviare il messaggio perchè non appartiene al gruppo
 *          0 il gruppo non esiste 
 *          1 successo operazione
 *        
 */
int IsGroup(message_t *req,char **tmp,int *cont,op_t type); 

/**
 * @function AddUserGroup
 * @brief Aggiunge un utente al gruppo
 * 
 * @param req messaggio che contiene tutte le informazioni per aggiungere l'utente
 *
 * @return -2 se l'utente non è registrato nel server
 *         -1 se l'utente non appartiene al gruppo
 *          0 il gruppo non esiste
 *          1 se il gruppo esiste e l'utente è stato aggiunto al gruppo 
 *        
 */
int AddUserGroup(message_t *req);

/**
 * @function IsIn
 * @brief COntrolla se un utente appartiene ad uno specifico gruppo 
 * 
 * @param i grupppo in cui ricercare la persona
 * @param name nome utente da ricercare

 * @return la posizione dell'utente nella lista degli utenti del gruppo i, 
 *          -1 altrimenti
 *        
 */
int IsIn(int i,char *name);


/**
 * @function DelGroup
 * @brief Elimina un singolo gruppo
 * 
 * @param nick nome del gruppo da eliminare 
 *
 * @return  0 il gruppo non esiste
 *          1 il gruppo è stato eliminato 
 *        
 */
int DelGroup(char *nick);


/**
 * @function DeleteGroup
 * @brief Trova il gruppo da eliminare e controlla se la richiesta di eliminazione 
 *        è stata fatta effettivamente dal creator
 * 
 * @param req messaggio che contiene i parametri per eliminare un gruppo: creator e nome gruppo essenzialmente 
 *
 * @return  0 il gruppo non esiste
 *          -1 il gruppo esiste ma l'utente non è il creatore quindi non può eliminarlo o non appartiene al gruppo
 *          1 l'utente è il creatore e quindi il gruppo può essere cancellato
 *        
 */
int DeleteGroup(message_t  *req);

/**
 * @function DeleteUserGroup
 * @brief    Elimina l'utente che ne ha fatto rchiesta dal gruppo a cui appartiene.
 *           Se il creatore decide di eliminarsi: n_membri>1 allora un utente viene eletto creatore del gruppo
 *                                                n_membri==1 allora l'intero gruppo viene cancellato  
 *
 * @param req messaggio che contiene i parametri per eliminare un utente: nome utente e nome gruppo essenzialmente 
 *
 * @return  0 il gruppo non esiste
 *          -1 l'utente non appartiene al gruppo  
 *           1 successo operazione
 *        
 */
int DelUserGroup(long fd,message_t *req);
