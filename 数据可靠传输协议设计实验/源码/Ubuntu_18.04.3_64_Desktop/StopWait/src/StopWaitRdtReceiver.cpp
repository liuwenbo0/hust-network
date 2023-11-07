
#include "Global.h"
#include "StopWaitRdtReceiver.h"


StopWaitRdtReceiver::StopWaitRdtReceiver(int num):expectSequenceNumberRcvd(0)
{
	protocolnum = num;
	seqsize = 10;
	windowsize = 5;
	if(protocolnum == 2){
		recv = (bool *)malloc(seqsize*sizeof(bool));
		if (recv == NULL) {
        printf("malloc failed!");
		}
		for (int i = 0; i < seqsize; i++)
			recv[i] = false;
		mbuffer = (Message *)malloc(seqsize*sizeof(Message));
	}
	lastAckPkt.acknum = -1; //初始状态下，上次发送的确认包的确认序号为-1，使得当第一个接受的数据包出错时该确认报文的确认号为-1
	lastAckPkt.checksum = 0;
	lastAckPkt.seqnum = -1;	//忽略该字段
	for(int i = 0; i < Configuration::PAYLOAD_SIZE;i++){
		lastAckPkt.payload[i] = '.';
	}
	lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);

}


StopWaitRdtReceiver::~StopWaitRdtReceiver()
{
}

void StopWaitRdtReceiver::receive(const Packet &packet) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(packet);

	//如果校验和正确，同时收到报文的序号等于接收方期待收到的报文序号一致
	if (checkSum == packet.checksum && this->expectSequenceNumberRcvd == packet.seqnum) {

		pUtils->printPacket("接收方正确收到发送方的报文", packet);
			
		//取出Message，向上递交给应用层
		Message msg;
		memcpy(msg.data, packet.payload, sizeof(packet.payload));
		pns->delivertoAppLayer(RECEIVER, msg);

		lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
		lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
		pUtils->printPacket("接收方发送确认报文", lastAckPkt);
		pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		this->expectSequenceNumberRcvd = (1 + this->expectSequenceNumberRcvd)%seqsize; //期待接收序号在0-seqsize之间切换

		if(protocolnum == 2){//also try to deliver buffered in-order packet
			recv[packet.seqnum] = false;
			int tmp = this->expectSequenceNumberRcvd;
			for (int i = tmp; i != (tmp + windowsize - 1) % seqsize; i = (i + 1) % seqsize)
			{
				if (recv[i] == 1)
				{
					pns->delivertoAppLayer(RECEIVER, mbuffer[i]);
					recv[i] = 0;
					this->expectSequenceNumberRcvd= (this->expectSequenceNumberRcvd + 1) % seqsize;
				}
				else
					break;
			} 
		}
	}
	else {
		if (checkSum != packet.checksum) {
			pUtils->printPacket("接收方没有正确收到发送方的报文,数据校验错误", packet);
		}
		else {
			pUtils->printPacket("接收方没有正确收到发送方的报文,报文序号不对", packet);
			if(protocolnum == 2)
			{
				//buffer packet
				if(isinwindow(packet.seqnum)){
					recv[packet.seqnum] = true;
					memcpy(mbuffer[packet.seqnum].data, packet.payload, sizeof(packet.payload));
				}
				lastAckPkt.acknum = packet.seqnum; //确认序号等于收到的报文序号
				lastAckPkt.checksum = pUtils->calculateCheckSum(lastAckPkt);
			}
			pUtils->printPacket("接收方发送确认报文", lastAckPkt);
			pns->sendToNetworkLayer(SENDER, lastAckPkt);	//调用模拟网络环境的sendToNetworkLayer，通过网络层发送确认报文到对方
		}
	}
}

bool StopWaitRdtReceiver::isinwindow(int seqNum)
{
	if ((this->expectSequenceNumberRcvd + windowsize) % seqsize > this->expectSequenceNumberRcvd)
		return (seqNum >= this->expectSequenceNumberRcvd) && (seqNum < (this->expectSequenceNumberRcvd + windowsize) % seqsize);
	else if ((this->expectSequenceNumberRcvd + windowsize) % seqsize < this->expectSequenceNumberRcvd)
		return (seqNum >= this->expectSequenceNumberRcvd) || (seqNum < (this->expectSequenceNumberRcvd + windowsize) % seqsize);
	else
		return false;
}