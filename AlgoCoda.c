#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h> 
#include <pthread.h>
#include<errno.h> 
#include<sys/wait.h>
#include<sys/stat.h>
#include "AlgoCoda.h"

/**
 * @file  AlgoCoda.c
 * @author Russo Alessio 545373
 *      Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *      originale dell'autore 
 * @brief Tale coda è stata inizialmente sviluppata da me durante il corso di algoritmica
          e successivamente resa concorrente con il laboratorio di sistemi operativi.
          Contiene le funzioni che implementano la coda concorrente dei descrittori pronti
          usata dai thread worker e listener del server, basato sul modello produttore/consumatore.
          Produttore= thread listener Consumatore=Thread Worker
 */


#define CHECK_PTR_CODA(X, str)	\
    if ((X)==NULL) {		\
	perror(#str);		\
	exit(EXIT_FAILURE);	\
    }

pthread_mutex_t mtx=PTHREAD_MUTEX_INITIALIZER;//variabile di mutua esclusione per accesso alla coda
pthread_cond_t empty=PTHREAD_COND_INITIALIZER;//variabile di condizione per la sincronizzazione
/**
 *@struct node
 *@brief struttura coda descrittori pronti
 *@var info descrittore di connesione
 *@var next puntatore al prossimo elemento
*/
struct node{
  long info;
  struct node *next;
};

//Variabili condivise
struct node *head=NULL; //inizio coda
struct node *last=NULL;  //fine coda
int nelem=0; //numero di elementi

/**
 * @function Push
 * @brief Inserisce un descrittore di connessione nella coda 
 *
 * @param fd file descriptor da inserire
 *
 * @return messaggio di errore in caso di insuccesso
 *         
 */
void Push(long fd){
    struct node *n;
    CHECK_PTR_CODA(n=(struct node *) malloc(sizeof(struct node)),Coda.Push.n);   
    n->info=fd;n->next=NULL;
    pthread_mutex_lock(&mtx);//ottengo l'uso esclusivo
    if(head==NULL){head=n;last=head;} //inserisco in testa se inizialmente vuota
    else{ 
      last->next=n;//inserisco in coda
      last=last->next;
    }
    nelem++;
    //se l'elemento inserito è -1, si attiva il protocollo di terminazione e si risvegliano tutti i worker sospesi
    if(n->info==-1)pthread_cond_broadcast(&empty);
    else pthread_cond_signal(&empty);//altrimenti si risveglia solo uno tra i worker sospesi
    pthread_mutex_unlock(&mtx);//rilascio il lock
}


/**
 * @function Pop
 * @brief Estrae un file descriptor dalla coda
 *
 * @return -1 in caso di attivazione del protocollo di terminazione
 *          altrimenti file descriptor estratto
 */
int pop(){
  pthread_mutex_lock(&mtx); //ottengo l'uso esclusivo
  while(nelem==0){ //se il numero di elementi è 0 mi sospendo sulla coda
    pthread_cond_wait(&empty,&mtx);
  }
  struct node * n=head; //altrimenti estraggo il primo in coda
  if(n->info==-1){//durante la fase di terminazione
    pthread_mutex_unlock(&mtx);//rilascio il lock
    return -1; //ritorno -1
  }
  head=head->next;//aggiorno
  nelem--;int r=n->info;//ritorno il descrittore estratto, rimuovendolo dalla coda
  pthread_mutex_unlock(&mtx);free(n);
  return r;
}


/**
 * @function DestoryList
 * @brief Dealloca quando il server vuole terminare la sua esecuzione
 *
 *
 */
void DestroyList(){
  free(head);
}

/**
 * @function Wake
 * @brief Risveglio un thread sospeso sulla coda
 *
 */
void Wake(){
  pthread_cond_signal(&empty);
}


