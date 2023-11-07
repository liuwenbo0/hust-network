#ifndef STOP_WAIT_RDT_RECEIVER_H
#define STOP_WAIT_RDT_RECEIVER_H
#include "RdtReceiver.h"
class StopWaitRdtReceiver :public RdtReceiver
{
private:
	int expectSequenceNumberRcvd;	// 期待收到的下一个报文序号
	Packet lastAckPkt;				//上次发送的确认报文

	int seqsize;
	int windowsize;
	int protocolnum;
	bool *recv;
	Message *mbuffer;

public:
	StopWaitRdtReceiver(int num);
	virtual ~StopWaitRdtReceiver();

public:
	
	void receive(const Packet &packet);	//接收报文，将被NetworkService调用
	bool isinwindow(int seqNum);
};

#endif

