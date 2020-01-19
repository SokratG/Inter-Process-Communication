#ifndef __ROUTING_TABLE_H__
#define __ROUTING_TABLE_H__

#include <string>
#include <list>
#include <stdint.h>

using std::string;
using std::list;

const size_t MAX_LENTH_TYPE_INTERFACE = 32;
const size_t MAX_IP_ADDR_LEN = 16;
const size_t MAC_ADDR_LEN = 18;

#define LOWBOUNDOIF 3
#define MAC_CHANGED 2
#define ROUTE_CHANGED 1
typedef enum{
    CREATE,
    UPDATE,
    DELETE,
    SHOW,
    FLUSH
} OPCODE;


typedef struct{
    char dest[MAX_IP_ADDR_LEN];
    uint16_t mask;
    char gateway[MAX_IP_ADDR_LEN];
    char oif[MAX_LENTH_TYPE_INTERFACE]; // name of interface
} routing_table_entry;

typedef struct{
    char mac[MAC_ADDR_LEN];
} mac_table_entry;

class RoutingTable
{
private:

    list<routing_table_entry> RTable;
    list<mac_table_entry> MTable;

public:
    RoutingTable() = default;
    RoutingTable& operator=(const RoutingTable&) = delete;
    RoutingTable(const RoutingTable&) = delete;
    bool CopyTable(const list<routing_table_entry>& rt);
    bool CopyTableList(const list<string>& rt);
    bool CopyMacTable(const list<mac_table_entry>& mt);
    bool CopyMacTableList(const list<string>& mt);
    bool CreateEntry(const routing_table_entry& rte);
    bool CreateEntry(const mac_table_entry& mte);
    bool UpdateEntry(const routing_table_entry& rte);
    bool DeleteEntry(const char* dest,const uint16_t mask);
    bool DeleteEntry(const char* macaddr);
    void DisplayTable() noexcept;
    void FlushData() noexcept;
    list<routing_table_entry> getRouteTable() const; 
    list<string> getListTable() const;
    list<string> getListMacTable() const;
};



void ParseCommand(OPCODE opc, string& args, RoutingTable& rtable);


#endif /* __ROUTING_TABLE_H__ */