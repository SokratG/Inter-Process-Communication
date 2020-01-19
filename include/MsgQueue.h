#ifndef __MESSAGE_QUEUE_H__
#define __MESSAGE_QUEUE_H__

#include <iostream>
#include <string>
#include <unistd.h>
#include <utility>
#include <memory>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <mqueue.h>


using std::string;
using std::pair;
using std::unique_ptr;

#define MSGNAME             "/server-msg-q"
#define MAX_MESSAGES        10
#define MAX_MSG_SIZE        256
#define MSG_BUFFER_SIZE     (MAX_MSG_SIZE + 10)
#define QUEUE_PERMISSIONS   0660

/*
typedef struct {
    long mq_flags;
    long mq_maxmsg;
    long mq_msgsize;
    long mq_curmsgs;
} mq_attr;
*/
/*
typedef enum{
    RECIEVER,
    SENDER
} MsgType;
*/



// To set msgQ attributes
// Test structure


using data_block = pair<unique_ptr<char[]>, int> ;

class MsgQueue
{
protected:
    string message_name;
    int msgq_fd = 0;
    int flags;
    mode_t mode = QUEUE_PERMISSIONS;
    unsigned priority = 0;
public:
    MsgQueue(const string& name, unsigned _priority);
    virtual ~MsgQueue() = 0;
    MsgQueue& operator=(const MsgQueue&) = delete;
    MsgQueue(const MsgQueue&) = delete;
    void sedMode(mode_t _mode);  
    int GetMsgQFD() const;
};

class MsgQueueRcvr : public MsgQueue
{
private:
    struct mq_attr msg_q_attr;
    size_t buffer_size;
public:
    MsgQueueRcvr(const string& name, unsigned _priority, size_t _buffer_size, const struct mq_attr* attr = nullptr);
    ~MsgQueueRcvr();
    MsgQueueRcvr& operator=(const MsgQueueRcvr&) = delete;
    MsgQueueRcvr(const MsgQueueRcvr&) = delete;

    data_block RecieveMessage();
};

class MsgQueueSendr : public MsgQueue
{
private:

public:
    MsgQueueSendr(const string& name, unsigned _priority);
    ~MsgQueueSendr();
    MsgQueueSendr& operator=(const MsgQueueSendr&) = delete;
    MsgQueueSendr(const MsgQueueSendr&) = delete;

    void SendMessage(const data_block& data);
};

class MsgServerReciever{
private:
    fd_set readfds;
    MsgQueueRcvr* msgrcvr;
    size_t array_size;
public:
    MsgServerReciever(MsgQueueRcvr* _msgrcvr, const size_t _array_size = 0);
    void ServerMessageLoop(const string& ends);
};

#endif /* _MESSAGE_QUEUE_H__ */