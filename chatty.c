/*
 * membox Progetto del corso di LSO 2017/2018
 *
 * Dipartimento di Informatica Università di Pisa
 * Docenti: Prencipe, Torquati
 * 
 */

/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 *
 *  @author Russo Alessio 545373
 *      Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *      originale dell'autore.
 */

#define _POSIX_C_SOURCE 200809L
#include <libgen.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/un.h>
#include <assert.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <errno.h> 
#include <sys/wait.h>
#include "message.h"
#include "connections.h"
#include <time.h>
#include <sys/select.h>
#include "config.h"
#include <limits.h>
#include "hash.h"
#include <sys/time.h>
#include <sys/stat.h>
#include "history.h"
#include "Group.h"
#include "stats.h"
#include "AlgoCoda.h"

//macro per controllo chiamate di libreria
#define CHECK_PTR(X, str)	\
    if ((X)==NULL) {		\
	perror(#str);		\
	exit(EXIT_FAILURE);	\
    }
//macro per controllo chiamate di sistema
#define CHECK_SYS(X, str)	\
    if ((X)==-1) {		\
	perror(#str);		\
	return -1;		\
    }
struct statistics chattyStats={0,0,0,0,0,0,0}; //struttura che contiene le statistiche
struct file_parameters configure; //struttura che contiene parametri di configurazione

int pfd[2]; //pipe per comunicazione tra worker e listener
volatile sig_atomic_t t=0;
volatile sig_atomic_t u=0;

/**
 * @function gestore_int
 * @brief  setta la variabile sig_Atomic t quando arriva un determinato segnale di terminazione:sigint,sigquit,sigstop
 * 
 */   
void gestore_int(int sig){t=1;}

/**
 * @function gestore_usr
 * @brief  setta la variabile sig_Atomic u quando arriva un determinato segnale: sigusr1
 * 
 */   
void gestore_usr(int sig){u=1;}

/**
 * @struct user
 * @brief Struttura utenti online
 * @var us nome utente online 
 * @var fd descrittore di connessione
 */
typedef struct user{
  char us[MAX_NAME_LENGTH+1]; 
  long fd;
}on;

on online[MAX_USER];//vettore utenti online
int n_us_online;//numero di utenti online
pthread_mutex_t mtxo=PTHREAD_MUTEX_INITIALIZER;//mutex per accedere al vettore online
pthread_mutex_t mtxsend[MAX_USER]={PTHREAD_MUTEX_INITIALIZER}; //array di mutex corrispondente al vettore utenti online
//per l'invio di un messaggio ad un utente online in mutua esclusione



/**
 * @function InitUserOnline
 * @brief Inizializza il vettore utenti online, fd=-2 inizialmente
 *        
 */
void InitUserOnline(){
  for(int d=0;d<MAX_USER;d++){
    online[d].fd=-2;
  }
  n_us_online=0;
  }

/**
 * @function SendBroadcast
 * @brief Invia un messaggio a tutti gli utenti online
 * 
 * @param req pacchetto contenente il messaggio e relative informazione
 *
 * @return numero di messaggi inviati
 *        
 */
int SendBroadcast(message_t *req){
  int cont=0;
  pthread_mutex_lock(&mtxo);
  for(int d=0;d<MAX_USER;d++){//invio il messaggio a tutti gli utenti online
    long fd1=online[d].fd;
    if(strncmp(online[d].us,req->hdr.sender,MAX_NAME_LENGTH+1)!=0){
      if(fd1>0){
        req->hdr.op=TXT_MESSAGE;
        SendMsg(fd1,req);
        cont++; 
      }
    }
   }
  pthread_mutex_unlock(&mtxo);
  return cont;
}

/**
 * @function IsOnline
 * @brief Controlla se un utente è online
 * 
 * @param receiver nome utente da controllare
 *
 * @return descrittore di connessione se è online
 *         -1 altrimenti
 */

long IsOnline(char *receiver){
  pthread_mutex_lock(&mtxo);//prendo il lock sul vettore
  for(int p=0;p<MAX_USER;p++){//scandisco tutto il vettore in cerca di receiver
    if(online[p].fd!=-2 && strncmp(receiver,online[p].us,MAX_NAME_LENGTH+1)==0){
        pthread_mutex_unlock(&mtxo);
        return online[p].fd;
    } 
  }
  pthread_mutex_unlock(&mtxo);//rilascio il lock
  return -1;
}


/**
 * @function FindPosFree
 * @brief  Trova una posizione libera per inserire un utente nel vettore onlibe
 * 
 * @return p posizione libera nell'array online
 *         -1 altrimenti
 */
int FindPosFree(){
  for(int p=0;p<MAX_USER;p++){
    if(online[p].fd==-2){//trovo e restituisco una posizione libera
      return p;
   }
  }
  return -1;  
}


/**
 * @function FindPosOnline
 * @brief  Trova l'indice nel vettore online di un utente
 * 
 * @param fd descrittore di connessione di un utente
 *
 * @return p indice posizione nell'array online
 *         -1 altrimenti
 */
int FindPosOnline(long fd){
  for(int p=0;p<MAX_USER;p++){
    if(online[p].fd==fd){
      return p;
   }
  }
  return -1;  
}

/**
 * @function InsertUserOnline
 * @brief  Inserisce l'utente nel vettore online
 * 
 * @param nick username utente da inserire
 * @param fd descrittore di connessione
 *
 */
void InsertUserOnline(char *nick,long fd){
  pthread_mutex_lock(&mtxo);//prendo il lock sul vettore
  int p=FindPosFree(); //trovo una posizione libera
  strncpy(online[p].us,nick,MAX_NAME_LENGTH+1); //setto i valori
  online[p].fd=fd;
  n_us_online++;
  pthread_mutex_unlock(&mtxo);//rilascio il lock
}

/**
 * @function DeleteOnline
 * @brief  Elimina l'utente dal vettore online
 * 
 * @param fd descrittore di connessione relativo all'utente da eliminare
 *
 * @return -1 in caso di errore
 *         
 */
void DeleteOnline(long fd){
  pthread_mutex_lock(&mtxo);//prendo il lock sul vettore
  for(int p=0;p<MAX_USER;p++){//scandisco il vettore
    if(online[p].fd!=-2 && fd==online[p].fd){//se lo trovo lo elimino
      online[p].fd=-2;
      n_us_online--;
      pthread_mutex_unlock(&mtxo); //rilascio il lock
      return ;
    } 
  }
  pthread_mutex_unlock(&mtxo);  //rilascio il lock
  return ;
}


/**
 * @function SendList
 * @brief  Invia la lista utenti online
 * 
 * @param fd descrittore di connessione relativo all'utente da eliminare
 * @param req messaggio di richiesta
 *
 * @return 1 userlist inviata
 *         -1 altrimenti
 */
int  SendList(long fd,message_t *req){
  int l=(MAX_NAME_LENGTH+1)*n_us_online;//calcolo la grandezza della userlist
  req->data.hdr.len=l;//grandezza byte da notificare al client per la lettura
  CHECK_SYS(writen(fd,&(req->data.hdr),sizeof(message_data_hdr_t)),Chatty.SendList); //invio l'header contente la dimensione dell'userlist
  for(int i=0;i<MAX_USER;i++){
    if(online[i].fd>-1){
      CHECK_SYS(writen(fd,online[i].us, MAX_NAME_LENGTH+1),Chatty.SendList);//invio un utente alla volta se online
    }  
  }
  return 1;
}


/**
 * @function PrepareList
 * @brief  Prepara l'invio della lista online verso un client
 * 
 * @param fd descrittore di connessione su cui inviare
 * @param req messaggio di richiesta
 *
 */
void PrepareList(long fd,message_t *req){
  pthread_mutex_lock(&mtxo); //prendo la lock sul vettore online
  int pos=FindPosOnline(fd); //estraggo la posizione
  pthread_mutex_lock(&mtxsend[pos]);//prendo la lock su quella cella
  SendList(fd,req); //invio sul fd della cella
  pthread_mutex_unlock(&mtxsend[pos]);//rilascio il lock sul descrittore
  pthread_mutex_unlock(&mtxo); //rilascio il lock sul vettore online
}


/**
 * @function Reply
 * @brief  Prepara l'invio di una risposta in seguito ad una richiesta
 * 
 * @param fd descrittore di connessione su cui inviare
 * @param op messaggio di risposta
 *
 */
void Reply(long fd, op_t op){
  pthread_mutex_lock(&mtxo); //prendo la lock sul vettore online
  int pos=FindPosOnline(fd);//estraggo la cella del descrittore
  pthread_mutex_lock(&mtxsend[pos]);//prendo il lock sulla cella
  pthread_mutex_unlock(&mtxo);//rilascio il lock sul vettore
  SendHeader(fd,op); //invio in mutua esclusione su quel fd
  pthread_mutex_unlock(&mtxsend[pos]);//rilascio il lock su quel fd
}

/**
 * @function PrepareSendMsg
 * @brief  Prepara l'invio di un messaggio ad un utente online
 * 
 * @param fd1 descrittore di connessione su cui inviare
 * @param req messaggio di richesta
 *
 */
void PrepareSendMsg(long fd1,message_t *req){
    pthread_mutex_lock(&mtxo); //prendo il lock sul vettore
    int pos=FindPosOnline(fd1);//trovo la cella del vettore di quel fd
    pthread_mutex_lock(&mtxsend[pos]); //prendo il lock su quella cella
    pthread_mutex_unlock(&mtxo); //rilascio il lock del vettore online
    SendMsg(fd1,req); //invio in mutua esclusione
    pthread_mutex_unlock(&mtxsend[pos]);//rilascio il lock sul descrittore
}

/**
 * @function SignOut
 * @brief  Deregistra un utente dal server oppure cancella un gruppo
 * 
 * @param fd descrittore di connessione 
 * @param req messaggio di richesta
 * @return 1 in ogni caso
 *
 */
int SignOut(long fd,message_t *req){
  int p=DeleteGroup(req); //Controlla se è un gruppi
  if(p==1){Reply(fd,OP_OK);return 1;} //Gruppo->cancellato
  if(p==-1){Reply(fd,OP_NICK_UNKNOWN);return 1;} //È un gruppo ma non può essere cancellato
  //Non è un gruppo
  if(FindUserHash(req->hdr.sender)==1){//controlla se l'utente è registrato
    DeleteUserHash(req->hdr.sender);//elimina l'utente dalla hash
    DeleteUserHistory(req->hdr.sender);//elimina l'utente dalla history
    DeleteOnline(fd);//elimina l'utente dal vettore online
    Reply(fd,OP_OK); //invia feedback positivo
    chattyStats.nusers--;//aggiorna le statistiche
  }
  else{
    Reply(fd,OP_NICK_UNKNOWN);//utente non registrato 
    chattyStats.nerrors++;
  }
  return 1;
}


/**
 * @function SignIn
 * @brief  Registra un utente nel server
 * 
 * @param fd descrittore di connessione 
 * @param req messaggio di richesta
 * @return 1 in ogni caso
 *
 */
int SignIn(long fd,message_t *req){
  if(InsertUserHash(req->hdr.sender)==1){//Controlla se non esite e inserisce un utente
    Reply(fd,OP_NICK_ALREADY); //Utente già esiste
    chattyStats.nerrors++;
    return 1;
  }
  else{
    InsertUserOnline(req->hdr.sender,fd);//Inserisce l'utente nel vettore online
    InsertHistory(req->hdr.sender);//Inserisce l'utente nella history
    Reply(fd,OP_OK);//utente registrato
    chattyStats.nusers++;
  }
  PrepareList(fd,req);//Prepara l'invio lista online
  return 1;
}

/**
 * @function List
 * @brief  Usata in seguito di una richiesta esplicita di invio lista utenti online
 * 
 * @param fd descrittore di connessione 
 * @param req messaggio di richesta
 * @return 1 
 *
 */
int List(long fd,message_t *req){
  Reply(fd,OP_OK);//invio ok
  PrepareList(fd,req);//preparo ad invia la userlist
  return 1;
}


/**
 * @function Connect
 * @brief  Inserisce l'utente nel vettore online
 * 
 * @param fd descrittore di connessione 
 * @param req messaggio di richesta
 * @return -1 in caso di errore
 *          1 successo
 */
int Connect(long fd,message_t *req){
  if(FindUserHash(req->hdr.sender)==1){ //se un utente è registato viene inserito nel vettore online
    InsertUserOnline(req->hdr.sender,fd);
  }
  else{
    Reply(fd,OP_NICK_UNKNOWN);//utente non registrato
    chattyStats.nerrors++;
    return -1;
  }
  Reply(fd,OP_OK);
  PrepareList(fd,req);//prepare l'invio lista utenti online
  return 1;
}


/**
 * @function Broadcast
 * @brief  Inva un messaggio a tutti gli utenti online
 * 
 * @param fd descrittore di connessione 
 * @param req messaggio di richesta che contiene il messaggio e il sender
 * @return -1 in caso di errore
 *          1 successo
 */
int Broadcast(long fd,message_t *req){
  if(req->data.hdr.len>configure.MaxMsgSize){//Controllo la lunghezza del messaggio
    Reply(fd,OP_MSG_TOOLONG);
    chattyStats.nerrors++;
    return -1;
  }
  Reply(fd,OP_OK);
  //salvo i messaggi nella history e dopo provo a inviare a tutti quelli online
  int cont=InsertMsgHistoryBroadcast(req->data.buf,req->hdr.sender,"21");//Mantengo traccia del numero di messaggi inviati ma non consegnati
  chattyStats.nnotdelivered+=cont;
  cont=SendBroadcast(req);//Invio il messaggio a tutti quelli online
  chattyStats.nnotdelivered-=cont;//aggiorno le statistiche
  chattyStats.ndelivered+=cont; 
  return 1;
}



/**
 * @function Clear
 * @brief funzione di comodo per deallocare
 *   
 * @param tmp array di puntatori
 * 
 */
void Clear(char **tmp){
   for(int i=0;i<configure.MaxUserGroup;i++){free(tmp[i]);}
   free(tmp);
}


/**
 * @function UpdateMsgsOnline
 * @brief  Aggiorna le statistiche invio messaggi/file
 *  
 * @param op tipo di messaggio consegnato
 * 
 */
void UpdateMsgsOnline(op_t op){
  if(op==TXT_MESSAGE){//aggiorno i messaggi consegnati e non consegnati
        chattyStats.nnotdelivered-=1;
        chattyStats.ndelivered+=1;
  }
  else{//aggiorno i file consegnati e non consegnati
    chattyStats.nfilenotdelivered-=1;
     chattyStats.nfiledelivered+=1;
  }   
}


/**
 * @function TryGroup
 * @brief  Controlla se il destinatario di un messaggio e  un gruppo e in caso
 *         di risposta affermativa, inserisce il messaggio nella history dei 
 *         partecipanti e prova ad inviarli se online 
 * 
 *  
 * @param req messaggio di richesta che contiene messaggio,sender e receiver
 * @param type tipo di messaggio da inviare
 * 
 * @return  1 successo
 *         -2 gruppo inesistente 
 */

int TryGroup(long fd,message_t *req,op_t type){
  char **tmp;
  CHECK_PTR(tmp=malloc(configure.MaxUserGroup*sizeof(char*)),Chatty.TryGroup.malloc);
  for(int i=0;i<configure.MaxUserGroup;i++){CHECK_PTR(tmp[i]=malloc(MAX_NAME_LENGTH*sizeof(char)),Chatty.TryGroup.tmp.i);}
  int cont=0;
  int trovato=IsGroup(req,tmp,&cont,type);//se il destinatario è un gruppo, invia il messaggio alla history di ogni partecipante
  if(trovato==1){
  for(int i=0;i<cont;i++){//se ho inviato un txt messagge, aggiorno le statistiche dei messaggi
    if(type==TXT_MESSAGE){
      chattyStats.nnotdelivered+=1;
    }
    else{//altrimenti aggiorno le statistiche dei file
      chattyStats.nfilenotdelivered+=1;
    }  
    long fd1=IsOnline(tmp[i]);//controllo se gli utenti del gruppo sono online
    if(fd1>0){//provo a consegnare il messaggio
      req->hdr.op=type;
      PrepareSendMsg(fd1,req);//Preparo l'invio 
      UpdateMsgsOnline(type);//Aggiorno il numero di messagi di testo o file consegnati
    }  
   }
   Reply(fd,OP_OK);  //invio la risposta ok
   Clear(tmp); 
   return 1;
  }
  else if(trovato==-2){//gruppo non esistente
    Reply(fd,OP_NICK_UNKNOWN);  
    Clear(tmp); 
   return -1;
 }
  Clear(tmp);
  return -2;
}

/**
 * @function SendMessage
 * @brief  Inserisce un messaggio nella history del destinatario e prova ad inviare se è online
 * 
 * @param req messaggio di richesta che contiene messaggio,sender e receiver
 * 
 * @return  1 successo
 *         -1 errore 
 */
int SendMessage(long fd,message_t *req){
  if(req->data.hdr.len>configure.MaxMsgSize){//controllo dimensione 
    Reply(fd,OP_MSG_TOOLONG);//errore messaggio troppo lungo
    chattyStats.nerrors++;
    return -1;
  }
  //controllo se è un gruppo
  int p=TryGroup(fd,req,TXT_MESSAGE);
  if(p==1)return 1;
  //receiver
  if(FindUserHash(req->data.hdr.receiver)==1){  
    InsertMsgHistory(req->data.hdr.receiver,req->data.buf,req->hdr.sender,"21");
    chattyStats.nnotdelivered+=1; 
    Reply(fd,OP_OK);  
  }
  else{
    Reply(fd,OP_NICK_UNKNOWN);
    chattyStats.nerrors++;
    return -1;
  }
  //provo a inviare se è online
  long fd1=IsOnline(req->data.hdr.receiver);
  if(fd1>0){
    req->hdr.op=TXT_MESSAGE;
    PrepareSendMsg(fd1,req);
    UpdateMsgsOnline(TXT_MESSAGE); 
  } 
  return 1;
}

/**
 * @function NewFile
 * @brief  Crea un nuovo file
 * 
 * @param req messaggio di richesta che contiene nome file
 * @param data contenuto di un file da salvare
 * 
 * @return  1 successo
 */
int NewFile(message_t *req,message_data_t data){
  int fp; char *bname;//In seguito ad una post txt,il server crea un nuovo file dove salva il contenuto del file inviato dal client
  char *filename=NULL;//in un percorso specificato dal file di configurazione
  CHECK_PTR(filename=calloc(MAX_NAME_LENGTH,sizeof(char)),Chatty.NewFile.filename);
  strncpy(filename,configure.DirName,strlen(configure.DirName));
  CHECK_PTR(bname=basename(req->data.buf),NewFile.basename);//estraggo il nome del file
  strcat(filename,bname);
  CHECK_SYS(fp=open(filename,O_CREAT|O_RDWR|O_TRUNC,0777,"w"),Chatty.NewFile.open.filename);//Do i diritti di esecuzione
  CHECK_SYS(writen(fp,data.buf,data.hdr.len),Chatty.write.file);
  CHECK_SYS(close(fp),Chatty.close.file);
  free(filename);
  return 1;
}  

/**
 * @function PostFile
 * @brief  Invia e salva il contenuto di un file
 * 
 * @param fd descrittore di connessione 
 * @param req messaggio di richesta che contiene nome file, sender e receiver
 * @param data messaggio che contiene il contenuto del file 
 * @return  1 successo
 */
int PostFile(long fd,message_t *req,message_data_t data){
  if(data.hdr.len>configure.MaxFileSize*1024 || req->data.hdr.len>configure.MaxMsgSize){//Controllo dimensione file e dimensione nome file
    Reply(fd,OP_MSG_TOOLONG);//dimensione troppo grande
    chattyStats.nerrors++;
    return 1;
  }   
  if(TryGroup(fd,req,FILE_MESSAGE)==1){//Controllo il receiver è un gruppo e in caso invio
    NewFile(req,data);//salvo il fle nella cartella del server
    return 1;
  }    
  //receiver è una persona
  if(FindUserHash(req->data.hdr.receiver)==1){//se esiste salvo il messaggio nella history
      InsertMsgHistory(req->data.hdr.receiver,req->data.buf,req->hdr.sender,"22");
      chattyStats.nfilenotdelivered+=1;
      Reply(fd,OP_OK);
    }
    else{
      Reply(fd,OP_NICK_UNKNOWN);
      chattyStats.nerrors++;
      return 1;
    }
  NewFile(req,data);
  long fd1=IsOnline(req->data.hdr.receiver);//Controllo se è online e in caso invio messaggio
  if(fd1>0){
    req->hdr.op=FILE_MESSAGE;
    PrepareSendMsg(fd1,req);  
    UpdateMsgsOnline(req->hdr.op);//aggiorno le statistiche
  }
  return 1;
}


/**
 * @function GetFile
 * @brief  Scarica il contenuto di un file
 * 
 * @param fd descrittore di connessione 
 * @param req messaggio di richesta che contiene nome file e utente
 * @return  1 successo
 */
int GetFile(long fd,message_t *req){
  int pos;
  Reply(fd,OP_OK);
  UpdateMsgsOnline(req->hdr.op);//aggiorna statistiche
  char *filename;
  CHECK_PTR(filename=calloc(MAX_NAME_LENGTH,sizeof(char)),Chatty.GetFile.filename);
  strncpy(filename,configure.DirName,strlen(configure.DirName));
  strncat(filename,req->data.buf,strlen(req->data.buf));
  int fp;
  CHECK_SYS(fp=open(filename,O_RDWR,0666),Chatty.GetFile.filename);//apro file da scaricare
  struct stat st;
  stat(filename, &st);
  int size=st.st_size;//prelevo la sua dimensione
  char *buf;
  CHECK_PTR(buf=calloc(size,sizeof(char)),Chatty.GetFile.buf);
  CHECK_SYS(readn(fp,buf,size),Chatty.GetFile);//prelevo il suo contenuto
  req->data.hdr.len=size;//imposto la size per il messaggio di risposta
  free(req->data.buf);
  CHECK_PTR(req->data.buf=calloc(req->data.hdr.len,sizeof(char)),Chatty.GetFile.buf);
  strcpy(req->data.buf,buf);
  pthread_mutex_lock(&mtxo); 
  pos=FindPosOnline(fd);
  pthread_mutex_lock(&mtxsend[pos]); 
  pthread_mutex_unlock(&mtxo);   
  sendData(fd,&(req->data));//invio il contenuto del file in mutua esclusione
  free(buf);free(filename);
  pthread_mutex_unlock(&mtxsend[pos]); 
  return 1;
}


/**
 * @function CreateGroup
 * @brief  Crea un nuovo gruppo
 * 
 * @param fd descrittore di connessione 
 * @param req messaggio di richesta che contiene gruppo e creatore
 * @return  1 successo
 */
int CreateGroup(long fd,message_t *req){
 int p=InsertGroup(req); //crea nuovo gruppo
 if(p==-1){ 
     Reply(fd,OP_NICK_ALREADY); //gruppo già esistente
     return 1;
   }
  if(p==1){
    Reply(fd,OP_OK);
    return 1;
  }
  return 1;
 }

/**
 * @function AddGroup
 * @brief  Aggiunge un utente al gruppo
 * 
 * @param fd descrittore di connessione 
 * @param req messaggio di richesta che contiene utente e gruppo
 * @return  1 successo
 */
int AddGroup(long fd,message_t *req){
  int flag=AddUserGroup(req);
  if(flag==-2){Reply(fd,OP_NICK_UNKNOWN);return 1;}//Utente non registrato
  if(flag==-1){Reply(fd,OP_NICK_ALREADY);return 1;}//Gruppo inesistente
  if(flag==0){Reply(fd,OP_NICK_ALREADY);return 1;}//utente già presente
  Reply(fd,OP_OK);
  return 1;
}

/**
 * @function worker
 * @brief  Preleva un descrittore pronto dalla coda e serve la sua richiesta 
 * 
 */   
static void *worker(){
  int s=0;
  while(1){
    s=0;
    long fd=pop();int n=0; 
    if(fd==-1){Wake();break;}
    message_t *req=calloc(1,sizeof(message_t));     
    n=readHeader(fd,&(req->hdr));//ricevo la richiesta
    if(n<=0){
      DeleteOnline(fd); 
    }
    else{
      switch(req->hdr.op){//analizzo il tipo di richiesta e agisco di conseguenza
        case REGISTER_OP: {
            s=SignIn(fd,req);
        }   break;
       case UNREGISTER_OP: {
         if(readn(fd,&(req->data.hdr),sizeof(message_data_hdr_t))==-1){
           perror("Unregistered: ");
           return (void*)-1;
         }
         SignOut(fd,req);
        }   break;
       case CONNECT_OP: {
          s=Connect(fd,req);
        }break;
        case POSTTXT_OP: {
          readData(fd,&(req->data));
          s=SendMessage(fd,req);
          free(req->data.buf);
        }break;
        case POSTTXTALL_OP: {
          readData(fd,&(req->data));
          s=Broadcast(fd,req);free(req->data.buf);
        }break;
        case POSTFILE_OP: {
          readData(fd,&(req->data));
          message_data_t data;
          readData(fd,&(data));
          s=PostFile(fd,req,data);
          free(req->data.buf);
          free(data.buf);
        }break;
        case GETFILE_OP: {
          readData(fd,&(req->data));
          s=GetFile(fd,req);
          free(req->data.buf);
        }break;
        case CREATEGROUP_OP: {
          if(readn(fd,&(req->data.hdr),sizeof(message_data_hdr_t))==-1){
            perror("CreateGroup: ");
            return (void*)-1;
          }
          s=CreateGroup(fd,req);
        }break;
        case ADDGROUP_OP: {
          readn(fd,&(req->data.hdr),sizeof(message_data_hdr_t));
          s=AddGroup(fd,req);
        }break;
        case DELGROUP_OP: {
          if(readn(fd,&(req->data.hdr),sizeof(message_data_hdr_t))==-1){
   	     perror("DeleteGroup: ");
            return (void*)-1;
          }
          int flag=DelUserGroup(fd,req);
          if(flag==-1){Reply(fd,OP_NICK_UNKNOWN);free(req);return (void*)1;}
          Reply(fd,OP_OK);s=1;
        }break;
        case GETPREVMSGS_OP: {
          pthread_mutex_lock(&mtxo); //lock sul vettore online
          int pos=FindPosOnline(fd); //trovo la cella
          pthread_mutex_lock(&mtxsend[pos]); //lock sulla cella,ovvero sul descrittore
          pthread_mutex_unlock(&mtxo); //rilascio il lock sul vettore online  
          int mex=0;int file=0;
          s=GetHistory(fd,req,&mex,&file); //invio al descrittore tutta la sua history in mutua esclusione
          pthread_mutex_unlock(&mtxsend[pos]); //rilascio il lock sul descrittore
          chattyStats.nfilenotdelivered-=file;
          chattyStats.nfiledelivered+=file;
          chattyStats.nnotdelivered-=mex;
          chattyStats.ndelivered+=mex;
        }break;
        case USRLIST_OP: {
          s=List(fd,req);
         }break;
        default: {
          fprintf(stderr, "ERRORE: risposta non validaWorker\n");
          return (void*)NULL;
        }
      }
      if(s>=0){
        if(writen(pfd[1],&fd,sizeof(long))==-1){
          perror("Error Writing pipe: ");
        }//scrivo nella pipe il descrittore una volta servito
      }
   }   
   free(req);
 }
return (void*) 1;
}


/**
 * @function aggiorna
 * @brief  aggiorna il descrittore max
 * 
 * @param set set di descrittori registrati nella select 
 * @param max massimo attuale
 * @return  max
 */
int aggiorna(fd_set *set,int max){
  int fd;int fd_num=0;
  for(fd=0;fd<=max;fd++){
    if(FD_ISSET(fd,set)){
      if(fd>fd_num)fd_num=fd;  
    }
  } 
  return fd_num;
}

/**
 * @function PrintStat
 * @brief  In seguito all'arrivo del segnale, salva le statiscyiche in file specifico
 * 
 */
int PrintStat(){
  pthread_mutex_lock(&mtxo);
  chattyStats.nonline=n_us_online-1;
  pthread_mutex_unlock(&mtxo);
  FILE *fp;
  CHECK_PTR(fp=fopen(configure.StatFileName,"w"),Chatty.opening.chattystat);
  printStats(fp);//scrivo le statistiche in un file indicato nel file di configurazione
  fclose(fp);
  return 1;
}

/**
 * @function Gestisci
 * @brief  In seguito all'arrivo del segnale di terminazione, inserisce -1 in coda 
 *         dando inizio al protocollo di terminazione 
 */
void Gestisci(){
  if(t==1){
    Push(-1);//immetto il messaggio di terminazione
  }
  return ;
}


/**
 * @function listener
 * @brief thread che ha il compito di ascoltare nuove richieste di connessione 
 *        e accettarle se possibile.
 * 
 */
static void *listener(void* a){
  int fd_pipe=pipe(pfd);//creo una pipe per la comunicazione all'indietro tra worker-listener
  //il worker una volta soddisfatta la singola richiesta, scrivere nella pipe l'fd.
  //il listener leggerà dalla pipe l'fd e lo riaggiungerà nel set.
  if(fd_pipe==-1){
    perror("Error pipe: ");
  } 
  long fd_server=0;long fd_num=0;
  fd_set set,rdset;
  long fd_c=0;long fd=0;
  struct sockaddr_un sa;//imposto i parametri del socket
  strncpy(sa.sun_path,configure.UnixPath,UNIX_PATH_MAX);
  sa.sun_family=AF_UNIX;
  fd_server=socket(AF_UNIX,SOCK_STREAM,0);
  if(fd_server==-1){
    perror("Error socket:");
    exit(errno);
  }
  if(bind(fd_server,(struct sockaddr *)&sa,sizeof(sa))==-1){
    perror("Error bind:");
    exit(errno);
  }
  if(listen(fd_server,configure.MaxConnections)==-1){
    perror("Error listen: ");
    exit(errno);
  }
  if(fd_server>fd_num)fd_num=fd_server;
  FD_ZERO(&set);
  struct timeval tv;//imposto il timer della select.
  tv.tv_sec=2;tv.tv_usec=1000;
  FD_SET(pfd[0],&set);//inserisco nel set il descrittore della pipe in lettura
  FD_SET(fd_server,&set);//inserisco nella set il descrittore del server su cui viene fatta l'accept
  while(1){
    FD_ZERO(&rdset);//non posso perdere la maschera originaria
    rdset=set;//faccio una copia che passo alla select
    int ret=select(fd_num+1,&rdset,NULL,NULL,&tv);
    if(ret==-1){perror("Error select: ");break;}
    else if(ret==0){ //è scaduto il timer e posso andare a controllare se è arrivato qualche segnale
      if(u==1){
        u=0;
        PrintStat();
      }
      if(t==1){
        Gestisci();//gestione terminazione
        close(fd_server);
        break;
      }
    }
    else{
      for(fd=0;fd<=fd_num;fd++){
        if(FD_ISSET(fd,&rdset)){
          if(fd==fd_server){//se è pronto
            fd_c=accept(fd_server,NULL,0);//ho una nuova connessione
            FD_SET(fd_c,&set);//registro fd della nuova connessione nel set
            if(fd_c>fd_num)fd_num=fd_c;
          }
        else if(fd==pfd[0]){//se un worker ha scritto in pipe
        if(readn(pfd[0],&fd_c,sizeof(long))==-1){//leggo dalla pipe
            perror("Reading pipe: ");
        }
          FD_SET(fd_c,&set);//registro l'fd
          if(fd_c>fd_num)fd_num=fd_c;
        }
        else{
          Push(fd);//se un fd di un client qualsiasi è pronto lo inserisco nella coda per essere servito dai thread
          FD_CLR(fd,&set);//lo rimuovo dal set
          fd_num=aggiorna(&set,fd_num);//aggiorno fd max
          }
        }
      }
    }
  }     
  return (void*) 2;
}


/**
 * @function Configure
 * @brief Parser file di configurazione.
 * Ho preso spunto da un parser in internet(https://www.unix.com/programming/168018-parsing-text-file-using-c.html) e personalizzato.
 * @param file file di configurazione da dove estrarre i parametri
 * @return 1 successo
 */
int Configure(char *file){
  char buf[MAX_LINE];FILE *fp;
  CHECK_PTR(fp=fopen(file, "r"),Chatty:opening Data/chatty.conf);
  while(fgets(buf,MAX_LINE, fp)){
    char *str=buf;
    char name[MAX_LINE],val[MAX_LINE];
    while(isspace(*str)) str++;
    if(((*str) == '\0') || ((*str) == '#')){}
    else if(sscanf(str, "%[^= ] = %[^ \n]", name, val) == 2) {
      if(strncmp(name,"MaxConnections",strlen("MaxConnections"))==0){configure.MaxConnections=atoi(val);}
      if(strncmp(name,"DirName",strlen("DirName")) ==0){strcpy(configure.DirName,val);strcat(configure.DirName,"/");}
      if(strncmp(name,"MaxFileSize",strlen("MaxFileSize"))==0) configure.MaxFileSize=atoi(val);
      if(strncmp(name,"StatFileName",strlen("StatFileName"))==0) strcpy(configure.StatFileName,val);
      if(strncmp(name,"MaxHistMsgs",strlen("MaxHistMsgs"))==0) configure.MaxHistMsgs=atoi(val);
      if(strncmp(name,"MaxMsgSize",strlen("MaxMsgSize"))==0) configure.MaxMsgSize=atoi(val);
      if(strncmp(name,"ThreadsInPool",strlen("ThreadsInPool"))==0) configure.ThreadsInPool=atoi(val);
      if(strncmp(name,"UnixPath",strlen("UnixPath"))==0) strcpy(configure.UnixPath,val);
      if(strncmp(name,"MaxUserGroup",strlen("MaxUserGroup"))==0) configure.MaxUserGroup=atoi(val);
    }              
  }  
  fclose(fp);
  return 1;
}

/**
 * @function Signal
 * @brief Installazione signal handler
 */
void Signal(){
  struct sigaction s; 
  memset(&s,0,sizeof(s));
  s.sa_handler=gestore_int;
  if(sigaction(SIGINT,&s,NULL)==-1)
    perror("sigaction SIGINT"); 
  if(sigaction(SIGQUIT,&s,NULL)==-1)
    perror("sigaction SIGQUIT"); 
  if(sigaction(SIGTERM,&s,NULL)==-1)
    perror("sigaction SIGTERM");
  s.sa_handler=gestore_usr;
  if(sigaction(SIGUSR1,&s,NULL)==-1)
    perror("sigaction SIGUSR1");
  s.sa_handler=SIG_IGN; 
  if(sigaction(SIGPIPE,&s,NULL)==-1)//ignoro
    perror("sigaction SIGPIPE"); 
}


/**
 * @function main
 * @brief  funzione main dove vengono inizializzate tutte le strutture del server
 *         e dove viene creato e inizializzato il threadpool e listener
 *         Prima di terminare qualsiasi struttura o risorsa viene deallocata
 * 
 */
int main(int argc,char *argv[]){
  const char optstring[] = "f:";
  int optc=0;char *file=NULL;
  while ((optc = getopt(argc, argv,optstring)) != -1) {
    switch (optc) {
      case 'f': {file=optarg;}break;
      default: { printf("Error Parsing"); return -1;}
    }
  }
  Configure(file);
  InitGroup();
  InitHistory();
  Signal();
  InitUserOnline();
  InitHash();
  pthread_t *ptr;
  CHECK_PTR(ptr=(pthread_t *)malloc((configure.ThreadsInPool+1)*sizeof(pthread_t)),Chatty.malloc.ThreadPool);
  int *status;
  CHECK_PTR(status=(int *)malloc((configure.ThreadsInPool+1)*sizeof(int)),Chatty.malloc.status);  
  pthread_t pl;int sl;
  if(pthread_create(&pl,NULL,listener,NULL)!=0){//creo il thread listener
    perror("Error Listener: ");
    exit(EXIT_FAILURE);
  }
  for(int i=0;i<configure.ThreadsInPool;i++){//creo i thread del worker
    if(pthread_create(&ptr[i],NULL,worker,NULL)!=0){
      perror("Error ThreadInPool: ");
      exit(EXIT_FAILURE);
    }
  }
  //il thread main a questo punto aspetta la terminazione del server
  pthread_join(pl,(void*)&sl);//aspetto la terminazione del listener
  for(int i=0;i<configure.ThreadsInPool;i++){
    pthread_join(ptr[i],(void*)&status[i]);//aspetto che tutti i thread worker siano terminati
  } 
  //una volta che i thread sono terminati e tutte le loro risorse sono state rilasciate
  //libera tutte le altre strutture usate dal server
  FreeGroup();
  Destroy();
  DestroyList();
  FreeHistory();
  free(ptr);
  free(status);
  close(pfd[0]);
  close(pfd[1]);
  unlink(configure.UnixPath);
  exit(0);
}
