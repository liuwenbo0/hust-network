// StopWait.cpp : 定义控制台应用程序的入口点。
//


#include "Global.h"
#include "RdtSender.h"
#include "RdtReceiver.h"
#include "StopWaitRdtSender.h"
#include "StopWaitRdtReceiver.h"
#include <cstdio>
#include <string>

int main(int argc, char* argv[])
{
	FILE *file;
	file = fopen("setting.txt","r");
    if (file == NULL) {
        printf("Failed to open file\n");
        return 1;
    }
	char buffer[100];
    if (fgets(buffer, sizeof(buffer), file) != NULL) {
        // Remove the newline character if it's present
        if (buffer[strlen(buffer) - 1] == '\n') {
            buffer[strlen(buffer) - 1] = '\0';
        }
	}
	else {
        printf("File is empty\n");
    }
	RdtSender* ps = nullptr;
    RdtReceiver* pr = nullptr;
	std::string protocolName(buffer);
	if (protocolName == "GBN") {
		ps = new StopWaitRdtSender(1);
		pr = new StopWaitRdtReceiver(1);
	} else if (protocolName == "SR") {
		ps = new StopWaitRdtSender(2);
		pr = new StopWaitRdtReceiver(2);
	} else if (protocolName == "TCP") {
		ps = new StopWaitRdtSender(3);
		pr = new StopWaitRdtReceiver(3);
	} else {
		printf("Unknown protocol: %s\n", protocolName.c_str());
		fclose(file);
		return 1;
	}
	pns->setRunMode(1);  //VERBOS模式
//	pns->setRunMode(1);  //安静模式
	pns->init();
	pns->setRtdSender(ps);
	pns->setRtdReceiver(pr);
	pns->setInputFile("input.txt");
	pns->setOutputFile("output.txt");

	pns->start();

	delete ps;
	delete pr;
	delete pUtils;									//指向唯一的工具类实例，只在main函数结束前delete
	delete pns;										//指向唯一的模拟网络环境类实例，只在main函数结束前delete
	
	return 0;
}

