#include<iostream>
#include<fstream>
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

#define RRQ   1
#define WRQ   2
#define DATA  3
#define ACK   4
#define ERROR 5
#define max_size 1000024

// global variable
string output_buffer ;
int socket_id , bind_id , seraddr_len ;
int packet_len , packet_count , packet_cur_size , fbytes, sbytes , charcount ;
char packet_buffer[max_size] ;
const string mode = "netascii" ;
struct hostent *hp ;
struct sockaddr_in myaddr , seraddr;
bool last_packet ;
char host[500] ;

void print_error(int i) ;
string Fill_buffer(string inputfile) ;

void conn_start()
{
    // create socket
    memset((char *)&myaddr , 0 , sizeof(myaddr)) ;
    myaddr.sin_family = AF_INET ;
    myaddr.sin_addr.s_addr = htonl(INADDR_ANY) ;
    myaddr.sin_port = htons(0) ;

    // bind socket
    socket_id = socket(AF_INET,SOCK_DGRAM,0) ;
    bind_id = bind(socket_id,(struct sockaddr *)&myaddr,sizeof(myaddr)) ;


    // server config
    hp = gethostbyname(host) ;
    memset((char*)&seraddr, 0, sizeof(seraddr));
    seraddr.sin_family = AF_INET;
    seraddr.sin_port = htons(69);
    memcpy((void *)&seraddr.sin_addr, hp->h_addr_list[0], hp->h_length);
}

void conn_close()
{
    close(socket_id) ;
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


    sendto(socket_id, packet_buffer, packet_len, 0 , (struct sockaddr *)&seraddr, sizeof(seraddr)) ;
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
                cout << endl << endl << "*** Packet No. "<< sbytes << " of size = " << retsize - 4 << " bytes is Recieved ***" << endl << endl ;
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
            fbytes = (int)packet_buffer[2] ;
            sbytes = (int)packet_buffer[3] ;
            sbytes = sbytes + ( fbytes << 8 ) ;
            print_error(sbytes) ;
            return false ;
        }

}

void send_ack()
{
    int temp = packet_count ;
    packet_len = 4 ;
    packet_buffer[0] = '\0' ;
    packet_buffer[1] = ACK + '\0' ;
    packet_buffer[3] = '\0' + (temp & 255) ;
    temp = temp >> 8 ;
    packet_buffer[2] = '\0' + (temp & 255) ;
    sendto(socket_id, packet_buffer, packet_len, 0 , (struct sockaddr *)&seraddr, sizeof(seraddr)) ;
    return ;
}


void write_request(string FileName)
{
    packet_count = 0 ;
    packet_len = 2 ;
    packet_buffer[0] = '\0' ;
    packet_buffer[1] = WRQ + '\0' ;
    charcount = 0 ;

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

    sendto(socket_id, packet_buffer, packet_len, 0 , (struct sockaddr *)&seraddr, sizeof(seraddr)) ;
    return ;
}


bool get_ack()
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
        return false ;
    }
    fbytes = (int)packet_buffer[0] ;
    sbytes = (int)packet_buffer[1] ;
    if(sbytes==4)
    {
        fbytes = (int)packet_buffer[2] ;
        sbytes = (int)packet_buffer[3] ;
        sbytes = sbytes + ( fbytes << 8 ) ;
        if(sbytes == packet_count)
        {
            cout << endl << endl << "*** Packet No. "<< sbytes << " is Ackhnoledged ***" << endl << endl ;
            return true ;
        }else
            cout << endl << endl << "*** Duplicate Packet No. "<< sbytes << " is Ackhnoledged ***" << endl << endl ;
        return false ;

    }else if(sbytes==5)
    {
        fbytes = (int)packet_buffer[2] ;
        sbytes = (int)packet_buffer[3] ;
        sbytes = sbytes + ( fbytes << 8 ) ;
        print_error(sbytes) ;
        return false ;

    }else
    {
        cout << "Something Wrong" << sbytes << endl ;
    }
}


void send_data(bool got_ack)
{
    int temp ;
    packet_len = 4 ;
    packet_buffer[0] = '\0' ;
    packet_buffer[1] = DATA + '\0' ;
    if(got_ack)
    {
        packet_count++ ;
        temp = packet_count ;
        packet_buffer[3] = '\0' + (temp & 255) ;
        temp = temp >> 8 ;
        packet_buffer[2] = '\0' + (temp & 255) ;
        int count = 1 ;
        int i ;
        for(i = charcount ; i<output_buffer.length() && count <= 512 ; i++ , charcount++ , packet_len++ , count++)
        {
            packet_buffer[packet_len] = output_buffer[i] ;
        }
        sendto(socket_id, packet_buffer, packet_len, 0 , (struct sockaddr *)&seraddr, sizeof(seraddr)) ;
        if(count < 512 && i >= output_buffer.length())
            last_packet = true ;
        return ;
    }
    temp = packet_count ;
    packet_buffer[3] = '\0' + (temp & 255) ;
    temp = temp >> 8 ;
    packet_buffer[2] = '\0' + (temp & 255) ;
    int count = 1 ;
    if(charcount-512 < 0)
        charcount = 0 ;
    else
        charcount -= 512 ;
    for(int i = charcount ; i<output_buffer.length() && count <= 512 ; i++ , charcount++ , packet_len++ , count++)
    {
        packet_buffer[packet_len] = output_buffer[i] ;
    }
    sendto(socket_id, packet_buffer, packet_len, 0 , (struct sockaddr *)&seraddr, sizeof(seraddr)) ;
    return ;
}


// function should return sender address info (for the code the server)

int main(int argc, char* argv[])
{
    bool got_ack ;
    int i ;
    string FileName,FileName1 ;
    strcpy(host,"localhost") ;
    conn_start() ;

    while(1)
    {
        cout << endl ;
        cout << "Enter 1:   If you want to change ServerName (Current Surver is \"Localhost\")." << endl ;
        cout << "Enter 2:   If you want to read file from server." << endl ;
        cout << "Enter 3:   If you want to write file to server." << endl << endl ;
        cout << "Enter Something else to EXIT." << endl ;
        cin >> i ;

        if(i==1)
        {
            cout << "Enter Host Name : " ;
            scanf("%s",host) ;
            cout << endl ;
            conn_close() ;
            conn_start() ;

        }else if(i==2)
        {
            cout << "Enter File Name(without space) : " ;
            cin >> FileName ;
            cout << endl ;
            read_request(FileName) ;
            while(get_data())
            {
                send_ack() ;
            }

        }else if(i==3)
        {
            cout << "Enter File Name(Give Complete Path if file is not in home folder) : " ;
            cin >> FileName ;
            cout << endl ;
            cout << "Enter File Name(This name will be shown on server) : " ;
            cin >> FileName1 ;
            Fill_buffer(FileName) ;
            write_request(FileName1) ;
            last_packet = false ;
            while(1)
            {
                got_ack = get_ack() ;
                if(got_ack && last_packet)
                    break ;
                send_data(got_ack) ;
            }
        }else
        {
            break ;
        }
    }
   conn_close() ;
   return 0 ;
}



string Fill_buffer(string inputfile)
{
  ifstream fd (inputfile.c_str());
  if (fd.is_open())
  {
    getline (fd, output_buffer , '\0') ;
    fd.close();
  }
  else cout << "Unable to open file" <<  endl ;
}


void print_error(int i)
{
    switch(i)
    {
            case 1 :    cout << "File not found" << endl ;break ;
            case 2 :    cout << "Access Violation" << endl ;break ;
            case 3 :    cout << "Disk Full or allocation exceeded" << endl ;break ;
            case 4 :    cout << "Illegal TFTP operation" << endl ;break ;
            case 5 :    cout << "Unknown transfer ID" << endl ;break ;
            case 6 :    cout << "File already exists" << endl ;break ;
            case 7 :    cout << "No such user" << endl ;break ;
            default:    cout << "undefined, see error message " << endl ; break ;
    }
}
