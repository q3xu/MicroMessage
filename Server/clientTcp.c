#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string>
#include <assert.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>
#include <poll.h>
#include <arpa/inet.h>
#include <map>
#include <list>

#define SERVERADDR              "106.14.3.125"
#define SERVERPORT              62840

#define BACKLOG                 10
#define MAX_CLIENTS             10

#define MAGIC_CODE              0x5457
#define MSG_VERSION             1

#define MSG_TYPE_LOGIN 			1
#define MSG_TYPE_CONTACT		2
#define MSG_TYPE_LOGOUT			3
#define MSG_TYPE_USERLIST		4
#define MSG_TYPE_LOGINFAIL      5

#define TAG_USER_NAME           1
#define TAG_PASSWARD            2
#define TAG_MSG                 3
#define TAG_USERLIST            4

#define MAX_USERNAME_LEN        256
#define MAX_MSG_LEN             20480
#define MICRO_MSG_HDR_LEN       sizeof(microMsgHeader)


int clientfd = -1;
int recvTail = 0;
unsigned char send_buf[MAX_MSG_LEN] = {0};
unsigned char recv_buf[MAX_MSG_LEN] = {0};
unsigned char client_addr[256] = {0};
unsigned short client_port;
pthread_t g_recvThreadId = 0;

typedef struct microMsgHeader
{
    unsigned short magicCode;   // 0x5457
    unsigned short length;	    // not include hdr size
    unsigned char  version;
    unsigned char  msgType;
    unsigned short reserved;
}MsgHeader;

typedef struct stringTLV
{
    unsigned short tag;
    unsigned short len;
    unsigned char content[0];
}StringTlv;

void ntohMsgHeader(MsgHeader * pHeader)
{
    if (NULL == pHeader)
        return;

    pHeader->magicCode = ntohs(pHeader->magicCode);
    pHeader->length = ntohs(pHeader->length);
    pHeader->reserved = ntohs(pHeader->reserved);
}

void htonMsgHeader(MsgHeader * pHeader)
{
    if (NULL == pHeader)
        return;

    pHeader->magicCode = htons(pHeader->magicCode);
    pHeader->length = htons(pHeader->length);
    pHeader->reserved = htons(pHeader->reserved);
}

void ntohStringTlv(StringTlv * pTlv)
{
    if (NULL == pTlv)
        return;

    pTlv->tag = ntohs(pTlv->tag);
    pTlv->len = ntohs(pTlv->len);
}

void htonStringTlv(StringTlv * pTlv)
{
    if (NULL == pTlv)
        return;

    pTlv->tag = htons(pTlv->tag);
    pTlv->len = htons(pTlv->len);
}

void buildMsgHeader(MsgHeader * pHeader, unsigned char msgType, unsigned short size)
{
    if (NULL == pHeader)
        return;

    pHeader->magicCode = MAGIC_CODE;
    pHeader->length = size;
    pHeader->version = MSG_VERSION;
    pHeader->msgType = msgType;
    pHeader->reserved = 0;
}

void buildStringTlv(stringTLV * pTlv, unsigned char* content, unsigned short tag, unsigned short len)
{
    if (NULL == pTlv)
        return;

    pTlv->tag = tag;
    pTlv->len = len;
    memcpy(pTlv->content, content, len);
}

unsigned int buildPacketContact(unsigned char* username, unsigned char* msg, unsigned short userlen, unsigned short msglen)
{
    unsigned short size = 0;
    stringTLV* pTlv = (stringTLV*)(send_buf + MICRO_MSG_HDR_LEN);
    size += sizeof(stringTLV);
    buildStringTlv(pTlv, username, TAG_USER_NAME, userlen);
    htonStringTlv(pTlv);
    size += userlen;

    pTlv = (stringTLV*)(send_buf + MICRO_MSG_HDR_LEN + size);
    size += sizeof(stringTLV);
    buildStringTlv(pTlv, msg, TAG_MSG, msglen);
    htonStringTlv(pTlv);
    size += msglen;

    buildMsgHeader((MsgHeader*)send_buf, MSG_TYPE_CONTACT, size);
    htonMsgHeader((MsgHeader*)send_buf);
    printf("buildPacketContact. size=%d\n", size + MICRO_MSG_HDR_LEN);
    return (size + MICRO_MSG_HDR_LEN);
}

unsigned int buildPacketUserLogin(unsigned char* username, unsigned short len)
{
    unsigned short size = 0;
    stringTLV* pTlv = (stringTLV*)(send_buf + MICRO_MSG_HDR_LEN);
    size += sizeof(stringTLV);
    buildStringTlv(pTlv, username, TAG_USER_NAME, len);
    htonStringTlv(pTlv);
    size += len;

    buildMsgHeader((MsgHeader*)send_buf, MSG_TYPE_LOGIN, size);
    htonMsgHeader((MsgHeader*)send_buf);
    printf("buildPacketUserLogin. size=%d\n", size + MICRO_MSG_HDR_LEN);
    return (size + MICRO_MSG_HDR_LEN);
}

unsigned int buildPacketUserLogout(unsigned char* username, unsigned short len)
{
    unsigned short size = 0;
    stringTLV* pTlv = (stringTLV*)(send_buf + MICRO_MSG_HDR_LEN);
    size += sizeof(stringTLV);
    buildStringTlv(pTlv, username, TAG_USER_NAME, len);
    htonStringTlv(pTlv);
    size += len;

    buildMsgHeader((MsgHeader*)send_buf, MSG_TYPE_LOGOUT, size);
    htonMsgHeader((MsgHeader*)send_buf);
    printf("buildPacketUserLogout. size=%d\n", size + MICRO_MSG_HDR_LEN);
    return (size + MICRO_MSG_HDR_LEN);
}

int sendPacket(const void* buf, unsigned int len)
{
    struct pollfd pfd; // structure to poll()
    int wrtd = 0; // total data written
    int wnow = 0; // partial data written
    int ret = len;

    if (clientfd < 0)
    {
        printf("sendPacket: clientfd is not valid.\n");
        return -1;
    }

    // set poll events
    pfd.fd = clientfd;
    pfd.events = POLLOUT;
    pfd.revents = 0;

    // try to write all data requested
    while ((wrtd < len) && (ret > 0)) {
        int r = ::poll(&pfd, 1, -1);
        if (r > 0) // poll OK
        {
            wnow = ::write(pfd.fd , (char *) buf + wrtd, len - wrtd);
            if (wnow > 0) {
                wrtd += wnow; // update number of bytes sent
                ret = wrtd; // data written OK
            } else {
                ret = wnow; // some error ocurred (-1: error;  0: no data sent)
            }
        } else if (r == 0) // poll timeout
        {
            ret = -2;
        } else // poll error
        {
            ret = r;
        }
    }

    printf("sendPacket: ret=%d.\n", ret);
    return (int) ret;
}

void process_data(unsigned char * buffer)
{
    stringTLV* pTlv = NULL;
    stringTLV* pTlv2 = NULL;
    int size = 0, flag = 0;
    unsigned char username[MAX_USERNAME_LEN] = {0};
    unsigned char msg[MAX_MSG_LEN] = {0};

    MsgHeader * msgHeader = (MsgHeader*)buffer;
    ntohMsgHeader(msgHeader);
    printf("process_data: msgType = %u\n", msgHeader->msgType);
    switch(msgHeader->msgType)
    {
        case MSG_TYPE_LOGIN:
            pTlv = (stringTLV*)(buffer + MICRO_MSG_HDR_LEN);
            ntohStringTlv(pTlv);
            printf("process_data: tagPID=%u, login\n", pTlv->tag);
            assert(TAG_USER_NAME == pTlv->tag);
            memcpy(username, pTlv->content, pTlv->len);
            printf("process_data: username=%s, login\n", username);
            break;

        case MSG_TYPE_CONTACT:
            pTlv = (stringTLV*)(buffer + MICRO_MSG_HDR_LEN);
            ntohStringTlv(pTlv);
            printf("process_data: tagPID=%u, contact\n", pTlv->tag);
            assert(TAG_USER_NAME == pTlv->tag);
            memcpy(username, pTlv->content, pTlv->len);
            printf("process_data: username=%s, contact\n", username);
            pTlv2 = (stringTLV*)(buffer + MICRO_MSG_HDR_LEN + sizeof(stringTLV) + pTlv->len);
            ntohStringTlv(pTlv2);
            printf("process_data: tagPID=%u, contact\n", pTlv2->tag);
            assert(TAG_MSG == pTlv2->tag);
            memcpy(msg, pTlv2->content, pTlv2->len);
            printf("process_data: msg=%s, contact\n", msg);
            break;

        case MSG_TYPE_LOGOUT:
            pTlv = (stringTLV*)(buffer + MICRO_MSG_HDR_LEN);
            ntohStringTlv(pTlv);
            printf("process_data: tagPID=%u, logout\n", pTlv->tag);
            assert(TAG_USER_NAME == pTlv->tag);
            memcpy(username, pTlv->content, pTlv->len);
            printf("process_data: username=%s, logout\n", username);
            break;

        case MSG_TYPE_USERLIST:
            pTlv = (stringTLV*)(buffer + MICRO_MSG_HDR_LEN);
            ntohStringTlv(pTlv);
            printf("process_data: tagPID=%u, userlist\n", pTlv->tag);
            assert(TAG_USERLIST == pTlv->tag);
            flag = sizeof(stringTLV);
            size = pTlv->len;
            while (size > 0) {
                pTlv2 = (stringTLV*)(buffer + MICRO_MSG_HDR_LEN + flag);
                ntohStringTlv(pTlv2);
                size -= sizeof(stringTLV);
                flag += sizeof(stringTLV);
                assert(TAG_USER_NAME == pTlv2->tag);
                memset(username, 0, MAX_USERNAME_LEN);
                memcpy(username, pTlv2->content, pTlv2->len);
                printf("process_data: username=%s, userlist\n", username);
                size -= pTlv2->len;
                flag += pTlv2->len;
            }

            break;

        case MSG_TYPE_LOGINFAIL:
            printf("process_data: login in failed.\n");
            break;

        default:
            printf("process_data: error msg received.\n");
            break;
    }
    printf("process_data over.\n");
    return;
}

void process_server()
{
    if (recvTail == 0)
    {
        printf("process_server: data is null.\n");
        return;
    }

    while (recvTail >= sizeof(microMsgHeader))
    {
        microMsgHeader* phdr = (microMsgHeader*)recv_buf;
        int msgSize = ntohs(phdr->length)+sizeof(microMsgHeader);
        if (recvTail < msgSize)
        {
            printf("process_server: dataleft %d bytes.\n", recvTail);
            return;
        }

        process_data(recv_buf);
        memmove(recv_buf, recv_buf + msgSize, recvTail - msgSize);
        recvTail -= msgSize;
    }
    printf("process_server: data left %d bytes.\n", recvTail);
    return;
}

volatile bool bexit = false;
void *threadRecv(void *threadid)
{
    int ret = 0;
    int n = 0;
    struct timeval time = {0};
    fd_set readset, rset;

    /*select*/
    FD_ZERO(&readset);              	// clear readfd set
    FD_SET(clientfd, &readset);         // add listenfd to set

    while(!bexit)
    {
        rset = readset;
        time.tv_sec = 0;
        time.tv_usec = 500;

        // select socket fd set
        ret = select(clientfd+1, &rset, NULL, NULL,(struct timeval *)&time);
        if (ret == 0)
        {
            continue;
        }
        else if (ret < 0)
        {
            perror("select");
            break;
        }

        if(FD_ISSET(clientfd, &rset))
        {
            if (recvTail >= MAX_MSG_LEN)
            {
                recvTail = 0;
                memset(recv_buf, 0, MAX_MSG_LEN);
            }

            if((n = recv(clientfd, recv_buf+recvTail, MAX_MSG_LEN-recvTail, 0)) == 0)
            {
                printf("connection closed by server--clientfd[%d]\n", clientfd);
                //connection closed by server
                bexit = true;
                close(clientfd);
                FD_CLR(clientfd, &readset);
                clientfd = -1;
            }
            else if(n == -1)
            {
                printf("Read err from server clientfd[%d]\n", clientfd);
            }
            else
            {
                recvTail += n;
                printf("recv client %d bytes.\n", n);
                process_server();
            }
        }
    }/*while*/

    printf("threadRecv exit.\n");
}

void my_handler(int s)
{
    printf("\nCaught signal %d\n", s);
    if(clientfd >= 0)
    {
        close(clientfd);
        clientfd = -1;
    }

    bexit = true;
    pthread_join(g_recvThreadId, NULL);
    exit(1);
}

int main(int argc, char* argv[])
{
    std::string username;
    if (argc != 4)
    {
        printf("error: argc is no correct!\n");
        printf("./client login_name server_ip server_port!\n");
        return -1;
    }
    username = std::string(argv[1]);
    strncpy((char*)client_addr, (const char*)argv[2], strlen((const char*)argv[2]));
    client_port = atoi(argv[3]);
    printf("username = %s, client addr = %s, port = %d\n", username.c_str(), client_addr, client_port);

    signal(SIGCHLD, SIG_IGN);

    /* init */
    struct sockaddr_in client_sockaddr, server_sockaddr;
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // create socket
    if ((clientfd = socket(AF_INET,SOCK_STREAM,0))==-1)
    {
        perror("socket");
        exit(1);
    }
    printf("socket success!,sockfd=%d.\n", clientfd);

    int on=1;
    if((setsockopt(clientfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))<0)
    {
       perror("setsockopt failed");
       close(clientfd);
       exit(1);
    }

    client_sockaddr.sin_family=AF_INET;
    client_sockaddr.sin_port=htons(client_port);
    client_sockaddr.sin_addr.s_addr= inet_addr((const char*)client_addr);
    bzero(&(client_sockaddr.sin_zero),8);

    server_sockaddr.sin_family=AF_INET;
    server_sockaddr.sin_port=htons(SERVERPORT);
    server_sockaddr.sin_addr.s_addr= inet_addr(SERVERADDR);
    bzero(&(server_sockaddr.sin_zero),8);

    // bind socket
    if(bind(clientfd,(struct sockaddr *)&client_sockaddr,sizeof(struct sockaddr))< 0)
    {
        perror("bind");
        close(clientfd);
        exit(1);
    }
    printf("bind success!\n");

    // connect server
    if(connect(clientfd,(struct sockaddr *)&server_sockaddr,sizeof(struct sockaddr))< 0)
    {
        perror("connect");
        close(clientfd);
        exit(1);
    }
    printf("connect success!\n");


    int retv;
    retv=pthread_create(&g_recvThreadId, NULL, threadRecv, NULL);
    if (retv!=0)
    {
        printf ("Create pthread error!\n");
        close(clientfd);
        exit(1);
    }
    printf("threadRecv created success.\n");

    sleep(2);

    char a;
    int send_size=0;
    std::string contact = username + " is coming!";
    while(1)
    {
        printf("please input:\n");
        printf("0: login\n");
        printf("1: contact\n");
        printf("2: logout\n");

        scanf("%c", &a);
        while ('\n' != getchar());
        switch(a)
        {
            case '0':
                send_size = buildPacketUserLogin((unsigned char*)username.c_str(), username.length());
                sendPacket(send_buf, send_size);
                break;
            case '1':
                send_size = buildPacketContact((unsigned char*)username.c_str(), (unsigned char*)contact.c_str(), username.length(), contact.length());
                sendPacket(send_buf, send_size);
                break;
            case '2':
                send_size = buildPacketUserLogout((unsigned char*)username.c_str(), username.length());
                sendPacket(send_buf, send_size);
                break;
            default:
                break;
        }
    }

    return 0;
}
