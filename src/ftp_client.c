#include <stdio.h>
#include <csp/arch/csp_time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include <csp_ftp/ftp_client.h>
#include <csp_ftp/ftp_server.h>

void ftp_download_file(int node, int timeout, const char * filename, int version, char** dataout, int* dataout_size)
{
	*dataout = NULL;
	*dataout_size = 0;

	uint32_t time_begin = csp_get_ms();

	/* Establish RDP connection */
	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, FTP_PORT_SERVER, timeout, CSP_O_RDP | CSP_O_CRC32);
	if (conn == NULL)
		return;

	csp_packet_t * packet = csp_buffer_get(sizeof(ftp_request_t));
	if (packet == NULL)
		return;

	ftp_request_t * request = (void *) packet->data;
	request->version = version;
	request->type = FTP_SERVER_DOWNLOAD;
	packet->length = sizeof(ftp_request_t);

	size_t filename_len = strlen(filename);
	if( filename_len > MAX_PATH_LENGTH)
        return;

	strncpy(request->v1.address, filename, MAX_PATH_LENGTH);
	// This will only fail if MAX_PATH_LENGTH > UINT16_MAX
	request->v1.length = filename_len;

	/* Send request */
	csp_send(conn, packet);

	int err = csp_sfp_recv(conn, (void**) dataout, dataout_size, FTP_CLIENT_TIMEOUT);
	if (err != CSP_ERR_NONE) {
		printf("Client: Failed to recieve file with error %d\n", err);
		return;
	}

	csp_close(conn);

	uint32_t time_total = csp_get_ms() - time_begin;

	printf("Client: Downloaded %u bytes in %.03f s at %u Bps\n", (unsigned int) *dataout_size, time_total / 1000.0, (unsigned int) (*dataout_size / ((float)time_total / 1000.0)) );

}

void ftp_upload_file(int node, int timeout, const char * filename, int version, char * datain, int datain_size)
{
	uint32_t time_begin = csp_get_ms();

	/* Establish RDP connection */
	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, FTP_PORT_SERVER, timeout, CSP_O_RDP | CSP_O_CRC32);
	if (conn == NULL)
		return;

	csp_packet_t * packet = csp_buffer_get(sizeof(ftp_request_t));
	if (packet == NULL)
		return;

	ftp_request_t * request = (void *) packet->data;
	request->version = version;
	request->type = FTP_SERVER_UPLOAD;
	packet->length = sizeof(ftp_request_t);

	size_t filename_len = strlen(filename);
	if( filename_len > MAX_PATH_LENGTH)
        return;

	strncpy(request->v1.address, filename, MAX_PATH_LENGTH);
	// This will only fail if MAX_PATH_LENGTH > UINT16_MAX
	request->v1.length = filename_len;

	/* Send request */
	csp_send(conn, packet);
	
	int err = csp_sfp_send(conn, datain, datain_size, FTP_SERVER_MTU, FTP_CLIENT_TIMEOUT);
	if (err != CSP_ERR_NONE) {
		printf("Client: Failed to send file with error %d\n", err);
		return;
	}

	csp_close(conn);

	uint32_t time_total = csp_get_ms() - time_begin;

	printf("Client: Uploaded %u bytes in %.03f s at %u Bps\n", (unsigned int) datain_size, time_total / 1000.0, (unsigned int) (datain_size / ((float)time_total / 1000.0)) );

}

void ftp_list_files(int node, int timeout, const char * remote_directory, int version, char** filenames, int* file_count)
{
	*filenames = NULL;
	*file_count = 0;

	/* Establish RDP connection */
	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, FTP_PORT_SERVER, timeout, CSP_O_RDP | CSP_O_CRC32);
	if (conn == NULL)
		return;

	size_t remote_directory_len = strlen(remote_directory);
	if( remote_directory_len > MAX_PATH_LENGTH)
        return;

	csp_packet_t * packet = csp_buffer_get(sizeof(ftp_request_t));
	if (packet == NULL)
		return;

	ftp_request_t * request = (void *) packet->data;
	request->version = version;
	request->type = FTP_SERVER_LIST;
	packet->length = sizeof(ftp_request_t);

	strncpy(request->v1.address, remote_directory, MAX_PATH_LENGTH);
	// This will only fail if MAX_PATH_LENGTH > UINT16_MAX
	request->v1.length = remote_directory_len;

	/* Send request */
	csp_send(conn, packet);

	int *num_files = NULL;
	int file_count_size;

	int err = csp_sfp_recv(conn,(void**) &num_files, &file_count_size, FTP_CLIENT_TIMEOUT);
	if (err != CSP_ERR_NONE) {
		printf("Client: Failed to recieve number with error %d\n", err);
		return;
	}

	if (num_files == NULL || *num_files == 0) {
		printf("Client: directory is empty\n");
		free(num_files);
		return;
	}

	*file_count = *num_files;
	free(num_files);

	size_t expected_filenames_size = sizeof(char) * MAX_PATH_LENGTH * *file_count;
	int filenames_size = 0;

	err = csp_sfp_recv(conn, (void**) filenames, &filenames_size, FTP_CLIENT_TIMEOUT);
	if (err != CSP_ERR_NONE) {
		printf("Client: Failed to recieve filenames with error %d\n", err);
		return;
	}
	
	csp_close(conn);

	if (filenames == NULL) {
		printf("Client: Failed to recieve filenames\n");
		return;
	}

	if (filenames_size != expected_filenames_size) {
		printf("Client: recieved %d Bytes but expected %d", filenames_size, expected_filenames_size);
	}

}
