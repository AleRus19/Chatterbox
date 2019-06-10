/*
 * chatterbox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
#ifndef CONNECTIONS_H_
#define CONNECTIONS_H_

#define MAX_RETRIES     10
#define MAX_SLEEPING     3
#if !defined(UNIX_PATH_MAX)
#define UNIX_PATH_MAX  64
#endif

#include "message.h"

/**
 * @file  connections.h
 * @author Russo Alessio 545373
 *      Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *      originale dell'autore.
 *
 * @brief Contiene le funzioni che implementano il protocollo 
 *        tra i clients ed il server
 */

/**
 * @function openConnection
 * @brief Apre una connessione AF_UNIX verso il server 
 *
 * @param path Path del socket AF_UNIX 
 * @param ntimes numero massimo di tentativi di retry
 * @param secs tempo di attesa tra due retry consecutive
 *
 * @return il descrittore associato alla connessione in caso di successo
 *         -1 in caso di errore
 */
int openConnection(char* path, unsigned int ntimes, unsigned int secs);

// -------- server side ----- 
/**
 * @function readHeader
 * @brief Legge l'header del messaggio
 *
 * @param connfd     descrittore della connessione
 * @param hdr    puntatore all'header del messaggio da ricevere
 *
 * @return <=0 se c'e' stato un errore 
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
int readHeader(long connfd, message_hdr_t *hdr);
/**
 * @function readData
 * @brief Legge il body del messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al body del messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
int readData(long fd, message_data_t *data);

/**
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param data   puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */
int readMsg(long fd, message_t *msg);

/* da completare da parte dello studente con altri metodi di interfaccia */


// ------- client side ------
/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server 
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendRequest(long fd, message_t *msg);

/**
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendData(long fd, message_data_t *msg);


/* da completare da parte dello studente con eventuali altri metodi di interfaccia */ 

/**
 * @function readn
 * @brief Legge da un file
 *
 * @param fd FileDescriptor dal quale legge 
 * @param buf contiene il messaggio letto
 * @param size nbytes da leggere
 *
 * @return il numero di bytes letti
 *         -1 in caso di errore
 */
int readn(long fd, void *buf, size_t size) ;

/**
 * @function writen
 * @brief Invia un messaggio al destinatario
 *
 * @param fd FileDescriptor associato al destinatario
 * @param buf messaggio da scrivere
 * @param size nbytes da scrivere 
 *
 * @return il numero di bytes scritti
 *         -1 in caso di errore
 */
int writen(long fd, void *buf, size_t size);


/**
 * @function SendHeader
 * @brief Invia l'header messaggio
 *
 * @param fd     descrittore della connessione
 * @param op     esito richiesta
 *
 */
int SendHeader(long fd, op_t op);

/**
 * @function SendMsg
 * @brief Invia l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param req  puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 */
int SendMsg(long fd,message_t *req);
#endif /* CONNECTIONS_H_ */
