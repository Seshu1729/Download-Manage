#include "stdafx.h"
#include <winsock2.h>
#include <windows.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <conio.h>

//////////////////////////////////////////////
//											//
//		     ***CLIENT MACROS***			//
//											//
//////////////////////////////////////////////

#define buffer_size 1024

//////////////////////////////////////////////
//											//
//		 ***FUNCTION DEFINATIONS***			//
//											//
//////////////////////////////////////////////

DWORD WINAPI DOWNLOAD_MANAGER_THREAD(void* data);
char *allocate_string_memory();

//////////////////////////////////////////////
//											//
//		 ***STRUCTURE DEFINATIONS***		//
//											//
//////////////////////////////////////////////

struct thread_information
{
	char file_name[buffer_size];
	FILE *file_pointer;
	int thread_index;
	int block_count;
};

//////////////////////////////////////////////
//											//
//		  ***GLOBAL VARIABLES***			//
//											//
//////////////////////////////////////////////


int host_port = 1101;
char* host_name = "127.0.0.1";

unsigned short wVersionRequested;
WSADATA wsaData;
int err;

struct sockaddr_in my_addr;
int bytecount;

//////////////////////////////////////////////
//											//
//		  ***CLIENT FUNCTIONS***			//
//											//
//////////////////////////////////////////////

int opensocket()
{
	int hsock, *p_int;

	//INITIALIZING SOCKET
	hsock = socket(AF_INET, SOCK_STREAM, 0);
	if (hsock == -1)
	{
		printf("ERROR INITIALIZING SOCKET %d\n", WSAGetLastError());
		return -1;
	}

	p_int = (int*)malloc(sizeof(int));
	*p_int = 1;

	if ((setsockopt(hsock, SOL_SOCKET, SO_REUSEADDR, (char*)p_int, sizeof(int)) == -1) || (setsockopt(hsock, SOL_SOCKET, SO_KEEPALIVE, (char*)p_int, sizeof(int)) == -1))
	{
		printf("ERROR IN SETTING OPTION %d\n", WSAGetLastError());
		free(p_int);
		return -1;
	}

	free(p_int);

	return hsock;
}

void send_message(char *buffer, int hsock)
{

	if (connect(hsock, (struct sockaddr*)&my_addr, sizeof(my_addr)) == SOCKET_ERROR)
	{
		fprintf(stderr, "ERROR IN CONNECTING SOCKET %d\n", WSAGetLastError());
		getchar();
		exit(1);
	}

	if ((bytecount = send(hsock, buffer, strlen(buffer), 0)) == SOCKET_ERROR)
	{
		fprintf(stderr, "ERROR IN SENDING DATA TO SOCKET %d\n", WSAGetLastError());
		getchar();
		exit(1);
	}
}

char *receive_message(int hsock)
{
	char *buffer = allocate_string_memory();

	if ((bytecount = recv(hsock, buffer, buffer_size, 0)) == SOCKET_ERROR)
	{
		fprintf(stderr, "ERROR RECEIVING DATA %d\n", WSAGetLastError());
		getchar();
		exit(1);
	}

	return buffer;
}

char *process_message(char *buffer)
{
	char *return_buffer;
	int hsock = opensocket();

	send_message(buffer, hsock);
	return_buffer = receive_message(hsock);
	closesocket(hsock);

	return return_buffer;
}


//////////////////////////////////////////////
//											//
//		   ***HELPER FUNCTIONS***			//
//											//
//////////////////////////////////////////////

char *allocate_string_memory()
{
	char *string;
	string = (char *)malloc(buffer_size);
	memset(string, '\0', buffer_size);
	return string;
}

int get_multiplier(int value)
{
	int multiplier = 1;

	while (value > 9)
	{
		value /= 10;
		multiplier *= 10;
	}
	return multiplier;
}

int calculate_thread_count(int file_size)
{
	int multiplier = get_multiplier(file_size);

	if (file_size % multiplier != 0)
		file_size += (multiplier % (file_size % multiplier));

	while (file_size > 9)
		file_size /= 10;

	if (file_size == 1)
		file_size *= 10;

	return file_size;
}

void print_file_statics(char *file_name,int file_size, int thread_count)
{
	printf("FILE NAME         ::  %s\n", file_name);
	printf("FILE SIZE         ::  %d KB\n", file_size);
	printf("THREAD CREATED    ::  %d\n", thread_count);
	printf("EACH THREAD PART  ::  %d KB\n\n", file_size / thread_count);
}

//////////////////////////////////////////////
//											//
//    ***DOWNLOAD MANAGER FUNCTIONS***      //
//											//
//////////////////////////////////////////////

void Mini_Download_Manager()
{
	char option = 'y';
	FILE *file_pointer;
	int file_size, block_count, thread_count, hsock;

	struct thread_information *information;

	while (option == 'Y' || option == 'y')
	{
		char *file_name = allocate_string_memory();
		char *buffer = allocate_string_memory();
		char *file_address = allocate_string_memory();

		printf("SEARCH FILE NAME::");
		fflush(stdin); gets(file_name);

		sprintf(buffer, "%s/%s/", "FILESIZE", file_name);
		file_size = atoi(process_message(buffer));

		if (file_size != -1)
		{
			thread_count = calculate_thread_count(file_size);
			block_count = file_size / thread_count;
			print_file_statics(file_name, file_size, thread_count);

			strcpy(file_address, "Downloads/");
			strcat(file_address, file_name);
			remove(file_address);

			file_pointer = fopen(file_address, "ab+");

			for (int index = 0; index < thread_count; index++)
			{
				information = (thread_information *)malloc(sizeof(thread_information));

				strcpy(information->file_name, file_name);
				information->file_pointer = file_pointer;
				information->thread_index = index;
				information->block_count = block_count;

				CreateThread(0, 0, &DOWNLOAD_MANAGER_THREAD, (void*)information, 0, 0);
			}

		}
		else
			printf("FILE NOT FOUND.TRY WITH OTHER NAME.\n");
		printf("ENTER ANY KEY TO CONTINUE...");
		fflush(stdin); getchar();

		fclose(file_pointer);

		printf("DO YOU WANT TO DOWNLOAD OTHER FILE[Y/N]::");
		fflush(stdin); scanf("%c", &option);
	}
}

DWORD WINAPI DOWNLOAD_MANAGER_THREAD(void *data)
{
	int block_position, hsock;
	char *buffer, *responce;
	struct thread_information *information = (thread_information *)data;

	for (int index = 0; index < information->block_count; index++)
	{
		buffer = allocate_string_memory();
		responce = allocate_string_memory();
		block_position = index + information->block_count * information->thread_index;

		sprintf(buffer, "%s/%s/%d/", "FILEDATA", information->file_name, block_position);
		strcpy(responce, process_message(buffer));
		
		fseek(information->file_pointer, block_position * buffer_size, SEEK_SET);
		fwrite(responce, strlen(responce), 1, information->file_pointer);
	}

	return 0;
}

//////////////////////////////////////////////
//											//
//		***CLIENT CORE FUNCTIONS***			//
//											//
//////////////////////////////////////////////

void socket_client()
{
	wVersionRequested = MAKEWORD(2, 2);
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0 || (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2))
	{
		fprintf(stderr, "Could not find sock dll %d\n", WSAGetLastError());
		return;
	}

	my_addr.sin_family = AF_INET;
	my_addr.sin_port = htons(host_port);

	memset(&(my_addr.sin_zero), 0, 8);
	my_addr.sin_addr.s_addr = inet_addr(host_name);

	Mini_Download_Manager();

}