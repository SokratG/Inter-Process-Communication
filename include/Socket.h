#ifndef __SOCKET_H__
#define __SOCKET_H__

#include <iostream>
#include <string>
#include <list>
#include <sstream>
#include <utility>
#include <memory>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

using std::string;
using std::pair;
using std::unique_ptr;
using std::list;
using data_block = pair<unique_ptr<char[]>, int> ;
using std::stringstream;

#define SOCKET_NAME "/tmp/DemoSocket"
#define BUFFER_SIZE 256
#define BACK_LOG    20
#define MAX_CLIENT_SUPPORTED    32
#define ZERO 0

enum domain_socket_type{
    SOCKET_STREAM = SOCK_STREAM,
    SOCKET_DGRAM = SOCK_DGRAM
};


/*  
    struct sockaddr_un {
        sa_family_t sun_family;               // AF_UNIX 
        char        sun_path[108];            // pathname 
    };
*/

class Socket
{
protected:
    size_t Buffer_size = 0;
    int ret;
    int connection_socket;
    int result;
    string socket_name;
public:
    Socket(size_t buf_size, string sock_name, bool isUnlink);
    size_t getBufferSize() const;
    virtual ~Socket() = 0;
};




class SocketDomain : public Socket
{
private:
    domain_socket_type type_socket;
    struct sockaddr_un sockaddr;
    int data_socket = -1;
public:
    data_block RecieveData(const string& EndSymbolic);
    stringstream RecieveStream(const string& EndSymbolic);
    void SendData(const data_block& data);
    void SendString(const string& data);
public:
    SocketDomain(size_t buf_size, string socket_name, domain_socket_type type_sock, bool isUnlink = false);
    int bindConnection();
    int listenConnection(const int backlog = BACK_LOG);
    int AcceptConnection();
    int RequestConnect() noexcept;
    void SetDataSocket(int SetSock = -1) noexcept;
    void CloseDataSocket() noexcept;
    int GetHandleSocket(bool isConSock = false) const;
    ~SocketDomain();
};






#endif /* __SOCKET_H__ */
