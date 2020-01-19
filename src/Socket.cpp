#include "Socket.h"
#include <assert.h>
#include <iomanip>


#define OK 0
#define FAIL -1
//#define LOG_ON

using std::cout;
using std::cerr;
using std::endl;



Socket::Socket(size_t buf_size, string sock_name, bool isUnlink) : Buffer_size(buf_size)
{

    assert(buf_size > 0);


    // check if this socket is exist
    if (isUnlink)
        unlink(sock_name.c_str());

};

size_t Socket::getBufferSize() const
{
    return Buffer_size;
}

Socket::~Socket(){}


SocketDomain::SocketDomain(size_t buf_size, string sock_name, domain_socket_type type_sock, bool isUnlink) 
: Socket(buf_size, sock_name, isUnlink)
{ 
    
    // SOCK_DGRAM for Datagram based communication or SOCK_STREAM for stream data for communication
    Socket::connection_socket = socket(AF_UNIX, type_sock, 0);

    if (Socket::connection_socket == FAIL){
        cerr << "Fail create socket connection" << endl;
        exit(EXIT_FAILURE);
    }
#ifdef LOG_ON
    cout << "Socket was created" << endl;
#endif
    // Initialize
    memset(&this->sockaddr, 0, sizeof(struct sockaddr_un));

    // Specify the socket Cridentials
    sockaddr.sun_family = AF_UNIX; // unix domain sockets
    strncpy(sockaddr.sun_path, sock_name.c_str(), sizeof(sockaddr.sun_path) - 1);
};

int SocketDomain::bindConnection()
{

    // Bind socket to socket name.
    /* Purpose of bind() system call is that application() dictate the underlying 
     * operating system the criteria of recieving the data. Here, bind() system call
     * is telling the OS that if sender process sends the data destined to socket "name", 
     * then such data needs to be delivered to this server process (the server process)
    */
    Socket::ret = bind(Socket::connection_socket, (const struct sockaddr *) &sockaddr,
                        sizeof(struct sockaddr_un));

    if (ret == FAIL) {
        cerr << "Fail bind socket connection" << endl;
        return FAIL;
    }
#ifdef LOG_ON
    cout << "bind call succeed" << endl;
#endif    
    return OK;
}


int SocketDomain::listenConnection(const int backlog)
{

    /*
     * Prepare for accepting connections. The backlog size is set
     * to 20. So while one request is being processed other requests
     * can be waiting.
    */
    Socket::ret = listen(Socket::connection_socket, backlog);
    if (ret == FAIL) {
        cerr << "Fail listen socket connection" << endl;
        return FAIL;
    }
#ifdef LOG_ON
    cout << "listen call succeed" << endl;
#endif
    return OK;
}

void SocketDomain::SetDataSocket(int SetSock) noexcept
{
    assert(Socket::connection_socket != FAIL);

#ifdef LOG_ON
    cout << "set data socket..." << endl;
#endif

    this->data_socket = SetSock;

#ifdef LOG_ON
    cout << "data socket is set" << endl;
#endif
}

int SocketDomain::AcceptConnection()
{
    assert(Socket::connection_socket != FAIL);

#ifdef LOG_ON
    cout << "waiting to accept() sys call" << endl;
#endif
    this->data_socket = accept(Socket::connection_socket, NULL, NULL);

    if (data_socket == FAIL) {
        cerr << "Fail accept socket connection" << endl;
        return FAIL;    
    }  

#ifdef LOG_ON
    cout << "Connection accepted" << endl;
#endif
    return OK;
}


data_block SocketDomain::RecieveData(const string& EndSymbolic)
{
    assert(Socket::connection_socket != FAIL);
    assert(this->data_socket != FAIL);

    Socket::result = 0;
    std::unique_ptr<char[]> buffer(new char[Socket::Buffer_size]);

    std::stringstream buff;
    //std::streambuf *sbuf = std::clog.rdbuf(buff.rdbuf());
    //std::ostream read_buff(sbuf);
    
    while(true){
        // Prepare the buffer to recv the data
        memset(buffer.get(), 0, Socket::Buffer_size);

        // Wait for next data packet. 
        // Server is blocked here. Waiting for the data to arrive from client
        // 'read' is a blocking system call
#ifdef LOG_ON
        cout << "Waiting for data..." << endl;
#endif
        Socket::ret = read(this->data_socket, buffer.get(), Socket::Buffer_size);

        if (Socket::ret == FAIL){
            cerr << "Fail read socket data" << endl;
            return data_block(unique_ptr<char[]>(nullptr), NULL);  
        }

        if(EndSymbolic == buffer.get() || buff.str().size() >= Socket::Buffer_size) 
            break;

        // Collect received data.
        buff << buffer.get();
        Socket::result += strlen(buffer.get());
    }

    // write data buffer
    unique_ptr<char[]> retval(new char[Socket::result+1]);
    buff.read(retval.get(), Socket::result);

#ifdef LOG_ON
        cout << "Data complete read from the client" << endl;
#endif

    return data_block(std::move(retval), Socket::result);
};



stringstream SocketDomain::RecieveStream(const string& EndSymbolic)
{
    assert(Socket::connection_socket != FAIL);
    assert(this->data_socket != FAIL);

    Socket::result = 0;
    std::unique_ptr<char[]> buffer(new char[Socket::Buffer_size]);
    
    stringstream StreamBuffer;
    size_t esz = EndSymbolic.size();
    while(true){

        memset(buffer.get(), 0, Socket::Buffer_size);
  
#ifdef LOG_ON
        cout << "Waiting for data..." << endl;
#endif

        Socket::ret = read(this->data_socket, buffer.get(), Socket::Buffer_size);

        if (Socket::ret == FAIL){
            cerr << "Fail read socket data" << endl;
            return std::move(stringstream());  
        }

        stringstream ss;
        ss << buffer.get();

        if (!ss.str().compare(ss.str().size() - esz, esz, EndSymbolic)){
            string str{ss.str().substr(0, ss.str().size() - esz)};
            StreamBuffer << str;
            Socket::result += strlen(buffer.get());
            break;
        }
        

        // Collect received data.
        StreamBuffer << buffer.get();
        Socket::result += strlen(buffer.get());
    }

#ifdef LOG_ON
    cout << "Data complete read from the client" << endl;
#endif

    return std::move(StreamBuffer);
};



void SocketDomain::SendData(const data_block& data)
{
    if (data.second == 0)
        return;

    assert(Socket::connection_socket != FAIL);
    assert(this->data_socket != FAIL);

#ifdef LOG_ON
    cout << "sending data to the socket" << endl;
#endif

    Socket::ret = write(data_socket, data.first.get(), data.second);
    if (ret == FAIL) {
        cerr << "Fail send data to socket connection" << endl;
        return;
    }
#ifdef LOG_ON
    cout << "Data sending successfully" << endl;
#endif
};



void SocketDomain::SendString(const string& data)
{
    if (data.empty())
        return;

    assert(Socket::connection_socket != FAIL);
    assert(this->data_socket != FAIL);

#ifdef LOG_ON
    cout << "sending data to the socket" << endl;
#endif

    Socket::ret = write(data_socket, data.c_str(), data.size()); //Socket::Buffer_size
    if (ret == FAIL) {
        cerr << "Fail send data to socket connection" << endl;
        return;
    }

#ifdef LOG_ON
    cout << "Data sending successfully" << endl;
#endif
}




int SocketDomain::RequestConnect() noexcept
{
    assert(Socket::connection_socket != FAIL);    

    if (this->data_socket == FAIL){
        data_socket = Socket::connection_socket;
    }
#ifdef LOG_ON
    cout << "Requset connection..." << endl;
#endif
    Socket::ret = connect(data_socket, (const struct sockaddr *) &sockaddr,
                            sizeof(struct sockaddr_un));

    if (Socket::ret == FAIL) {
        cout <<  "The request server is down" << endl;
        return FAIL;
    }
#ifdef LOG_ON
    cout << "Requset connection complete successfully" << endl;
#endif
    return OK;
}

void SocketDomain::CloseDataSocket() noexcept
{
    if (data_socket == FAIL)
        return;
    // close the master socket
    close(data_socket);
#ifdef LOG_ON
    cout << "connection data socket closed.." << endl;
#endif
}

int SocketDomain::GetHandleSocket(bool isConSock) const
{  
    if (isConSock){
        return Socket::connection_socket;
    }
    else{
        return data_socket;
    }
}


SocketDomain::~SocketDomain()
{
    // close the master socket
    close(Socket::connection_socket);
    close(data_socket);
#ifdef LOG_ON
    cout << "connection closed.." << endl;
#endif
    /* Server should release resources before getting terminated.
     * Unlink the socket. */

    unlink(Socket::socket_name.c_str());
}