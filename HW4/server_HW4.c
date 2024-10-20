#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>


#define PACKET_SIZE 1000
#define S_WAIT "[*] wait for client ... "
#define S_CONNECT "[*] wait for client"
#define S_CLOSE "[*] session closed"

void error_handling(char *message);


int main(int argc, char *argv[])
{
    FILE *fp;
    int file_size;
    int i = 0, j = 0;
    int serv_sock, clnt_sock = -1;
    struct sockaddr_in serv_addr, clnt_addr;
    socklen_t clnt_addr_size;
    char buf[PACKET_SIZE];
    char s_command[1000];
    int bytes;
    int original_stdin, original_stdout;

    // ���� ���� 2�� �ƴ� �� ���� ó��
    if (argc != 2) {
        printf("Usage : %s <port>\n", argv[0]);
    }

    
    serv_sock = socket(PF_INET, SOCK_STREAM, 0);
    if (serv_sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    if (bind(serv_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
        error_handling("bind() error");

    if (listen(serv_sock, 5) == -1)
        error_handling("listen() error");

    if (clnt_sock < 0) {
        clnt_addr_size = sizeof(clnt_addr);
    }

    // ������ ���� STDIN, STDOUT ���� ����
    original_stdin = dup(STDOUT_FILENO);
    original_stdout = dup(STDIN_FILENO);

    
    while (1) {
        // "wait for clients" ���
        puts(S_WAIT);

        // Ŭ���̾�Ʈ���� ���� ����� ����
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        // Ŭ���̾�Ʈ���� ������ �� �� ����� ���� ó��
        if (clnt_sock == -1) {
            error_handling("accept() error");
        }
        // " [*] client connected " ���
        puts(S_CONNECT);

        // clnt_sock ���� ��ũ���͸� ǥ�� ��/������� ����
        // ���� ���μ����� ǥ��������� Ŭ���̾�Ʈ ���� ���� ��ũ���Ϳ� redirect��
        dup2(clnt_sock, STDIN_FILENO);
        dup2(clnt_sock, STDOUT_FILENO);

        
        
        // Get Total size of the code (argv[3] of client process) 
        read(clnt_sock, buf, sizeof(buf));
        file_size = atoi(buf);
        // Read the code, write it down to "filename_code" 

        // ������ Ŭ���̾�Ʈ�κ��� �ڵ����� ���� Ƚ�� = ��ü ���� ������ / ���ۻ����� + 1
        int totReadCnt = file_size / PACKET_SIZE + 1;

        // Ŭ���̾�Ʈ�κ��� ���� �ڵ� ������ ������ "code_write_test.c"
        fp = fopen("code_write_test.c", "w");

        i = 0;
        while (i++ < totReadCnt) {
            // Ŭ���̾�Ʈ�κ��� ���� �ڵ� ���� ������ buf�� ����
            bytes = read(clnt_sock, buf, sizeof(buf));
            if (buf[bytes - 1] == '\0') {
                break;
            }
            // buf�� ���� "code_write_test.c"�� ����
            fwrite(buf, sizeof(char), bytes, fp);
        }
        // "code_write_test.c" ���� ������ fclose
        fclose(fp);
        
        // system call�� �ڵ� ������ �� ����
        system("gcc -o code code_write_test.c");
        system("./code");

        // ���� �� ���� ����
        close(clnt_sock);
        
        // �ٽ� ǥ�� �Է°� ǥ�� ����� ���� ��ũ���ͷ� ���� ����
        dup2(original_stdin, STDIN_FILENO);
        dup2(original_stdout, STDOUT_FILENO);
        puts(S_CLOSE);
    }

    return (0);
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}