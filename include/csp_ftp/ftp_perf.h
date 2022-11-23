#ifndef LIB_CSP_FTP_INCLUDE_FTP_PERF_H_
#define LIB_CSP_FTP_INCLUDE_FTP_PERF_H_

#include <csp/csp.h>
#include <csp_ftp/ftp_server.h>

void perf_download(csp_conn_t * conn, ftp_request_t * request);
void perf_upload(csp_conn_t * conn, ftp_request_t * request);

#endif /* LIB_CSP_FTP_FTP_INCLUDE_FTP_PERF_H_ */
