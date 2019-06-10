#include <limits.h>
#include<string.h>
#include<stdio.h>
#include<stdlib.h>
#include<pthread.h>
#include "hash.h"

/**
 * @file  hash.c
 * @author Russo Alessio 545373
 *      Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *      originale dell'autore  
 * @brief Contiene le funzioni che implementano la tabella hash
 *        per la gestione degli utenti registrati
 *      
 */


/*  
  define che riguardano la funzione di hash presa dalla icl_hash
  fornita dal docente
*/
#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))

#define NUM 8  //Partizionamento della tabella Hash
#define HASHTOT 1024 //Dimensione della tabella Hash

#define CHECK_PTR_HASH(X, str)	\
    if ((X)==NULL) {		\
	perror(#str);		\
	exit(EXIT_FAILURE);	\
    } 


pthread_mutex_t mtxh[NUM]={PTHREAD_MUTEX_INITIALIZER}; //Array di mutex
//In questo modo quando si accede alla tabella per registare un utente o
//effetuare una ricerca non viene lockata l'intera tabella ma solo una porzione.
//Consentendo così accessi e inserimenti su zone di hash diverse concorrenti

item **Registrati;//Nome della tabella Hash

/**
 * @function InitHash
 * @brief Inizializza la tabella Hash 
 *
 * @return codice di errore in caso di fallimento
 *        
 */
void InitHash(){
  CHECK_PTR_HASH(Registrati=(item **)malloc((HASHTOT)*sizeof(item *)),Hash.Init);
  for(int i=0;i<HASHTOT;i++){Registrati[i]=NULL;}  
} 


/**
 * @function hash_Register
 * @brief calcolo chiave hash . Fornita dal docente icl_hash.
 * 
 * @param key chiave su cui applicare la funzione di hash
 *
 * @return posizione nella tabella hash
 *        
 */
unsigned int hash_Register(void* key){
    char *datum = (char *)key;
    unsigned int hash_value, i;
    if(!datum) return 0;
    for (hash_value = 0; *datum; ++datum) {
        hash_value = (hash_value << ONE_EIGHTH) + *datum;
        if ((i = hash_value & HIGH_BITS) != 0)
            hash_value = (hash_value ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
    }
    return (hash_value)%HASHTOT;
}


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
int FindUserHash(char *nick){
  int i=hash_Register(nick);//calcolo l'hash
  int pos=(i%NUM);//calcolo la porzione di hash da lockare
  pthread_mutex_lock(&mtxh[pos]);//locko quella parte di hash per effettuare la ricerca
  item *ptr=Registrati[i];int trovato=0;//Ricerco nella lista di trabocco se è presente il nick
  while(ptr!=NULL && trovato==0){
    if(strncmp(ptr->value,nick,MAX_NAME_LENGTH+1)==0)trovato=1;//se trovato allora esco subito
    ptr=ptr->next;
 }
  pthread_mutex_unlock(&mtxh[pos]);//rilascio il lock sulla porzione di hash
  return trovato; 
}


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
int InsertUserHash(char * x){
  int i=0;
  i=hash_Register(x);//calcolo l'hash di x
  int pos=(i%NUM);//calcolo la porzione di hash
  pthread_mutex_lock(&mtxh[pos]);//prendo il lock sulla porzione di hash interressata
  if(Registrati[i]==NULL){//Nel caso non ci sono collisioni: inserisco creo un nuovo utente x
    item *new;
    CHECK_PTR_HASH(new=(item *)malloc(sizeof(item)),Hash.Inserisci);
    new->next=NULL;
    strncpy(new->value,x,MAX_NAME_LENGTH+1);
    Registrati[i]=new;
    pthread_mutex_unlock(&mtxh[pos]);//rilascio il lock sulla porzione
    return 0;
  }else{//se ho collisioni, devo controllare che l'utente non sia già stato inserito
    item *ptr=Registrati[i];
    int trovato=0;
    while(ptr!=NULL && trovato==0){
      if(strncmp(ptr->value,x,MAX_NAME_LENGTH+1)==0)trovato=1;
      ptr=ptr->next;
    }
    if(trovato==0){//se non esiste lo inserisco
      item *new;
      CHECK_PTR_HASH(new=(item *)malloc(sizeof(item)),Hash.Inserisci.trovato);
      strncpy(new->value,x,MAX_NAME_LENGTH+1);
      new->next=Registrati[i];
      Registrati[i]=new;
      pthread_mutex_unlock(&mtxh[pos]);//rilascio il lock sulla porzione di hash
      return 0;
    }
  }
  pthread_mutex_unlock(&mtxh[pos]);//rilascio il lock sulla porzione di hash
  return 1;
}


/**
 * @function DeleteUserHash
 * @brief Elimina l'elemento dalla tabella Hash 
 * 
 * @param nick nome dell'utente da rimuovere
 *        
 */
void DeleteUserHash(char *nick){
  int i=hash_Register(nick); //calcolo l'hash su nick
  int pos=(i%NUM); //individuo la porzione da lockare
  pthread_mutex_lock(&mtxh[pos]);//prendo la lock su quella porzione
  item *ptr=Registrati[i];item *prev=NULL;//Inizio la ricerca di nick, quando lo trovo lo elimino
  while(ptr!=NULL){
    if(strncmp(ptr->value,nick,MAX_NAME_LENGTH+1)==0){
      if(prev==NULL){
        Registrati[i]=ptr->next;
        ptr->next=NULL;
        free(ptr); 
        ptr=Registrati[i];
      }
      else{
        prev->next=ptr->next;
        ptr->next=NULL;
        free(ptr);
        ptr=prev->next;
      }
    }
    else {prev=ptr; ptr=ptr->next;}
   }
   pthread_mutex_unlock(&mtxh[pos]);//rilascio la lock su quella porzione di hash
}


/**
 * @function Destroy
 * @brief Dealloca l'intera tabella Hash al momento della terminazione del server 
 * 
 */

void Destroy(){
 item *prev=NULL;item *ptr=NULL;int i=0;item *old=ptr;
 for(;i<HASHTOT;i++){ 
    ptr=Registrati[i];prev=NULL;old=ptr;
    while(old!=NULL){
      prev=ptr->next;
      free(ptr);
      old=prev;
      ptr=prev;
    }
  }
  free(Registrati);
}
