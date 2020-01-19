#include "Socket.h"
#include "Routing_table.h"
#include <iostream>
#include <algorithm>
#include <signal.h>

using std::cout;
using std::endl;
using std::cerr;


#define CORRUPT -1

extern int NotifyChanged;
static RoutingTable Route_Table;
bool trigCangeData = false;


static void signal_handler(int signal_num);

class ServerMultipexing
{
private:
    pid_t ServPID = 0;
    list<pair<pid_t, int>> TableIdClient;
    bool isLoop;
    SocketDomain sock;
    const size_t max_client = MAX_CLIENT_SUPPORTED;
    fd_set readfds;
    string end;
    /*  An array of File descriptors which the server process
     *  is maintaining in order to talk with the connected clients.
     *  Master skt FD is also a member of this array */
    int monitored_fd_set[MAX_CLIENT_SUPPORTED];
    /* Each connected client's intermediate result is 
     * maintained in this client array. */ 
    int client_result[MAX_CLIENT_SUPPORTED] = {0};
    // Remove all the FDs, if any, from the the array
    void intitiaze_monitor_fd_set();
    // Add a new FD to the monitored_fd_set array
    void add_to_monitored_fd_set(int skt_fd);
    // Remove the FD from monitored_fd_set array
    void remove_from_monitored_fd_set(int skt_fd);
    // Clone all the FDs in monitored_fd_set array into fd_set
    void refresh_fd_set(fd_set *fd_set_ptr);
    // Get the numerical max value among all FDs which server is monitoring
    int  get_max_fd() const;
private:
    bool send_route_table_connection(const string& endsymbol);
    bool send_mac_table_connection(const string& endsymbol);
    bool erasePID_client(int sock);
public:
    ServerMultipexing(bool loop = false) : isLoop(loop), sock(BUFFER_SIZE, SOCKET_NAME, domain_socket_type::SOCKET_STREAM, true){
        
        intitiaze_monitor_fd_set();

        add_to_monitored_fd_set(ZERO);
    }
    void Run();
    void HandleCommand(const string& strcmd);
    void FlushData();
    void Stop();
};

ServerMultipexing servM;

int main(int argc, char* argv[])
{

    cout << "Welcome to the IPC Routering!" << endl;
    cout << "Please enter your command with arguments(enter \"help\" for get list of commands)" << endl;

    servM.Run();
   

    return 0;
}

void ServerMultipexing::Run()
{
    string cmd;

    sighandler_t ret = signal(SIGINT, signal_handler);
    if (ret == SIG_ERR){
        cerr << "Error: unable to set signal handler." << endl;
        exit(EXIT_FAILURE);
    }

    sock.bindConnection();
    sock.listenConnection();
    
    // Add master socket to Monitored set of FDs
    add_to_monitored_fd_set(sock.GetHandleSocket(true));
    
    end = "-n-";
    
    while(true){

        refresh_fd_set(&readfds); // Copy the entire monitored FDs to readfds

        // Call the select system call, server process blocks here. 
        // Linux OS keeps this process blocked untill the connection initiation request Or 
        // data requests arrives on any of the file Drscriptors in the 'readfds' set
        select(get_max_fd() + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(sock.GetHandleSocket(true), &readfds)){

            // Data arrives on Master socket only when new client connects with the server('connect' call is invoked on client side)
            cout << "New connection recieved recvd, accept the connection" << endl;
            
            sock.AcceptConnection(); //accept sock connection

            add_to_monitored_fd_set(sock.GetHandleSocket()); //add data socket for monitor FD
            
            // get pid new connection process
            string pid_str;
            sock.RecieveStream(end) >> pid_str;
            TableIdClient.emplace_back(std::stoi(pid_str), sock.GetHandleSocket());
            cout << "Get pid connection process: " << pid_str << endl;


            if (!send_route_table_connection(end)){
                cerr << "Data table don't send a new connection!" << endl;
            }
            if (!send_mac_table_connection(end)){
                cerr << "Data mac table don't send a new connection!" << endl;
            }
        }
        else if(FD_ISSET(0, &readfds)){
            sock.SetDataSocket(0);
            // get command
            std::getline(std::cin, cmd);

            // transofrm command to lower case
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            // handle command
            HandleCommand(cmd);
            
            if (NotifyChanged > 0){

                for (size_t i = 2; i < max_client; i++){

                    if (monitored_fd_set[i] == CORRUPT)
                        continue;
                    sock.SetDataSocket(monitored_fd_set[i]);
                    if (NotifyChanged == ROUTE_CHANGED){
                        sock.SendString("route\n");
                        sock.SendString(end);
                        if (!send_route_table_connection(end))
                            cerr << "Data table don't send a new connection!" << endl;
                    }
                    else if (NotifyChanged == MAC_CHANGED){
                        sock.SendString("mac\n");
                        sock.SendString(end);
                        if (!send_mac_table_connection(end))
                            cerr << "Data mac table don't send a new connection!" << endl;
                    }
                    else
                        cerr << "Unknow type changed message" << endl;
                }
                NotifyChanged = 0;
            }

            continue;
        }
        else{
        
            for (size_t i = 2; i < max_client; i++){

                if (monitored_fd_set[i] == CORRUPT)
                    continue;

                if(FD_ISSET(monitored_fd_set[i], &readfds)){
                    
                    sock.SetDataSocket(monitored_fd_set[i]);
                   
                    string buff;
                    sock.RecieveStream(end) >> buff;
                    
                    if (buff == "disc"){
                        //Try read data from socket
                        remove_from_monitored_fd_set(sock.GetHandleSocket());
                        sock.CloseDataSocket();
                    }
                    
                }
                //client_result[i] += TotalDataBytes; // sum bytes of data;
            }

        }

    }
}


void ServerMultipexing::HandleCommand(const string& strcmd)
{
    OPCODE opcode;
    if (strcmd.empty()){
        return;
    }
    else if (strcmd == "help"){
            cout << "Please select from the following options:\n";
            cout << "1.CREATE <Destination IP> <Mask (0-32)> <Gateway IP> <OIF (3-15)>\n";
            cout << "2.UPDATE <Destination IP> <Mask (0-32)> <New Gateway IP> <New OIF(3-15)>\n";
            cout << "3.DELETE <Destination IP> <Mask (0-32)>\n";
            cout << "4.CREATE <MAC>\n";
            cout << "5.DELETE <MAC>\n";
            cout << "6.SHOW\n";
            cout << "7.FLUSH\n";
            cout << "8.EXIT\n";
    }
    else{
        size_t ItSpace = strcmd.find(" ");
        string args;
        if (ItSpace != string::npos){
            string cmd{strcmd.begin(), strcmd.begin() + ItSpace};
            if (cmd == "create")
                opcode = OPCODE::CREATE;
            else if (cmd == "update")
                opcode = OPCODE::UPDATE;
            else if (cmd == "delete")
                opcode = OPCODE::DELETE;
            else{
                std::cerr << "Error: unknow type of command " << "\"" << cmd << "\"." << endl;
                return;
            }
            args.assign(strcmd.begin() + ItSpace + 1, strcmd.end());
        }
        else{
            if (strcmd == "show")
                opcode = OPCODE::SHOW;
            else if (strcmd == "flush"){
                opcode = OPCODE::FLUSH;
                FlushData();
                return;
            }
            else if(strcmd == "exit"){
                cout << "Goodbye..." << endl;
                exit(0);
            }
            else{
                std::cerr << "Error: unknow type of command " << "\"" << strcmd << "\"." << endl;
                return;
            }
        }
        ParseCommand(opcode, args, Route_Table);
    }
}

void ServerMultipexing::intitiaze_monitor_fd_set()
{
    for (size_t i = 0; i < max_client; ++i){
        monitored_fd_set[i] = CORRUPT;
    }
}

void ServerMultipexing::add_to_monitored_fd_set(int skt_fd)
{
    for (size_t i = 0; i < max_client; ++i){
        if (monitored_fd_set[i] != CORRUPT)
            continue;
        monitored_fd_set[i] = skt_fd;
        break;
    }
}

void ServerMultipexing::remove_from_monitored_fd_set(int skt_fd)
{
    for (size_t i = 0; i < max_client; ++i){
        if (monitored_fd_set[i] != skt_fd)
            continue;
        monitored_fd_set[i] = CORRUPT;
        break;
    }
}

void ServerMultipexing::refresh_fd_set(fd_set *fd_set_ptr)
{
    FD_ZERO(fd_set_ptr);
    for (size_t i = 0; i < max_client; ++i){
        if (monitored_fd_set[i] != CORRUPT)
            FD_SET(monitored_fd_set[i], fd_set_ptr);
    }
}

int ServerMultipexing::get_max_fd() const
{
    int max = -1;
    for (size_t i = 0; i < max_client; ++i){
        if (monitored_fd_set[i] > max)
            max = monitored_fd_set[i];
    }
    return max;
}


bool ServerMultipexing::send_route_table_connection(const string& endsymbol)
{
    list<string> EntriesTable{Route_Table.getListTable()};

    if (EntriesTable.empty()){
        sock.SendString(endsymbol);
        return false;
    }
    
    for (std::list<string>::iterator It = EntriesTable.begin(); It != EntriesTable.end(); ++It){
        string buffer{*It};
        sock.SendString(buffer);
    }
    
    // end data 
    sock.SendString(endsymbol);
    cout << "Table was sending to client.." << endl;
    return true;
}

bool ServerMultipexing::send_mac_table_connection(const string& endsymbol)
{
    list<string> EntriesTable{Route_Table.getListMacTable()};

    if (EntriesTable.empty()){
        sock.SendString(endsymbol);
        return false;
    }
    
    for (std::list<string>::iterator It = EntriesTable.begin(); It != EntriesTable.end(); ++It){
        string buffer{*It};
        sock.SendString(buffer);
    }
    
    // end data 
    sock.SendString(endsymbol);
    cout << "Mac table was sending to client.." << endl;
    return true;
}


void ServerMultipexing::Stop(){

    for (size_t i = 2; i < max_client; i++){

        if (monitored_fd_set[i] == CORRUPT)
            continue;

        //Try read data from socket
        sock.SetDataSocket(monitored_fd_set[i]);
        sock.SendString("disc");
        sock.SendString(end);
        remove_from_monitored_fd_set(sock.GetHandleSocket());
        sock.CloseDataSocket();
    }
    Route_Table.FlushData();
    exit(EXIT_SUCCESS);
}

void ServerMultipexing::FlushData()
{
    for (std::list<pair<pid_t, int> >::const_iterator It = TableIdClient.cbegin(); It != TableIdClient.cend(); ++It){
        kill(It->first, SIGUSR1);
        cout << It->first << " " << It->second << endl;
    }
}

bool ServerMultipexing::erasePID_client(int sock){
    bool complete = false;

    if (sock < 0)
        return complete;
    
    for (std::list<pair<pid_t, int> >::const_iterator It = TableIdClient.cbegin(); It != TableIdClient.cend(); ++It){
        if (It->second == sock){
            TableIdClient.erase(It);
            complete = true;
            break;
        }
    }

    return complete;   
}

static void signal_handler(int signal_num)
{
    if (signal_num == SIGINT)
    {
        servM.Stop(); 
    }
}

