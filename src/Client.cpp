#include "Socket.h"
#include "Routing_table.h"
#include <iostream>
#include <signal.h>

using std::cout;
using std::endl;
using std::cerr;

#define CORRUPT -1

static bool stop = false;
static RoutingTable Route_Table;
// Break out of main infinit loop and infrom server of intent to disconnect
static string end{"-n-"};

static void signal_handler(int signal_num);

class Client
{
private:
    bool isLoop;
    SocketDomain sock;
    list<string> ReformattedString(stringstream& streambuff);
public:
    Client(bool loop = false) : isLoop(loop), sock(BUFFER_SIZE, SOCKET_NAME, domain_socket_type::SOCKET_STREAM){
        
    }
    void ClientLoop();
    void GetTable(const string& endsymbol);
    void GetMacTable(const string& endsymbol);
    void Disconnect();
};

Client client;

int main(int argc, char* argv[])
{
    
    cout << "Welcome to the IPC Routering!" << endl;
    client.ClientLoop();

    return 0;
}




void Client::ClientLoop()
{
    static pid_t pid = getpid();

    sighandler_t ret = signal(SIGINT, signal_handler);
    if (ret == SIG_ERR){
        cerr << "Error: unable to set signal handler." << endl;
        exit(EXIT_FAILURE);
    }
    ret = signal(SIGUSR1, signal_handler);
    if (ret == SIG_ERR){
        cerr << "Error: unable to set signal handler." << endl;
        exit(EXIT_FAILURE);
    }

    if (sock.RequestConnect() == -1){
        cerr << "Can't connect to server" << endl;
        exit(EXIT_FAILURE);
    }

    sock.SendString(std::to_string(pid));
    sock.SendString(end);
    cout << "PID of the process was sending: " <<  pid << endl;


    GetTable(end);

    GetMacTable(end);

    do{
        cout << "Get new routing table!" << endl;

        Route_Table.DisplayTable();

        string command;
        sock.RecieveStream(end) >> command;
        cout << command << endl;
        if (command == "disc"){
            Route_Table.FlushData();
            return;
        }
        else if(command == "route"){
            GetTable(end);
        }
        else if(command == "mac"){
            GetMacTable(end);
        }
        else{
            cerr << "Unknow command" << endl;
        }


    } while(!stop);
};


list<string> Client::ReformattedString(stringstream& streambuff)
{
    if (!streambuff)
        return list<string>();
    list<string> listString;

    string buff;

    while(std::getline(streambuff, buff)){
        listString.emplace_back(buff);
    }

    return std::move(listString);
}

void Client::GetTable(const string& endsymbol)
{
    stringstream tablestream(std::move(sock.RecieveStream(endsymbol)));

    if (tablestream.str().empty())
        return;    

    list<string> tablelist(std::move(ReformattedString(tablestream)));
  
    Route_Table.CopyTableList(tablelist);

    cout << "Was get from connection" << endl;
}


void Client::GetMacTable(const string& endsymbol)
{
    stringstream tablestream(std::move(sock.RecieveStream(endsymbol)));

    if (tablestream.str().empty())
        return;  

    list<string> tableMaclist(std::move(ReformattedString(tablestream)));

    Route_Table.CopyMacTableList(tableMaclist);

    cout << "Was get from connection" << endl;
}

void Client::Disconnect(){
    sock.SendString("disc");
    sock.SendString(end);
    sock.CloseDataSocket();
}

static void signal_handler(int signal_num){

    if (signal_num == SIGINT){
        client.Disconnect();
        stop = true;
    }
    else if (signal_num == SIGUSR1){
        Route_Table.FlushData();
    }   

}