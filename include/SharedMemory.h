#include <iostream>
#include <memory>
#include <string>
#include <sys/mman.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility>

using std::string;
using std::pair;
using std::unique_ptr;

using data_block = pair<unique_ptr<char[]>, int> ;

#define DEFAULT_NAME         "/datasegment"
#define MEMORY_PERMISSIONS   0660
#define MEM_BUFFER_SIZE      256

typedef enum{
    CREAT = O_CREAT,
    RDONLY = O_RDONLY,
    WRONLY = O_WRONLY,
    RDWR = O_RDWR,
    APPEND = O_APPEND,
    TRUNC = O_TRUNC,
    EXCL = O_EXCL
} shared_flag;



class SharedMemory
{
protected:
    int shmem_fd;
    string mem_name;
    mode_t mode = MEMORY_PERMISSIONS;
    void* Mem_Addr;
    size_t Mem_size;
protected:
    virtual void* GetMem(int Flags);
public:
    SharedMemory(const string& name, size_t _mem_size);
    virtual ~SharedMemory() = 0;
    void setmode(mode_t _mode);
    int resize(size_t _newsize);
};



class ShdMemReader : public SharedMemory
{
private:

public:
    ShdMemReader(const string& name, size_t _mem_size);
    data_block readMem();
    ~ShdMemReader();
};



class ShdMemWriter : public SharedMemory
{
private:
    void Truncate();

public:
    ShdMemWriter(const string& name, size_t _mem_size);
    void writeMem(const data_block& data, bool empty = false);
    ~ShdMemWriter();
};



#ifdef TEST
class ShdMemRdWr : public SharedMemory
{
private:
    
public:
    ShdMemRdWr(const string& name, size_t _mem_size);
    ~ShdMemRdWr();
};
#endif