#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/un.h>
#include<unistd.h>
#include<errno.h>

/**
 * @file  connections.c
 * @author Russo Alessio 545373
 *      Si dichiara che il contenuto di questo file e' in ogni sua parte opera  
 *      originale dell'autore.
 * @brief Contiene le implementazioni delle funzioni la comunicazione
 *        tra i clients ed il server.
 *        L'invio della parte del pacchetto che contiene i dati si divide in 2 fasi:
 *        1) invio dell'header di data che contiene la size del contenuto da leggere/scrivere(packet->data.hdr)
 *        2) invio del contenuto (packet->data.buf)
 */


#include<sys/stat.h>

#include<fcntl.h>

#ifndef CONNECTIONS_H_
#define CONNECTIONS_H_
#include "connections.h"

#define MAX_RETRIES     10
#define MAX_SLEEPING     3
#if !defined(UNIX_PATH_MAX)
#define UNIX_PATH_MAX  64
#endif

#include "config.h"
#include "message.h"

//macro per controllarel'esito delle chiamate di libreria
#define CHECK_PTR_CONN(X, str)	\
    if ((X)==NULL) {		\
	perror(#str);		\
	exit(EXIT_FAILURE);	\
    }

//macro controllo chiamate di sistema
#define SYSCALL(r,c,e) \
    if((r=c)==-1) {return -1; } \


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
int openConnection(char *path,unsigned int ntimes,unsigned int secs){
  long fd_client=0; 
  struct sockaddr_un sa;
  strncpy(sa.sun_path,path,UNIX_PATH_MAX);
  sa.sun_family=AF_UNIX;//comunicazione processi nella stessa macchina
  int i=0;
  fd_client=socket(AF_UNIX,SOCK_STREAM,0);
  if(fd_client<0){perror("Error socket: ");return -1;}
  while(connect(fd_client,(struct sockaddr*)&sa,sizeof(sa))==-1 && i<ntimes){//10 tentativi di connessione
    if(errno==ENOENT){printf("In attesa di connesione..\n");i++;sleep(secs);}
    else return -1;
  }
  if(ntimes==i)return -1;
  return fd_client;
}

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


int writen(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
	if ((r=write((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;  
        left    -= r;
	bufptr  += r;
    }
    return size;
}



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

int readn(long fd, void *buf, size_t size) {
    size_t left = size;
    int r;
    char *bufptr = (char *)buf;
    while(left>0) {
	if ((r=read((int)fd ,bufptr,left)) == -1) {
	    if (errno == EINTR) continue;
	    return -1;
	}
	if (r == 0) return 0;   // gestione chiusura socket
        left    -= r;
	bufptr  += r;
    }
    return size;
}

/**
 * @function sendData
 * @brief Invia il body del messaggio al server
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int  sendData(long fd, message_data_t *msg){
  int r;
  SYSCALL(r,writen(fd,&(msg->hdr),sizeof(message_data_hdr_t)),"Connections.sendData.writen");//invio l'header dei dati (packet->data.hdr)
  SYSCALL(r,writen(fd,msg->buf,msg->hdr.len),"Connections.sendData.writen");//invio il contenuto del messaggio(packet->data.buf)
  return r;
}

/**
 * @function sendRequest
 * @brief Invia un messaggio di richiesta al server 
 *
 * @param fd     descrittore della connessione
 * @param msg    puntatore al messaggio da inviare
 *
 * @return <=0 se c'e' stato un errore
 */
int sendRequest(long fd, message_t *msg){
  int r;
  SYSCALL(r,writen(fd,&(msg->hdr),sizeof(message_hdr_t)),"Connections.SendRequest.writen");//invio l'header del messaggio
  if(msg->data.hdr.len!=0){//se il buf del messaggio è pieno, invio anche la parte dei dati
    sendData(fd,&(msg->data));
  }
  //nel caso in cui il buf del messaggio è vuoto, ma sono presenti altre informazioni nel pacchetto invio l'header dei dati
  //questo caso si presenta nelle richieste dei gruppi
  if(msg->hdr.op==CREATEGROUP_OP){
     SYSCALL(r,writen(fd,&(msg->data.hdr),sizeof(message_data_hdr_t)),"Connections.SendRequest.writen");    
  }
  else  if(msg->hdr.op==ADDGROUP_OP){
     SYSCALL(r,writen(fd,&(msg->data.hdr),sizeof(message_data_hdr_t)),"Connections.SendRequest.writen");    
  }
  else  if(msg->hdr.op==DELGROUP_OP){
     SYSCALL(r,writen(fd,&(msg->data.hdr),sizeof(message_data_hdr_t)),"Connections.SendRequest.writen");    
  }
  if(msg->hdr.op==UNREGISTER_OP){
     SYSCALL(r,writen(fd,&(msg->data.hdr),sizeof(message_data_hdr_t)),"Connections.SendRequest.writen");    
  }
  return 1;
}

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
int readHeader(long connfd, message_hdr_t *hdr){
  int r=readn(connfd,&(hdr->op),sizeof(message_hdr_t));
  return r;
}

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

int readData(long fd, message_data_t *data){
  int r;
  SYSCALL(r,readn(fd,&(data->hdr),sizeof(message_data_hdr_t)),"Connections.readData.readn");//leggo prima l'header dei dati (packet->data.hdr)
  if(data->hdr.len==0)return 1;
  //dalla readn precedente so quanti byte devo leggere
  CHECK_PTR_CONN(data->buf=calloc(data->hdr.len,sizeof(char)),Connections.readData);//alloca con la dimensione esatta 
  SYSCALL(r,readn(fd,data->buf,data->hdr.len),"Connections.readData.readn");//(packet->data.buf)
  return r; 
}


/**
 * @function readMsg
 * @brief Legge l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param msg   puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 *         (se <0 errno deve essere settato, se == 0 connessione chiusa) 
 */

int readMsg(long fd, message_t *msg){
  int r;
  SYSCALL(r,readn(fd,&(msg->hdr),sizeof(message_hdr_t)),"Connections.readMsg.readn");//legge l'header del pacchetto (packet->hdr)
  SYSCALL(r,readn(fd,&(msg->data.hdr),sizeof(message_data_hdr_t)),"Connections.readMsg.readn");//legge l'header dei dati (packet->data.hdr)
  if(msg->data.hdr.len==0)return 1;
  CHECK_PTR_CONN(msg->data.buf=malloc(msg->data.hdr.len*sizeof(char)),Connections.readMsg);
  SYSCALL(r,readn(fd,msg->data.buf,msg->data.hdr.len),"Connections.readMsg.readn");//legge il contenuto (packet->data.buf)
  return r;
}

/**
 * @function SendMsg
 * @brief Invia l'intero messaggio
 *
 * @param fd     descrittore della connessione
 * @param req  puntatore al messaggio
 *
 * @return <=0 se c'e' stato un errore
 */

int SendMsg(long fd,message_t *req){
  int r;
  SYSCALL(r,writen(fd,&(req->hdr),sizeof(message_hdr_t)),"Connections.SendMsg.writen1"); //invio l'header del pacchetto(packet->hdr)
  SYSCALL(r, writen(fd,&(req->data.hdr),sizeof(message_data_hdr_t)),"Connections.SendMsg.writen2"); //invio l'header dei dati(packet->data.hdr)
  SYSCALL(r,writen(fd,req->data.buf,req->data.hdr.len),"Connections.SendMsg.writen3");//invio il contenuto del messaggio(packet->data.buf)
  return 1;
} 


/**
 * @function SendHeader
 * @brief Invia l'header messaggio con l'esito dell'operazione
 *
 * @param fd     descrittore della connessione
 * @param op     esito richiesta
 *
 */
int SendHeader(long fd, op_t op){
  int r;
  message_hdr_t * reply;
  CHECK_PTR_CONN(reply=calloc(1,sizeof(message_hdr_t)),Connection.SendHeader);
  reply->op=op; //setto l'esito dell'operazione
  SYSCALL(r,writen(fd,reply,sizeof(message_hdr_t)),"Connections.SendHeader.writen"); //invio l'header (packet->hdr)
  free(reply);
  return 1;
}




#endif
