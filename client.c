//decrypter
#include "include.h"

int  main(int argc, char* argv[])
{
	checkInputValidation(argc, argv) ;
	bool runForever = FALSE;
	int counterLoop = 0 ; 
	mqd_t clientQueueID;
	mqd_t serverQueueID;
	
	unsigned int decrypterNum = atoi(argv[1]);
	if (argc == 2)
	{
		runForever = TRUE;
	}
	else if(argc == 4 )
	{
		counterLoop = atoi(argv[3]);
	}else
	{
		printf("Wrong number of arguments, please try again\n");
	}
	
	char* decrypterQueueName = makeClientQueue(&clientQueueID, decrypterNum);

	serverQueueID = mq_open(MQ_SERVER_ID, O_WRONLY);
	assert_if(serverQueueID)
	
	
	sendConnectionRequest(decrypterNum, decrypterQueueName, serverQueueID);
	startDecrypting(clientQueueID,serverQueueID,decrypterNum,counterLoop, runForever);	
	sendDisconnectionRequest(decrypterNum, clientQueueID, decrypterQueueName, serverQueueID);
	
	return 0; 
}


char* makeClientQueue(mqd_t* i_Client_q, unsigned int i_Decrypter_num)
{
	struct mq_attr attr = {0};
	char* queueName = (char*)malloc(MQ_NAME_MAX_LEN); 
	
	attr.mq_maxmsg = MQ_MAX_MSG;
	attr.mq_msgsize = MQ_MAX_MSG_SIZE;
	sprintf(queueName, DECRYPTER_MQ, i_Decrypter_num);
	mq_unlink(queueName);
	mq_close(*i_Client_q); 
	
	*i_Client_q = mq_open(queueName, O_CREAT | O_EXCL, S_IRWXU | S_IRWXG, &attr);
	assert_if(*i_Client_q)
	
	
	return queueName;
}
//queueNode
void sendConnectionRequest(unsigned int i_ID, char* i_DecrypterQueueName, mqd_t i_ServerQueueID)
{
	int status;
	
	MSG_T* msg = (MSG_T*)malloc(sizeof(MSG_T) + sizeof(CONNECT_MSG));
	msg->type = CONNECTION;
	((CONNECT_MSG*)(msg->data))->decrypterID = i_ID;
	((CONNECT_MSG*)(msg->data))->clientPid = getpid();
	strcpy(((CONNECT_MSG*)(msg->data))->my_mq_identifier, i_DecrypterQueueName);
	beforeConnectionPrinting(((CONNECT_MSG*)(msg->data)));
	status = mq_send(i_ServerQueueID, (char*)msg, MQ_MAX_MSG_SIZE,0);
	afterConnectionPrinting(((CONNECT_MSG*)(msg->data)));
	
	assert_if(status)
	
	
	free(msg);
}

bool isPrintable(char* i_GeneratedPassword, int i_PassLen)
{
	bool res = TRUE;
	int i;

	for(i = 0; i < i_PassLen; i++)
	{
		if(isprint(i_GeneratedPassword[i])== FALSE)
		{
			
			res = FALSE;
		}
	}
	
	return res;
}

void beforeConnectionPrinting(CONNECT_MSG* i_Data)
{
	int id = i_Data->decrypterID;

	printf("[CLIENT #%d]      [INFO] Sending connect request on %s\n", id, i_Data->my_mq_identifier);
}

void afterConnectionPrinting(CONNECT_MSG* i_Data)
{
	int id = i_Data->decrypterID;

	printf("[CLIENT #%d]      [INFO] Sent connect request on %s\n", id, i_Data->my_mq_identifier);
}
   
void sendToServerQueue(mqd_t i_ServerQueueID, char* i_DecryptedPass, unsigned int i_DecryptedPassLen ,unsigned int i_DecrypterNum,unsigned int i_Iterations)	
{
	int status;
	struct mq_attr serverAttr = {0};
	
	MSG_T* msg = malloc(sizeof(MSG_T) + sizeof(GUESED_PASS_MSG));
	msg->type = GUESSED_PASS;
	((GUESED_PASS_MSG*)msg->data)->passLen = i_DecryptedPassLen;
	((GUESED_PASS_MSG*)msg->data)->decrypterID = i_DecrypterNum;
	((GUESED_PASS_MSG*)msg->data)->iteration = i_Iterations;
	memcpy(((GUESED_PASS_MSG*)msg->data)->decryptedPassword, i_DecryptedPass,i_DecryptedPassLen);
	status = mq_send(i_ServerQueueID, (char*)msg, MQ_MAX_MSG_SIZE, 0);
	assert_if(status);/////////////////// Test
	free(msg);
}

void startDecrypting(mqd_t i_ClientQueueID, mqd_t i_ServerQueueID,unsigned int i_DecrypterNum,int i_CounterLoop, bool runForever)
{
	int rounds = 0;
	unsigned int iterations = 0;
	unsigned int encryptedLength;
	char* encryptPassword;
	unsigned int keyLength = PASS_LEN/8;
	unsigned int* plainDataLengthPtr = (unsigned int*)malloc(sizeof(unsigned int));
	char* key = (char*)calloc(keyLength, sizeof(char));
	bool printable;
	char plainData[MAX_BUFFER];
	MTA_CRYPT_RET_STATUS status;
	char* decreyptedPass = (char*) malloc(sizeof(char));
	struct mq_attr attr = {0};
	
	MSG_T* msg = (MSG_T*)malloc(MQ_MAX_MSG_SIZE);
	int mqCheck;
	bool isFirstLoop = TRUE;

	while(rounds < i_CounterLoop || runForever)
	{	
		mq_getattr(i_ClientQueueID, &attr);
		
		if (attr.mq_curmsgs > 0 || isFirstLoop) 
		{	
			mqCheck = mq_receive(i_ClientQueueID,(char*)msg, MQ_MAX_MSG_SIZE, NULL);	
			assert_if(mqCheck)

			if(msg->type == GUESSED_PASS)
			{
				if(((GUESED_PASS_MSG*)msg->data)->decrypterID == i_DecrypterNum)
				{
					printclient((GUESED_PASS_MSG*)msg->data , encryptPassword);
				}
				
				mqCheck = mq_receive(i_ClientQueueID,(char*)msg, MQ_MAX_MSG_SIZE, NULL);	//block until gets the encrypted password
				assert_if(mqCheck)
			}

			if(msg->type == ENCRYPTED_PASS) 
			{	
				if(!isFirstLoop)
				{
					free(encryptPassword);
				}
				
				iterations = 0;
				encryptedLength = ((ENCRYPTED_MSG*)msg->data)->encrypted_pass_len;
				encryptPassword = (char*)malloc(sizeof(char)*encryptedLength);
				memcpy(encryptPassword, (char*)((ENCRYPTED_MSG*)(msg->data))->encrypted_pass , encryptedLength);
				isFirstLoop = FALSE;
				printf("[CLIENT #%d]      [INFO] Recieved new encrypted password: %s\n", i_DecrypterNum, encryptPassword);
			}	
		}	
	
		iterations++;
		MTA_get_rand_data(key, keyLength);		
		status = MTA_decrypt(key, keyLength, encryptPassword, encryptedLength, plainData, plainDataLengthPtr);
		assert_if(status)
		free(decreyptedPass);
		decreyptedPass = (char*)malloc(sizeof(char)*(*plainDataLengthPtr));
		memcpy(decreyptedPass, plainData, *plainDataLengthPtr);		
		printable = isPrintable(decreyptedPass, *plainDataLengthPtr);

		if(printable)
		{

			sendToServerQueue(i_ServerQueueID, decreyptedPass, *plainDataLengthPtr, i_DecrypterNum, iterations);
			rounds++;	

		}
	}
	
	free(key);
}

void printclient(GUESED_PASS_MSG* i_Data, char* i_EncryptPassword)
{
	char tPass[i_Data->passLen+1];
	char c;
	int i;
	
	for(i = 0; i < i_Data->passLen; i++)
	{
		c = i_Data->decryptedPassword[i];
		tPass[i] = c;
	}
	tPass[i] = '\0';
	printf("[CLIENT #%d]      [INFO] After decryption: (%s) with encrypt password: (%s), sending to server after %d iterations \n", i_Data->decrypterID ,tPass, i_EncryptPassword, i_Data->iteration);
}

void sendDisconnectionRequest(unsigned int i_ID, mqd_t clientQueueID, char* i_DecrypterQueueName, mqd_t i_ServerQueueID)
{
	MSG_T* msg = (MSG_T*)malloc(sizeof(MSG_T) + sizeof(DISCONNECT_PASS_MSG));
	msg->type = DISCONNECT;
	((DISCONNECT_PASS_MSG*)(msg->data))->decrypterID = i_ID;
	strcpy(((DISCONNECT_PASS_MSG*)(msg->data))->my_mq_identifier , i_DecrypterQueueName);
	
	int status = mq_send(i_ServerQueueID, (char*)msg, MQ_MAX_MSG_SIZE,0 );
	assert_if(status)
	
	
	mq_close(clientQueueID);
	free(msg);
}

void checkInputValidation(int i_Argc, char** i_Argv)
{
    if( i_Argc==2 || i_Argc == 4) 
    {
        if(isNumber(i_Argv[1]) == FALSE)
        {
            printf("Second argument input should be a number, please try again! \n");
            exit(1);
        }
        else if(i_Argc == 4)
        {
            if(isNumber(i_Argv[3]) == FALSE && i_Argv[3]!= NULL)
           {
                printf("number of rounds argument input should be a number, please try again! \n");
                exit(1);
           }
           if(strcmp(i_Argv[2],"-n")!=0)
           {
               printf("Wrong flag, please try again with -n \n");
                exit(1);
           }
        }
    }else
    {
        printf("Wrong number of arguments, please try again\n");
        exit(1);
    }

}

bool isNumber(char* number)
{
    for(int i=0 ; number[i]!='\0'; i++)
    {
        if(number[i]>'9' || number[i] < '0')
        {
            return FALSE;
        }
    }
    return TRUE;
}


