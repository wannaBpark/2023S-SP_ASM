#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

//송수신 버퍼 사이즈
#define PACKET_SIZE 1000

void* p_read_from_server();
void* p_write_to_server();
void error_handling(char *message);

pthread_t p_thread[2];
int isEnd = 1;
char buf[PACKET_SIZE];
struct sockaddr_in serv_addr;
int sock, a, b;
int bytes;

int main(int argc, char *argv[])
{
    
    long file_size;
    FILE* fp_code;
    char* path = argv[3];
    char msg[2];
    char on[2] = "1";
    int str_len;
    int status, thr_id1, thr_id2;

    if (argc != 4) {
        printf("Usage : <PATH> <IP> <port> <filename> \n");
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock == -1)
        error_handling("socket() error");

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    // 서버 소켓과의 연결
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1) {
        error_handling("connect() error");
    }

    // 보낼 코드 파일을 엶
    fp_code = fopen(path, "r");
    if (fp_code == NULL) {
        error_handling("Error caused from File open");
    }

    // fssek파일 크기를 ftell함수로 계산하여 서버에게 전송
    fseek(fp_code, 0, SEEK_END);
    file_size = ftell(fp_code);
    fseek(fp_code, 0, SEEK_SET);
    snprintf(buf, sizeof(buf), "%d", file_size);
    write(sock, buf, sizeof(buf));
    
    // 코드 파일 전송
    while ( (bytes = fread((void*)buf, sizeof(char), PACKET_SIZE, fp_code)) > 0) {
        send(sock, buf, bytes, 0);
    }

    // 코드 파일 전송이 끝났음을 \0으로 표기
    write(sock, '\0', 1);
    fclose(fp_code);

    // Create two threads for read from server, send data to server
    thr_id1 = pthread_create(&p_thread[0], NULL, p_read_from_server, NULL);
    thr_id2 = pthread_create(&p_thread[1],NULL,  p_write_to_server, NULL);

    pthread_join(p_thread[0], (void**)&status);
    pthread_join(p_thread[1], (void**)&status);
    
    // close(sock);

    
    return (0);
}

// 서버로부터 데이터를 읽는 함수
void* p_read_from_server()
{
    char s[9] = "[remote] ";
    char s2[] = "[*] session closed\n";
    while ( (isEnd = read(sock, buf, sizeof(buf))) > 0) {  
        // " [remote] " 출력
        write(STDOUT_FILENO, s, (sizeof(s) / sizeof(s[0])) );  
        
        // 컴파일 및 실행한 결과 출력
        write(STDOUT_FILENO, buf, isEnd);
    }
    // "[*] session closed" 출력
    write(STDOUT_FILENO, s2, sizeof(s2));

    // 소켓 닫고, 종료
    close(sock);
    exit(0);    
}

// 서버에게 데이터를 보내는 함수
void* p_write_to_server()
{
    // DO LOOP only if the input still exists
    while (isEnd != -1) {
        // 클라이언트의 표준 입력을 buf에 저장
        bytes = read(STDIN_FILENO, buf, sizeof(buf));

        // 서버로 buf를 전송, 서버 소켓 닫혀있으면 send()가 EOF를 반환하므로 종료
        if (send(sock, buf, bytes, 0) == -1) { 
            exit(0);
        }
    }
}

void error_handling(char *message)
{
    fputs(message, stderr);
    fputc('\n', stderr);
    exit(1);
}