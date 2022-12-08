#include <stdio.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <csp/csp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <csp/csp.h>
#include <csp/arch/csp_time.h>

#include <csp_ftp/ftp_server.h>
#include <csp_ftp/ftp_perf.h>

void perf_download(csp_conn_t * conn, perf_header_t* header) {
    uint32_t t0 = csp_get_ms();

    int bytes_recieved = 0;

    for (int i = 0; i< header->n; i++) {
        switch (header ->protocole) {
            case NORMAL:
                {
                    csp_packet_t* packet = csp_read(conn, FTP_SERVER_TIMEOUT);
                    bytes_recieved += packet->length;
                    csp_buffer_free(packet);
                }
                break;
            case SFP: 
                {
                    char* data = NULL;
                    int data_size = 0;
                    int err = csp_sfp_recv(conn, (void**) &data, &data_size, FTP_SERVER_TIMEOUT);
                    if (err != CSP_ERR_NONE) {
                        printf("Failed to recieve chunk with error %d\n", err);
                        return;
                    }
                    bytes_recieved += data_size;
                    free(data);
                }
                break;
            default:
                break;
        }
    }

    uint32_t t1 = csp_get_ms();

    printf("Received %d Bytes in %d ms\n", bytes_recieved, t1 - t0);
}

void perf_upload(csp_conn_t * conn, perf_header_t* header) {
    uint32_t t0 = csp_get_ms();

    int bytes_recieved = 0;

    char* payload = malloc(header->chunk_size);

    for (int i = 0; i< header->n; i++) {
        switch (header ->protocole) {
            case NORMAL:
                {
                    csp_packet_t * packet = csp_buffer_get(header->chunk_size);
                    if (packet == NULL)
                        return;

                    char* data = (void *) packet->data;
                    memcpy(data, payload, header->chunk_size);
                    packet->length = header->chunk_size;

                    /* Send request */
                    csp_send(conn, packet);
                    bytes_recieved += header->chunk_size;
                }
                break;
            case SFP:
                {
                    int err = csp_sfp_send(conn, payload, header->chunk_size, header->mtu, FTP_SERVER_TIMEOUT);
                    if (err != CSP_ERR_NONE) {
                        printf("Failed to send chunk with error %d\n", err);
                        return;
                    }
                    bytes_recieved += header->chunk_size;
                }
                break;
            default:
                break;
        }
    }

    free(payload);

    uint32_t t1 = csp_get_ms();

    printf("Send %d Bytes in %d ms\n", bytes_recieved, t1 - t0);

}
