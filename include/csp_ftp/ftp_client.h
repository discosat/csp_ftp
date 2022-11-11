/*
 * vmem_client.h
 *
 *  Created on: Oct 27, 2016
 *      Author: johan
 */

#ifndef CSP_FTP_INCLUDE_FTP_CLIENT_H_
#define CSP_FTP_INCLUDE_FTP_CLIENT_H_

#include <stdint.h>

#define FTP_CLIENT_TIMEOUT 10000

void ftp_download_file(int node, int timeout, const char * filename, int version, char** dataout, int* dataout_size);
void ftp_upload_file(int node, int timeout, const char * filename, int version, char * datain, int datain_size);
void ftp_list_files(int node, int timeout, const char * remote_directory, int version, char** filenames, int* file_count);

#endif /* CSP_FTP_INCLUDE_FTP_CLIENT_H_ */
