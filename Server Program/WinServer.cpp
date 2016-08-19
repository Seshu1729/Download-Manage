#include "stdafx.h"
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#define thread_count 4
#define buffer_size 1024

DWORD WINAPI SocketHandler(void*);
char *allocate_string_memory();


//////////////////////////////////////////////
//											//
//		  ***SERVER FUNCTIONS***			//
//											//
//////////////////////////////////////////////

void socket_server() 
{
	int host_port= 1101;
	unsigned short wVersionRequested;
	WSADATA wsaData;
	int err, hsock, *p_int, *csock;

	wVersionRequested = MAKEWORD( 2, 2 );
 	err = WSAStartup( wVersionRequested, &wsaData );

	if ( err != 0 || ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 ))
	{
	    fprintf(stderr, "NO SOCK DLL %d\n",WSAGetLastError());
		goto FINISH;
	}

	//INITIALIZING SOCKET
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if(hsock == -1)
	{
		printf("ERROR IN INITIALIZING SOCKET %d\n",WSAGetLastError());
		goto FINISH;
	}
	
	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;

	//SETTING OPTIONS
	if( (setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1 )|| (setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1 ) )
	{
		printf("ERROR SETTING OPTION %d\n", WSAGetLastError());
		free(p_int);
		goto FINISH;
	}
	free(p_int);

	struct sockaddr_in my_addr;
	my_addr.sin_family = AF_INET ;
	my_addr.sin_port = htons(host_port);
	
	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = INADDR_ANY ;

	//BINDING SOCKET
	if( bind( hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == -1 )
	{
		fprintf(stderr,"ERROR BINDING SOCKET %d\n",WSAGetLastError());
		goto FINISH;
	}

	//LISTENING SOCKET
	if(listen( hsock, 10) == -1 )
	{
		fprintf(stderr, "ERROR LISTENING SOCKET %d\n",WSAGetLastError());
		goto FINISH;
	}

	sockaddr_in sadr;
	int	addr_size = sizeof(SOCKADDR);
	
	while(true)
	{
		csock = (int*)malloc(sizeof(int));
		
		if((*csock = accept( hsock, (SOCKADDR*)&sadr, &addr_size))!= INVALID_SOCKET )
		{
			CreateThread(0, 0, &SocketHandler, (void*)csock, 0, 0);
		}
		else
		{
			fprintf(stderr, "ERROR ACCEPTING SOCKET %d\n",WSAGetLastError());
		}
	}
	FINISH:
		/*DO NOTHING*/;
}

char *receive_message(int *csock)
{
	char *buffer = allocate_string_memory();
	int bytecount;
	if ((bytecount = recv(*csock, buffer, buffer_size, 0)) == SOCKET_ERROR)
	{
		fprintf(stderr, "ERROR RECEIVING DATA %d\n", WSAGetLastError());
		free(csock);
	}

	return buffer;
}

void send_message(int *csock,char *buffer)
{
	int bytecount;
	if ((bytecount = send(*csock, buffer, strlen(buffer), 0)) == SOCKET_ERROR)
	{
		fprintf(stderr, "ERROR SENDING DATA %d\n", WSAGetLastError());
		free(csock);
	}
}

//////////////////////////////////////////////
//											//
//		   ***HELPER FUNCTIONS***			//
//											//
//////////////////////////////////////////////

int get_file_size(char *file_address)
{
	int file_size_in_bytes, file_size;
	FILE *file_pointer = fopen(file_address, "r");

	if (file_pointer != NULL)
	{
		fseek(file_pointer, 0, SEEK_END);
		file_size_in_bytes = ftell(file_pointer);
		fclose(file_pointer);

		file_size = file_size_in_bytes / 1024;
		if (file_size_in_bytes % 1024)
			file_size++;
		return file_size;
	}

	return -1;
}

char *allocate_string_memory()
{
	char *string;
	string = (char *)malloc(buffer_size);
	memset(string, '\0', buffer_size);
	return string;
}

//////////////////////////////////////////////
//											//
//	    ***COMMAND PROCESS FUNCTIONS***	    //
//											//
//////////////////////////////////////////////

char *file_size_command()
{
	char *parser = allocate_string_memory();
	char *buffer = allocate_string_memory();
	char *file_address = allocate_string_memory();

	strcpy(parser, strtok(NULL, "/"));
	strcpy(file_address, "Documents/");
	strcat(file_address, parser);

	sprintf(buffer, "%d", get_file_size(file_address));
	return buffer;
}

char *file_data_command()
{
	char *parser = allocate_string_memory();
	char *buffer = allocate_string_memory();
	char *file_address = allocate_string_memory();
	int block_position;

	strcpy(parser, strtok(NULL, "/"));
	strcpy(file_address, "Documents/");
	strcat(file_address, parser);

	strcpy(parser, strtok(NULL, "/"));
	block_position = atoi(parser);

	FILE *file_pointer = fopen(file_address, "rb");
	fseek(file_pointer, block_position * buffer_size, SEEK_SET);
	fread(buffer, buffer_size, 1, file_pointer);
	fclose(file_pointer);

	return buffer;
}

char *execute_command(char *command)
{
	char *parser = allocate_string_memory();
	strcpy(parser, strtok(command, "/"));

	if (!strcmp(parser, "FILESIZE"))
		return file_size_command();
	else if (!strcmp(parser, "FILEDATA"))
		return file_data_command();
	else
		return "INVALID COMMAND.\nPLEASE CONTACT DEVELOPER.\n";
}

void process_message(int *csock)
{
	char *command = allocate_string_memory();

	strcpy(command, receive_message(csock));
	printf("COMMAND RECEIVED : %s\n", command);

	send_message(csock, execute_command(command));

}

//////////////////////////////////////////////
//											//
//	    ***SERVER CORE FUNCTIONS***			//
//											//
//////////////////////////////////////////////

DWORD WINAPI SocketHandler(void *data)
{
    int *csock = (int *)data;
	process_message(csock);
    return 0;
}