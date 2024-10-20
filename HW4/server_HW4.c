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

    // 인자 값이 2가 아닐 때 오류 처리
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

    // 서버의 기존 STDIN, STDOUT 원본 저장
    original_stdin = dup(STDOUT_FILENO);
    original_stdout = dup(STDIN_FILENO);

    
    while (1) {
        // "wait for clients" 출력
        puts(S_WAIT);

        // 클라이언트와의 연결 명시적 수락
        clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
        // 클라이언트와의 연결이 안 될 경우의 에러 처리
        if (clnt_sock == -1) {
            error_handling("accept() error");
        }
        // " [*] client connected " 출력
        puts(S_CONNECT);

        // clnt_sock 파일 디스크립터를 표준 입/출력으로 복사
        // 서버 프로세스의 표준입출력을 클라이언트 소켓 파일 디스크립터에 redirect함
        dup2(clnt_sock, STDIN_FILENO);
        dup2(clnt_sock, STDOUT_FILENO);

        
        
        // Get Total size of the code (argv[3] of client process) 
        read(clnt_sock, buf, sizeof(buf));
        file_size = atoi(buf);
        // Read the code, write it down to "filename_code" 

        // 서버가 클라이언트로부터 코드파일 받을 횟수 = 전체 파일 사이즈 / 버퍼사이즈 + 1
        int totReadCnt = file_size / PACKET_SIZE + 1;

        // 클라이언트로부터 받은 코드 파일을 저장할 "code_write_test.c"
        fp = fopen("code_write_test.c", "w");

        i = 0;
        while (i++ < totReadCnt) {
            // 클라이언트로부터 받은 코드 파일 내용을 buf로 받음
            bytes = read(clnt_sock, buf, sizeof(buf));
            if (buf[bytes - 1] == '\0') {
                break;
            }
            // buf의 내용 "code_write_test.c"에 저장
            fwrite(buf, sizeof(char), bytes, fp);
        }
        // "code_write_test.c" 파일 포인터 fclose
        fclose(fp);
        
        // system call로 코드 컴파일 및 실행
        system("gcc -o code code_write_test.c");
        system("./code");

        // 실행 후 소켓 닫음
        close(clnt_sock);
        
        // 다시 표준 입력과 표준 출력을 기존 디스크립터로 원상 복구
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