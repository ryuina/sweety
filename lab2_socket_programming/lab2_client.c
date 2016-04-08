/*----------------------------------------------------
TCP SOCKET PROGRAMMING
lab2_client.c
last modified : 160404
*/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <errno.h>



#define PROTO_SIZE  8 //byte
#define BUF_SIZE  (10*1024*1024) //byte
#define TEMP 1024


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
    
    n= recv( s, buf, 4, 0);
    while(n<4){
        n+= recv(s, buf+n, 4, 0);
    }
    length = (uint32_t) (buf[0]<<24) | (uint32_t) (buf[1]<<16) | (uint32_t) (buf[2]<<8) | (uint32_t) (buf[3]);
    
    while(total < length) {
        n = recv(s, buf+total, bytesleft, 0);
        total += n;
        bytesleft -= n;
    }
    
    return total; // return reduced number
}
int sendall1(int sockfd, unsigned char *buf, int *len, uint8_t *buf2){
    uint8_t *temp;
    uint8_t *temp2;
    int length = *len;
    int total=0;
    int lengh, n, nb;
    int a=0;
    
    lengh = 0;
    temp2 = (uint8_t *)malloc(BUF_SIZE);
    
    while (total<=length){
        a=0;
        temp = (uint8_t *)malloc(TEMP);
        while (a<=1020){
            memcpy( &temp[a++], &buf[total++], 1);
            if (temp[a-1] =='\\'){
                temp[a] ='\\';
                a+=1;
            }
        }
        temp[a] = '\\';
        temp[a+1] = '0';
        n = send(sockfd, temp, a+2, 0);
        nb = recvall1(sockfd, temp2+lengh, n);
        lengh +=nb-2;
        
    }
    n=0;
    int i=0;
    while (n<lengh){
        
        memcpy( &buf2[i], &temp2[n], 1);
        if (buf2[i] == '\\'&& temp2[n+1] =='\\')
            n++;
        
        i++;
        n++;
    }
    close( sockfd);
    return i-1;
}

int sendall2(int sockfd, uint8_t *from, int *len, uint8_t *to) {
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
    n = send(sockfd, temp2, 4, 0);
    memcpy(temp2, from, length);
        n += send(sockfd, temp2, length, 0);
        nb = recvall2(sockfd, to, &n);
    close( sockfd);

    
    return nb;
   
}



int main(int argc, char *argv[])
{
    int sockfd, len, a;
    FILE *fp;
    struct sockaddr_in   server_addr;
    char *input_name;
    uint8_t  *buf;
    uint8_t protocol;
    unsigned long trans_id;
    uint16_t temp;
    unsigned char *msg;
    char *rcv;
    int send_fsize;
    char file_name[50] ;
    uint8_t *buf2;
    buf2 = (uint8_t *)malloc(BUF_SIZE);

    
    msg = (unsigned char *)malloc(BUF_SIZE);
    input_name = (char *) malloc(100);
    buf = (uint8_t *)malloc(10);
    if (argc != 7) {
        printf("Error : Please follow the argument format.\n");
        exit(1);
    }
    if (strcmp(argv[1], "-h") !=0 || strcmp(argv[3], "-p") !=0 || strcmp(argv[5], "-m") !=0)  {
        printf("Error : Please follow the argument format.");
        exit(1);
    }
    
    sockfd = socket( PF_INET, SOCK_STREAM, 0);
    
    if( -1 == sockfd)
    {
        printf( "socket 생성 실패\n");
        exit( 1);
    }
    memset( &server_addr, 0, sizeof( server_addr));
    server_addr.sin_family     = AF_INET;
    server_addr.sin_port       = htons( atoi(argv[4]) );
    server_addr.sin_addr.s_addr= inet_addr( argv[2]);
    
    if( -1 == connect( sockfd, (struct sockaddr*)&server_addr, sizeof( server_addr) ) )
    {
        printf( "접속 실패\n");
        exit( 1);
    }

    /* phase 1 */
    
    uint8_t *phase1_msg;
    
    phase1_msg = (uint8_t *)malloc(8);
    protocol = (uint8_t) atoi(argv[6]); //
    trans_id = time(NULL) % 5000;
    uint16_t checksum;
    uint16_t recv_checksum;
    checksum = 0xffff-((uint16_t)protocol+(uint16_t)(trans_id>>16)+(uint16_t)((trans_id<<16)>>16));
    
    phase1_msg[0]=(uint8_t)0x00;
    phase1_msg[1]=(uint8_t) atoi(argv[6]);
    phase1_msg[2]=(uint8_t) (checksum>>8);
    phase1_msg[3]=(uint8_t) ((checksum<<8)>>8);
    phase1_msg[4]=(uint8_t)(trans_id>>24);
    phase1_msg[5]=(uint8_t)((trans_id<<8)>>24);
    phase1_msg[6]=(uint8_t)((trans_id<<16)>>24);
    phase1_msg[7]=(uint8_t)((trans_id<<24)>>24);
    
    a= send( sockfd, phase1_msg, 8, 0);
    a= recv( sockfd, buf, 8, 0);
    
    while(a<8){
        a+= recv(sockfd, buf+a, 8, 0);
    }
    
    protocol = (unsigned char) buf[1];  //protocol change
    temp = ((uint16_t) buf[0]<<8 | (uint16_t) buf[1]) + ((uint16_t) buf[2]<<8 | (uint16_t) buf[3]) + ((uint16_t) buf[4]<<8 | (uint16_t)buf[5]) + ((uint16_t) buf[6]<<8 | (uint16_t) buf[7]);
    
    if( trans_id != ((unsigned long) buf[4]<<24 | (unsigned long)buf[5]<<16 | (unsigned long) buf[6]<<8 | (unsigned long) buf[7])){
        printf("Error : Trans_id not match.\n");
        exit(1);
    }
    recv_checksum = (uint16_t) (buf[2]<<8) | (uint16_t) (buf[3]);
    if ((0xffff-temp) !=0){
        printf("Error : server Checksum is not right\n");
        exit(1);
    }
    

    /* phase 2 */
    while(1){

        fseek(stdin, 0, SEEK_END);
        send_fsize = ftell(stdin);
        fseek(stdin, 0, SEEK_SET);
        
        len = fread(msg, sizeof(char), send_fsize, stdin);
        
        fclose(stdin);
        if (protocol == 1) {
            if ((a= sendall1(sockfd, msg, &send_fsize, buf2)) == -1) {
                perror("sendall");
                printf("We only sent %d bytes because of the error!\n", len);
            }
        }
        
        else if (protocol ==2){
            a=sendall2(sockfd, msg, &send_fsize, buf2);
        }
        else {
            printf("protocol error %d\n", protocol);
            close( sockfd);
            exit(1);
        }

        fwrite(buf2, sizeof(char), a, stdout);
                exit(0);
    }
    return 0;
}
   



