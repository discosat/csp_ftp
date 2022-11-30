#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <csp/csp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/stat.h>
#include <csp/arch/csp_time.h>

#include <csp_ftp/ftp_server.h>
#include "csp_ftp/ftp_perf.h"
#include "csp_ftp/ftp_status.h"

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

        printf("Server: Recieved connection on %d\n", csp_conn_dport(conn));

		/* Handle RDP service differently */
		if (csp_conn_dport(conn) == FTP_PORT_SERVER) {
			ftp_server_handler(conn);
			csp_close(conn);
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
	if (packet == NULL) {
        printf("Server: Recieved no header\n");
		return;
    }

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
    } else if (request->type == FTP_MOVE) {
        printf("Server: Handling move request\n");
		handle_server_move(conn, request);
    } else if (request->type == FTP_REMOVE) {
        printf("Server: Handling remove request\n");
		handle_server_remove(conn, request);
    } else if (request->type == FTP_PERFORMANCE_UPLOAD) {
        printf("Server: Handling performance upload request\n");
		perf_upload(conn, &request->perf_header);
    } else if (request->type == FTP_PERFORMANCE_DOWNLOAD) {
        printf("Server: Handling performance download request\n");
		perf_download(conn, &request->perf_header);
    } else {
        printf("Server: Unknown request\n");
	}

    csp_buffer_free(packet);
    
}

ftp_status_t mkdirs(char* address) {
    // Get length of directory without filename
    int dir_len = strlen(address) - 1;
    while (dir_len > 0) {
        if (address[dir_len] == '/' || address[dir_len] == '\\') {
            break;
        }
        dir_len -= 1;
    }

    // Create directory and sub directory
    if (dir_len >= 0) {
        // Here dir_len contains the index of the last character.
        // So an index of 0 would mean there is 1 character in the path
        // It therefore needs adjustment
        dir_len += 1;

        int sub_dir_idx = 0;
        while (sub_dir_idx < dir_len) {
            if (address[sub_dir_idx] == '/' || address[sub_dir_idx] == '\\') {
                // Remove any garbage values in the path
                char dir_adress[MAX_PATH_LENGTH];
                for (int i = 0; i < MAX_PATH_LENGTH; i++)
                    dir_adress[i] = 0;

                // Copy the sub directory path
                memcpy( dir_adress, address, sub_dir_idx);

                printf("Server: Creating folder %s idx %d\n", dir_adress, sub_dir_idx);
                if (mkdir(dir_adress, S_IRWXU | S_IRWXG | S_IRWXO) == -1) {
                    printf("Server: Failed to create folders\n");
                    return IO_ERROR;
                }
            }
            
            sub_dir_idx++;
        }
    }

    return OK;
}

void handle_server_download(csp_conn_t * conn, ftp_request_t * request) {
    char * address = request->v1.address;

    // Open the file
    printf("Server: Starts sending back %s\n", address);
    FILE* f = fopen(address, "rb");
    if (f == NULL) {
        printf("Server: Failed to open file\n");
        ftp_send_status(conn, IO_ERROR);
        return;
    } else {
        ftp_send_status(conn, OK);
    }

    // Determin the size of the file
    fseek(f, 0L, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0L, SEEK_SET);	
    printf("Server: Sending %ld Bytes\n", file_size);

    // Send back content of file
    char* content = malloc(sizeof(char) * file_size);
    fread(content, file_size, 1, f);
    int err = csp_sfp_send(conn, content, sizeof(char) * file_size, FTP_SERVER_MTU, FTP_SERVER_TIMEOUT);
    if (err != CSP_ERR_NONE) {
        free(content);
		printf("Server: Failed to send file with error %d\n", err);
		return;
	}

    // Cleanup
    free(content);
    fclose(f);
}

void handle_server_upload(csp_conn_t * conn, ftp_request_t * request) {
    char * address = request->v1.address;

    ftp_status_t dir_status = mkdirs(address);
    if (ftp_status_is_err(&dir_status)) {
        ftp_send_status(conn, dir_status);
        return;
    }

    // make sure the file is accessible
    FILE* test_f = fopen(address, "wb");
    if (test_f == NULL) {
        printf("Server: Failed to open file\n");
        ftp_send_status(conn, IO_ERROR);
        return;
    } else {
        fclose(test_f);
        ftp_send_status(conn, OK);
    }

    // Recieve content of file
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

    // Write the content of the file
    printf("Server: Saving result to %s\n", address);
    FILE* f = fopen(address, "wb");
    if (f == NULL) {
        printf("Server: Failed to open file\n");
        ftp_send_status(conn, IO_ERROR);
        return;
    } else {
        ftp_send_status(conn, OK);
    }

    fwrite(file_content, file_size, 1, f);

    // Clean up
    fclose(f);
    free(file_content);

    printf("Server: Received %d Bytes\n", file_size);
}

int is_directory(const char *path) {
   struct stat statbuf;
   if (stat(path, &statbuf) != 0)
       return 0;
   return S_ISDIR(statbuf.st_mode);
}

void handle_server_list(csp_conn_t * conn, ftp_request_t * request) {
    char * address = request->v1.address;

    // Ensure that we don't try to list a file
    if (!is_directory(address)) {
        printf("Server: Cannot list file\n");
        ftp_send_status(conn, IO_ERROR);
        return;
    }

    // Create a handler for the directory
    DIR * dirp = opendir(address);
    if (dirp == NULL) {
        printf("Server: Could not open directory" );
        ftp_send_status(conn, IO_ERROR);
        return;
    } else {
        ftp_send_status(conn, OK);
    }

    // Figure out how many files are in the directory
    printf("Server: Getting dir size\n");
    int file_count = 0;
    struct dirent * entry;
    while ((entry = readdir(dirp)) != NULL) {
        file_count++;
    }

    rewinddir(dirp);

    // Create a space to store the filenames
    size_t filenames_size = sizeof(char) * MAX_PATH_LENGTH * (file_count);
    char *filenames = calloc(1, filenames_size);

    // Read content of directory
    printf("Server: getting files\n");
    size_t offset = 0;
    while ((entry = readdir(dirp)) != NULL && offset < file_count) {
        strncpy(filenames + offset * MAX_PATH_LENGTH , entry->d_name, MAX_PATH_LENGTH);
        offset += 1;
    }
    closedir(dirp);

    // Debug content
    printf("Server: Sending %d files of size %d\n", file_count, filenames_size);
    for (int i = 0; i < file_count; i++) {
        printf("Server: - %s\n", filenames + MAX_PATH_LENGTH * i);
    }

    // Send back number of files
    int err = csp_sfp_send(conn, &file_count, sizeof(int), FTP_SERVER_MTU, FTP_SERVER_TIMEOUT);
    if (err != CSP_ERR_NONE) {
        printf("Server: Failed to send file count with error %d\n", err);
        return;
    }

    // Send back list of files
    err = csp_sfp_send(conn, filenames, filenames_size, FTP_SERVER_MTU, FTP_SERVER_TIMEOUT);
    if (err != CSP_ERR_NONE) {
        printf("Server: Failed to send filename with error %d\n", err);
        return;
    }

    printf("Server: Finnished sending files\n");

    // Cleanup
    free(filenames);
}

void handle_server_move(csp_conn_t * conn, ftp_request_t * request) {
    char * address = request->v1.address;

    char *new_address = NULL;
    int address_len = 0;
    int err = csp_sfp_recv(conn, (void**) &new_address, &address_len, FTP_SERVER_TIMEOUT);
    if (err != CSP_ERR_NONE) {
        printf("Server: Failed to recieve new address for %s\n", address);
        return;
    }

    ftp_status_t dir_status = mkdirs(new_address);
    if (ftp_status_is_err(&dir_status)) {
        ftp_send_status(conn, dir_status);
        return;
    }

    printf("Server: Moving file from %s to %s\n", address, new_address);
    int rename_err = rename(address, new_address);
    if (rename_err == -1) {
        printf("Server: Failed to move file\n");
        ftp_send_status(conn, IO_ERROR);
        return;
    } else {
        ftp_send_status(conn, OK);
    }
}

void handle_server_remove(csp_conn_t * conn, ftp_request_t * request) {
    char * address = request->v1.address;
    int err = remove(address);
    if (err == -1) {
        printf("Server: Failed to remove file\n");
        ftp_send_status(conn, IO_ERROR);
        return;
    } else {
        ftp_send_status(conn, OK);
    }
}