/**
 * @file  history.c
 *  
 * @author Russo Alessio 545373
 *      Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *      originale dell'autore.
 * @brief Struttura dati che permette di gestire la history di ogni utente registrato.
 *        Contiene informazioni : nome utente, messaggi con i rispettivi mittenti
 *        l'inizio dei messaggi ancora da leggere.
 */

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<pthread.h>
#include <limits.h>

#include "config.h"
#include "message.h"
#include "history.h"
#include "connections.h"

//define usate per il calcolo della funzione di hash
#define BITS_IN_int     ( sizeof(int) * CHAR_BIT )
#define THREE_QUARTERS  ((int) ((BITS_IN_int * 3) / 4))
#define ONE_EIGHTH      ((int) (BITS_IN_int / 8))
#define HIGH_BITS       ( ~((unsigned int)(~0) >> ONE_EIGHTH ))

//macro controllo puntatori
#define CHECK_PTR_HIST(X, str)	\
    if ((X)==NULL) {		\
	perror(#str);		\
	exit(EXIT_FAILURE);	\
    }

//parametri di configurazione per la hsitory
extern struct file_parameters configure;

#define NUM 8//n_porzioni tavola hash_history
#define HISTOT 1024//dimensione fissata history

pthread_mutex_t mtxhist[NUM]={PTHREAD_MUTEX_INITIALIZER};//array di mutex per l'accesso in mutua esclusione alle porzioni della hash_history 
hist **buffer;//hash_history
int h;//contatore


/**
 * @function InitHistory
 * @brief Inizializza la tabella hash per la history
 *
 * @return codice di errore in caso di fallimento
 *        
 */
void InitHistory(){
  CHECK_PTR_HIST(buffer=(hist **)malloc((HISTOT)*sizeof(hist *)),History.InitHistory.buffer);
  for(int i=0;i<HISTOT;i++){
    buffer[i]=NULL;
  }  
} 

/**
 * @function FreeHistory
 * @brief Dealloca l'intera History al momento della terminazione del server 
 * 
 */
void FreeHistory(){
  for(int p=0;p<HISTOT;p++){
    hist *ptr=buffer[p];
    while(ptr!=NULL){
      hist *prev=ptr;
      for(int i=0;i<configure.MaxHistMsgs;i++){
          free(prev->buf[i]);free(prev->type[i]);
          free(prev->sender[i]);
        }
        free(prev->buf);free(prev->type);
        free(prev->sender);free(prev->letto);free(prev->user);
        
      ptr=ptr->next; free(prev);
    }
    buffer[p]=NULL;
  }
  free(buffer);
} 

/**
 * @function hash_History
 * @brief calcolo chiave hash 
 * 
 * @param key chiave su cui applicare la funzione di hash
 *
 * @return posizione nella tabella hash
 *        
 */

unsigned int hash_History(void* key){
    char *datum = (char *)key;
    unsigned int hash_value, i;
    if(!datum) return 0;
    for (hash_value = 0; *datum; ++datum) {
        hash_value = (hash_value << ONE_EIGHTH) + *datum;
        if ((i = hash_value & HIGH_BITS) != 0)
            hash_value = (hash_value ^ (i >> THREE_QUARTERS)) & ~HIGH_BITS;
    }
    return (hash_value)%HISTOT;
}


/**
 * @function InsertHistory
 * @brief Registra e inizializza un nuovo utente nella history 
 * 
 * @param nick nome dell'utente da inserire
 *
 * @return 1 se l'utente è stato regstrato, altrimenti operazione fallita
 *        
 */
int InsertHistory(char *nick){
  int i=0;
  i=hash_History(nick);
  int pos=(i%NUM);
  hist *new;
  CHECK_PTR_HIST(new=(hist *)malloc(sizeof(hist)),History.InserHistory.new);
  new->next=NULL;
  CHECK_PTR_HIST(new->user=malloc(MAX_NAME_LENGTH*sizeof(char)),History.Insert.new.user);
  CHECK_PTR_HIST(new->buf=malloc(configure.MaxHistMsgs*sizeof(char*)),History.Insert.new.buf);
  for(int i=0;i<configure.MaxHistMsgs;i++){ CHECK_PTR_HIST(new->buf[i]=malloc(configure.MaxMsgSize*sizeof(char)),History.Insert.new.buf.i);}
  CHECK_PTR_HIST(new->sender=malloc(configure.MaxHistMsgs*sizeof(char*)),History.Insert.new.sender);
  for(int i=0;i<configure.MaxHistMsgs;i++){CHECK_PTR_HIST(new->sender[i]=malloc(MAX_NAME_LENGTH*sizeof(char)),History.Insert.new.sender.i);}
  CHECK_PTR_HIST(new->type=malloc(configure.MaxHistMsgs*sizeof(char *)),History.Insert.new.type);
  for(int i=0;i<configure.MaxHistMsgs;i++){CHECK_PTR_HIST(new->type[i]=malloc(MAX_NAME_LENGTH*sizeof(char)),History.Insert.new.type.i);}
  CHECK_PTR_HIST(new->letto=malloc(configure.MaxHistMsgs*sizeof(int)),History.Insert.new.letto);
  pthread_mutex_lock(&mtxhist[pos]);//prendo il lock sulla porzione interssata
  if(buffer[i]==NULL){//se non abbiamo collisioni lo inserisco direttamente, inizializando i valori
    strncpy(new->user,nick,MAX_NAME_LENGTH);
    new->start=0; new->end=0;
    new->tot=0;
    for(int j=0;j<configure.MaxHistMsgs;j++)new->letto[j]=-1;
    buffer[i]=new;
    pthread_mutex_unlock(&mtxhist[pos]);
    return 0;
  }else{//ci sono state collisioni
    hist *ptr=buffer[i];
    int trovato=0;
    while(ptr!=NULL && trovato==0){
      if(strncmp(ptr->user,nick,MAX_NAME_LENGTH)==0)trovato=1;
      ptr=ptr->next;
    }
    if(trovato==0){//se non lo trovo nella lista di trabocco allora lo inserisco
      strncpy(new->user,nick,MAX_NAME_LENGTH);
      new->next=buffer[i];
      new->start=0; new->end=0;
      new->tot=0;
      for(int j=0;j<configure.MaxHistMsgs;j++)new->letto[j]=-1;
      buffer[i]=new;
      pthread_mutex_unlock(&mtxhist[pos]);
      return 0;
    }
  }
  pthread_mutex_unlock(&mtxhist[pos]);
  return 1;
}



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
int InsertMsgHistory(char *nick,char *msg,char *sender,char *type){
  int i=0;
  i=hash_History(nick);
  int pos=(i%NUM);
  pthread_mutex_lock(&mtxhist[pos]);//lock sulla porzione di tabella hash_history
   hist *ptr=buffer[i];
   int trovato=0;
   while(ptr!=NULL && trovato==0){//trovo l'utente e inserisco il messaggio nella sua history, aggiornando tutti i valori
     if(strncmp(ptr->user,nick,MAX_NAME_LENGTH)==0){
       trovato=1;//algoritmo di aggiornamento degli array circolari della history
       if(((ptr->end)%configure.MaxHistMsgs)==((ptr->start)%configure.MaxHistMsgs)&&(ptr->end!=0)){ptr->end=ptr->start;ptr->start++;}
       strncpy(ptr->sender[(ptr->end%configure.MaxHistMsgs)],sender,MAX_NAME_LENGTH); 
       strcpy(ptr->type[(ptr->end%configure.MaxHistMsgs)],type); 
       strncpy(ptr->buf[(ptr->end%configure.MaxHistMsgs)],msg,configure.MaxMsgSize); 
       ptr->end++;ptr->letto[(ptr->end%configure.MaxHistMsgs)]=1;
       if(ptr->tot<configure.MaxHistMsgs){ptr->tot++;}
     }
     ptr=ptr->next;
   }
  pthread_mutex_unlock(&mtxhist[pos]);
  return trovato;//esito
}


/**
 * @function DeleteUserHistory
 * @brief Elimina un utente e tutte le sue informazioni 
 * 
 * @param nick nome dell'utente da rimuovere
 *
 * @return 1 in caso di successo
 *        
 */
int DeleteUserHistory(char *nick){
  int i=0;
  i=hash_History(nick);
  int pos=(i%NUM);
  pthread_mutex_lock(&mtxhist[pos]);//prendo il lock sulla porzione di tabella
  hist *ptr=buffer[i];hist *prev=NULL;int trovato=0;
  while(ptr!=NULL && trovato==0){
    if(strncmp(ptr->user,nick,MAX_NAME_LENGTH)==0){ //cerco ed elimino l'utente se presente e tutte le informazioni
      if(prev==NULL){
        buffer[i]=ptr->next;
        ptr->next=NULL;
        for(int i=0;i<configure.MaxHistMsgs;i++){
          free(ptr->buf[i]);free(ptr->type[i]);
          free(ptr->sender[i]);      
        }
        free(ptr->buf);free(ptr->type);
        free(ptr->sender);free(ptr->letto);free(ptr->user);
        free(ptr); trovato=1;
        ptr=buffer[i];
      }
      else{
        prev->next=ptr->next;
        ptr->next=NULL;
         for(int i=0;i<configure.MaxHistMsgs;i++){
          free(ptr->buf[i]);free(ptr->type[i]);
          free(ptr->sender[i]);       
        }
        free(ptr->buf);free(ptr->type);
        free(ptr->sender);free(ptr->letto);free(ptr->user);
        
        free(ptr);trovato=1;
        ptr=prev->next;
      }
    }
    else {prev=ptr; ptr=ptr->next;}
  } 
  pthread_mutex_unlock(&mtxhist[pos]);//rilascio il lock sulla porzione
  return 1;
}



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
int InsertMsgHistoryBroadcast(char *msg,char *sender,char *type){
   int cont=0;
   for(int p=0;p<HISTOT;p++){//scandisco l'intera hash 
     int pos=(p%NUM);
     pthread_mutex_lock(&mtxhist[pos]);
     hist *ptr=buffer[p];
     while(ptr!=NULL){//algoritmo di aggiornamento array circolare per messaggi: start e end
       if(((ptr->end)%configure.MaxHistMsgs)==((ptr->start)%configure.MaxHistMsgs)&&(ptr->end!=0)){ptr->end=ptr->start;ptr->start++;}
       strncpy(ptr->sender[(ptr->end%configure.MaxHistMsgs)],sender,MAX_NAME_LENGTH); 
       strcpy(ptr->type[(ptr->end%configure.MaxHistMsgs)],type); 
       strncpy(ptr->buf[(ptr->end%configure.MaxHistMsgs)],msg,configure.MaxMsgSize); 
       ptr->end++;cont++;ptr->letto[(ptr->end%configure.MaxHistMsgs)]=1;
       if(ptr->tot<configure.MaxHistMsgs){ptr->tot++;}
       ptr=ptr->next;
     }
     pthread_mutex_unlock(&mtxhist[pos]);

   } 
   return cont;
}




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
int GetHistory(long fd,message_t *req,int *mex,int *file){
  int i=0;
  i=hash_History(req->hdr.sender);
  int pos=(i%NUM);
  pthread_mutex_lock(&mtxhist[pos]);//prendo il lock sulla porzione di history interessata
  hist *ptr=buffer[i];int trovato=0;
  while(ptr!=NULL && trovato==0){
     if(strncmp(ptr->user,req->hdr.sender,MAX_NAME_LENGTH)==0){//trovo l'utente
       trovato=1; 
       int l=0;
       SendHeader(fd,OP_OK);
       l=ptr->tot; 
       req->data.hdr.len=sizeof(size_t);
       CHECK_PTR_HIST(req->data.buf=calloc(1,sizeof(char)*req->data.hdr.len),History.Insert.GetHistory);
       strcpy(req->data.buf,(char*)&l);
       sendData(fd,&(req->data));//invio l'header che contiene il numero di messaggi da inviare
       free(req->data.buf);
       int i=(ptr->start)%configure.MaxHistMsgs;
       for(int j=0;j<ptr->tot;++j){//preparo e invio ogni singolo messaggio della history
         strncpy(req->hdr.sender,ptr->sender[i],MAX_NAME_LENGTH);
         if(strcmp(ptr->type[i],"21")==0)req->hdr.op=TXT_MESSAGE;
         else req->hdr.op=FILE_MESSAGE;
         req->data.hdr.len=strlen(ptr->buf[i]);
         if(req->data.hdr.len!=0){
           CHECK_PTR_HIST(req->data.buf=calloc(req->data.hdr.len,sizeof(char)),History.GetHistory);
           strncpy(req->data.buf,ptr->buf[i],req->data.hdr.len);
           SendMsg(fd,req);
           //per evitare di contare più volte gli stessi messaggi
           //controllo se il messaggio è già stato letto o ancora no
           //mex e file contatori di messaggi non ancora letti prima della richiesta
           if(req->hdr.op==TXT_MESSAGE && ptr->letto[i]==1){ptr->letto[i]=0; mex++;}
           if(req->hdr.op==FILE_MESSAGE && ptr->letto[i]==1){ptr->letto[i]=0; file++;}
           i++;i=i%configure.MaxHistMsgs;free(req->data.buf);
         }
       }
     }
    ptr=ptr->next;
   } 
  pthread_mutex_unlock(&mtxhist[pos]);
  return trovato;  
}
