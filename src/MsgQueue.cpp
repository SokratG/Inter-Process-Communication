#include "MsgQueue.h"
#include <string.h>
#include <assert.h>

#define OK 0
#define FAIL -1
//#define LOG_ON

using std::cout;
using std::cerr;
using std::endl;


MsgQueue::MsgQueue(const string& name, unsigned _priority) : message_name(name), priority(_priority)
{

}

MsgQueue::~MsgQueue(){

}

int MsgQueue::GetMsgQFD() const{
    return msgq_fd;
}

void MsgQueue::sedMode(mode_t _mode)
{
    assert(_mode >= 0);

    this->mode = _mode;
}



MsgQueueSendr::MsgQueueSendr(const string& name, unsigned _priority) : MsgQueue(name, _priority)
{
#ifdef LOG_ON
    cout << "try create message queue sender..." << endl;
#endif

    MsgQueue::flags = O_WRONLY | O_CREAT;

    MsgQueue::mode = 0;

    MsgQueue::msgq_fd = mq_open(name.c_str(), MsgQueue::flags, MsgQueue::mode, nullptr);

    assert(msgq_fd != FAIL);

#ifdef LOG_ON
    cout << "message queue sender was created" << endl;
#endif
};

MsgQueueSendr::~MsgQueueSendr(){
#ifdef LOG_ON
    cout << "Closing msg queue..." << endl;
#endif
    int result = mq_close(MsgQueue::msgq_fd);

    assert(result != FAIL);
#ifdef INCLUDE_UNLINK
    result = mq_unlink(MsgQueue::message_name.c_str());
    assert(result != FAIL);
#endif
}

void MsgQueueSendr::SendMessage(const data_block& data)
{
#ifdef LOG_ON
    cout << "Sending data to host process..." << endl;
#endif
    int result = 0;

    result = mq_send(MsgQueue::msgq_fd, data.first.get(), data.second + 1, MsgQueue::priority);

    if (result == FAIL){
        cerr << "Fail sending data " << endl;
    }
    
#ifdef LOG_ON
    cout << "Data sending complete" << endl;
#endif  
};


MsgQueueRcvr::MsgQueueRcvr(const string& name, unsigned _priority, 
                            size_t _buffer_size, const struct mq_attr* attr) : MsgQueue(name, _priority)
{
#ifdef LOG_ON
    cout << "try create message queue reciever..." << endl;
#endif
    if (attr == nullptr){
        msg_q_attr.mq_flags = 0;
        msg_q_attr.mq_maxmsg = MAX_MESSAGES;
        msg_q_attr.mq_msgsize = MAX_MSG_SIZE;
        msg_q_attr.mq_curmsgs = 0;
    }
    else
        msg_q_attr = *attr;

    buffer_size = _buffer_size;

    MsgQueue::flags = O_RDONLY | O_CREAT;

    MsgQueue::msgq_fd = mq_open(name.c_str(), MsgQueue::flags, MsgQueue::mode, &msg_q_attr);

    assert(msgq_fd != FAIL);

#ifdef LOG_ON
    cout << "message queue reciever was created" << endl;
#endif
};

MsgQueueRcvr::~MsgQueueRcvr(){
#ifdef LOG_ON
    cout << "Closing msg queue..." << endl;
#endif
    int result = mq_close(MsgQueue::msgq_fd);

    assert(result != FAIL);
#ifdef INCLUDE_UNLINK
    result = mq_unlink(MsgQueue::message_name.c_str());
    assert(result != FAIL);
#endif
}

data_block MsgQueueRcvr::RecieveMessage()
{
#ifdef LOG_ON
    cout << "Recieving data from host process..." << endl;
#endif

    data_block buffer(new char[buffer_size], buffer_size);

    memset(buffer.first.get(), 0, buffer.second);

    int result = 0;
    
    unsigned prio = MsgQueue::priority;

    result = mq_receive(MsgQueue::msgq_fd, buffer.first.get(), buffer.second, &prio);

    if (result == FAIL){
        cerr << "Fail recieving data " << endl;
    }

    buffer.second = strlen(buffer.first.get());

#ifdef LOG_ON
    cout << "Data Recieving complete" << endl;
#endif 

    return buffer;
};

MsgServerReciever::MsgServerReciever(MsgQueueRcvr* _msgrcvr, const size_t _array_size) : array_size(_array_size)
{
    assert(msgrcvr != nullptr); 
#ifdef LOG_ON
    cout << "Create server message" << endl;
#endif 
    msgrcvr = _msgrcvr;

    FD_ZERO(&readfds);
}

void MsgServerReciever::ServerMessageLoop(const string& ends)
{

#ifdef LOG_ON
    cout << "Try run server message..." << endl;
#endif 

    while(true){
        cout << "Wait recieve message .." << endl;

        int msgq_fds = msgrcvr->GetMsgQFD();

        FD_ZERO(&readfds);
        
        FD_SET(msgq_fds, &readfds);
        
        select(msgq_fds + 1, &readfds, NULL, NULL, NULL);

        if(FD_ISSET(msgq_fds, &readfds)){
            
            data_block buff(std::move(msgrcvr->RecieveMessage()));
            
            if (ends == buff.first.get())
                break;

            cout << "Data: " << buff.first.get() << " size: " << buff.second << endl;
#ifdef LOG_ON
            cout << "Recieve complete .." << endl;
#endif
        } 
    }
}