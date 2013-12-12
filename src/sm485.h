//Simplemotioh RS 485


#ifndef SM485
#define SM485

#include "Serial.h"
#include "types.h"
#include "RingBuffer.h"
#include "SMCommandInterpreter.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"


#define SM_CRCINIT 0x0
#define BUFSIZE 128
#define PAYLOAD_BUFSIZE 128
#define CMDBUFSIZE 2048

#define SMREGISTER_SIZE 32

//cmd number must be 0-31

#define SM_VERSION 90
#define SM_VERSION_COMPAT 85

#define MAX_PAYLOAD_BYTES 120

//00xb
#define SMCMD_MASK_0_PARAMS 0
//01xb, 2 bytes as payload
#define SMCMD_MASK_2_PARAMS 2
//10xb
#define SMCMD_MASK_N_PARAMS 4
//11xb
#define SMCMD_MASK_RESERVED 6
//xx1b
#define SMCMD_MASK_RETURN 1
//110b
#define SMCMD_MASK_PARAMS_BITS 6


//registers 0-8191 read write
//registers 8192-16383 read only
#define SMREGISTER_BUS_MODE 0
#define SMREGISTER_BUFFERED_CMD_PERIOD 1
//0 if not running buffered cmds, cmdclock is kept reset then
#define SMREGISTER_BUFFERED_CMD_ACTIVE 2
#define SMREGISTER_PROC_VARIABLE_OFFSET 3
#define SMREGISTER_PROC_RETURN_OFFSET 4
#define SMREGISTER_PROC_RETURN_DATA_LEN 5

#define SMREGISTER_SMVERSION 8192
#define SMREGISTER_SMVERSION_COMPAT 8193
#define SMREGISTER_BUF_FREE 8194

#define SMERR_CRC 1
#define SMERR_INVALID_CMD 2
#define SMERR_BUF_OVERFLOW 4
#define SMERR_INVALID_PARAMETER 8
#define SMERR_PAYLOAD_SIZE 16


#define SMCMD_ID(cmdnumber, flags) (((cmdnumber)<<3)|flags)

#define SMCMD_ERROR_RET SMCMD_ID(3,SMCMD_MASK_2_PARAMS|SMCMD_MASK_RETURN)


//format 5, u8 bytesfollowing,u8 toaddr,cmddata,u16 crc
//return, 6, u8 bytesfollowing,u8 fromaddr, returndata,u16 crc
#define SMCMD_INSTANT_CMD SMCMD_ID(4,SMCMD_MASK_N_PARAMS)
#define SMCMD_INSTANT_CMD_RET SMCMD_ID(4,SMCMD_MASK_N_PARAMS|SMCMD_MASK_RETURN)


//addr always 0 = read by all nodes
//payload data is process image. each node takes own process data from offset SMREGISTER_PROC_VARIABLE_OFFSET
//return packet is similar but return data is loated in payload at SMREGISTER_PROC_RETURN_OFFSET
//teturn packet is specially generated by many nodes instead of one
#define SMCMD_PROCESS_IMAGE SMCMD_ID(5,SMCMD_MASK_N_PARAMS)
#define SMCMD_PROCESS_IMAGE_RET SMCMD_ID(5,SMCMD_MASK_N_PARAMS|SMCMD_MASK_RETURN)


//format  ID, u8 bytesfollowing,u8 toaddr,cmddata,u16 crc
//return, ID, u8 bytesfollowing,u8 fromaddr,returndata,u16 crc
#define SMCMD_BUFFERED_CMD SMCMD_ID(6,SMCMD_MASK_N_PARAMS)
#define SMCMD_BUFFERED_CMD_RET SMCMD_ID(6,SMCMD_MASK_N_PARAMS|SMCMD_MASK_RETURN)

//read return data from buffered cmds. repeat this command before SMCMD_BUFFERED_CMD until N!=120 from download
//#define SMCMD_BUFFERED_RETURN_DATA SMCMD_ID(7,SMCMD_MASK_0_PARAMS)
//#define SMCMD_BUFFERED_RETURN_DATA_RET SMCMD_ID(7,SMCMD_MASK_N_PARAMS|SMCMD_MASK_RETURN)


// cmd u8 ID, u8 toaddr, u8 crc
// ret u8 ID, u8 fromaddr, u16 bytes,u16 crc
//#define SMCMD_BUF_FREE SMCMD_ID(8,SMCMD_MASK_0_PARAMS)
//#define SMCMD_BUF_FREE_RET SMCMD_ID(8,SMCMD_MASK_2_PARAMS|SMCMD_MASK_RETURN)


//cmd 20,u8 toaddr, u8 crc
//ret 21, u8=2, u8 fromaddr, u16 clock,u16 crc, muut clientit lukee t?n
//oldret 21, u8 fromaddr, u16 clock,u16 crc, muut clientit lukee t?n
//SM clock rate is 10000Hz
#define SMCMD_GET_CLOCK SMCMD_ID(10,SMCMD_MASK_0_PARAMS)
#define SMCMD_GET_CLOCK_RET SMCMD_ID(10,SMCMD_MASK_2_PARAMS|SMCMD_MASK_RETURN)

//payload data is sent to the bus unmodified, no other return data
#define SMCMD_ECHO SMCMD_ID(12,SMCMD_MASK_N_PARAMS)

//modes:
//0=default, return data follows after each smcmd
//1=hold return data, must ask with SMCMD_GET_RETURN_DATA
//used for multiaxis chaining with single write & read

//cmd u8=4, u8 toaddr, u16 registernum, u16 registervalue ,u16 crc
//ret u8 fromaddr,u16 crc
#define SMCMD_SET_REGISTER SMCMD_ID(9,SMCMD_MASK_N_PARAMS)
#define SMCMD_SET_REGISTER_RET SMCMD_ID(9,SMCMD_MASK_0_PARAMS|SMCMD_MASK_RETURN)

// cmd u8 ID, u8 toaddr, u16 registernum u8 crc
// ret u8 ID, u8 fromaddr, u16 registervalue ,u16 crc
#define SMCMD_GET_REGISTER SMCMD_ID(8,SMCMD_MASK_2_PARAMS)
#define SMCMD_GET_REGISTER_RET SMCMD_ID(8,SMCMD_MASK_2_PARAMS|SMCMD_MASK_RETURN)


//cmd 28, u8 toaddr, u16 mode,u16 crc
//ret 29, u8 fromaddr,u16 crc
//#define SMCMD_GET_RETURN_DATA SMCMD_ID(15,SMCMD_MASK_0_PARAMS)

void SM_init();
void SM_buffered_cmd_update();
u8 SM_update();



class PhysicalIO;
class System;

class SimpleMotionComm
{
public:
	SimpleMotionComm( Serial *port, System *parent, int myAddress );
	~SimpleMotionComm();
	/*poll serial line and handle data*/
	u8 poll();
	/*executes command from user cmd buffer */
	void bufferedCmdUpdate();


	/* in getters "attribute" is SMP parameter attribute bits (actual value/min/max selector) */
	int getMyAddress(u16 attribute) const;
    u16 getSmBusClock(u16 attribute) const;
    void setMyAddress(int myAddress);
    void setSmBusClock(u16 smBusClock);
    //get amount of free space in cmd buffer
    //int getCmdBufferBytesFree(u16 attribute) const;
    s32 getBusBaudRate(u16 attribute) const;
    u16 getBusMode(u16 attribute) const;
    bool setBusBaudRate(s32 busBaudRate);//return false on fail
    bool setBusMode(u16 busMode);//return false on fail
    u16 getBusBufferedCmdPeriod(u16 attribute) const;
    u16 getBusTimeout(u16 attribute) const;
    bool setBusBufferedCmdPeriod(u16 busBufferedCmdPeriod);//return false on fail, unit 100us
    bool setBusTimeout(u16 busTimeout);//return false on fail, unit 100us
    s16 getSMBusVersion(u16 attribute) const; //this interpreter latest supported protocol version
    s16 getSMBusCompatVersion(u16 attribute) const; //oldest protocol version supported by this interpreter
    s16 getBufferedCmdStatus(u16 attribute) const;
    u32 getSMBusBufferFreeBytes(u16 attribute) const;
    void loopBackComm();
    void incrementSmBusClock( int cycles ); //increment internal clock
    void abortBufferedCmdExecution();//abort buffered cmd execution and return reference to safe value. call for example by user stop command or if comm error occurs (SM watchdog, buffer underrun etc)

    xSemaphoreHandle bufferMutex;
    xSemaphoreHandle SimpleMotionBufferedTaskSemaphore;

  //  typedef enum {Idle=0, Running=1 } BufferedCmdState;
private:
    bool receptionTimeouted();//return true if reception time has passed

    //friend class SMCommandInterpreter;//so SMCommandInterpreter can access private vars of this class
	//interpreters for some commands that are handled also in this side
	//SMCommandInterpreter localInstantCmdInterpreter;
	//SMCommandInterpreter localBufferedCmdInterpreter;

    void makeReturnPacket(u8 retid, u8 datalen, u8 *cmddata = NULL);
    void makeReturnPacket(u8 retid, u8 byte1, u8 byte2);
    void makeReturnPacket( u8 retid, RingBuffer &returnPayload );
    bool executeBufferedCmd();
    void executeSMcmd();
    void startProcessingBufferedCommands();
    void resetReceiverState();
    //returns number of bytes used and stores SMP command to newcmd
    int extractSMPayloadCommandFromBuffer( RingBuffer &buffer, SMPayloadCommandForQueue &newcmd, bool &notEnoughBytes );
    //return number of bytes written
    int insertSMPayloadRetToBuffer( RingBuffer &buffer, SMPayloadCommandRet32 ret );
    int myAddress;
    Serial *comm;
    RingBuffer userCmds, userCmdRets;
    u16 smBusClock;//running clock (incremented with GC cycles)
    u16 cmdClock;//clock of current buffer cmds
    s16 receivedPayloadSize/*, recvStorePosition*/; //number of bytes to expect data in cmd, -1=wait cmd header
    u8 receivedCMDID; // cmdid=0 kun ed komento suoritettu
    u8 receivedAddress;
    u16 receivedCRC;
    u16 receivedCRCHiByte; //bottom bits will be contains only 1 byte when read
    u16 userCommandsInBuf;
    s32 busBaudRate;
    u16 busMode;
    u16 busTimeout;//unit 100us/1. how long to wait before forget the incoming package
    u16 busBufferedCmdPeriod;//unit 100us/1
    u16 bufferedCmdStatus;//see SM_BUFCMD_STAT_ defs
    u32 receptionBeginTime;//in us, for timeouting
    enum RecvState{ WaitCmdId, WaitAddr, WaitPayloadSize, WaitPayload, WaitCrcHi, WaitCrcLo};
    enum RecvState receiverState, receiverNextState;

    RingBuffer payloadIn, payloadOut;

    xQueueHandle localInstantCmdDelayQueue,localBufferedCmdDelayQueue;
    xSemaphoreHandle mutex;

    System *parentSystem;
};

void SimpleMotionTask( void *pvParameters );
void SimpleMotionBufferedTask( void *pvParameters );

#endif
