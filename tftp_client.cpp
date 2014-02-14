#include<iostream>
#include<vector>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<string>
#include<netdb.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>

using namespace std ;

#define WRQ   2
#define RRQ   1
#define DATA  3
#define ACK   4
#define ERROR 5
#define max_size 1000024

int packet_len , packet_count , packet_cur_size ;
char packet_buffer[max_size] ;
const string mode = "netascii" ;
int socket_id ;
int bind_id ;
struct hostent *hp ;
struct sockaddr_in myaddr ;
struct sockaddr_in seraddr ;
int seraddr_len ;
int fbytes,sbytes , charcount ;

void conn_start()
{
    // create socket
    memset((char *)&myaddr , 0 , sizeof(myaddr)) ;
    myaddr.sin_family = AF_INET ;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY) ;
    myaddr.sin_port = htons(0) ;

    // bind socket
    socket_id = socket(AF_INET,SOCK_DGRAM,0) ;
    cout << socket_id << endl ;
    bind_id = bind(socket_id,(struct sockaddr *)&myaddr,sizeof(myaddr)) ;
    cout << bind_id << endl ;

    // server config
    char *host = "localhost" ;
    hp = gethostbyname(host) ;
    memset((char*)&seraddr, 0, sizeof(seraddr));
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(69);
    memcpy((void *)&seraddr.sin_addr, hp->h_addr_list[0], hp->h_length);
    cout << seraddr.sin_addr.s_addr << endl ;

}

void conn_close()
{
    close(socket_id) ;
}

void recvudp()
{
        seraddr_len = sizeof(seraddr) ;
        int retsize = recvfrom(socket_id, packet_buffer, max_size , 0, (sockaddr*) &seraddr, (socklen_t *)&seraddr_len);
        if(retsize == -1)
        {
            cout << "\nRecv Error : " ;

     /*       cout << "\nRecv Error : " << WSAGetLastError();

            if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == 0)
            {
                return "";
            }
    */
            return ;
        }
        charcount = 0 ;
        fbytes = (int)packet_buffer[0] ;
        sbytes = (int)packet_buffer[1] ;
        if(sbytes==3)
        {
            fbytes = (int)packet_buffer[2] ;
            sbytes = (int)packet_buffer[3] ;
            sbytes = sbytes + ( fbytes << 8 ) ;
            cout << "Packet No."<< sbytes << "Recieved  of size =" << retsize - 4 << " bytes" << endl ;

            packet_count++ ;
            for(int i=4;i<retsize;i++,charcount++)
                cout << packet_buffer[i]  ;
        }else
        {
            cout << "ERROR :=" << endl ;
        }
        return ;
}

void send()
{
    char *my_messsage = "this is a test message" ;
    cout << sendto(socket_id,packet_buffer, packet_len, 0, (struct sockaddr *)&seraddr, sizeof(seraddr)) << endl ;
}

void read_request(string FileName)
{
    packet_count = 0 ;
    packet_len = 2 ;
    packet_buffer[0] = '\0' ;
    packet_buffer[1] = RRQ + '\0' ;

    for(int i=0;i<FileName.length();i++,packet_len++)
    {
        packet_buffer[packet_len] = FileName[i] ;
    }
    packet_buffer[packet_len++] = '\0' ;

    for(int i=0;i<mode.length();i++,packet_len++)
    {
        packet_buffer[packet_len] = mode[i] ;
    }
    packet_buffer[packet_len++] = '\0' ;


    cout << sendto(socket_id, packet_buffer, packet_len, 0 , (struct sockaddr *)&seraddr, sizeof(seraddr)) << endl ;
    return ;
}

void ack()
{
    int temp = packet_count ;
    packet_len = 4 ;
    packet_buffer[0] = '\0' ;
    packet_buffer[1] = ACK + '\0' ;
    packet_buffer[3] = '\0' + (temp & 255) ;
    temp = temp >> 8 ;
    packet_buffer[2] = '\0' + (temp & 255) ;
    cout << sendto(socket_id, packet_buffer, packet_len, 0 , (struct sockaddr *)&seraddr, sizeof(seraddr)) << endl ;
    return ;
}


bool get_data()
{
        bool flag = true ;
        seraddr_len = sizeof(seraddr) ;
        int retsize = recvfrom(socket_id, packet_buffer, max_size , 0, (sockaddr*) &seraddr, (socklen_t *)&seraddr_len);
        if(retsize == -1)
        {
            cout << "\nRecv Error : " ;

     /*       cout << "\nRecv Error : " << WSAGetLastError();

            if (WSAGetLastError() == WSAEWOULDBLOCK || WSAGetLastError() == 0)
            {
                return "";
            }
    */
            return flag ;
        }
        charcount = 0 ;
        fbytes = (int)packet_buffer[0] ;
        sbytes = (int)packet_buffer[1] ;
        if(sbytes==3)
        {
            fbytes = (int)packet_buffer[2] ;
            sbytes = (int)packet_buffer[3] ;
            sbytes = sbytes + ( fbytes << 8 ) ;
            if(sbytes == packet_count+1)
            {
                cout << "Packet No."<< sbytes << "Recieved  of size =" << retsize - 4 << " bytes" << endl ;
                packet_count++ ;

                for(int i=2;i<retsize;i++,charcount++)
                    cout << packet_buffer[i]  ;

                if(retsize-4 < 512)
                return false ;

            }else
                cout << sbytes << " IS A DUPLICATE PACKET " << endl ;
            return true ;
        }else
        {
            cout << "ERROR :=" << endl ;
            return false ;
        }

}



// function should return sender address info (for the code the server)

int main()
{
    conn_start() ;
    read_request("input") ;

    while(get_data())
    {
        ack() ;
    }

    //cout << recvudp() << endl ;
    conn_close() ;
 //   int p = (int)sizeof(seraddr) ;
//  cout << recvudp(socket_id,500,seraddr,p) ;
    return 0 ;
}

