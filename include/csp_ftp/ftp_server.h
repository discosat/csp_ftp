#ifndef LIB_CSP_FTP_INCLUDE_FTP_SERVER_H_
#define LIB_CSP_FTP_INCLUDE_FTP_SERVER_H_

#include <csp/csp.h>

#define FTP_SERVER_TIMEOUT 10000
#define FTP_SERVER_MTU 200
#define FTP_PORT_SERVER 13
#define FTP_VERSION 1

#define MAX_PATH_LENGTH 256

typedef enum {
	// Upload a file
	FTP_SERVER_UPLOAD,
	// Download a file
	FTP_SERVER_DOWNLOAD,
	// List files in directory
	FTP_SERVER_LIST,
	FTP_MOVE,
	FTP_REMOVE,
	FTP_PERFORMANCE_UPLOAD,
	FTP_PERFORMANCE_DOWNLOAD,
} ftp_request_type;

typedef enum {
	NORMAL = 0,
	SFP = 1,
} transport_protocole_t;

typedef struct {
	uint16_t n;
	uint16_t chunk_size;
	uint16_t mtu;
	transport_protocole_t protocole;
} __attribute__((packed)) perf_header_t;

// Header for any request to the server
typedef struct {
	uint8_t version;
	uint8_t type;
	union {
		struct {
            // Path to the file
			char address[MAX_PATH_LENGTH];
            // Length of the path
            uint16_t length;
		} v1;
		perf_header_t perf_header;
	};
} __attribute__((packed)) ftp_request_t;

typedef struct {
	uint32_t vaddr;
	uint32_t size;
	uint8_t vmem_id;
	uint8_t type;
	char name[5];
} __attribute__((packed)) ftp_list_t;

void ftp_server_handler(csp_conn_t * conn);
void handle_server_download(csp_conn_t * conn, ftp_request_t * request);
void handle_server_upload(csp_conn_t * conn, ftp_request_t * request);
void handle_server_list(csp_conn_t * conn, ftp_request_t * request);
void handle_server_move(csp_conn_t * conn, ftp_request_t * request);
void handle_server_remove(csp_conn_t * conn, ftp_request_t * request);
void ftp_server_loop(void * param);

#endif /* LIB_CSP_FTP_FTP_INCLUDE_FTP_SERVER_H_ */
