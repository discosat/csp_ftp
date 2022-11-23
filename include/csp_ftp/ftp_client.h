#ifndef CSP_FTP_INCLUDE_FTP_CLIENT_H_
#define CSP_FTP_INCLUDE_FTP_CLIENT_H_

#include <stdint.h>
#include <csp/csp.h>

#define FTP_CLIENT_TIMEOUT 10000

// void send_v1_header(csp_conn_t* conn, ftp_request_type action, char* filename);
// void send_ftp_request(csp_conn_t* conn, ftp_request_t* ftp_request);

void ftp_download_file(int node, int timeout, const char * filename, int version, char** dataout, int* dataout_size);
void ftp_upload_file(int node, int timeout, const char * filename, int version, char * datain, int datain_size);
void ftp_list_files(int node, int timeout, const char * remote_directory, int version, char** filenames, int* file_count);

#endif /* CSP_FTP_INCLUDE_FTP_CLIENT_H_ */
