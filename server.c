//server
#include "include.h"

//global variable
unsigned int g_LenOfEncryptedPassword;
queueList g_QueuesList;
char* g_OriginPassword;
char* g_EncryptedPassword; 

int main(int argc, char* argv[])
{
	mqd_t server_q;
	makeServerQueue(&server_q);
	g_EncryptedPassword= (char*)malloc(sizeof(char));
	g_OriginPassword = (char*)malloc(sizeof(char)* PASS_LEN);
	initList();
	g_OriginPassword = generateAndEncryptPassword();
	updateQueuesNewPass();
    readFromServerMQ(&server_q);
	
	return 0;
}

void readFromServerMQ(mqd_t* i_serverMq)
{
    MSG_T* newMessage = malloc(MQ_MAX_MSG_SIZE);
	struct mq_attr ServerAttr = {0};
	int status; 

    for(;;)
    {
		status= mq_receive(*i_serverMq,(char*) newMessage, MQ_MAX_MSG_SIZE,0);
		assert_if(status);
	
		switch(newMessage->type)
		{
			case CONNECTION: 
				connectMethod((CONNECT_MSG*)newMessage->data, TRUE);
			break;
			case DISCONNECT: 
				disconnectMethod((DISCONNECT_PASS_MSG*)newMessage->data);
			break;
			case GUESSED_PASS: 
				guessedMethod(newMessage, i_serverMq);
			break;
		}
    }
}

void connectMethod(CONNECT_MSG* i_Data, bool i_IsNewEncryptedPass)
{
	char mqName[MQ_NAME_MAX_LEN];
	
	sprintf(mqName, DECRYPTER_MQ,i_Data->decrypterID);
	queueNode* newNode = createNode(mqName,i_Data->clientPid);
	addToTail(newNode);
	connectionPrinting(i_Data);

	if(i_IsNewEncryptedPass)
	{
		sendEncyptedPassToClient(newNode);
	}
}

void disconnectMethod(DISCONNECT_PASS_MSG* i_Data)
{
	char* mqName = i_Data->my_mq_identifier;
	
	disconnectionPrinting(i_Data); 
	mq_unlink(mqName);
	deleteNode(mqName);
}

void guessedMethod(MSG_T* i_Msg, mqd_t* i_serverMq)
{
	if(cmpPass(((GUESED_PASS_MSG*)(i_Msg->data))->decryptedPassword, ((GUESED_PASS_MSG*)(i_Msg->data))->passLen, g_OriginPassword))
	{
		printf("[SERVER]         [OK]   Password decrypted successfully by decrypter #%d, recieved %s\n", ((GUESED_PASS_MSG*)(i_Msg->data))->decrypterID, ((GUESED_PASS_MSG*)(i_Msg->data))->decryptedPassword);
		sendCorrectAnswerToDecrypter(i_Msg);
		cleanQueue(i_serverMq);
		g_OriginPassword = generateAndEncryptPassword();
		updateQueuesNewPass();
	}
}

void cleanQueue(mqd_t* i_serverMq)
{
	MSG_T* newMessage = malloc(MQ_MAX_MSG_SIZE);
	struct mq_attr ServerAttr = {0};
	int status; 
	
	mq_getattr(*i_serverMq, &ServerAttr);

	while(ServerAttr.mq_curmsgs > 0)
	{
		status = mq_receive(*i_serverMq,(char*) newMessage, MQ_MAX_MSG_SIZE,0);
   		assert_if(status);
	
		switch(newMessage->type)
		{
			case CONNECTION: 
				connectMethod((CONNECT_MSG*)newMessage->data,FALSE);
			break;
			case DISCONNECT: 
				disconnectMethod((DISCONNECT_PASS_MSG*)newMessage->data);
			break;
			case GUESSED_PASS:  // ignore all the passwords in the mq (that came after good guessed password)
			break;
		}
		
		mq_getattr(*i_serverMq, &ServerAttr);
	}
	
	free(newMessage);
}


void sendCorrectAnswerToDecrypter(MSG_T* i_Msg)
{
	char queueName[MQ_NAME_MAX_LEN];
	sprintf(queueName, DECRYPTER_MQ, ((GUESED_PASS_MSG*)(i_Msg->data))->decrypterID);
	mqd_t clientMqId;
	int status;

	clientMqId = mq_open(queueName, O_WRONLY);
	
	if(clientMqId == (mqd_t)-1 && errno == EAGAIN)
	{
		printf("client MQ is full\n");
	} 
	else
	{
		assert_if(clientMqId)
	}

	status = mq_send(clientMqId,(char*) i_Msg , MQ_MAX_MSG_SIZE, 0);
	assert_if(status)
	mq_close(clientMqId);
}

void sendEncyptedPassToClient(queueNode* i_CurrNode)
{	
	int killStatus = kill(i_CurrNode->clientPid, 0);	
	if(errno == ESRCH && killStatus == -1)
	{
		deleteNode(i_CurrNode->mqName);
	}
	else
	{
		MSG_T* message = malloc(sizeof(MSG_T) + sizeof(ENCRYPTED_MSG)); 
		mqd_t clientMqId;
		int status;

		message->type = ENCRYPTED_PASS;
	
		clientMqId = mq_open(i_CurrNode->mqName, O_WRONLY);
		
		if(clientMqId == -1 && errno == EAGAIN)
		{
			printf("client MQ is full\n");
		} 
		if(clientMqId < 0)
		{	
			assert_if(clientMqId)
		}

		memcpy(((ENCRYPTED_MSG*)(message->data))->encrypted_pass ,g_EncryptedPassword, g_LenOfEncryptedPassword);
		((ENCRYPTED_MSG*)(message->data))->encrypted_pass_len = g_LenOfEncryptedPassword;
		status = mq_send(clientMqId,(char*) message , MQ_MAX_MSG_SIZE, 0); 
		assert_if(status)
		mq_close(clientMqId);
		free(message);
	}
}
  
void makeServerQueue(mqd_t* i_Server_q)
{
	struct mq_attr attr_server = {0};
	
	attr_server.mq_maxmsg = MQ_MAX_MSG;
	attr_server.mq_msgsize = MQ_MAX_MSG_SIZE;
	mq_unlink(MQ_SERVER_ID);
	mq_close(*i_Server_q); 
	*i_Server_q = mq_open(MQ_SERVER_ID, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG, &attr_server); 
	
	if(*i_Server_q == -1 && errno == EAGAIN)
	{
		printf("client MQ is full\n");
	} 
	
	if(*i_Server_q < 0)
	{	
		assert_if(*i_Server_q)
	}	
}


void initList()
{
	g_QueuesList.head = NULL;
	g_QueuesList.tail = NULL;
	g_QueuesList.size = 0;
}

void addToTail(queueNode* i_Node)
{
	if(g_QueuesList.head == NULL)
	{
		g_QueuesList.head = i_Node;
		g_QueuesList.tail = i_Node;
	}
	else
	{
		g_QueuesList.tail->next = i_Node;
		g_QueuesList.tail = i_Node;
	}

	g_QueuesList.size++; 
}

void deleteNode(char* i_QueueName)
{
	queueNode* curr = g_QueuesList.head;
	bool keepRunning = TRUE;
	queueNode* prev = g_QueuesList.head;
	
	while( curr != NULL && keepRunning)
	{		
		if(strcmp(curr->mqName, i_QueueName) == 0)
		{
			keepRunning = FALSE;
		}
		
		if(keepRunning == TRUE)
		{
			prev = curr;
			curr = curr->next;
		}
	}

	if(keepRunning == FALSE)
	{
		g_QueuesList.size--;

		if(curr == g_QueuesList.head) 
		{
			deleteHead(curr);
		}
		else if(curr == g_QueuesList.tail) 
		{
			deleteTail(prev, curr);
		}
		else
		{
			deleteInner(prev, curr);
		}
	}

}

void deleteHead(queueNode* i_Curr)
{
	if(g_QueuesList.head->next != NULL)
	{
		g_QueuesList.head = g_QueuesList.head->next;
	}
	else
	{
		g_QueuesList.head = g_QueuesList.tail = NULL;
	}
	
	free(i_Curr);
}

void deleteTail(queueNode* i_Prev, queueNode* i_Curr)
{
	if(g_QueuesList.head != i_Curr)
	{
		g_QueuesList.tail = i_Prev;
		i_Prev->next = NULL;
	}
	else
	{
		g_QueuesList.head = g_QueuesList.tail = NULL;
	}
	
	free(i_Curr);
}

void deleteInner(queueNode* i_Prev, queueNode* i_Curr)
{
	i_Prev->next = i_Curr->next;
	free(i_Curr);
}

queueNode* createNode(char* i_QueueName, pid_t i_ClientPid)
{
	queueNode* node = (queueNode*)malloc(sizeof(queueNode));
	node->mqName = (char*)malloc(sizeof(char)*strlen(i_QueueName));
	strcpy(node->mqName, i_QueueName);
	node->clientPid = i_ClientPid;
	
	return node;
}

void printNewPasswordGenerated(char* i_GeneratedPassword, unsigned int i_PassLen, char* i_Key, unsigned int i_KeyLen, unsigned int i_EncryptedLength)
{
	char tPass[i_PassLen+1];
	char tKey[i_KeyLen+1];
	char c;
	int i;
	
	for(i = 0; i < i_PassLen; i++)
	{
		c = i_GeneratedPassword[i];
		tPass[i] = c;
	}
	
	tPass[i] = '\0';
	
	for(i = 0; i < i_KeyLen; i++)
	{
		c = i_Key[i];
		tKey[i] = c;
	}
	
	tKey[i] = '\0';
	
	printf("[SERVER]         [INFO] New password generated: %s, key: %s, After encryption: %s \n", tPass, tKey, g_EncryptedPassword);
}


char* generateAndEncryptPassword()
{
	char* generatedPassword = (char*)malloc(sizeof(char)* PASS_LEN);
	char passwordBuffer[MAX_BUFFER];
	MTA_CRYPT_RET_STATUS status;
	unsigned int keyLength = PASS_LEN/8;
	char* key = (char*)calloc(keyLength, sizeof(char));
	char currChar;
	int i=0; 
	
	while(i < PASS_LEN)
	{
		currChar = MTA_get_rand_char();
		
		if(isprint(currChar))
		{
			generatedPassword[i] = currChar;
			i++;
		}
	}

	MTA_get_rand_data(key, keyLength);
	status = MTA_encrypt(key, keyLength, generatedPassword, PASS_LEN, passwordBuffer, &g_LenOfEncryptedPassword); // encrypts it using a radomized key
	assert_if(status)
	
	free(g_EncryptedPassword);
	g_EncryptedPassword = (char*)malloc(sizeof(char)*g_LenOfEncryptedPassword);
	memcpy(g_EncryptedPassword, passwordBuffer, g_LenOfEncryptedPassword);
	
	printNewPasswordGenerated(generatedPassword, PASS_LEN, key, keyLength, g_LenOfEncryptedPassword);
	
	free(key);
	
	return generatedPassword;
}

void updateQueuesNewPass() 
{
	queueNode* currNode = g_QueuesList.head;
	queueNode* temp; 
	while(currNode != NULL)
	{
		temp = currNode;
		currNode = currNode->next;
		sendEncyptedPassToClient(temp);
	}
}

bool cmpPass(char* i_DecryptedPassword, unsigned int i_PassLen, char* i_GeneratedPassword)
{
	bool res = TRUE;
	
	if(i_PassLen == PASS_LEN)
	{
		for(int i = 0; i < PASS_LEN; i++)
		{
			if(i_DecryptedPassword[i] != i_GeneratedPassword[i])
			{
				res = FALSE;
			}
		}
	}
	else
	{
		res = FALSE;
	}
	
	return res;
}

void connectionPrinting(CONNECT_MSG* i_Data)
{
	int id = i_Data->decrypterID;
	
	printf("[SERVER]         [INFO] Received connection request from decrypter id %d, queue name %s \n", id, i_Data->my_mq_identifier);
}

void disconnectionPrinting(DISCONNECT_PASS_MSG* i_Data)
{
	int id = i_Data->decrypterID;
	
	printf("[SERVER]         [INFO] Received disconnection request,--------------------- bye bye decrypter :) %d\n", id);
}






