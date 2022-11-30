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

static int slash_csp_upload_file(struct slash *slash)
{
    char* local_filename = "";
    char* remote_filename = "";
    unsigned int timeout = FTP_CLIENT_TIMEOUT;
    unsigned int node = slash_dfl_node;

    optparse_t * parser = optparse_new("upload_file", "");
    optparse_add_help(parser);
    optparse_add_unsigned(parser, 'n', "node", "NUM", 0, &node, "node (default = <env>)");
    optparse_add_unsigned(parser, 't', "timout", "NUM", 0, &timeout, "timout for connection (default = FTP_CLIENT_TIMEOUT)");
    optparse_add_string(parser, 'l', "file", "FILE", &local_filename, "locale path to file");
    optparse_add_string(parser, 'r', "file", "FILE", &remote_filename, "remote path to file");

    int argi = optparse_parse(parser, slash->argc - 1, (const char **) slash->argv + 1);
    if (argi < 0) {
        optparse_del(parser);
	    return SLASH_EINVAL;
    }

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
    //void ftp_upload_file(int node, int timeout, const char * filename, int version, char * datain, uint32_t length);
    ftp_upload_file(node, timeout, remote_filename, file_content, file_size);
    printf("Client: Transfer complete\n");

    /* free the memory we used for the buffer */
    free(file_content);

	return SLASH_SUCCESS;
}

slash_command(upload_file, slash_csp_upload_file, NULL, "Upload a file to the target node");

static int slash_csp_download_file(struct slash *slash)
{
	char* local_filename = "";
    char* remote_filename = "";
    unsigned int timeout = FTP_CLIENT_TIMEOUT;
    unsigned int node = slash_dfl_node;

    optparse_t * parser = optparse_new("download_file", "");
    optparse_add_help(parser);
    optparse_add_unsigned(parser, 'n', "node", "NUM", 0, &node, "node (default = <env>)");
    optparse_add_unsigned(parser, 't', "timout", "NUM", 0, &timeout, "timout for connection (default = FTP_CLIENT_TIMEOUT)");
    optparse_add_string(parser, 'r', "file", "FILE", &remote_filename, "remote path to file");
    optparse_add_string(parser, 'l', "file", "FILE", &local_filename, "locale path to file");

    int argi = optparse_parse(parser, slash->argc - 1, (const char **) slash->argv + 1);
    if (argi < 0) {
        optparse_del(parser);
	    return SLASH_EINVAL;
    }
    /**/

    if (strlen(local_filename) == 0) {
        printf("Local file path cannot be empty\n");
        return SLASH_EINVAL;
    }

    if (strlen(remote_filename) == 0) {
        printf("Remote file path cannot be empty\n");
        return SLASH_EINVAL;
    }

    printf("Client: Downloading file\n");
    char *file_content = NULL;
    int file_content_size = 0;

    ftp_download_file(node, timeout, remote_filename, &file_content, &file_content_size);
    printf("Client: Transfer complete\n");

    if (file_content_size == 0) {
        printf("Client Recieved empty file\n");
        return SLASH_SUCCESS;
    }

    FILE* f = fopen(local_filename, "wb");
    fwrite(file_content, file_content_size, 1, f);
    fclose(f);

    printf("Client Saved file to %s\n", local_filename);

    free(file_content);

	return SLASH_SUCCESS;
}

slash_command(download_file, slash_csp_download_file, NULL, "Download a file from the target node");


static int slash_csp_list_file(struct slash *slash)
{
    char* remote_directory = "";
    unsigned int timeout = FTP_CLIENT_TIMEOUT;
    unsigned int node = slash_dfl_node;

    optparse_t * parser = optparse_new("list_file", "");
    optparse_add_help(parser);
    optparse_add_unsigned(parser, 'n', "node", "NUM", 0, &node, "node (default = <env>)");
    optparse_add_unsigned(parser, 't', "timout", "NUM", 0, &timeout, "timout for connection (default = FTP_CLIENT_TIMEOUT)");
    optparse_add_string(parser, 'd', "dir", "FILE", &remote_directory, "remote path to directory");

    int argi = optparse_parse(parser, slash->argc - 1, (const char **) slash->argv + 1);
    if (argi < 0) {
        optparse_del(parser);
	    return SLASH_EINVAL;
    }

    if (strlen(remote_directory) == 0) {
        printf("Remote file path cannot be empty\n");
        return SLASH_EINVAL;
    }
    
    printf("Client: List file\n");

    char* filenames;
    int file_count;
    ftp_status_t status = ftp_list_files(node, timeout, remote_directory, &filenames, &file_count);
    if (ftp_status_is_err(&status)) {
        printf("Client: Transfer failed with error %d\n", status);
        return SLASH_EIO;
    }

    printf("Client: Transfer complete\n");

	printf("Client: Recieved %d files of size %d\n", file_count, file_count * MAX_PATH_LENGTH);
	for (int i = 0; i < file_count; i++) {
		printf("Client: - %s\n", filenames + MAX_PATH_LENGTH * i);
	}

    free(filenames);
	return SLASH_SUCCESS;
}

slash_command(list_file, slash_csp_list_file, NULL, "List all files in directory");

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

    ftp_status_t status = ftp_move_file(node, timeout, source_address, destination_address);
    if (ftp_status_is_err(&status)) {
        printf("Client: Move failed with error %d\n", status);
        return SLASH_EIO;
    }

    printf("Client: Move complete\n");

	return SLASH_SUCCESS;
}

slash_command(move_file, slash_csp_move, NULL, "Move a file");

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

    ftp_status_t status = ftp_remove_file(node, timeout, source_address);
    if (ftp_status_is_err(&status)) {
        printf("Client: Removal failed with error %d\n", status);
        return SLASH_EIO;
    }

    printf("Client: Removal Complete\n");

	return SLASH_SUCCESS;
}

slash_command(remove_file, slash_csp_remove, NULL, "Remove a file");
