#include<iostream>
#include<fstream>
#include<vector>
#include<cstdio>
#include<cstdlib>
#include<cstring>
#include<string>
#include<netdb.h>
#include<iostream>
#include<fstream>
#include<sys/socket.h>
#include<sys/types.h>
#include<netinet/in.h>
#include<sys/poll.h>

using namespace std ;

#define RRQ   1
#define WRQ   2
#define DATA  3
#define ACK   4
#define ERROR 5
#define MAX_SIZE 1000024

// global variable
string output_buffer ;
int socket_id , bind_id , seraddr_len , poll_count , poll_break ;
int packet_len , packet_count , packet_cur_size , fbytes, sbytes , charcount ;
char packet_buffer[MAX_SIZE] ;
const string mode = "octet" ;
struct hostent *hp ;
struct sockaddr_in myaddr , seraddr;
struct pollfd ufds ;
bool last_packet ;
char host[500] ;
ofstream myfile ;
void print_error(int i) ;
bool Fill_buffer(string inputfile) ;
string CONN_ERROR = "problem in sending data to server try after some time" ;

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


    // update values for polling
    ufds.fd = socket_id ;
    ufds.events = POLLIN ; // check for normal or out-of-band
}

void conn_close()
{
    close(socket_id) ;
}

bool read_request(string FileName)
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


    if(sendto(socket_id, packet_buffer, packet_len, 0 , (struct sockaddr *)&seraddr, sizeof(seraddr))==-1)
    {
        cout << "Unable to Send Read Request Resending..." << endl  ;
        return false ;
    }
    cout << "Read Request send to server for file name =" << FileName << endl  ;
    return true ;
}

bool get_data()
{
        int rv  ;
        seraddr_len = sizeof(seraddr) ;
        poll_break = 0 ;
        poll_count = 5 ;
        while(poll_count!=0)
        {
            rv = poll(&ufds, 1 , 3500);
            if(rv > 0)
                break ;

            if (rv == -1) {
                perror("poll"); // error occurred in poll()
                printf("poll error \n");
            } else if (rv == 0) {
                printf("Timeout occurred!  No data after 3.5 seconds.\n");
            }
            poll_count-- ;
        }
        if(!poll_count)
        {
            poll_break = 1 ;
            return true ;
        }
        if (rv == -1) {
            perror("poll"); // error occurred in poll()
            printf("poll error \n");
            return true ;
        } else if (rv == 0) {
            printf("Timeout occurred!  No data after 3.5 seconds.\n");
            return true ;
        }
        int retsize = recvfrom(socket_id, packet_buffer, MAX_SIZE , 0, (sockaddr*) &seraddr, (socklen_t *)&seraddr_len);
        if(retsize == -1)
        {
            cout << "\nRecv Error : " ;

            return true ;
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
                {
                    myfile << packet_buffer[i] ;
                    cout << packet_buffer[i]  ;
                }
                if(retsize-4 < 512)
                return false ;

                return true ;
            }else
                cout << sbytes << " IS A DUPLICATE PACKET " << endl ;
            return true ;
        }else
        {
            fbytes = (int)packet_buffer[2] ;
            sbytes = (int)packet_buffer[3] ;
            sbytes = sbytes + ( fbytes << 8 ) ;
            print_error(sbytes) ;
            poll_break = 1 ;
            return false ;
        }

}

bool send_ack()
{
    int temp = packet_count ;
    packet_len = 4 ;
    packet_buffer[0] = '\0' ;
    packet_buffer[1] = ACK + '\0' ;
    packet_buffer[3] = '\0' + (temp & 255) ;
    temp = temp >> 8 ;
    packet_buffer[2] = '\0' + (temp & 255) ;
    if(sendto(socket_id, packet_buffer, packet_len, 0 , (struct sockaddr *)&seraddr, sizeof(seraddr))==-1)
    {
        cout << "Unable to send acknoledge Resending..." << endl ;
        return false ;
    }
    return true ;
}


bool write_request(string FileName)
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

    if(sendto(socket_id, packet_buffer, packet_len, 0 , (struct sockaddr *)&seraddr, sizeof(seraddr))==-1)
    {
        cout << "Unable to Send Write Request Resending..." << endl  ;
        return false ;
    }
    cout << "Write Request send to server for file name =" << FileName << endl  ;
    return true ;
}


bool get_ack()
{
    seraddr_len = sizeof(seraddr) ;
    int rv  ;

    poll_break = 0 ;
    poll_count = 5 ;
    while(poll_count!=0)
    {
        rv = poll(&ufds, 1 , 3500);
        if(rv > 0)
            break ;

        if (rv == -1) {
            perror("poll"); // error occurred in poll()
            printf("poll error \n");
        } else if (rv == 0) {
            printf("Timeout occurred!  No data after 3.5 seconds.\n");
        }
        poll_count-- ;
    }
    if(!poll_count)
    {
        poll_break = 1 ;
        return false ;
    }
    if (rv == -1) {
        perror("poll"); // error occurred in poll()
        printf("poll error \n");
        return false ;
    } else if (rv == 0) {
        printf("Timeout occurred!  No data after 3.5 seconds.\n");
        return false ;
    }


    int retsize = recvfrom(socket_id, packet_buffer, MAX_SIZE , 0, (sockaddr*) &seraddr, (socklen_t *)&seraddr_len);

    if(retsize == -1)
    {
        cout << "\nRecv Error : " ;
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
        poll_break = 1 ;
        return false ;

    }else
    {
        cout << "Something Wrong" << sbytes << endl ;
    }
}


bool send_data(bool got_ack)
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
        if(sendto(socket_id, packet_buffer, packet_len, 0 , (struct sockaddr *)&seraddr, sizeof(seraddr))==-1)
        {
            cout << "Unable to send data Resending..." << endl ;
            return false ;
        }
        if(count < 512 && i >= output_buffer.length())
            last_packet = true ;
        return true ;
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
    if(sendto(socket_id, packet_buffer, packet_len, 0 , (struct sockaddr *)&seraddr, sizeof(seraddr))==-1)
    {
        cout << "Unable to send data Resending..." << endl ;
        return false ;
    }
    return true ;
}


// function should return sender address info (for the code the server)

int main(int argc, char* argv[])
{
    bool got_ack , flag ;
    int i , count ;



    string FileName,FileName1 ;
    strcpy(host,"localhost") ;
    conn_start() ;

    while(1)
    {
        cout << endl ;
        cout << "Enter 1:   If you want to change ServerName (Current Surver is \""<< host << "\")." << endl ;
        cout << "Enter 2:   If you want to read file from server." << endl ;
        cout << "Enter 3:   If you want to write file to server." << endl << endl ;
        cout << "Enter Something else to EXIT." << endl ;
        cin >> i ;

        if(i==1){
            cout << "Enter Host Name : " ;
            scanf("%s",host) ;
            cout << endl ;
            conn_close() ;
            conn_start() ;

        }else if(i==2)
        {
            conn_close() ;
            conn_start() ;

            cout << "Enter File Name(without space) : " ;
            cin >> FileName ;
            cout << endl ;

            myfile.open(FileName.c_str()) ;

            count = 15 ;
            flag = read_request(FileName) ;
            while(!flag && count)
            {
                flag = read_request(FileName) ;
                count-- ;
            }
            if(count==0)
            {
                cout << CONN_ERROR << endl ;
                continue ;
            }
            poll_break =  0 ;
            while(get_data())
            {
                if(poll_break)
                {
                    cout << CONN_ERROR << endl ;
                    break ;
                }

                count = 15 ;
                flag = send_ack() ;
                while(!flag && count)
                {
                    flag = send_ack() ;
                    count-- ;
                }
                if(count==0)
                {
                    cout << CONN_ERROR << endl ;
                    continue ;
                }
            }
            if(poll_break)
            {
                myfile.close() ;
                continue ;
            }
            count = 15 ;
            flag = send_ack() ;
            while(!flag && count)
            {
                flag = send_ack() ;
                count-- ;
            }
            if(count==0)
            {
                cout << CONN_ERROR << endl ;
                continue ;
            }
            myfile.close() ;

        }else if(i==3)
        {
            conn_close() ;
            conn_start() ;

            cout << "Enter File Name(File should in current folder) : " ;
            cin >> FileName ;
            cout << endl ;

            if(!Fill_buffer(FileName))
                continue ;


            count = 15 ;
            flag = write_request(FileName) ;
            while(!flag && count)
            {
                flag = write_request(FileName) ;
                count-- ;
            }
            if(count==0)
            {
                cout << CONN_ERROR << endl ;
                continue ;
            }

            last_packet = false ;
            poll_break =  0 ;
            while(1)
            {
                got_ack = get_ack() ;

                if(poll_break)
                {
                    cout << CONN_ERROR << endl ;
                    break ;
                }

                if(got_ack && last_packet)
                    break ;

                count = 15 ;
                flag = send_data(got_ack) ;
                while(!flag && count)
                {
                    flag = send_data(got_ack) ;
                    count-- ;
                }
                if(count==0)
                {
                    cout << CONN_ERROR << endl ;
                    continue ;
                }
            }

        }else
        {
            break ;
        }
    }
   conn_close() ;
   return 0 ;
}



bool Fill_buffer(string inputfile)
{
  ifstream fd (inputfile.c_str());
  if (fd.is_open())
  {
    getline (fd, output_buffer , '\0') ;
    fd.close();
    return true ;
  }
  else cout << "Unable to open file. \n  file should be in same directory." <<  endl ;
  return false ;
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
