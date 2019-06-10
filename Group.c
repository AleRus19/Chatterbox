#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include <limits.h>
#include "Group.h"
#include "config.h"
#include "history.h"
#include "hash.h"

/**
 * @file  Group.h
 * @author Russo Alessio 545373
 *      Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *      originale dell'autore.
 * @brief Struttura dati:hash
 *        Permette la gestione dei gruppi
 */

//define per funzione di hash
#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))

#define CHECK_PTR_GROUP(X, str)	\
    if ((X)==NULL) {		\
	perror(#str);		\
	exit(EXIT_FAILURE);	\
    }

#define NUM 8 //numero di partizione della tbella
#define GROUPTOT 1024//dimensione tabella hash

extern struct file_parameters configure;//parametri di configurazione

//pthread_mutex_t *mtxgroup;//mutex per l'accesso alla hash gruppi
group **gruppi;//struttura hash gruppi


pthread_mutex_t mtxgroup[NUM]={PTHREAD_MUTEX_INITIALIZER};//array di mutex per l'accesso in mutua esclusione alle porzioni della hash_gruppi



/**
 * @function InitGroup
 * @brief Inizializza la tabella hash dei Gruppi
 *
 * @return codice di errore in caso di fallimento
 *        
 */
void InitGroup(){
  CHECK_PTR_GROUP(gruppi=(group **)malloc(GROUPTOT*sizeof(group *)),Group.InitGroup.gruppi);
  for(int i=0;i<GROUPTOT;i++){
    gruppi[i]=NULL;
  } 
} 

/**
 * @function FreeGroup
 * @brief Dealloca l'intera tabella hash Gruppi al momento della terminazione del server 
 * 
 */
void FreeGroup(){
  for(int p=0;p<GROUPTOT;p++){
    group *ptr=gruppi[p];
    while(ptr!=NULL){
      group *prev=ptr;
      for(int i=0;i<configure.MaxUserGroup;i++){
        if(prev->GroupList[i]!=NULL)free(prev->GroupList[i]);
      }
      free(prev->GroupList);
      ptr=ptr->next;free(prev);
    }
    gruppi[p]=NULL;
  }
  free(gruppi);
} 


/**
 * @function hash_Gruppi
 * @brief calcolo chiave hash 
 * 
 * @param key chiave su cui applicare la funzione di hash
 *
 * @return posizione nella tabella hash
 *        
 */
unsigned int hash_Gruppi(void* key){
    char *datum = (char *)key;
    unsigned int hash_value, i;
    if(!datum) return 0;
    for (hash_value = 0; *datum; ++datum) {
        hash_value = (hash_value << ONE_EIGHTH) + *datum;
        if ((i = hash_value & HIGH_BITS) != 0)
            hash_value = (hash_value ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
    }
    return (hash_value)%GROUPTOT;
}



/**
 * @function InsertGroup
 * @brief Registra e inizializza un nuovo gruppo nella tabella gruppi e il suo creatore
 * 
 * @param req messaggio richiesta che contiene le informazioni per registrare un gruppo
 *
 * @return 1 se il gruppo è stato registrato correttamene,-1 se esite già, altrimenti messaggio di errore 
 *        
 */
int InsertGroup(message_t *req){
  int i=0; 
  i=hash_Gruppi(req->data.hdr.receiver);
  int pos=(i%NUM);
  group *new;
  pthread_mutex_lock(&mtxgroup[pos]);  //prendo la lock sui gruppi solo nella porzione interessata
  if(gruppi[i]==NULL){//gruppo inesistente -> lo registro
    CHECK_PTR_GROUP(new=(group *)calloc(1,sizeof(group)),Group.InserGroup.new); 
    new->next=NULL;
    CHECK_PTR_GROUP(new->GroupList=malloc(configure.MaxUserGroup*sizeof(char*)),Group.Insert.new.buf);
    for(int i=0;i<configure.MaxUserGroup;i++){ CHECK_PTR_GROUP(new->GroupList[i]=malloc(MAX_NAME_LENGTH*sizeof(char)),Group.Insert.new.GroupList.i);}
    strncpy(new->name,req->data.hdr.receiver,MAX_NAME_LENGTH+1);
    strncpy(new->creator,req->hdr.sender,MAX_NAME_LENGTH+1);
    strncpy(new->GroupList[0],req->hdr.sender,MAX_NAME_LENGTH+1);
    gruppi[i]=new;new->n_membri=1;
    pthread_mutex_unlock(&mtxgroup[pos]);
    return 1;
  }else{//scandisco la lista di trabocco a causa delle collisioni
    group *ptr=gruppi[i];
    int trovato=0;
    while(ptr!=NULL && trovato==0){//controllo se il gruppo già esiste
      if(strncmp(ptr->name,req->data.hdr.receiver,MAX_NAME_LENGTH+1)==0)trovato=1;
      ptr=ptr->next;
    }
    if(trovato==0){//gruppo inesistente -> lo registro
      CHECK_PTR_GROUP(new=(group *)calloc(1,sizeof(group)),Group.InserGroup.new); 
      new->next=NULL;
      CHECK_PTR_GROUP(new->GroupList=malloc(configure.MaxUserGroup*sizeof(char*)),Group.Insert.new.buf);
      for(int i=0;i<configure.MaxUserGroup;i++){ CHECK_PTR_GROUP(new->GroupList[i]=malloc(MAX_NAME_LENGTH*sizeof(char)),Group.Insert.new.GroupList.i);}
      strncpy(new->name,req->data.hdr.receiver,MAX_NAME_LENGTH+1);
      strncpy(new->creator,req->hdr.sender,MAX_NAME_LENGTH+1);
      strncpy(new->GroupList[0],req->hdr.sender,MAX_NAME_LENGTH+1);
      gruppi[i]=new;new->n_membri=1;
      pthread_mutex_unlock(&mtxgroup[pos]);
      return 1;
    }
    else{
      pthread_mutex_unlock(&mtxgroup[pos]);
      return -1;  
    }
  }
  pthread_mutex_unlock(&mtxgroup[pos]);
  return -1;
}

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
int IsIn(int i,char *name){
  group *ptr=gruppi[i];int trovato=-1;
  while(ptr!=NULL && trovato==-1){
    for(int j=0;j<ptr->n_membri;j++){//controllo se un utente è membro del gruppo
      if(strncmp(name,ptr->GroupList[j],MAX_NAME_LENGTH+1)==0){
        trovato=j;
      }
    }
   ptr=ptr->next;
  }
  return trovato;  
}


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
int AddUserGroup(message_t *req){
  int i=0;
  if(FindUserHash(req->hdr.sender)!=1)return -2;//Controllo se l'utente esiste
  i=hash_Gruppi(req->data.hdr.receiver);
  int pos=(i%NUM);
  pthread_mutex_lock(&mtxgroup[pos]);//prendo la lock sulla partizione
  group *ptr=gruppi[i];
  int trovato=0;
    while(ptr!=NULL && trovato==0){
     if(strncmp(ptr->name,req->data.hdr.receiver,MAX_NAME_LENGTH+1)==0){
       trovato=1;//ho trovato il gruppo e se l'utente non appartiene al gruppo, lo inserisco
       if(IsIn(i,req->hdr.sender)==-1){strncpy(ptr->GroupList[ptr->n_membri],req->hdr.sender,MAX_NAME_LENGTH+1);ptr->n_membri++;}
       else{trovato=-1;}
     }
     ptr=ptr->next;
  }
  pthread_mutex_unlock(&mtxgroup[pos]);//rilascio il lock sulla porzione
  return trovato;
}


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
int DelGroup(char *nick){
  int i=hash_Gruppi(nick);
  group *ptr=gruppi[i];group *prev=NULL;
  while(ptr!=NULL){//trovo il gruppo e lo elimino se esiste
    if(strncmp(ptr->name,nick,MAX_NAME_LENGTH+1)==0){
      if(prev==NULL){//scorro la lista
        gruppi[i]=ptr->next;
        for(int i=0;i<configure.MaxUserGroup;i++){free(ptr->GroupList[i]);}//libero la lista partecipanti
        ptr->next=NULL;free(ptr->GroupList);
        free(ptr); 
        ptr=gruppi[i];
        return 1;
      }
      else{
        prev->next=ptr->next;
        ptr->next=NULL;
        for(int i=0;i<configure.MaxUserGroup;i++){free(ptr->GroupList[i]);}
        free(ptr->GroupList);
        free(ptr);
        ptr=prev->next;
        return 1;
      }
    }
    else {prev=ptr; ptr=ptr->next;}
   }
   return  0;
}


/**
 * @function DeleteGroup
 * @brief Trova il gruppo da eliminare e controlla se la richiesta di eliminazione 
 *        è stata fatta effettivamente dal creator
 * 
 * @param req messaggio che contiene i parametri per eliminare un gruppo: creator e nome gruppo essenzialmente 
 *
 * @return  0 il gruppo non esiste
 *          -1 il gruppo esiste ma l'utente non è il creatore quindi non può eliminarlo o non IsIn al gruppo
 *          1 l'utente è il creatore e quindi il gruppo può essere cancellato
 *        
 */
int DeleteGroup(message_t  *req){
  int i=0;
  i=hash_Gruppi(req->data.hdr.receiver);
  int pos=(i%NUM);
  pthread_mutex_lock(&mtxgroup[pos]);//prendo il lock sulla porzione di tabella
  group *ptr=gruppi[i];
  int trovato=0;
  while(ptr!=NULL && trovato==0){//scorro la lista per trovare il gruppo
     //se il gruppo esiste e chi ha fatto la ricerca appartiene a tale gruppo
     if(strncmp(ptr->name,req->data.hdr.receiver,MAX_NAME_LENGTH+1)==0 && IsIn(i,req->hdr.sender)>=0){
       //ed è il creatore del gruppo
       if(strncmp(req->hdr.sender,ptr->creator,MAX_NAME_LENGTH+1)==0){
         DelGroup(ptr->name); trovato=1;//elimina gruppo
       }  
       else{trovato=-1;}
     }
     else{trovato=-1;}
     if(trovato!=1)ptr=ptr->next;
  }
  pthread_mutex_unlock(&mtxgroup[pos]);//rilascio il lock sulla porzione di tabella
  return trovato;
}

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
int IsGroup(message_t *req,char **tmp,int *cont,op_t type){
  int i=0;
  i=hash_Gruppi(req->data.hdr.receiver);
  int pos=(i%NUM);
  pthread_mutex_lock(&mtxgroup[pos]);//prendo il lock sulla porzione di tabella
  group *ptr=gruppi[i];
  int trovato=0;
  while(ptr!=NULL && trovato==0){
     //se trovo il gruppo e chi vuole inviare il messaggio appartiene al gruppo
     if(strncmp(ptr->name,req->data.hdr.receiver,MAX_NAME_LENGTH+1)==0 && IsIn(i,req->hdr.sender)>=0){
        //invia il messaggio a tutti quelli presenti nel gruppo incluso te stesso
       for(int i=0;i<ptr->n_membri;i++){
          if(type==TXT_MESSAGE){InsertMsgHistory(ptr->GroupList[i],req->data.buf,req->hdr.sender,"21");}
          else{InsertMsgHistory(ptr->GroupList[i],req->data.buf,req->hdr.sender,"22");}
          strncpy(tmp[*cont],ptr->GroupList[i],MAX_NAME_LENGTH+1);(*cont)++;
       }
       trovato=1;
      }
      else if(IsIn(i,req->hdr.sender)==-2){pthread_mutex_unlock(&mtxgroup[pos]);return -2;}
     ptr=ptr->next;
   }
   pthread_mutex_unlock(&mtxgroup[pos]);
   return trovato;
}



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
int DelUserGroup(long fd,message_t *req){
  int i=0; 
  i=hash_Gruppi(req->data.hdr.receiver);
  int pos=(i%NUM);
  pthread_mutex_lock(&mtxgroup[pos]);//prendo il lock sulla porzione di tabella
  group *ptr=gruppi[i];
  int trovato=0;
  while(ptr!=NULL && trovato==0){
    if(strncmp(ptr->name,req->data.hdr.receiver,MAX_NAME_LENGTH+1)==0){
      int j=IsIn(i,req->hdr.sender);  
      if(j>=0){
        if(strncmp(req->hdr.sender,ptr->creator,MAX_NAME_LENGTH+1)==0){
          if(ptr->n_membri>1){//elimina il creatore e scegli qualcun'altro che prenda il suo posto
             strncpy(ptr->creator,ptr->GroupList[ptr->n_membri-1],MAX_NAME_LENGTH+1); 
             ptr->GroupList[j]=ptr->GroupList[ptr->n_membri-1];
             free(ptr->GroupList[ptr->n_membri-1]);
             ptr->n_membri--;trovato=1;
           }//dato che nel gruppo vi è solo il creatore, elimino tutto il gruppo
           else{pthread_mutex_unlock(&mtxgroup[pos]);DeleteGroup(req);trovato=1;return trovato;}
         }
       }
       else{trovato=-1;}
     }
     else{trovato=-1;}
     ptr=ptr->next;
  }
   pthread_mutex_unlock(&mtxgroup[pos]);//rilascio la porzione della tabella
  return trovato;
}

