
#include "Global.h"
#include "StopWaitRdtSender.h"


StopWaitRdtSender::StopWaitRdtSender(int num):expectSequenceNumberSend(0)
{
	protocolnum = num;
	seqsize = 10;
	windowsize = 5;
	ack = NULL;
	pbuffer = (Packet *)malloc(seqsize*sizeof(Packet));
	if(protocolnum == 2){
		ack = (bool *)malloc(windowsize*sizeof(bool));
		for (int i = 0; i < seqsize; i++)
			ack[i] = false;
	}else if(protocolnum == 3){
		frcnt = 0;
	}
	sendbase = 0;
}


StopWaitRdtSender::~StopWaitRdtSender()
{
}

bool StopWaitRdtSender::getWaitingState() {
	return (sendbase + windowsize)%seqsize == this->expectSequenceNumberSend;
}

bool StopWaitRdtSender::send(const Message &message) {
	//sendbase is the oldest unacked packet num, send() send all packet in window then wait
	if (getWaitingState()) { //发送方处于等待确认状态
	 	return false;
	}
	//if(isinwindow(this->expectSequenceNumberSend))
	this->packetWaitingAck.acknum = -1; //忽略该字段
	this->packetWaitingAck.seqnum = this->expectSequenceNumberSend;
	this->packetWaitingAck.checksum = 0;
	memcpy(this->packetWaitingAck.payload, message.data, sizeof(message.data));
	this->packetWaitingAck.checksum = pUtils->calculateCheckSum(this->packetWaitingAck);

	pbuffer[this->packetWaitingAck.seqnum].acknum = -1; //忽略该字段
	pbuffer[this->packetWaitingAck.seqnum].seqnum = this->expectSequenceNumberSend;
	pbuffer[this->packetWaitingAck.seqnum].checksum = 0;
	memcpy(pbuffer[this->packetWaitingAck.seqnum].payload, message.data, sizeof(message.data));//buffer the packet to resend
	pbuffer[this->packetWaitingAck.seqnum].checksum = pUtils->calculateCheckSum(pbuffer[this->packetWaitingAck.seqnum]);

	pUtils->printPacket("发送方发送报文", this->packetWaitingAck);
	if(protocolnum == 1|| protocolnum == 3){
		if(this->packetWaitingAck.seqnum == sendbase)//timer for oldest in-flight packet
		{
			pns->startTimer(SENDER, Configuration::TIME_OUT,this->expectSequenceNumberSend);			//启动发送方定时器
			cout << "启动发送方定时器 " << this->expectSequenceNumberSend << endl;
		}
	}else if(protocolnum == 2){
		pns->startTimer(SENDER, Configuration::TIME_OUT,this->expectSequenceNumberSend);			//启动发送方定时器
		cout << "启动发送方定时器 " << this->expectSequenceNumberSend << endl;
		ack[this->expectSequenceNumberSend]=false;
	}
	pns->sendToNetworkLayer(RECEIVER, this->packetWaitingAck);								//调用模拟网络环境的sendToNetworkLayer，通过网络层发送到对方
	this->expectSequenceNumberSend = (this->expectSequenceNumberSend + 1) %seqsize;      //下一个发送序号在0-seqsize之间切换					
	return true;
	
}

void StopWaitRdtSender::receive(const Packet &ackPkt) {
	//检查校验和是否正确
	int checkSum = pUtils->calculateCheckSum(ackPkt);

	//如果校验和正确
	if (checkSum == ackPkt.checksum ) {
		if(protocolnum == 1|| protocolnum == 3){//不管是否确认序号=发送方已发送并等待确认的数据包序号，cumulative ACK都使sendbase移动至少一格
			if(isinwindow(ackPkt.acknum)){
				pns->stopTimer(SENDER, sendbase);		//关闭定时器
				cout << "关闭发送方定时器 " << sendbase << endl;
				sendbase = (ackPkt.acknum + 1) % seqsize;	
				//pUtils->printPacket("发送方正确收到确认", ackPkt);
				if(this->expectSequenceNumberSend != sendbase){//sendbase之后有包发出去了（包括sendbase），说明sendbase是oldest-infligt packet
					pns->startTimer(SENDER, Configuration::TIME_OUT, sendbase);			//重新启动发送方定时器
					cout << "启动发送方定时器 " << sendbase << endl;
				printSlideWindow();
				}
				if(protocolnum == 3){
					frcnt = 0;
				}
			}else if(protocolnum == 3){
				ofstream outfile("fast_retransmit.txt", std::ios::app);//// 使用追加模式打开文件来保存输出
				streambuf* original = std::cout.rdbuf();// 保存原来的标准输出流
				cout.rdbuf(outfile.rdbuf());// 重定向标准输出到文件
				frcnt++;
				cout<<"收到确认包 "<<ackPkt.acknum<<endl;
				if(frcnt>=3){
					pns->sendToNetworkLayer(RECEIVER, pbuffer[sendbase]); //resend unACKed segment with smallest seq #
					pns->startTimer(SENDER, Configuration::TIME_OUT,sendbase);
					cout << "快速重传发送报文 "<<sendbase<<endl;
					cout << "启动发送方定时器 "<<sendbase << endl;
				}
				cout.rdbuf(original);// 恢复标准输出
			}
		}else if(protocolnum == 2){
			if(isinwindow(ackPkt.acknum)){
				int tmp = sendbase;
				int i;
				for(i=tmp; i !=(sendbase+windowsize - 1)%seqsize ; i=(i+1)%seqsize){
					if(!ack[i]) break;
				} 
				if(ackPkt.acknum == i){
					sendbase = (ackPkt.acknum + 1)%seqsize;//if n smallest unACKed packet, advance window base to next unACKed seq #
					printSlideWindow();
				}else ack[ackPkt.acknum] = true;//mark packet n as received
				pns->stopTimer(SENDER, ackPkt.acknum);		//关闭定时器
				cout << "关闭发送方定时器 " << ackPkt.acknum << endl;
			}else{

			}//ignore
		}
	}
	else 
		pUtils->printPacket("确认包损坏", ackPkt);

}

void StopWaitRdtSender::timeoutHandler(int seqNum) {
	//唯一一个定时器,无需考虑seqNum
	//pUtils->printPacket("发送方定时器时间到，GBN重新发送所有包", this->packetWaitingAck);
	if(protocolnum == 1){
		cout << "发送方定时器 "<<seqNum<<" 时间到，GBN重新发送所有包"  << endl;
		pns->stopTimer(SENDER,seqNum);										//首先关闭定时器
		pns->startTimer(SENDER, Configuration::TIME_OUT,sendbase);		//重新开始定时器
		cout << "启动发送方定时器 "<<seqNum << endl;
		for(int i = sendbase; i != expectSequenceNumberSend; i=(i+1)%seqsize)//重新发送数据包
		{
			pns->sendToNetworkLayer(RECEIVER, pbuffer[i]);
			pUtils->printPacket("发送方发送报文", pbuffer[i]);
		}			
	}else if(protocolnum == 2 || protocolnum == 3){
		if(protocolnum == 2) cout << "发送方定时器 "<<seqNum<<" 时间到，SR重新发送包 "<< seqNum  << endl;
		if(protocolnum == 3) cout << "发送方定时器 "<<seqNum<<" 时间到，TCP重新发送包 "<< seqNum  << endl;
		pns->stopTimer(SENDER,seqNum);										//首先关闭定时器
		pns->sendToNetworkLayer(RECEIVER, pbuffer[seqNum]);
		pUtils->printPacket("发送方发送报文", pbuffer[seqNum]);
		pns->startTimer(SENDER, Configuration::TIME_OUT,seqNum);		//重新开始定时器
		cout << "启动发送方定时器 "<<seqNum << endl;
	}

}

void StopWaitRdtSender::printSlideWindow()
{
	ofstream outfile("slide.txt", std::ios::app);//// 使用追加模式打开文件来保存输出
	streambuf* original = std::cout.rdbuf();// 保存原来的标准输出流
	cout.rdbuf(outfile.rdbuf());// 重定向标准输出到文件
	int i;
	for (i = 0; i < seqsize; i++)
	{
		if (i == sendbase)
			cout << "(";
		if (i == expectSequenceNumberSend)
			cout << "[" << i <<"]";
		else
		{
			cout << i;
		}
		if (i == (sendbase + windowsize - 1) % seqsize)
			cout << ")";
		cout << "  ";
	}
	cout<<endl <<endl;
	cout.rdbuf(original);// 恢复标准输出
}

bool StopWaitRdtSender::isinwindow(int seqNum)
{
	if ((sendbase + windowsize) % seqsize > sendbase)
		return (seqNum >= sendbase) && (seqNum < (sendbase + windowsize) % seqsize);
	else if ((sendbase + windowsize) % seqsize < sendbase)
		return (seqNum >=sendbase) || (seqNum < (sendbase + windowsize) % seqsize);
	else
		return false;
}