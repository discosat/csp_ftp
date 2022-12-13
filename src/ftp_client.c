#include <stdio.h>
#include <csp/arch/csp_time.h>
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>

#include <csp_ftp/ftp_client.h>
#include <csp_ftp/ftp_server.h>
#include <csp_ftp/ftp_status.h>

void send_ftp_request(csp_conn_t* conn, ftp_request_t* ftp_request) {
	csp_packet_t * packet = csp_buffer_get(sizeof(ftp_request_t));
	if (packet == NULL)
		return;

	ftp_request_t * request = (void *) packet->data;
	*request = *ftp_request;
	packet->length = sizeof(ftp_request_t);

	// Send request
	csp_send(conn, packet);
}

void send_v1_header(csp_conn_t* conn, ftp_request_type action, const char * filename) {
	ftp_request_t request;
	request.version = 1;
	request.type = action;

	size_t filename_len = strlen(filename);
	if( filename_len > MAX_PATH_LENGTH) {
		printf("Client: Exceeded characher limit of %u with path of %d charactes for path %s", MAX_PATH_LENGTH, filename_len, filename);
        return;
	}

	for (int i = 0; i < MAX_PATH_LENGTH; i++) {
		request.v1.address[i] = 0;
	};

	strncpy(request.v1.address, filename, filename_len);
	// This will only fail if MAX_PATH_LENGTH > UINT16_MAX
	request.v1.length = filename_len;

	send_ftp_request(conn, &request);
}

ftp_status_t ftp_download_file(csp_conn_t* conn, int timeout, const char * filename, char** dataout, int* dataout_size)
{
	*dataout = NULL;
	*dataout_size = 0;

	uint32_t time_begin = csp_get_ms();

	printf("Client: %s\n", filename);

	// Send header file
	send_v1_header(conn, FTP_SERVER_DOWNLOAD, filename);

	// Check that the header was recieved propperly
	ftp_status_t request_status = ftp_recieve_status(conn);
	if (ftp_status_is_err(&request_status)) {
		printf("Client: Request failed with error code %d\n", request_status);
		csp_close(conn);
		return request_status;
	}

	// Recieve content
	int err = csp_sfp_recv(conn, (void**) dataout, dataout_size, FTP_CLIENT_TIMEOUT);
	if (err != CSP_ERR_NONE) {
		printf("Client: Failed to recieve file with error %d\n", err);
		csp_close(conn);
		return CLIENT_FAILURE;
	}

	csp_close(conn);

	uint32_t time_total = csp_get_ms() - time_begin;
	printf("Client: Downloaded %u bytes in %.03f s at %u Bps\n", (unsigned int) *dataout_size, time_total / 1000.0, (unsigned int) (*dataout_size / ((float)time_total / 1000.0)) );

	return OK;
}

ftp_status_t ftp_upload_file(csp_conn_t* conn, int timeout, const char * filename, char * datain, int datain_size)
{
	uint32_t time_begin = csp_get_ms();

	// Send header file
	send_v1_header(conn, FTP_SERVER_UPLOAD, filename);

	// Check that header was recieved propperly
	ftp_status_t header_status = ftp_recieve_status(conn);
	if (ftp_status_is_err(&header_status)) {
		printf("Client: Recieved error code %d after sending header\n", header_status);
		csp_close(conn);
		return header_status;
	}

	// Send content
	int err = csp_sfp_send(conn, datain, datain_size, FTP_SERVER_MTU, FTP_CLIENT_TIMEOUT);
	if (err != CSP_ERR_NONE) {
		printf("Client: Failed to send file with error %d\n", err);
		return CLIENT_FAILURE;
	}

	// Check that body was send propperly
	ftp_status_t body_status = ftp_recieve_status(conn);
	if (ftp_status_is_err(&body_status)) {
		printf("Client: Recieved error code %d after sending content\n", body_status);
		csp_close(conn);
		return body_status;
	}

	csp_close(conn);

	uint32_t time_total = csp_get_ms() - time_begin;
	printf("Client: Uploaded %u bytes in %.03f s at %u Bps\n", (unsigned int) datain_size, time_total / 1000.0, (unsigned int) (datain_size / ((float)time_total / 1000.0)) );

	return OK;
}

ftp_status_t ftp_list_files(csp_conn_t* conn, int timeout, const char * remote_directory, char** filenames, int* file_count)
{
	*filenames = "";
	*file_count = 0;

	// Validate remote path
	size_t remote_directory_len = strlen(remote_directory);
	if( remote_directory_len > MAX_PATH_LENGTH) {
		printf("Client: Exceeded characher limit of %u with path of %d charactes for path %s", MAX_PATH_LENGTH, remote_directory_len, remote_directory);
        return CLIENT_FAILURE;
	}

	// Send header file
	send_v1_header(conn, FTP_SERVER_LIST, remote_directory);

	// Check that header was recieved propperly
	ftp_status_t header_status = ftp_recieve_status(conn);
	if (ftp_status_is_err(&header_status)) {
		printf("Client: Recieved error code %d after sending header\n", header_status);
		csp_close(conn);
		return header_status;
	}

	// Recieve metadata bout files
	int *num_files = NULL;
	int file_count_size;
	int err = csp_sfp_recv(conn,(void**) &num_files, &file_count_size, FTP_CLIENT_TIMEOUT);
	if (err != CSP_ERR_NONE) {
		printf("Client: Failed to recieve number with error %d\n", err);
		csp_close(conn);
		return CLIENT_FAILURE;
	}

	// handle edge case where there is nothing in the directory
	if (num_files == NULL || *num_files == 0) {
		printf("Client: directory is empty\n");
		free(num_files);
		csp_close(conn);
		return CLIENT_FAILURE;
	}

	*file_count = *num_files;
	free(num_files);

	// Recieve list of files
	size_t expected_filenames_size = sizeof(char) * MAX_PATH_LENGTH * *file_count;
	int filenames_size = 0;
	err = csp_sfp_recv(conn, (void**) filenames, &filenames_size, FTP_CLIENT_TIMEOUT);
	csp_close(conn);

	if (err != CSP_ERR_NONE) {
		*file_count = 0;
		printf("Client: Failed to recieve filenames with error %d\n", err);
		return CLIENT_FAILURE;
	}

	if (filenames == NULL) {
		*file_count = 0;
		printf("Client: Failed to recieve filenames\n");
		return CLIENT_FAILURE;
	}

	if (filenames_size != expected_filenames_size) {
		printf("Client: recieved %d Bytes but expected %d", filenames_size, expected_filenames_size);
		return CLIENT_FAILURE;
	}

	return OK;
}

ftp_status_t ftp_move_file(csp_conn_t* conn, int timeout, const char* source_file, const char* destination_file) {
	size_t source_file_len = strlen(source_file);
	if( source_file_len > MAX_PATH_LENGTH) {
		printf("Client: Exceeded characher limit of %u with path of %d charactes for path %s", MAX_PATH_LENGTH, source_file_len, source_file);
        return CLIENT_FAILURE;
	}

	size_t destination_file_len = strlen(destination_file);
	if( destination_file_len > MAX_PATH_LENGTH) {
		printf("Client: Exceeded characher limit of %u with path of %d charactes for path %s", MAX_PATH_LENGTH, destination_file_len, destination_file);
        return CLIENT_FAILURE;
	}

	// Send header file
	send_v1_header(conn, FTP_MOVE, source_file);

	int err = csp_sfp_send(conn, destination_file, destination_file_len, FTP_SERVER_MTU, FTP_CLIENT_TIMEOUT);
	if (err != CSP_ERR_NONE) {
		printf("Client: Failed to send with error code %d\n", err);
		return CLIENT_FAILURE;
	}

	ftp_status_t status = ftp_recieve_status(conn);
	if (ftp_status_is_err(&status)) {
		printf("Client: Recieved error code %d after sending header\n", status);
		csp_close(conn);
		return status;
	}

	return OK;
}

ftp_status_t ftp_remove_file(csp_conn_t* conn, int timeout, const char* source_file) {
	size_t source_file_len = strlen(source_file);
	if( source_file_len > MAX_PATH_LENGTH)
        return CLIENT_FAILURE;

	// Send header file
	send_v1_header(conn, FTP_REMOVE, source_file);

	return OK;
}