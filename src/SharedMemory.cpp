#include "SharedMemory.h"
#include <string.h>
#include <assert.h>
#include <errno.h>
#define OK 0
#define FAIL -1
//#define LOG_ON

using std::cout;
using std::cerr;
using std::endl;


SharedMemory::SharedMemory(const string& name, size_t _mem_size) : mem_name(name), Mem_size(_mem_size)
{

    Mem_Addr = nullptr;

}

void* SharedMemory::GetMem(int Flags){

    void* ptrM = mmap(nullptr,  // let the kernel chose to return the base address of shm memory
                    Mem_size,   // sizeof the shared memory to map to virtual address space of the process
                    Flags,  // shared memory is Read
                    MAP_SHARED, // shared memory is accessible by different processes
                    SharedMemory::shmem_fd, // file descriptor of the shared memory 
                    0);         // offset from the base address of the physical/shared memory to be mapped
    
    if(ptrM == MAP_FAILED){
        cerr << "Error on mapping when using mmap()" << endl;
        return nullptr;
    }

    return ptrM;
}

SharedMemory::~SharedMemory(){    
};


ShdMemReader::ShdMemReader(const string& name, size_t _mem_size) : SharedMemory(name, _mem_size)
{
    /*Create a shared memory object in kernel space. If shared memory already
     * exists it will truncate it to zero bytes again*/

    SharedMemory::shmem_fd = shm_open(SharedMemory::mem_name.c_str(), O_CREAT | O_RDONLY, SharedMemory::mode);

    if (SharedMemory::shmem_fd == FAIL){
        cerr << "Error can't open file descriptor for shared memory." << endl;
        exit(EXIT_FAILURE);
    }

    // Now map the shared memory in kernel space into process's Virtual address space
    SharedMemory::Mem_Addr = SharedMemory::GetMem(PROT_READ);
    
    assert(SharedMemory::Mem_Addr != nullptr);

}


data_block ShdMemReader::readMem()
{
    assert(SharedMemory::Mem_Addr != nullptr);

    data_block buffer(new char[Mem_size+1], 0);

    memcpy(buffer.first.get(), SharedMemory::Mem_Addr, Mem_size);

    buffer.second = strlen(buffer.first.get());

    return std::move(buffer);
}

ShdMemReader::~ShdMemReader()
{
    int res =  munmap(SharedMemory::Mem_Addr, Mem_size);  

    if(res == FAIL){
        cerr << "munmap failed shared mem read" << endl;
    }

    res = close(shmem_fd);
    if(res == FAIL){
        cerr << "failed close file descriptor in shared mem read" << endl;
    }
}


ShdMemWriter::ShdMemWriter(const string& name, size_t _mem_size) : SharedMemory(name, _mem_size)
{

    SharedMemory::shmem_fd = shm_open(SharedMemory::mem_name.c_str(), O_CREAT | O_RDWR | O_TRUNC, SharedMemory::mode);

    if (SharedMemory::shmem_fd == FAIL){
        cerr << "Error can't open file descriptor for shared memory." << endl;
        exit(EXIT_FAILURE);
    }

    Truncate();


    // Now map the shared memory in kernel space into process's Virtual address space
    SharedMemory::Mem_Addr = SharedMemory::GetMem(PROT_WRITE);
}


void ShdMemWriter::Truncate(){
    if (ftruncate(SharedMemory::shmem_fd, Mem_size) == FAIL) {
        cerr << "Error on ftruncate to allocate size of shared memory region" << endl;
        return;
    }
}

void ShdMemWriter::writeMem(const data_block& data, bool empty){

    assert(SharedMemory::Mem_Addr != nullptr);
    /* Mem_Addr is the address in process's Virtual address space, just like any other address.
     * The Linux paging mechanism maps this address to starting address of the shared memory region 
     * in kernel space. Any operation performed by process on Mem_Addr address is actually the operation
     * performed in shared memory which resides in kernel*/
    memset(SharedMemory::Mem_Addr, 0, Mem_size);
    if (empty)
        return;
    memcpy(SharedMemory::Mem_Addr, data.first.get(), data.second);

}

ShdMemWriter::~ShdMemWriter(){

   int res =  munmap(SharedMemory::Mem_Addr, Mem_size);  

    if(res == FAIL){
        cerr << "munmap failed shared mem write" << endl;
    }

    res = close(shmem_fd);
    if(res == FAIL){
        cerr << "failed close file descriptor in shared mem write" << endl;
    }
}


