#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <time.h>
#define  TEMP   1024
#define BUF_SIZE  (10*1024*1024)-1 //byte



int sendall1(int sockfd, unsigned char *buf, int *len){
    uint8_t *temp;
    int length = *len;
    int total=0;
    int lengh, n, nb;
    int a=0;
    n=0;
    lengh = 0;
    a=0;
    temp = (uint8_t *)malloc(TEMP);
    while (total<length){
        memcpy( &temp[a], &buf[total], 1);
        a++;
        total++;
        if (temp[a-1] =='\\' && temp[a-2] != '\\'){
            temp[a] ='\\';
            a+=1;
        }
    }
    temp[a] = '\\';
    temp[a+1] = '0';
    n = send(sockfd, temp, a+2, 0);
    printf("%d data send\n", n);
    lengh += n-2;


    return lengh;
}
int sendall2(int sockfd, uint8_t *from, int *len) {
    uint8_t *temp;
    uint8_t *temp2;
    int length = *len;
    
    
    int total=0;
    int lengh, n, nb;
    int a=0;
    
    
    uint32_t header;
    header =(uint32_t) length;
    temp2 = (uint8_t *)malloc(BUF_SIZE+4);
    
    temp2[0] = (uint8_t) (header>>24);
    temp2[1] = (uint8_t) ((header<<8)>>24);
    temp2[2] = (uint8_t) ((header<<16)>>24);
    temp2[3] = (uint8_t) ((header<<24)>>24);
    printf("protocol2 send\n%02x, %02x, %02x, %02x\n", temp2[0], temp2[1], temp2[2], temp2[3]);
    n = send(sockfd, temp2, 4, 0);//header
    
    
    
    memcpy(temp2, from, length);
    n += send(sockfd, temp2, length, 0);
    printf("send %dB\n", n);
    return n;
}

int recvall1(int sockfd, uint8_t *msg, int recvsize){
    int a;
    
    a = recv( sockfd, msg, recvsize, 0);
    
    if (msg[a-2] == '\\' && msg[a-1] =='0') {
        return a;
    }
    
    else {
        
        return a + recvall1( sockfd, msg+a, recvsize);
    }
}

int recvall2(int s, uint8_t *buf, int *len)
{
    int total = 0;        // how many bytes we've recved
    int bytesleft = *len; // how many we have left to recv
    int n;
    uint32_t length;
    uint8_t *msg;
    msg = (uint8_t *)malloc(BUF_SIZE);
    n= recv( s, buf, 4, 0);
    while(n<4){
        n+= recv(s, buf+n, 4, 0);
    }
    length = (uint32_t) (buf[0]<<24) | (uint32_t) (buf[1]<<16) | (uint32_t) (buf[2]<<8) | (uint32_t) (buf[3]);
    printf("protocol2 recv\n%d\n", length);   //header
    
    
    n= recv( s, buf, length, 0);
    
    while(n<length){
        printf("recv %ddata\n", n);
        n+= recv(s, buf+n, length-n, 0);
    }
    
    
    int i=0;
    int a=0;
    while (i<n){
        if (a==0){
            msg[0] = buf[0];
            a++;
            i++;
        }
        else{
            if (buf[i] == msg[a-1]){
                i++;
            }
            else{
                msg[a] = buf[i];
                a++;
                i++;
                
            }
        }
    }
    n = sendall2(s, msg, &a);
    n -=4;

    return n; // return reduced number
}

int   main(int argc, char *argv[]){
    int   server_socket;
    int   client_socket;
    uint32_t   client_addr_size;
    int   pid;
    struct sockaddr_in   server_addr;
    struct sockaddr_in   client_addr;
    
    uint8_t   *buff_rcv;
    uint8_t client_op, server_op;
    uint8_t protocol;
    uint32_t trans_id;
    uint16_t recv_checksum, send_checksum;
    char   buff_snd[TEMP+5];
    uint8_t *msg;
    uint8_t *buf;
    uint8_t temp[8];
    char temp2[5];
    uint8_t *temp3;
    
    int a;
    int i;
    int n;
    
    
    
    server_socket  = socket( PF_INET, SOCK_STREAM, 0);
    if( -1 == server_socket)
    {
        printf( "Error : server socket\n");
        exit( 1);
    }
    
    memset( &server_addr, 0, sizeof( server_addr));
    server_addr.sin_family     = AF_INET;
    server_addr.sin_port       = htons( atoi(argv[2]));
    server_addr.sin_addr.s_addr= htonl( INADDR_ANY);
    
    if( -1 == bind( server_socket, (struct sockaddr*)&server_addr, sizeof( server_addr) ) )
    {
        printf( "Error : bind()\n");
        exit( 1);
    }
    
    if( -1 == listen(server_socket, 5))
    {
        printf( "Error : listen()\n");
        exit( 1);
    }
    
    while (1) {
        client_addr_size  = sizeof( client_addr);
        client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_addr_size);
        
        if (client_socket < 0) {
            perror("ERROR : accept()\n");
            exit(1);
        }
        
        /* Create child process */
        pid = fork();
        
        if (pid < 0) {
            perror("ERROR : fork()\n");
            exit(1);
        }
        
        if (pid == 0) {
            /* This is the client process */
            close(server_socket);
            /* phase 1 */
            buff_rcv = (uint8_t *)malloc(8);
            read ( client_socket, buff_rcv, 8);
            printf("%02x, %02x, %02x, %02x, %02x, %02x, %02x, %02x\n", buff_rcv[0], buff_rcv[1], buff_rcv[2], buff_rcv[3], buff_rcv[4], buff_rcv[5], buff_rcv[6], buff_rcv[7]);

            client_op = (uint8_t) buff_rcv[0];
            protocol = (uint8_t) buff_rcv[1];
            trans_id = (uint32_t) (buff_rcv[4]<<24)|(uint32_t) (buff_rcv[5]<<16) | (uint32_t) (buff_rcv[6]<<8) |(uint32_t) (buff_rcv[7]);
            recv_checksum = (uint16_t) (buff_rcv[2]<<8) | (uint16_t) (buff_rcv[3]);

            printf("op : %02x\nprotocol : %02x\nchecksum : %04x\ntrans_id : %08x\n", client_op, protocol, recv_checksum, trans_id);
            
            if (recv_checksum != 0xffff-((uint16_t)protocol+(uint16_t)(trans_id>>16)+(uint16_t)((trans_id<<16)>>16))){
                printf("Error : Checksum is not right\n");
                exit(1);
            }
            else{
                printf("---------------receive---------------\n");
                printf("op : %02x\nprotocol : %02x\n\ntrans_id : %08lx\n", client_op, protocol, (unsigned long)buff_rcv[4]<<24 | (unsigned long)buff_rcv[5]<<16 | (unsigned long)buff_rcv[6]<<8 | (unsigned long)buff_rcv[7]);
                
                if (protocol ==0){
                    protocol = time(NULL) % 2 +1;   //protocol => 1 or 2
                }
                server_op = 1;
                send_checksum = 0xffff-(((uint16_t)protocol|(uint16_t)(server_op<<8))+(uint16_t)(trans_id>>16)+(uint16_t)((trans_id<<16)>>16));
                temp[0]=(uint8_t) server_op;
                temp[1]=(uint8_t) protocol;
                temp[2]=(uint8_t)(send_checksum>>8);
                temp[3]=(uint8_t)((send_checksum<<8)>>8);
                temp[4]=(uint8_t)buff_rcv[4];
                temp[5]=(uint8_t)buff_rcv[5];
                temp[6]=(uint8_t)buff_rcv[6];
                temp[7]=(uint8_t)buff_rcv[7];

               
                write( client_socket, temp, 8);          // +1: NULL까지 포함해서 전송
                
                msg = (uint8_t *)malloc(BUF_SIZE);
                
                int d;
                int sn;
                if (protocol ==1){  //state machine
                    buf = (uint8_t *)malloc(TEMP);
                    a=0; //total (reduced)file index
                    while (1) {
                        printf(".....\n");
                        temp3 = (uint8_t *)malloc(TEMP);
                        d=0;  //delta
                        n=0;
                        n = recvall1(client_socket, buf, TEMP);
                        
                        printf("%d size received\n%s\n", n, buf);
                        i =0;
                        if (n==0){
                            printf("0 bytes received\n");
                        }
                        while (i<n-2){
                            

                            if (a==0){
                                msg[0] = buf[0];
                                temp3[0] = buf[0];
                                a++;
                                d++;
                                i++;
                                
                            }
                            else{
                                if (buf[i] == msg[a-1]){
                                    i++;
                                }
                                else{
                                    msg[a] = buf[i];
                                    temp3[d] = buf[i];
                                    a++;
                                    d++;
                                    i++;
                                }
                            }
                        }

                        sn = sendall1(client_socket, temp3, &d);
                    }
                    printf("complete!\n");
                }
                
                
                else if (protocol ==2){
                    printf("protocol2!\n");
                    d = BUF_SIZE;
                    a=recvall2(client_socket, msg, &d);
                }
                else {
                    printf("protocol error\n");
                    exit(1);
                }
            }
            exit(0);
        }
        else {
            close(client_socket);
        }
        
    }
}