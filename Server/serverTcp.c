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
#include <sys/stat.h>
#include <fcntl.h>
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

struct ClientMsg {
    unsigned char Buf[MAX_MSG_LEN];
    unsigned char username[MAX_USERNAME_LEN];
    int  tail;
    int  sockfd;

    ClientMsg()
    {
        tail = 0;
        sockfd = -1;
        memset(username, 0, MAX_USERNAME_LEN);
        memset(Buf, 0, MAX_MSG_LEN);
    };

    void reset()
    {
        tail = 0;
        sockfd = -1;
        memset(username, 0, MAX_USERNAME_LEN);
        memset(Buf, 0, MAX_MSG_LEN);
    };
};

int listenfd;
int save_fd;
int maxfd;
std::map<int, ClientMsg> clientMap;
unsigned char send_buf[MAX_MSG_LEN] = {0};
pthread_t g_listenThreadId = 0;

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

unsigned int buildPacketUserlist()
{
    unsigned short size = 0;
    stringTLV* pTlv = NULL;
    stringTLV* pTlvUserlist = (stringTLV*)(send_buf + MICRO_MSG_HDR_LEN);
    size += sizeof(stringTLV);
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clientMap[i].sockfd < 0 || strlen((const char*)clientMap[i].username)==0)
            continue;
        pTlv = (stringTLV*)(send_buf + MICRO_MSG_HDR_LEN + size);
        size += sizeof(stringTLV);
        buildStringTlv(pTlv, clientMap[i].username, TAG_USER_NAME, strlen((const char*)clientMap[i].username));
        htonStringTlv(pTlv);
        size += strlen((const char*)clientMap[i].username);
    }
    pTlvUserlist->tag = TAG_USERLIST;
    pTlvUserlist->len = size - sizeof(stringTLV);
    htonStringTlv(pTlvUserlist);

    buildMsgHeader((MsgHeader*)send_buf, MSG_TYPE_USERLIST, size);
    htonMsgHeader((MsgHeader*)send_buf);
    printf("buildPacketUserlist. size=%d\n", size + MICRO_MSG_HDR_LEN);
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

unsigned int buildPacketUserLoginFail()
{
    unsigned short size = 0;
    stringTLV* pTlv = (stringTLV*)(send_buf + MICRO_MSG_HDR_LEN);

    buildMsgHeader((MsgHeader*)send_buf, MSG_TYPE_LOGINFAIL, size);
    htonMsgHeader((MsgHeader*)send_buf);

    printf("buildPacketUserLoginFail. size=%d\n", size + MICRO_MSG_HDR_LEN);
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

int sendPacket(int index, const void* buf, unsigned int len)
{
    struct pollfd pfd; // structure to poll()
    int wrtd = 0; // total data written
    int wnow = 0; // partial data written
    int ret = len;

    if (clientMap[index].sockfd < 0)
    {
        printf("socket[%d] is not valid.\n",  index);
        return -1;
    }

    // set poll events
    pfd.fd = clientMap[index].sockfd;
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

    printf("sendPacket[%d]: ret=%d.\n", index, ret);
    return (int) ret;
}

void sendPacketOnlyTo(int index, int len)
{
    int ret = 0;

    if(clientMap[index].sockfd < 0)
        return;
    ret = sendPacket(index, send_buf, len);
}

void sendPacketExcept(int index, int len)
{
    int ret = 0;

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clientMap[i].sockfd < 0 || i==index)
        {
            continue;
        }
        ret = sendPacket(i, send_buf, len);
    }
}

void process_data(int index, unsigned char * buffer)
{
    stringTLV* pTlv = NULL;
    stringTLV* pTlv2 = NULL;
    unsigned char username[MAX_USERNAME_LEN] = {0};
    unsigned char msg[MAX_MSG_LEN] = {0};
    unsigned int send_size = 0;

    MsgHeader * msgHeader = (MsgHeader*)buffer;
    ntohMsgHeader(msgHeader);
    printf("process_data[%d]: msgType = %u\n", index, msgHeader->msgType);
    switch(msgHeader->msgType)
    {
        case MSG_TYPE_LOGIN:
            pTlv = (stringTLV*)(buffer + MICRO_MSG_HDR_LEN);
            ntohStringTlv(pTlv);
            printf("process_data[%d]: tagPID=%u, login\n", index, pTlv->tag);
            assert(TAG_USER_NAME == pTlv->tag);
            memcpy(username, pTlv->content, pTlv->len);
            printf("process_data[%d]: username=%s, login\n", index, username);

            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (!strcmp((const char*)clientMap[i].username, (const char*)username))
                {
                    send_size = buildPacketUserLoginFail();
                    sendPacketOnlyTo(index, send_size);
                    return;
                }
            }

            // send online clients list to login client
            send_size = buildPacketUserlist();
            sendPacketOnlyTo(index, send_size);

            // send loginin user to online clients
            send_size = buildPacketUserLogin(username, pTlv->len);
            sendPacketExcept(index, send_size);

            // add username to online clients
            memcpy(clientMap[index].username, username, pTlv->len);
            break;

        case MSG_TYPE_CONTACT:
            pTlv = (stringTLV*)(buffer + MICRO_MSG_HDR_LEN);
            ntohStringTlv(pTlv);
            printf("process_data[%d]: tagPID=%u, contact\n", index, pTlv->tag);
            assert(TAG_USER_NAME == pTlv->tag);
            memcpy(username, pTlv->content, pTlv->len);
            printf("process_data[%d]: username=%s, contact\n", index, username);
            pTlv2 = (stringTLV*)(buffer + MICRO_MSG_HDR_LEN + sizeof(stringTLV) + pTlv->len);
            ntohStringTlv(pTlv2);
            printf("process_data[%d]: tagPID=%u, contact\n", index, pTlv2->tag);
            assert(TAG_MSG == pTlv2->tag);
            memcpy(msg, pTlv2->content, pTlv2->len);
            printf("process_data[%d]: msg=%s, contact\n", index, msg);

            // send msg to other online clients
            send_size = buildPacketContact(username, pTlv2->content, pTlv->len, pTlv2->len);
            sendPacketExcept(index, send_size);
            break;

        case MSG_TYPE_LOGOUT:
            pTlv = (stringTLV*)(buffer + MICRO_MSG_HDR_LEN);
            ntohStringTlv(pTlv);
            printf("process_data[%d]: tagPID=%u, logout\n", index, pTlv->tag);
            assert(TAG_USER_NAME == pTlv->tag);
            memcpy(username, pTlv->content, pTlv->len);
            printf("process_data[%d]: username=%s, logout\n", index, username);

            // send offline user to other client
            send_size = buildPacketUserLogout(username, pTlv->len);
            sendPacketExcept(index, send_size);

            // delete username from online userlist
            memset(clientMap[index].username, 0, MAX_USERNAME_LEN);
            break;

        default:
            printf("process_data[%d]: error msg received.\n", index);
            break;
    }

    printf("process_data[%d]: over.\n", index);
    return;
}

void process_client(int index)
{
    if (index >= MAX_CLIENTS)
    {
        printf("process_client: index out of range.\n");
        return;
    }

    if (clientMap[index].tail == 0)
    {
        printf("process_client[%d]: data is null\n", index);
        return;
    }

    while (clientMap[index].tail >= MICRO_MSG_HDR_LEN)
    {
        microMsgHeader* phdr = (microMsgHeader*)clientMap[index].Buf;
        int msgSize = ntohs(phdr->length)+MICRO_MSG_HDR_LEN;
        if (clientMap[index].tail < msgSize)
        {
            printf("process_client[%d]: data left %d bytes.\n", index, clientMap[index].tail);
            return;
        }

        process_data(index, clientMap[index].Buf);
        memmove(clientMap[index].Buf, clientMap[index].Buf + msgSize, clientMap[index].tail - msgSize);
        clientMap[index].tail -= msgSize;
    }
    printf("process_client[%d]: data left %d bytes.\n", index, clientMap[index].tail);
    return;
}

volatile bool bexit = false;
void *threadListen(void *threadid)
{
    socklen_t sin_size;
    int sockfd, connfd;
    int ret = 0;
    int n = 0;
    int i = 0;
    struct timeval time = {0};
    fd_set readset, rset;
    struct sockaddr_in client_sockaddr;

    /*select*/
    FD_ZERO(&readset);              	// clear readfd set
    FD_SET(listenfd, &readset);         // add listenfd to set
    maxfd = listenfd + 1;

    while(!bexit)
    {
        rset = readset;
        time.tv_sec = 0;
        time.tv_usec = 500;

        // select socket fd set
        ret = select(maxfd, &rset, NULL, NULL,(struct timeval *)&time);
        if (ret == 0)
        {
            continue;
        }
        else if (ret < 0)
        {
            perror("select");
            break;
        }

        // listendfd is readable, means new client connected
        if(FD_ISSET(listenfd, &rset)>0)
        {
            sin_size=sizeof(struct sockaddr_in);
            //client_sockaddr£º¿Í»§¶ËµØÖ·
            connfd = accept(listenfd, (struct sockaddr *)&client_sockaddr, &sin_size);
            printf("client %s:%d connected!\n", inet_ntoa(client_sockaddr.sin_addr), client_sockaddr.sin_port);
            //find an unused
            for(i = 0; i < MAX_CLIENTS; i++)
            {
                if(clientMap[i].sockfd < 0)
                {
                    clientMap[i].sockfd = connfd;
                    break;
                }
            }

            if(i == MAX_CLIENTS)
            {
                printf("too many clients, close it!\n");
                close(connfd);
            }
            else
            {
                //add new client's decriptor to set
                FD_SET(connfd, &readset);

                //max used file descriptor
                if(connfd >= maxfd)
                    maxfd = connfd + 1;
            }

            //no more readable fd
            if(--ret == 0)
               continue;
        } /*if*/

        //check all clients for data
        for(i = 0; i < MAX_CLIENTS; i++)
        {
            if (bexit)
                break;

            if((sockfd = clientMap[i].sockfd)<0)
                continue;

            if(FD_ISSET(sockfd, &rset))
            {
                if (clientMap[i].tail >= MAX_MSG_LEN)
                {
                    memset(clientMap[i].Buf, 0, MAX_MSG_LEN);
                    clientMap[i].tail = 0;
                    printf("error read data from client. reset data--sokefd[%d]\n", sockfd);
                }

                if((n = recv(sockfd, clientMap[i].Buf+clientMap[i].tail, MAX_MSG_LEN-clientMap[i].tail, 0)) == 0)
                {
                    printf("connection closed by client--sokefd[%d]\n", sockfd);
                    //connection closed by client
                    close(sockfd);
                    FD_CLR(sockfd, &readset);

                    // send user logout to other online userlist.
                    int send_size = buildPacketUserLogout(clientMap[i].username, strlen((const char*)clientMap[i].username));
                    sendPacketExcept(i, send_size);
                    clientMap[i].reset();
                }
                else if(n == -1)
                {
                    printf("Read err from client [%d] sokefd[%d]\n", i, sockfd);
                }
                else
                {
                    clientMap[i].tail += n;
                    printf("recv client %d bytes.\n", n);
                    process_client(i);
                }

                //no more readable fd
                if(--ret == 0)
                    break;
            }
        }
    }/*while*/
}

void my_handler(int s)
{
    printf("\nCaught signal %d\n", s);
    bexit = true;

    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(clientMap[i].sockfd >= 0)
        {
            close(clientMap[i].sockfd);
            clientMap[i].sockfd = -1;
        }
        close(listenfd);
    }
    pthread_join(g_listenThreadId, NULL);
    close(save_fd);
    exit(1);
}

int main(int argc, char* argv[])
{
    if (argc != 1)
    {
        printf("error: argc is no correct!\n");
        return -1;
    }

    signal(SIGCHLD, SIG_IGN);

    fflush(stdout);
    setvbuf(stdout, NULL, _IONBF, 0);
    save_fd = open("output.txt",(O_RDWR | O_CREAT), 0644);
    dup2(save_fd, STDOUT_FILENO);

    /* init */
    for(int i = 0; i<MAX_CLIENTS; i++)
        clientMap.insert(std::pair<int, ClientMsg>(i, ClientMsg()));

    struct sockaddr_in server_sockaddr;
    struct sigaction sigIntHandler;

    sigIntHandler.sa_handler = my_handler;
    sigemptyset(&sigIntHandler.sa_mask);
    sigIntHandler.sa_flags = 0;
    sigaction(SIGINT, &sigIntHandler, NULL);

    // create socket
    if((listenfd = socket(AF_INET,SOCK_STREAM,0))<0)
    {
        perror("socket");
        close(save_fd);
        exit(1);
    }
    printf("socket success!,sockfd=%d\n",listenfd);

    int on=1;
    if((setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)))<0)
    {
       perror("setsockopt failed");
       close(save_fd);
       exit(1);
    }

    server_sockaddr.sin_family=AF_INET;
    server_sockaddr.sin_port=htons(SERVERPORT);
    //server_sockaddr.sin_addr.s_addr= inet_addr(SERVERADDR);
    server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bzero(&(server_sockaddr.sin_zero),8);

    // bind socket
    if(bind(listenfd,(struct sockaddr *)&server_sockaddr,sizeof(struct sockaddr))<0)
    {
        perror("bind");
        close(listenfd);
        close(save_fd);
        exit(1);
    }
    printf("bind success!\n");

    // listen
    if(listen(listenfd, BACKLOG)==-1)
    {
        perror("listen");
        close(listenfd);
        close(save_fd);
        exit(1);
    }
    printf("listening....\n");

    int retv;
    retv=pthread_create(&g_listenThreadId, NULL, threadListen, NULL);
    if (retv!=0)
    {
        printf ("Create pthread error!\n");
        close(listenfd);
        close(save_fd);
        exit(1);
    }
    printf("threadListen created success\n");

    sleep(2);
    while(1)
    {
       //printf("main loop continue.\n");
       sleep(100);
    }
    return 0;
}
