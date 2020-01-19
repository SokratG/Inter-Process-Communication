#include "Routing_table.h"
#include "SharedMemory.h"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <arpa/inet.h>

using std::cerr;
using std::cout;
using std::endl;

int NotifyChanged = 0;

static int isValidIP(const char* ip){
    struct sockaddr_in sa;
    return ip && (inet_pton(AF_INET, ip, &(sa.sin_addr)) != 0);
}
static int isValidMask(const uint16_t mask){
    return mask <= 32;
}
static int isValidMAC(const char* macaddr){
    if (macaddr == nullptr)
        return 0;
    
    size_t i = 0;
    for(; i < MAC_ADDR_LEN - 1; ++i){
        if (i % 3 != 2 && !isxdigit(macaddr[i]))
            return 0;
        if (i % 3 == 2 && macaddr[i] != ':')
            return 0;
    }
    
    return macaddr[i] == '\0';
}

static int isValidInterface(const char* oif){
    size_t lenoif = strlen(oif);
    if (lenoif < LOWBOUNDOIF || lenoif > MAX_LENTH_TYPE_INTERFACE)
        return 0;
    return 1;
}

static string getNextArgs(string& args){
    size_t offset = args.find(" ");
    if (offset == string::npos){
        return "";
    }
    string arg{args.begin(),args.begin() + offset};
    args.assign({args.begin()+ offset + 1, args.end()});
    return arg;
}

static bool createEntryRT(string& args,routing_table_entry* rte)
{
    string dest{getNextArgs(args)};
    if (dest.empty()){
        cout << "Empty Destination IP in " << args << " Try again." << endl;
        return false;
    } 
    strncpy(rte->dest, dest.c_str(), MAX_IP_ADDR_LEN);

    string mask{getNextArgs(args)};
    if (mask.empty()){
        cout << "Empty Subnet of IP in " << args << " Try again." << endl;
        return false;
    }
    rte->mask = std::stoul(mask);

    string gateway{getNextArgs(args)};
    if (gateway.empty()){
        cout << "Empty Gateway IP in " << args << " Try again." << endl;
        return false;
    }
    strncpy(rte->gateway, gateway.c_str(), MAX_IP_ADDR_LEN);

    string oif{args}; //last parameters
    strncpy(rte->oif, oif.c_str(), MAX_LENTH_TYPE_INTERFACE);
    return true;
}

void ParseCommand(OPCODE opc, string& args, RoutingTable& rtable)
{
    std::stringstream sargs(args); string isMAC;
    switch(opc){
        case OPCODE::CREATE:{
            routing_table_entry rte;
            mac_table_entry     mte;
            sargs >> isMAC;
            
            if (isValidMAC(isMAC.c_str())){
                strncpy(mte.mac, isMAC.c_str(), isMAC.size());
                if (rtable.CreateEntry(mte)){
                    cout << "Entry of mac create in table successfully." << endl;
                    NotifyChanged = MAC_CHANGED;
                }   
                else
                    cout << "Entry of mac fail create in table." << endl;
                return;
            }


            if (!createEntryRT(args, &rte))
                return;

            if (rtable.CreateEntry(rte)){
                cout << "Entry of ip create in table successfully." << endl;
                NotifyChanged = ROUTE_CHANGED;
            }   
            else
                cout << "Entry fail create in table." << endl;
            break;
        }
        case OPCODE::UPDATE:{
            routing_table_entry rte;
            if (!createEntryRT(args, &rte))
                return;
            
            if (rtable.UpdateEntry(rte)){
                cout << "Entry update in table successfully." << endl;
                NotifyChanged = ROUTE_CHANGED;
            }  
            else
                cout << "Entry fail update in table." << endl;
            
            break;
        }            
        case OPCODE::DELETE:{
            sargs >> isMAC;

            if (isValidMAC(isMAC.c_str())){
                if (rtable.DeleteEntry(isMAC.c_str())){
                    cout << "Entry of mac delete in table successfully.." << endl;
                    NotifyChanged = MAC_CHANGED;
                }  
                else
                    cout << "Entry of mac fail delete in table." << endl;
                return;
            } 

            string dest{getNextArgs(args)};
            if (dest.empty()){
                cout << "Empty Destination IP in " << args << " Try again." << endl;
                return;
            } 
            
            if (args.empty()){
                cout << "Empty Subnet of IP in " << args << " Try again." << endl;
                return;
            }
            uint16_t mask = std::stoul(args);
            
            if (rtable.DeleteEntry(dest.c_str(), mask)){
                cout << "Entry delete in table successfully." << endl;
                NotifyChanged = ROUTE_CHANGED;
            } 
            else
                cout << "Entry fail delete in table." << endl;
            break;
        }
        case OPCODE::SHOW:{
            rtable.DisplayTable();
            break;
        } 
        default:
            cerr << "error type of op command!" << endl;
            return;
    }
};


bool RoutingTable::CreateEntry(const routing_table_entry& rte)
{
    if (!isValidIP(rte.dest))
        return false;
    if (!isValidMask(rte.mask))
        return false;
    if (!isValidIP(rte.gateway))
        return false;
    if (!isValidInterface(rte.oif)){
        return false;
    }
    
    this->RTable.emplace_back(rte);
    return true;
}

bool RoutingTable::CreateEntry(const mac_table_entry& mte)
{
    this->MTable.emplace_back(mte);

    ShdMemWriter shmwr(mte.mac, strlen(mte.mac));

    cout << "Please enter ip address for connect to mac: ";
    string ip;
    std::getline(std::cin, ip);

    if (!isValidIP(ip.c_str()))
        return false;

    data_block db(new char[ip.size()+1], 0);
    strncpy(db.first.get(), ip.c_str(), ip.size());
    db.second = ip.size();

    shmwr.writeMem(db);

    return true;
}

bool RoutingTable::UpdateEntry(const routing_table_entry& rte)
{
    if (this->RTable.empty())
        return false;
    if (!isValidIP(rte.gateway))
        return false;
    if (!isValidInterface(rte.oif))
        return false;
    bool entryUpdate = false;
    
    for (std::list<routing_table_entry>::iterator It = RTable.begin(); It != RTable.end(); ++It){
        if (!strcmp(It->dest, rte.dest) && It->mask == rte.mask){
            strncpy(It->gateway, rte.gateway, MAX_IP_ADDR_LEN);
            strncpy(It->oif, rte.oif, MAX_LENTH_TYPE_INTERFACE);
            entryUpdate = true;
        }    
    }

    return entryUpdate;
}

bool RoutingTable::DeleteEntry(const char* dest,const uint16_t mask)
{
    if (this->RTable.empty())
        return false;
    if (!isValidIP(dest))
        return false;
    if (!isValidMask(mask))
        return false;
    bool entryDelete = false;

    //RTable.remove_if([&](routing_table_entry rte){return !strcmp(rte.dest, dest) && rte.mask == mask; }); 
    
    for (std::list<routing_table_entry>::iterator It = RTable.begin(); It != RTable.end(); ++It){
        if (!strcmp(It->dest, dest) && It->mask == mask){
            RTable.erase(It);
            entryDelete = true;
            break;
        }    
    }

    return entryDelete;
}

bool RoutingTable::DeleteEntry(const char* macaddr)
{
    bool entryDelete = false;
    string tempdelete;
    //MTable.remove_if([&](mac_table_entry mte){return !strcmp(mte.mac, macaddr); }); 
    
    for (std::list<mac_table_entry>::iterator It = MTable.begin(); It != MTable.end(); ++It){
        if (!strcmp(It->mac, macaddr)){
            tempdelete = It->mac;
            MTable.erase(It);
            entryDelete = true;
            break;
        }    
    }
    //delete entry in shared memory
    if (entryDelete){
        ShdMemWriter shmwr(tempdelete.c_str(), tempdelete.size());
        shmwr.writeMem(data_block(new char[MAC_ADDR_LEN+1], MAC_ADDR_LEN), true);
    }
   
    return entryDelete;
}

void RoutingTable::DisplayTable() noexcept
{
    using std::left;
    cout << "ROUTE TABLE:" << endl;
    cout << left << "|----------------------------------------------------------------------|" << endl; // 70 "-"
    cout << left << "|   " << "Destination Subnet(Key)" << "   |     ";
    cout << left << "Gateway IP" << "     |        ";
    cout << left << "OIF" << "        |" << endl;
    cout << left << "|----------------------------------------------------------------------|" << endl; // 70 "-"
    for (std::list<routing_table_entry>::const_iterator It = RTable.begin(); It != RTable.end(); ++It){
        printf("|   %s/%u          |   %s   |    %s       |\n", It->dest, It->mask, It->gateway, It->oif);
        cout << left << "|----------------------------------------------------------------------|" << endl; // 70 "-"
    }
    cout << "MAC TABLE:" << endl;
    cout << left << "|---------------------------------------------------------------|" << endl; //  "-"
    for (std::list<mac_table_entry>::const_iterator It = MTable.begin(); It != MTable.end(); ++It){
        ShdMemReader shmrd(It->mac, MAC_ADDR_LEN);
        data_block db(std::move(shmrd.readMem()));
        printf("|           %s           |     %s     |\n", It->mac, db.first.get());
        cout << left << "|---------------------------------------------------------------|" << endl; //  "-"
    }
    
}

list<routing_table_entry> RoutingTable::getRouteTable() const
{
    if (RTable.empty())
        return list<routing_table_entry>();

    return RTable;
}

list<string> RoutingTable::getListTable() const
{
    if (RTable.empty())
        return list<string>();

    list<string> temp;

    for (std::list<routing_table_entry>::const_iterator It = RTable.cbegin(); It != RTable.cend(); ++It){
        string buff{It->dest};
        buff.append(" ");
        buff.append(std::to_string(It->mask));
        buff.append(" ");
        buff.append(It->gateway);
        buff.append(" ");
        buff.append(It->oif);
        buff.append("\n");
        temp.emplace_back(buff);
    }
    return temp;
}


list<string> RoutingTable::getListMacTable() const
{
    if (MTable.empty())
        return list<string>();

    list<string> temp;

    for (std::list<mac_table_entry>::const_iterator It = MTable.cbegin(); It != MTable.cend(); ++It){
        string buff{It->mac};
        buff.append("\n");
        temp.emplace_back(buff);
    }
    return temp;
}


bool RoutingTable::CopyTable(const list<routing_table_entry>& rt){

    if (rt.empty())
        return false;

    RTable = std::move(rt);
    return true;
}

bool RoutingTable::CopyTableList(const list<string>& rt){

    if (rt.empty())
        return false;

    list<routing_table_entry> temptable;
    for (std::list<string>::const_iterator It = rt.cbegin(); It != rt.cend(); ++It){

        routing_table_entry rte;
        string buff{std::move(*It)};
        
        string temp{ getNextArgs(buff)};
        strncpy(rte.dest, temp.c_str(), temp.size());
       
        temp = std::move(getNextArgs(buff));
        rte.mask = std::stoul(temp);
        
        temp = std::move(getNextArgs(buff));
        strncpy(rte.gateway, temp.c_str(), temp.size());
        
        strncpy(rte.oif, buff.c_str(), buff.size());
        
        temptable.emplace_back(rte);
    }

    RTable = std::move(temptable);
    return true;
}

bool RoutingTable::CopyMacTable(const list<mac_table_entry>& mt){

    if (mt.empty())
        return false;

    MTable = std::move(mt);
    return true;
};

bool RoutingTable::CopyMacTableList(const list<string>& mt)
{
    if (mt.empty())
        return false;
    list<mac_table_entry> temptable;

    for (std::list<string>::const_iterator It = mt.cbegin(); It != mt.cend(); ++It){
        mac_table_entry mte;
        
        strncpy(mte.mac, It->c_str(), It->size());

        temptable.emplace_back(mte);
    }

    MTable = std::move(temptable);
    return true;
};


void RoutingTable::FlushData() noexcept
{
    if (!RTable.empty())
        RTable.clear();
    if (!MTable.empty())
        MTable.clear();
}