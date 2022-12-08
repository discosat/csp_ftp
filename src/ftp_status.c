#include <csp_ftp/ftp_status.h>
#include <csp_ftp/ftp_server.h>

void ftp_send_status(csp_conn_t * conn, ftp_status_t status) {
    csp_packet_t * packet = csp_buffer_get(sizeof(ftp_status_t));
	if (packet == NULL)
		return;

	ftp_status_t * request = (void *) packet->data;
	*request = status;
	packet->length = sizeof(ftp_status_t);

	/* Send request */
	csp_send(conn, packet);
}

ftp_status_t ftp_recieve_status(csp_conn_t * conn) {
    csp_packet_t* packet = csp_read(conn, FTP_SERVER_TIMEOUT);
    if (packet == NULL) {
        csp_buffer_free(packet);
        return CLIENT_FAILURE;
    }

    ftp_status_t * request = (void *) packet->data;
    ftp_status_t status = *request;

    csp_buffer_free(packet);

    return status;
}

bool ftp_status_is_err(ftp_status_t* status) {
    return *status < 0;
}