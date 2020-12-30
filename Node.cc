#include "Node.h"
#define DEBUG
#ifdef DEBUG
#include <iostream>
#include <fstream>
using namespace std;

#endif
#include<stdio.h>
#include<string.h>
#include<math.h>
#include<time.h>
#include<stdlib.h>

vector<Node*> Node::datanodev;
double startTime;

Define_Module(Node);

Node::Node()//构造函数
{
    std::srand((unsigned int)std::time(NULL));
    networkInitializationEvent = DataGenerationEvent = DataQueueEmptyEvent = DataPacketAccessEvent
    = DIFS_CheckEvent = CW_CheckEvent = DataPacketTransmissionEvent
    = TransmissionStatusOperationEvent = CWStatusMinusOneEvent = randomChooseMinislotEvent = NULL;
}
Node::~Node()//析构函数
{
    cancelAndDelete(networkInitializationEvent);
    cancelAndDelete(DataGenerationEvent);
    cancelAndDelete(DataQueueEmptyEvent);
    cancelAndDelete(DataPacketAccessEvent);
    cancelAndDelete(DIFS_CheckEvent);
    cancelAndDelete(CW_CheckEvent);
    cancelAndDelete(DataPacketTransmissionEvent);
    cancelAndDelete(TransmissionStatusOperationEvent);
    cancelAndDelete(CWStatusMinusOneEvent);
    cancelAndDelete(randomChooseMinislotEvent);
}
void Node::initialize()//网络/节点构造函数
{
    cout<<"Initializing Start!"<<endl;
    cModule* parent = this->getParentModule();//模块指针指向当前指针所指的上层模块
    this->dataNodeNum = parent->par("dataNodeNum");
    this->rounds = parent->par("rounds");
    this->networkWidth = parent->par("width");
    this->networkHeight = parent->par("height");
    this->queueSize = parent->par("queueSize");//每个节点的队列
    this->DIFS = parent->par("DIFS");
    this->DIFS_checkInterval = parent->par("DIFS_checkInterval");
    this->CW_checkInterval = parent->par("CW_checkInterval");
    this->CWmin = parent->par ( "CWmin");
    this->datapackettransmissiontime = parent->par("data_packet_transmission_time");
    this->retryLimit = parent->par("retryLimit");
    this->backoffStageLimit = parent->par("backoffStageLimit");
    this->lambda = parent->par("lambda");
    this->datapacketSize = parent->par("packetSize");
    this->networkSpeed = parent->par("networkSpeed");
//    this->x = static_cast<double>(std::rand()%200-100);
//    this->y = static_cast<double>(std::rand()%200-100);
    this->x = par("x");
    this->y = par("y");
    this->setGateSize("in",this->dataNodeNum+3);
    this->setGateSize("out", this->dataNodeNum+3);
    this->getDisplayString().setTagArg("i",1,"red");//设置节点样式

    datanodev.push_back(this);//节点入向量
    networkInitializationEvent = new cMessage("Network_Initialization");
    DataGenerationEvent = new cMessage("Data_Generation");
    DataQueueEmptyEvent = new cMessage("Data_Queue_Empty");
    DataPacketAccessEvent = new cMessage("Data_Packet_Access");
    DIFS_CheckEvent = new cMessage("DIFS_Check");
    CW_CheckEvent = new cMessage("CW_Check");
    DataPacketTransmissionEvent = new cMessage("Data_Packet_Transmission");
    TransmissionStatusOperationEvent = new cMessage("Transmission_Status_Operation");
    CWStatusMinusOneEvent = new cMessage("CW_Status_Minus_One");
    randomChooseMinislotEvent = new cMessage("random_Choose_Minislot");

    networkInitializationEvent->setKind(EV_NETWORKINITIALIZATION);//选择事件进行仿真
    this->scheduleAt(simTime()+SLOT_TIME, networkInitializationEvent);//线程开启，并校正时间
    cout<<"Node Initialization Finished!"<<endl;
}
void Node::handleMessage(cMessage* msg)
{
    if(msg->isSelfMessage()){
        switch(msg->getKind())
        {
        case EV_NETWORKINITIALIZATION:
            networkInitialization();
            break;
        case EV_DATAGENERATION:
            dataPacketGeneration();
            break;
        case EV_DATAPACKETACCESS:
            dataPacketAccess();
            break;
        case EV_DATAQUEUEEMPTY:
            IsDataQueueEmpty();
            break;
        case EV_DIFSCHECK:
            DIFScheck();
            break;
        case EV_CWCHECK:
            CWcheck();
            break;
        case EV_DATAPACKETTRANSMISSION:
            DataPacketTransmission();
            break;
        case EV_TRANSMISSIONSTATUSOPERATION:
            TransmissionStatusZero();
            break;
        case EV_CWSTATUSMINUSONE:
            CWStatusMinusOne();
            break;
        }
    }
    else{
        delete msg;
    }
}
void Node::finish()
{
    char fname[100];
    ofstream constructionStream;
    sprintf(fname,"csma_ca_result.txt");//输出结果到txt方便处理数据
    constructionStream.open(fname,ofstream::app);

    int totalSentDataPackets=0;//发送数据包总数
    double DataThroughput=0.0;//数据吞吐量
    double endTime=SIMTIME_DBL(simTime());//网络运行结束时间
    if(this->getId()==3){
        for(int i=0;i<datanodev.size();i++){
            totalSentDataPackets +=datanodev[i]->sentdatapackets;//发送数据包总数=每个数据节点发送的数据包数量之和
        }
        double runningTime=(double)(endTime - startTime);//总运行时间
        //数据吞吐量=并发数/平均响应时间
        DataThroughput=(double)(totalSentDataPackets*this->datapacketSize)/(double)(runningTime*this->networkSpeed);
        cout<<"Running time:"<<runningTime<<" , Data throughput:"<<DataThroughput<< endl;
        cout<<"Total sent data packet:"<<totalSentDataPackets<<endl;
        cout<<"Total number of data node:"<<this->dataNodeNum
            <<",Normalized data throughput:" << DataThroughput << endl;

        constructionStream<< " "<<this->lambda<<"    "<<DataThroughput <<"    "<<this->dataNodeNum<<endl;
        constructionStream.flush();
        constructionStream.close();
    }
}
void Node::networkInitialization()
{
    cout<<"Network Initilization Start~"<<endl;
    this->cur_round = 0;
    this->dataqueue = 0;
    this->sentPackets = 0;//已发送包个数
    this->lossPackets = 0;//丢包个数，当一个数据包发送次数超过retryLimit时会将该包丢弃。
    this->generatedPackets = 0;
    this->retryCounter = 0;
    this->CW_counter = -1;//窗口计数器
    this->backoffStage = 0;
    this->DIFS_counter = 0;//DIFS计数器
    this->first_time_data_transmission = 0;//节点第一次传输数据包
    this->generateddatapackets = 0;
    this->transmission_status = 0;
    this->losseddatapackets = 0;
    this->sentdatapackets = 0;
    Busy = 0;
    srand((unsigned)time(0));

    startTime = SIMTIME_DBL(simTime());//网络开始运行时间

    dataPacketGeneration();//节点服从泊松分布产生数据包，也就是相邻两包间隔服从指数分布

    this->destinationNode = getRandomDestination(this->getId());
    //set CSMAEvent
    DataPacketAccessEvent->setKind(EV_DATAPACKETACCESS);
    this->scheduleAt(simTime() + SLOT_TIME, DataPacketAccessEvent);//simTim()返回当前时间
    cout<<"Network Initilization Finished~"<< endl;
}
void Node::dataPacketGeneration(){//时间间隔遵循泊松分布（生成数据包）
    //data packet generation poisson arrival
    if(this->dataQueue.empty() || (this->dataQueue.size() < this->queueSize)){
        VoicePacket* data = new VoicePacket();
        data->setSource_id(this->getId());
        data->setKind(MSG_DATA);
        data->setDestination_id(getRandomDestination(this->getId())->getId());//生成起点终点
        data->setGeneration_time(simTime().dbl());
        this->dataQueue.push(data);
        this->generateddatapackets++;
    }
    double time = exponential(1/this->lambda);//生成数据——exponential指数函数
    DataGenerationEvent->setKind(EV_DATAGENERATION);
    this->scheduleAt(simTime() + time, DataGenerationEvent);
}
void Node::dataPacketAccess(){//结束运行
    label = 0;
    this->cur_round++;
    if(this->cur_round >= this->rounds){
        label = 1;
        endSimulation();
    }
    Busy = 0;
    IsDataQueueEmpty();
}
void Node::IsDataQueueEmpty(){
    if(!this->dataQueue.empty()){
        DIFS_CheckEvent->setKind(EV_DIFSCHECK);
        this->scheduleAt(simTime() + DIFS_checkInterval, DIFS_CheckEvent);
    }else{
        DataQueueEmptyEvent->setKind(EV_DATAQUEUEEMPTY);
        this->scheduleAt(simTime() + DIFS_checkInterval, DataQueueEmptyEvent);
    }
}
void Node::DIFScheck(){//针对节点
    if((Busy == 0) && (DIFS_counter < 5)){
        DIFS_counter++;
        if(DIFS_counter == 5){
            DIFS_counter = 0;
            if(first_time_data_transmission == 0){
                transmission_status = 1;
                DataPacketTransmissionEvent->setKind(EV_DATAPACKETTRANSMISSION);
                this->scheduleAt(simTime() + SLOT_TIME, DataPacketTransmissionEvent);
            }else if(first_time_data_transmission == 1){
                if(CW_counter == -1){
                    CW_counter = (int)uniform(1,CWmin*getBackoffTime(backoffStage));//退避时间后，再进行发送
                }
                CW_CheckEvent->setKind(EV_CWCHECK);
                this->scheduleAt(simTime() + CW_checkInterval, CW_CheckEvent);
            }
        }
        else{
            DIFS_CheckEvent->setKind(EV_DIFSCHECK);
            this->scheduleAt(simTime() + DIFS_checkInterval, DIFS_CheckEvent);
        }
    }
    else{
        DIFS_counter = 0;
        first_time_data_transmission = 1;

        DIFS_CheckEvent->setKind(EV_DIFSCHECK);
        this->scheduleAt(simTime() - DIFS_checkInterval + datapackettransmissiontime
                + DIFS_checkInterval, DIFS_CheckEvent);
    }
}
int Node::getBackoffTime(int backoffstage){
    int backofftime = 1;
    backofftime = (int)pow(2, (double)backoffstage);
    return backofftime;
}
void Node::CWcheck(){//退避时间过后，进行争用
    if(Busy == 0 && CW_counter != 0){
        CW_counter--;
        if(CW_counter != 0){
            CW_CheckEvent->setKind(EV_CWCHECK);
            this->scheduleAt(simTime() + CW_checkInterval, CW_CheckEvent);
        }else{
            DataPacketTransmissionEvent->setKind(EV_DATAPACKETTRANSMISSION);
            this->scheduleAt(simTime() + SLOT_TIME, DataPacketTransmissionEvent);
        }
    }else if(Busy == 1 && CW_counter != 0){
        DIFS_CheckEvent->setKind(EV_DIFSCHECK);
        this->scheduleAt(simTime() - CW_checkInterval + datapackettransmissiontime
                + DIFS_checkInterval, DIFS_CheckEvent);
    }
}
void Node::DataPacketTransmission(){
    Busy = 1;
    int collision_indication = 0;
    if(first_time_data_transmission == 0){
        first_time_data_transmission = 1;
        for(int i=0; i<datanodev.size(); i++){
            if(datanodev[i]->transmission_status == 1 && datanodev[i]->getId() != this->getId()){//其他节点在传输，同时到5
                this->retryCounter++;
                collision_indication = 1;
                break;
            }
        }
        TransmissionStatusOperationEvent->setKind(EV_TRANSMISSIONSTATUSOPERATION);
        this->scheduleAt(simTime() + SLOT_TIME, TransmissionStatusOperationEvent);
    }else{
        for(int i=0; i<datanodev.size(); i++){
            if(datanodev[i]->CW_counter == 0 && datanodev[i]->getId() != this->getId()){//不是第一次传输
                this->retryCounter++;
                this->backoffStage++;
                collision_indication = 1;
                break;
            }
        }
        CWStatusMinusOneEvent->setKind(EV_CWSTATUSMINUSONE);
        this->scheduleAt(simTime() + SLOT_TIME, CWStatusMinusOneEvent);
    }
    if(collision_indication == 1){//有冲突
        if(backoffStage > backoffStageLimit){
            backoffStage = backoffStageLimit;
        }
        if(retryCounter > retryLimit){//重传次数太多，导致丢包
            this->retryCounter = 0;
            this->backoffStage = 0;
            this->dataQueue.pop();
            this->losseddatapackets++;
        }
    }else{//无冲突
        this->dataQueue.pop();
        this->sentdatapackets++;
        this->backoffStage = 0;
        this->retryCounter = 0;
    }
    DataPacketAccessEvent->setKind(EV_DATAPACKETACCESS);
    this->scheduleAt(simTime() - SLOT_TIME + datapackettransmissiontime, DataPacketAccessEvent);
}
Node* Node::getRandomDestination(int nodeId)//获得一个随机的nodeId
{
    int id;
    while(true){
        id = uniform(0,datanodev.size());
        if(datanodev[id]->getId() != nodeId){
            return datanodev[id];
        }
    }
}
void Node::TransmissionStatusZero(){
    this->transmission_status = 0;
}
void Node::CWStatusMinusOne(){
    this->CW_counter = -1;
}
