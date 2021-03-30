#ifndef _EX2_H_
#define _EX2_H_

#include <mqueue.h> // -lrt (gcc ...)
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>

// include gabi files
# include <ctype.h>
# include <mta_rand.h>
# include <mta_crypt.h>

#define bool int
#define TRUE 1
#define FALSE 0
#define MAX_BUFFER 500
#define PASS_LEN 8
#define MQ_NAME_MAX_LEN 17 // "/decrepter_%d_q" 
#define DECRYPTER_MQ "/decrypter_%d_q"
#define MQ_MAX_MSG_SIZE 60 // maximum messages in the queue
#define MQ_MAX_MSG 50 // number of messages in the queue
#define MQ_SERVER_ID "/mq_server" // hardcoded name of the server MQ 
#define MQ_MAX_DATA 256
#define ClientFileName "./client.out"
#define ServerFileName "./server.out"

//macro 
#define assert_if(errnum) if(errnum < 0){printf("Error in line #%u: %m\n",__LINE__);exit(EXIT_FAILURE);}

typedef enum{
    CONNECTION = 1,
    DISCONNECT,
    GUESSED_PASS,
	ENCRYPTED_PASS
}MESSAGE_TYPE;

typedef struct msg{
    MESSAGE_TYPE type;
    char data[];
}MSG_T;

typedef struct encrypted_pass_message {
    unsigned int encrypted_pass_len;
   	char encrypted_pass[MQ_MAX_DATA];
}ENCRYPTED_MSG;

typedef struct connect_msg_data {
	int decrypterID;
	pid_t clientPid;	
	char my_mq_identifier[MQ_NAME_MAX_LEN];
}CONNECT_MSG;

typedef struct disconnect_msg_data {
	int decrypterID;
    char my_mq_identifier[MQ_NAME_MAX_LEN];
}DISCONNECT_PASS_MSG;

typedef struct guesed_pass_msg_data {
	int passLen;
	int decrypterID;
	int iteration;
	char decryptedPassword[MQ_MAX_DATA];
}GUESED_PASS_MSG;

typedef struct _queueNode{
	char* mqName;
	pid_t clientPid;
	struct _queueNode* next;
}queueNode;

typedef struct _queueList{
	queueNode* head;
	queueNode* tail;
	int size;
}queueList;

// password methods 
void printNewPasswordGenerated(char* i_GeneratedPassword, unsigned int i_PassLen, char* i_Key, unsigned int i_KeyLen, unsigned int i_EncryptedLength);
bool isPrintable(char* i_GeneratedPassword, int i_PassLen);
char* generateAndEncryptPassword();
bool cmpPass(char* i_DecryptedPassword, unsigned int i_PassLen, char* i_GeneratedPassword);
void sendEncyptedPassToClient(queueNode* i_CurrNode);

// list methods 
void initList();
void addToTail(queueNode* i_Node);
void deleteNode(char* i_QueueName);
void deleteHead(queueNode* i_Curr);
void deleteTail(queueNode* i_Prev, queueNode* i_Curr);
void deleteInner(queueNode* i_Prev, queueNode* i_Curr);
queueNode* createNode(char* i_QueueName, pid_t i_ClientPid);

//Message Queue methods
void cleanQueue(mqd_t* i_serverMq);
void sendCorrectAnswerToDecrypter(MSG_T* i_Msg);
void updateQueuesNewPass();
void makeServerQueue(mqd_t* i_Server_q);
void readFromServerMQ(mqd_t* i_serverMq);
char* makeClientQueue(mqd_t* i_Client_q, unsigned int i_Decrypter_num);
void sendToServerQueue(mqd_t i_ServerQueueID, char* i_DecryptedPass, unsigned int i_DecryptedPassLen ,unsigned int i_DecrypterNum,unsigned int i_Iterations);

//print methods
void connectionPrinting(CONNECT_MSG* i_Data);
void disconnectionPrinting(DISCONNECT_PASS_MSG* i_Data);
void beforeConnectionPrinting(CONNECT_MSG* i_Data);
void afterConnectionPrinting(CONNECT_MSG* i_Data);  
void printclient(GUESED_PASS_MSG* i_Data, char* i_EncryptPassword);

//connect/disconnect
void connectMethod(CONNECT_MSG* i_Data, bool i_IsNewEncryptedPass);
void disconnectMethod(DISCONNECT_PASS_MSG* i_Data);
void sendDisconnectionRequest(unsigned int i_ID, mqd_t clientQueueID, char* i_DecrypterQueueName, mqd_t i_ServerQueueID);
void sendConnectionRequest(unsigned int i_ID, char* i_DecrypterQueueName, mqd_t i_ServerQueueID);

//other methods
void guessedMethod(MSG_T* i_Msg, mqd_t* i_serverMq);
void startDecrypting(mqd_t i_ClientQueueID, mqd_t i_ServerQueueID,unsigned int i_DecrypterNum,int i_CounterLoop,bool runForever);

//input Validation 
void checkInputValidation(int argc,char** argv);
bool isNumber(char* number);

#endif

