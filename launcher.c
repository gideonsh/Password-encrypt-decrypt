#include "include.h"

//input : sudo ./launcher X -n Y 
    //OR 
 //     : sudo ./launcher X


int main(int argc, char* argv[])
{
    char* myargv[5];
    checkInputValidation(argc, argv);
    if(argc == 4 || argc == 2  )
    {
	    int numberOfDecrypters = atoi(argv[1]);
       
        if(fork() == 0) // run server
        {
            myargv[0] = strdup(ServerFileName);
            myargv[1] = NULL;
            execv(ServerFileName, myargv);
        }

        sleep(1);

        for(int i=1 ; i <= numberOfDecrypters ; i++)
        {
            myargv[0] = strdup(ClientFileName);
            myargv[1] = (char*)malloc(sizeof(strlen(argv[1]))+1);
            sprintf(myargv[1],"%d",i);

            if(argc == 4)
            {
                myargv[2] = "-n";
                myargv[3] = strdup(argv[3]);
                myargv[4] = NULL;
            }
            else if(argc == 2)
            {
                  myargv[2] = NULL;
            }          
            int status = fork();
            if(status == 0) // run the decrypter number "i"
            {
                execv(ClientFileName, myargv);
            }else
            {
                assert_if(status);
            }
        }   
    }
	
    return 0;
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
            if(isNumber(i_Argv[3]) == FALSE && i_Argv[3] != NULL)
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
