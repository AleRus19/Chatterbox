/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */
/**
 * @file config.h
 * @author Russo Alessio 545373
 *      Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *      originale dell'autore.
 * @brief File contenente alcune define con valori massimi utilizzabili
 */

#if !defined(CONFIG_H_)
#define CONFIG_H_

#define MAX_NAME_LENGTH                  32

/* aggiungere altre define qui */
#define UNIX_PATH_MAX  64

/**
 * @struct file_parameters
 * @brief Struttura parametri configurazione
 * @var DirName percorso cartelle memorizzare file
 * @var MaxFileSize dimensione massima del file
 * @var UnixPath percorso
 * @var StatFileName percorso e nome file statistiche
 * @var MaxHistMsg numero messaggi max in history
 * @var MaxMsgSize dimensione massima messaggi
 * @var THraedsInPool numero thread nel pool
 * @var MaxConnections numero massimo di connesioni pendenti
 * @var MaxUserGroup numero massimo di utenti in un gruppo
 */
struct file_parameters{
  char DirName[UNIX_PATH_MAX];
  int MaxFileSize;
  char UnixPath[UNIX_PATH_MAX];
  char StatFileName[UNIX_PATH_MAX];
  int MaxHistMsgs;
  int MaxMsgSize;
  int ThreadsInPool;
  int MaxConnections;
  int MaxUserGroup;
};


#define MAX_USER 			32 //Numero massimo di utenti online

#define MAX_LINE			512 //numero massimo di caratteri in un riga

// to avoid warnings like "ISO C forbids an empty translation unit"
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */
