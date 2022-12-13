#include <stdio.h>
#include <stdlib.h>
#include <csp/csp.h>
#include <csp/csp_cmp.h>
#include <csp_autoconfig.h>
#include <csp/csp_hooks.h>
#include <sys/types.h>
#include <slash/slash.h>
#include <slash/optparse.h>
#include <slash/dflopt.h>

#include <csp_ftp/ftp_server.h>
#include <csp_ftp/ftp_client.h>
#include <csp_ftp/ftp_perf.h>

static int slash_csp_upload_file(struct slash *slash)
{
    unsigned int node = slash_dfl_node;
    unsigned int timeout = FTP_CLIENT_TIMEOUT;

    optparse_t * parser = optparse_new("upload", "<local-file> <remote-file>");
    optparse_add_help(parser);
    optparse_add_unsigned(parser, 'n', "node", "NUM", 0, &node, "node (default = <env>)");
    optparse_add_unsigned(parser, 't', "timout", "NUM", 0, &timeout, "timout for connection (default = FTP_CLIENT_TIMEOUT)");

    int argi = optparse_parse(parser, slash->argc - 1, (const char **) slash->argv + 1);
    if (argi < 0) {
        optparse_del(parser);
	    return SLASH_EINVAL;
    }

	/* Check if name is present */
	if (++argi >= slash->argc) {
		printf("missing local filename\n");
		return SLASH_EINVAL;
	}

	char * local_filename = slash->argv[argi];

	/* Check if name is present */
	if (++argi >= slash->argc) {
		printf("missing remote filename\n");
		return SLASH_EINVAL;
	}

	char * remote_filename = slash->argv[argi];

    if (strlen(local_filename) == 0) {
        printf("Local file path cannot be empty\n");
        return SLASH_EINVAL;
    }

    if (strlen(remote_filename) == 0) {
        printf("Remote file path cannot be empty\n");
        return SLASH_EINVAL;
    }

    printf("%s\n", local_filename);

    FILE *f = fopen(local_filename, "rb");
    
    if(f == NULL)
        return 1;
    
    fseek(f, 0L, SEEK_END);
    long file_size = ftell(f);
    fseek(f, 0L, SEEK_SET);	
    
    char *file_content = (char*)calloc(file_size, sizeof(char));
    
    if(file_content == NULL)
        return 1;
    
    fread(file_content, sizeof(char), file_size, f);
    fclose(f);
    
    printf("Client: Uploading file of %ld bytes\n", file_size);

    csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, FTP_PORT_SERVER, timeout, CSP_O_RDP | CSP_O_CRC32);
	if (conn == NULL) {
        printf("Client: Failed to connect to server\n");
		return SLASH_EINVAL;
    }

    //void ftp_upload_file(int node, int timeout, const char * filename, int version, char * datain, uint32_t length);
    ftp_upload_file(conn, timeout, remote_filename, file_content, file_size);
    printf("Client: Transfer complete\n");

    /* free the memory we used for the buffer */
    free(file_content);

    csp_close(conn);

	return SLASH_SUCCESS;
}

slash_command_sub(ftp, upload, slash_csp_upload_file, "<loca-file> <remote-file>", "Upload a file to the target node");

static int slash_csp_download_file(struct slash *slash)
{
    unsigned int timeout = FTP_CLIENT_TIMEOUT;
    unsigned int node = slash_dfl_node;

    optparse_t * parser = optparse_new("download", "<remot-file> <local-file>");
    optparse_add_help(parser);
    optparse_add_unsigned(parser, 'n', "node", "NUM", 0, &node, "node (default = <env>)");
    optparse_add_unsigned(parser, 't', "timout", "NUM", 0, &timeout, "timout for connection (default = FTP_CLIENT_TIMEOUT)");

    int argi = optparse_parse(parser, slash->argc - 1, (const char **) slash->argv + 1);
    if (argi < 0) {
        optparse_del(parser);
	    return SLASH_EINVAL;
    }

	/* Check if name is present */
	if (++argi >= slash->argc) {
		printf("missing remote filename\n");
		return SLASH_EINVAL;
	}

	char * remote_filename = slash->argv[argi];

	/* Check if name is present */
	if (++argi >= slash->argc) {
		printf("missing local filename\n");
		return SLASH_EINVAL;
	}

	char * local_filename = slash->argv[argi];

    if (strlen(local_filename) == 0) {
        printf("Local file path cannot be empty\n");
        return SLASH_EINVAL;
    }

    if (strlen(remote_filename) == 0) {
        printf("Remote file path cannot be empty\n");
        return SLASH_EINVAL;
    }

    printf("Client: Downloading file\n");

    // Establish connection
	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, FTP_PORT_SERVER, timeout, CSP_O_RDP | CSP_O_CRC32);
	if (conn == NULL) {
        printf("Client: Failed to connect to server\n");
		return SLASH_EINVAL;
    }

    char *file_content = NULL;
    int file_content_size = 0;
    ftp_download_file(conn, timeout, remote_filename, &file_content, &file_content_size);
    printf("Client: Transfer complete\n");

    if (file_content_size == 0) {
        printf("Client Recieved empty file\n");
        csp_close(conn);
        return SLASH_SUCCESS;
    }

    FILE* f = fopen(local_filename, "wb");
    fwrite(file_content, file_content_size, 1, f);
    fclose(f);

    printf("Client Saved file to %s\n", local_filename);

    free(file_content);

    csp_close(conn);

	return SLASH_SUCCESS;
}

slash_command_sub(ftp,download, slash_csp_download_file, "<remote-dir> <local-dir>", "Download a file from the target node");


static int slash_csp_list_file(struct slash *slash)
{
    unsigned int timeout = FTP_CLIENT_TIMEOUT;
    unsigned int node = slash_dfl_node;

    optparse_t * parser = optparse_new("ls", "<remote-dir>");
    optparse_add_help(parser);
    optparse_add_unsigned(parser, 'n', "node", "NUM", 0, &node, "node (default = <env>)");
    optparse_add_unsigned(parser, 't', "timout", "NUM", 0, &timeout, "timout for connection (default = FTP_CLIENT_TIMEOUT)");

    int argi = optparse_parse(parser, slash->argc - 1, (const char **) slash->argv + 1);
    if (argi < 0) {
        optparse_del(parser);
	    return SLASH_EINVAL;
    }

	/* Check if name is present */
	if (++argi >= slash->argc) {
		printf("missing remode directory\n");
		return SLASH_EINVAL;
	}

	char * remote_directory = slash->argv[argi];

    if (strlen(remote_directory) == 0) {
        printf("Remote file path cannot be empty\n");
        return SLASH_EINVAL;
    }
    
    printf("Client: List file\n");

    // Establish connection
	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, FTP_PORT_SERVER, timeout, CSP_O_RDP | CSP_O_CRC32);
	if (conn == NULL) {
        printf("Client: Failed to connect to server\n");
		return SLASH_EINVAL;
    }

    char* filenames;
    int file_count;
    ftp_status_t status = ftp_list_files(conn, timeout, remote_directory, &filenames, &file_count);
    if (ftp_status_is_err(&status)) {
        printf("Client: Transfer failed with error %d\n", status);
        csp_close(conn);
        return SLASH_EIO;
    }

    csp_close(conn);

    printf("Client: Transfer complete\n");

	printf("Client: Recieved %d files of size %d\n", file_count, file_count * MAX_PATH_LENGTH);
	for (int i = 0; i < file_count; i++) {
		printf("Client: - %s\n", filenames + MAX_PATH_LENGTH * i);
	}

    free(filenames);
	return SLASH_SUCCESS;
}

slash_command_sub(ftp, ls, slash_csp_list_file, "[remote-dir]", "List all files in directory");

static int slash_csp_move(struct slash *slash)
{
    char* source_address = "";
    char* destination_address = "";
    unsigned int timeout = FTP_CLIENT_TIMEOUT;
    unsigned int node = slash_dfl_node;

    optparse_t * parser = optparse_new("move_file", "");
    optparse_add_help(parser);
    optparse_add_unsigned(parser, 'n', "node", "NUM", 0, &node, "node (default = <env>)");
    optparse_add_unsigned(parser, 't', "timout", "NUM", 0, &timeout, "timout for connection (default = FTP_CLIENT_TIMEOUT)");
    optparse_add_string(parser, 's', "source", "FILE", &source_address, "remote source file");
    optparse_add_string(parser, 'd', "destination", "FILE", &destination_address, "remote destination file");

    int argi = optparse_parse(parser, slash->argc - 1, (const char **) slash->argv + 1);
    if (argi < 0) {
        optparse_del(parser);
	    return SLASH_EINVAL;
    }

    if (strlen(source_address) == 0) {
        printf("Source file path cannot be empty\n");
        return SLASH_EINVAL;
    }

    if (strlen(destination_address) == 0) {
        printf("Destination file path cannot be empty\n");
        return SLASH_EINVAL;
    }
    
    printf("Client: Move file\n");

    // Establish connection
	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, FTP_PORT_SERVER, timeout, CSP_O_RDP | CSP_O_CRC32);
	if (conn == NULL) {
        printf("Client: Failed to connect to server\n");
		return SLASH_EINVAL;
    }

    ftp_status_t status = ftp_move_file(conn, timeout, source_address, destination_address);
    if (ftp_status_is_err(&status)) {
        printf("Client: Move failed with error %d\n", status);
        csp_close(conn);
        return SLASH_EIO;
    }

    printf("Client: Move complete\n");

    csp_close(conn);

	return SLASH_SUCCESS;
}

slash_command_sub(ftp, move_file, slash_csp_move, NULL, "Move a file");

static int slash_csp_remove(struct slash *slash)
{
    char* source_address = "";
    unsigned int timeout = FTP_CLIENT_TIMEOUT;
    unsigned int node = slash_dfl_node;

    optparse_t * parser = optparse_new("remove_file", "");
    optparse_add_help(parser);
    optparse_add_unsigned(parser, 'n', "node", "NUM", 0, &node, "node (default = <env>)");
    optparse_add_unsigned(parser, 't', "timout", "NUM", 0, &timeout, "timout for connection (default = FTP_CLIENT_TIMEOUT)");
    optparse_add_string(parser, 's', "source", "FILE", &source_address, "remote file to remove");

    int argi = optparse_parse(parser, slash->argc - 1, (const char **) slash->argv + 1);
    if (argi < 0) {
        optparse_del(parser);
	    return SLASH_EINVAL;
    }

    if (strlen(source_address) == 0) {
        printf("Source file path cannot be empty\n");
        return SLASH_EINVAL;
    }

    printf("Client: Remove file\n");

    // Establish connection
	csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, FTP_PORT_SERVER, timeout, CSP_O_RDP | CSP_O_CRC32);
	if (conn == NULL) {
        printf("Client: Failed to connect to server\n");
		return SLASH_EINVAL;
    }

    ftp_status_t status = ftp_remove_file(conn, timeout, source_address);
    if (ftp_status_is_err(&status)) {
        printf("Client: Removal failed with error %d\n", status);
        csp_close(conn);
        return SLASH_EIO;
    }

    printf("Client: Removal Complete\n");

    csp_close(conn);
	return SLASH_SUCCESS;
}

slash_command_sub(ftp, remove_file, slash_csp_remove, NULL, "Remove a file");


static int slash_csp_perf_upload(struct slash *slash) {
    unsigned int timeout = FTP_CLIENT_TIMEOUT;
    unsigned int node = slash_dfl_node;
    perf_header_t header;
    header.chunk_size = 10;
    header.mtu = FTP_SERVER_MTU;
    header.n = 10;
    header.protocole = NORMAL;

    unsigned int chunk_size = 10;
    unsigned int mtu = FTP_SERVER_MTU;
    unsigned int n = 10;
    unsigned int protocole = NORMAL;
    char* config = NULL;

    optparse_t * parser = optparse_new("perf_upload", "");
    optparse_add_help(parser);
    optparse_add_unsigned(parser, 'n', "node", "NUM", 0, &node, "node (default = <env>)");
    optparse_add_unsigned(parser, 't', "timout", "NUM", 0, &timeout, "timout for connection (default = FTP_CLIENT_TIMEOUT)");

    optparse_add_unsigned(parser, 'c', "count", "NUM", 0,  &n, "Number of packages to send");
    optparse_add_unsigned(parser, 's', "size", "NUM", 0, &chunk_size, "Size of each package");
    optparse_add_unsigned(parser, 'm', "mtu", "NUM", 0, &mtu, "Minimum transfer unit");
    optparse_add_unsigned(parser, 'p', "protocole", "NUM", 0, &protocole, "Protocole either normal (0) or sfp (1)");
    optparse_add_string(parser, 'f', "file", "FILE", &config, "Path to configuration file");

    int argi = optparse_parse(parser, slash->argc - 1, (const char **) slash->argv + 1);
    if (argi < 0) {
        optparse_del(parser);
	    return SLASH_EINVAL;
    }

    csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, FTP_PORT_SERVER, timeout, CSP_O_RDP | CSP_O_CRC32);
	if (conn == NULL) {
        printf("Client: Failed to connect to server\n");
		return CLIENT_FAILURE;
    }

    if (config != NULL) {
        printf("Client: Running using config file at %s", config);
        FILE* fp = fopen(config, "r");
        if (fp == NULL) {
            printf("Client: Failed to open config file\n");
            return SLASH_EINVAL;
        }

        char * line = NULL;
        size_t len = 0;
        ssize_t read;

        // Read each line as a seperate test
        while ((read = getline(&line, &len, fp)) != -1) {
            // Set default values
            header.chunk_size = chunk_size;
            header.mtu = mtu;
            header.n = n;
            header.protocole = protocole;

            int i0 = 0;
            int i1 = 0;

            int param_i = 0;
            // Parse a space seperated list of parameters
            // The order of parameters is used to determing what it is used for
            while (i1 <= read)  {
                if ((line[i1] == ' ' || line[i1] == '\n' || i1 == read) && i1 - i0 > 0) {
                    char* sub_string = calloc(1, i1 - i0);
                    memcpy(sub_string, &line[i0], i1 - i0);

                    i0 = i1;
                    int param = atoi(sub_string);
                    free(sub_string);

                    if (param_i == 0) {
                        header.n = param;
                    } else if (param_i == 1) {
                        header.chunk_size = param;
                    } else if (param_i == 2) {
                        header.mtu = param;
                    } else if (param_i == 3) {
                        header.protocole = param;
                    }

                    param_i += 1;
                }
                i1 += 1;
            }

            // Run test
            printf("Client: Upload test (size = %d, n = %d, mtu = %d, protocole = %d)\n", header.chunk_size, header.n, header.mtu, header.protocole);

            ftp_request_t request;
            request.version = 1;
            request.type = FTP_PERFORMANCE_UPLOAD;
            request.perf_header = header;
            send_ftp_request(conn, &request);

            perf_upload(conn, &header);
        }

        fclose(fp);
    } else {
        printf("Client: Upload test (size = %d, n = %d, mtu = %d, protocole = %d)\n", header.chunk_size, header.n, header.mtu, header.protocole);

        header.chunk_size = chunk_size;
        header.mtu = mtu;
        header.n = n;
        header.protocole = protocole;

        ftp_request_t request;
        request.version = 1;
        request.type = FTP_PERFORMANCE_DOWNLOAD;
        request.perf_header = header;
        send_ftp_request(conn, &request);

        perf_upload(conn, &header);
    }
    
    printf("Client: Upload test done\n");

    csp_close(conn);

    return SLASH_SUCCESS;
}

slash_command_sub(ftp, pupload, slash_csp_perf_upload, NULL, "Upload a varying amount of data");

static int slash_csp_perf_download(struct slash *slash) {
    unsigned int timeout = FTP_CLIENT_TIMEOUT;
    unsigned int node = slash_dfl_node;
    perf_header_t header;
    header.chunk_size = 10;
    header.mtu = FTP_SERVER_MTU;
    header.n = 10;
    header.protocole = NORMAL;

    unsigned int chunk_size = 10;
    unsigned int mtu = FTP_SERVER_MTU;
    unsigned int n = 10;
    unsigned int protocole = NORMAL;
    char* config = NULL;

    optparse_t * parser = optparse_new("perf_download", "");
    optparse_add_help(parser);
    optparse_add_unsigned(parser, 'n', "node", "NUM", 0, &node, "node (default = <env>)");
    optparse_add_unsigned(parser, 't', "timout", "NUM", 0, &timeout, "timout for connection (default = FTP_CLIENT_TIMEOUT)");

    optparse_add_unsigned(parser, 'c', "count", "NUM", 0,  &n, "Number of packages to send");
    optparse_add_unsigned(parser, 's', "size", "NUM", 0, &chunk_size, "Size of each package");
    optparse_add_unsigned(parser, 'm', "mtu", "NUM", 0, &mtu, "Minimum transfer unit");
    optparse_add_unsigned(parser, 'p', "protocole", "NUM", 0, &protocole, "Protocole either normal (0) or sfp (1)");
    optparse_add_string(parser, 'f', "file", "FILE", &config, "Path to configuration file");

    int argi = optparse_parse(parser, slash->argc - 1, (const char **) slash->argv + 1);
    if (argi < 0) {
        optparse_del(parser);
	    return SLASH_EINVAL;
    }

    csp_conn_t * conn = csp_connect(CSP_PRIO_HIGH, node, FTP_PORT_SERVER, timeout, CSP_O_RDP | CSP_O_CRC32);
	if (conn == NULL) {
        printf("Client: Failed to connect to server\n");
		return CLIENT_FAILURE;
    }

    if (config != NULL) {
        printf("Client: Running using config file at %s", config);
        FILE* fp = fopen(config, "r");
        if (fp == NULL) {
            printf("Client: Failed to open config file\n");
            return SLASH_EINVAL;
        }

        char * line = NULL;
        size_t len = 0;
        ssize_t read;

        // Read each line as a seperate test
        while ((read = getline(&line, &len, fp)) != -1) {
            // Set default values
            header.chunk_size = chunk_size;
            header.mtu = mtu;
            header.n = n;
            header.protocole = protocole;

            int i0 = 0;
            int i1 = 0;

            int param_i = 0;
            // Parse a space seperated list of parameters
            // The order of parameters is used to determing what it is used for
            while (i1 <= read)  {
                if ((line[i1] == ' ' || line[i1] == '\n' || i1 == read) && i1 - i0 > 0) {
                    char* sub_string = calloc(1, i1 - i0);
                    memcpy(sub_string, &line[i0], i1 - i0);

                    i0 = i1;
                    int param = atoi(sub_string);
                    free(sub_string);

                    if (param_i == 0) {
                        header.n = param;
                    } else if (param_i == 1) {
                        header.chunk_size = param;
                    } else if (param_i == 2) {
                        header.mtu = param;
                    } else if (param_i == 3) {
                        header.protocole = param;
                    }

                    param_i += 1;
                }
                i1 += 1;
            }

            // Run test
            printf("Client: Download test (size = %d, n = %d, mtu = %d, protocole = %d)\n", header.chunk_size, header.n, header.mtu, header.protocole);

            ftp_request_t request;
            request.version = 1;
            request.type = FTP_PERFORMANCE_UPLOAD;
            request.perf_header = header;
            send_ftp_request(conn, &request);

            perf_download(conn, &header);
        }

        fclose(fp);
    } else {
        printf("Client: Download test (size = %d, n = %d, mtu = %d, protocole = %d)\n", header.chunk_size, header.n, header.mtu, header.protocole);

        header.chunk_size = chunk_size;
        header.mtu = mtu;
        header.n = n;
        header.protocole = protocole;

        ftp_request_t request;
        request.version = 1;
        request.type = FTP_PERFORMANCE_UPLOAD;
        request.perf_header = header;
        send_ftp_request(conn, &request);

        perf_download(conn, &header);
    }
    
    printf("Client: Download test done\n");

    csp_close(conn);

    return SLASH_SUCCESS;
}

slash_command_sub(ftp, pdownload, slash_csp_perf_download,NULL, "Download a varying amount of data");
