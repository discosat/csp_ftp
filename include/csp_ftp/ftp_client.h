#ifndef CSP_FTP_INCLUDE_FTP_CLIENT_H_
#define CSP_FTP_INCLUDE_FTP_CLIENT_H_

#include <stdint.h>
#include <csp/csp.h>
#include <csp_ftp/ftp_server.h>
#include <csp_ftp/ftp_status.h>

#define FTP_CLIENT_TIMEOUT 10000

void send_ftp_request(csp_conn_t* conn, ftp_request_t* ftp_request);
void send_v1_header(csp_conn_t* conn, ftp_request_type action, const char * filename);

ftp_status_t ftp_download_file(csp_conn_t* conn, int timeout, const char* filename, char** dataout, int* dataout_size);
ftp_status_t ftp_upload_file(csp_conn_t* conn, int timeout, const char* filename, char* datain, int datain_size);
ftp_status_t ftp_list_files(csp_conn_t* conn, int timeout, const char* remote_directory, char** filenames, int* file_count);
ftp_status_t ftp_move_file(csp_conn_t* conn, int timeout, const char* source_file, const char* destination_file);
ftp_status_t ftp_remove_file(csp_conn_t* conn, int timeout, const char* source_file);

#endif /* CSP_FTP_INCLUDE_FTP_CLIENT_H_ */
