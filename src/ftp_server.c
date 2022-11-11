/*
 * vmem_server.c
 *
 *  Created on: Oct 27, 2016
 *      Author: johan
 */

#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <csp/csp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <csp/arch/csp_time.h>

#include <csp_ftp/ftp_server.h>

void ftp_server_loop(void * param) {

	/* Statically allocate a listener socket */
	static csp_socket_t ftp_server_socket = {0};

	/* Bind all ports to socket */
	csp_bind(&ftp_server_socket, FTP_PORT_SERVER);

	/* Create 10 connections backlog queue */
	csp_listen(&ftp_server_socket, 10);

	/* Pointer to current connection and packet */
	csp_conn_t *conn;

	/* Process incoming connections */
	while (1) {

		/* Wait for connection, 10000 ms timeout */
		if ((conn = csp_accept(&ftp_server_socket, CSP_MAX_DELAY)) == NULL) {
			continue;
        }

        printf("Server: Recieved connection\n");

		/* Handle RDP service differently */
		if (csp_conn_dport(conn) == FTP_PORT_SERVER) {
			ftp_server_handler(conn);
			csp_close(conn);
			continue;
		} else {
            printf("Server: Unrecognized connection type\n");
        }

		/* Close current connection, and handle next */
		csp_close(conn);

	}

}

void ftp_server_handler(csp_conn_t * conn)
{
	// Read request
	csp_packet_t * packet = csp_read(conn, FTP_SERVER_TIMEOUT);
	if (packet == NULL)
		return;

	// Copy data from request
	ftp_request_t * request = (void *) packet->data;

    // The request is not of an accepted type
    if (request->version > FTP_VERSION) {
        printf("Server: Unknown version\n");
        return;
    }

	int type = request->type;

	if (type == FTP_SERVER_DOWNLOAD) {
        printf("Server: Handling download request\n");
        handle_server_download(conn, request);
	} else if (request->type == FTP_SERVER_UPLOAD) {
        printf("Server: Handling upload request\n");
		handle_server_upload(conn, request);
    } else if (request->type == FTP_SERVER_LIST) {
        printf("Server: Handling list request\n");
		handle_server_list(conn, request);
    } else {
        printf("Server: Unknown request\n");
	}

    csp_buffer_free(packet);
    
}

void handle_server_download(csp_conn_t * conn, ftp_request_t * request) {
    char * address = request->v1.address;

    printf("Server: Starts sending back %s\n", address);
    FILE* f = fopen(address, "rb");
    if (f == NULL) {
        printf("Server: Failed to open file\n");
        csp_sfp_send(conn, NULL, 0, FTP_SERVER_MTU, FTP_SERVER_TIMEOUT);
        return;
    }

    fseek(f, 0L, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0L, SEEK_SET);	

    printf("Server: Sending %ld Bytes\n", file_size);

    char* content = malloc(sizeof(char) * file_size);
    fread(content, file_size, 1, f);
    int err = csp_sfp_send(conn, content, sizeof(char) * file_size, FTP_SERVER_MTU, FTP_SERVER_TIMEOUT);
    if (err != CSP_ERR_NONE) {
        free(content);
		printf("Server: Failed to send file with error %d\n", err);
		return;
	}

    free(content);

    fclose(f);
}

void handle_server_upload(csp_conn_t * conn, ftp_request_t * request) {
    char * address = request->v1.address;

    char* file_content = NULL;
    int file_size = 0;
    int err = csp_sfp_recv(conn, (void**) &file_content, &file_size, FTP_SERVER_TIMEOUT);
    if (err != CSP_ERR_NONE) {
		printf("Server: Failed to recieve file with error %d\n", err);
		return;
	}

    if (file_content == NULL) {
        printf("Server: Failed to recieve file\n");
        return;
    }

    printf("Server: Saving result to %s\n", address);
    FILE* f = fopen(address, "wb");
    fwrite(file_content, file_size, 1, f);
    free(file_content);

    printf("Server: Received %d Bytes\n", file_size);

    fclose(f);
}

int is_directory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

void handle_server_list(csp_conn_t * conn, ftp_request_t * request) {
    char * address = request->v1.address;    

    if (is_directory(address)) {
        DIR * dirp = opendir(address);
        if (dirp == NULL) {
            printf("Server: Could not open current directory" );
            return;
        }

        printf("Server: Getting dir size\n");
        int file_count = 0;
        struct dirent * entry;
        while ((entry = readdir(dirp)) != NULL) {
            if (entry->d_type == DT_REG)
                file_count++;
        }

        rewinddir(dirp);

        size_t filenames_size = sizeof(char) * MAX_PATH_LENGTH * file_count;
        char *filenames = malloc(filenames_size);

        printf("Server: getting files\n");
        size_t offset = 0;
        while ((entry = readdir(dirp)) != NULL && offset < file_count) {
            strncpy(filenames + offset * MAX_PATH_LENGTH , entry->d_name, MAX_PATH_LENGTH);
            offset += 1;
        }

        closedir(dirp);
        printf("Server: Sending %d files of size %d\n", file_count, filenames_size);
        for (int i = 0; i < file_count; i++) {
            printf("Server: - %s\n", filenames + MAX_PATH_LENGTH * i);
        }
        int err = csp_sfp_send(conn, &file_count, sizeof(int), FTP_SERVER_MTU, FTP_SERVER_TIMEOUT);
        if (err != CSP_ERR_NONE) {
            printf("Server: Failed to send file count with error %d\n", err);
            return;
        }
        err = csp_sfp_send(conn, filenames, filenames_size, FTP_SERVER_MTU, FTP_SERVER_TIMEOUT);
        if (err != CSP_ERR_NONE) {
            printf("Server: Failed to send filename with error %d\n", err);
            return;
        }

        free(filenames);

    } else {
        printf("Server: Sending zero\n");
        int zero = 0;
        csp_sfp_send(conn, &zero, sizeof(int), FTP_SERVER_MTU, FTP_SERVER_TIMEOUT);
    }
}