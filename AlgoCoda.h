


/**
 * @file  AlgoCoda.h
 * @author Russo Alessio 545373
 *      Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *      originale dell'autore 
 * @brief Tale coda è stata inizialmente sviluppata da me durante il corso di algoritmica
          e successivamente resa concorrente con il laboratorio di sistemi operativi.
          Contiene le funzioni che implementano la coda concorrente
          usata dai thread del server.
 */

/**
 * @function Push
 * @brief Inserisce un file descriptor nella coda 
 *
 * @param fd file descriptor da inserire
 *
 * @return messaggio di errore in caso di insuccesso
 *         
 */
void Push(long fd);


/**
 * @function Pop
 * @brief Estrae un file descriptor dalla coda
 *
 * @return -1 in caso di attivazione del protocollo di terminazione
 *          altrimenti file descriptor estratto
*/
int pop();

/**
 * @function Wake
 * @brief Risveglio un thread sospeso sulla coda
 *
*/
void Wake();

/**
 * @function DestoryList
 * @brief Dealloca quando il server vuole terminare la sua esecuzione
 *
 *
 */
void DestroyList();


