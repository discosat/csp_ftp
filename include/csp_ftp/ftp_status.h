#ifndef LIB_CSP_FTP_INCLUDE_FTP_STATUS_H_
#define LIB_CSP_FTP_INCLUDE_FTP_STATUS_H_

#include <csp/csp.h>

typedef enum {
	OK = 1,
    CLIENT_FAILURE = -1,
    SERVER_FAILURE = -2,
    IO_ERROR = -3,
} ftp_status_t;

void ftp_send_status(csp_conn_t * conn, ftp_status_t status);
ftp_status_t ftp_recieve_status(csp_conn_t * conn);
bool ftp_status_is_err(ftp_status_t* status);

#endif /* LIB_CSP_FTP_FTP_INCLUDE_FTP_SERVER_H_ */
