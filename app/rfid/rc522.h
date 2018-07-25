#ifndef __RC522_H__
#define __RC522_H__

#include "c_stdint.h"

#ifdef __cplusplus
//extern "C"{
#endif

enum {
  // page0: for command and status
  Reserved__00,
  CommandReg,
  CommIEnReg,
  DivIEnReg,
  CommIrqReg,
  DivIrqReq,
  ErrorReg,
  Status1Reg,
  Status2Reg,
  FIFODataReg,
  FIFOLevelReg,
  WaterLevelReg,
  ControlReg,
  BitFramingReg,
  CollReg,
  Reserved__0f,
  // page1: register for command
  Reserved__10,
  ModeReg,
  TxModeReg,
  RxModeReg,
  TxControlReg,
  TxAutoReg,
  TxSelReg,
  RxSelReg,
  RxThresholdReg,
  DemodReg,
  Reserved__1a,
  Reserved__1b,
  MifareReg,
  Reserved__1d,
  Reserved__1e,
  SerialSpeedReg,
  // page2: register for CFG
  Reserved__20,
  CRCResultRegL,
  CRCResultRegH,
  Reserved__23,
  ModWidthReg,
  Reserved__25,
  RFCfgReg,
  GsNReg,
  CWGsPReg,
  ModGsPReg,
  TModeReg,
  TPrescalerReg,
  TReloadRegH,
  TReloadRegL,
  TCounterValueRegH,
  TCounterValueRegL,
  // page3: register for test
  Reserved__30,
  TestSel1Reg,
  TestSel2Reg,
  TestPinEnReg,
  TestPinValueReg,
  TestBusReg,
  AutoTestReg,
  VersionReg,
  AnalogTestReg,
  TestDAC1Reg,
  TestDAC2Reg,
  TestADCReg,
  Reserved__3c,
  Reserved__3d,
  Reserved__3e,
  Reserved__3f
};

typedef uint8_t u8;
/* CommandReg */
typedef struct {
  u8 Command:4;
  u8 PowerDown:1, RcvOff:1;
} bCommandReg;
/* CommIEnReg */
typedef struct{
  u8 TimerIEn:1, ErrIEn:1, LoAlertIEn:1, HiAlertIEn:1;
  u8 IdleEn:1, RxIEn:1, TxIEn:1, IRqInv:1;
} bCommIEnReg;
/* DivIEnReg */
typedef struct {
u8 :2, CRCIEn:1, :1;
  u8 SigninActIEn:1, :2, IRQPushPull:1;
} bDivEnReg;
/* CommIRqReg */
typedef struct {
  u8 TimerIRq:1, ErrIRq:1, LoAlertIRq:1, HiAlertIRq:1;
  u8 IdleIRq:1, RxIRq:1, TxIRq:1, Set1:1;
} bCommIRqReg;
/* DivIRqReq */
typedef struct {
u8 :2, CRCIRq:1, :1;
  u8 SigninActIRq:1, :2, Set2:1;
} bDivIRqReq;
/* ErrorReg */
typedef struct {
  u8 ProtocolErr:1, ParityErr:1, CRCErr:1, CollErr:1;
  u8 BufferOvfl:1, :1, TempErr:1, WrErr:1;
} bErrorReg;
/* Status1Reg */
typedef struct {
  u8 LoAlert:1, HiAlert:1, :1, TRunning:1;
  u8 IRq:1, CRCReady:1, CRCOk:1;
} bStatus1Reg;
/* Status2Reg */
typedef struct {
  u8 ModemState:3, MFCrypto1On:1;
  u8 :2, I2CforceHS:1, TempSensOff:1;
} bStatus2Reg;
/* FIFODataReg */
typedef struct {
  u8 FIFOData;
} bFIFODataReg;
/* FIFOLevelReg */
typedef struct {
  u8 FIFOLevel:7, FlushBuffer:1;
} bFIFOLevelReg;
/* WaterLevelReg */
typedef struct {
  u8 WaterLevel:6;
} bWaterLevelReg;
/* ControlReg */
typedef struct {
  u8 RxLastBits:3, :1;
  u8 BitAlways1:1, :1, TStartNow:1, TStopNow:1;
} bControlReg;
/* BitFramingReg */
typedef struct {
  u8 TxLastBits:3, :1;
  u8 RxAlign:3, StartSend:1;
} bBitFramingReg;
/* CollReg */
typedef struct {
  u8 CollPos:5, CollPosNotValid:1, :1, ValuesAfterColl:1;
} bCollReg;
/* ModeReg */
typedef struct {
  u8 CRCPreset:2, :1, PolSignin:1;
u8 :1, TxWaitRF:1;
} bModeReg;
/* TxModeReg */
typedef struct {
u8 :3, InvMode:1;
  u8 TxSpeed:3, TxCRCEn:1;
} bTxModeReg;
/* RxModeReg */
typedef struct {
u8 :3, RxNoErr:1;
  u8 RxSpeed:3, RxCRCEn:1;
} bRxModeReg;
/* TxControlReg */
typedef struct {
  u8 Tx1RFEn:1, Tx2RFEn:1, :1, Tx2CW:1;
  u8 InvTx1RFOff:1, InvTx2RFOff:1, InvTx1RFOn:1, InvTx2RFOn:1;
} bTxControlReg;
/* TxAutoReg */
typedef struct {
u8 :6, Force100ASK:1;
} bTxAutoReg;
/* TxSelReg */
typedef struct {
  u8 SigOutSel:4;
  u8 DriverSel:2;
} bTxSelReg;
/* RxSelReg */
typedef struct {
  u8 RxWait: 6, UartSel:2;
} bRxSelReg;
/* RxThresholdReg */
typedef struct {
  u8 CollLevel:3, :1;
  u8 MinLevel:4;
} bRxThresholdReg;
/* DemodReg */
typedef struct {
  u8 TauSync:2, TauRcv:2;
u8 :1, FixIQ:1, AddIQ:2;
} bDemodReg;
/* SerialSpeedReg */
typedef struct {
  u8 BR_T1:5, BR_T0:3;
} bSerialSpeedReg;
/* CRCResultRegH, CRCResultRegL */
typedef struct {
  u8 CRCResultLSB;
} bCRCResultRegL;
typedef struct {
  u8 CRCResultMSB;
} bCRCResultRegH;
/* ModWidthReg */
typedef struct {
  u8 ModWidth;
} bModWidthReg;
/* RFCfgReg */
typedef struct {
  u8 BitAlways1:4, RxGain:3;
} bRFCfgReg;
/* GsNReg */
typedef struct {
  u8 ModGsN:4, CWGsN:4;
} bGsNReg;
/* CWGsPReg */
typedef struct {
  u8 CWGsP:6;
} bCWGsPReg;
/* ModGsPReg */
typedef struct {
  u8 ModGsP:6;
} bModGsPReg;
/* TModeReg */
typedef struct {
  u8 TPrescaler_Hi:4;
  u8 TAutoRestart:1, TGated:2, TAuto:1;
} bTModeReg;
/* TPrescalerReg */
typedef struct {
  u8 TPrescaler_Lo;
} bTPrescalerReg;
/* TReloadRegH, TReloadRegL */
typedef struct {
  u8 TReloadVal_Lo;
} bTReloadRegL;
typedef struct {
  u8 TReloadVal_Hi;
} bTReloadRegH;
/* TCounterValRegH, TCounterValRegL */
typedef struct {
  u8 TCounterVal_Lo;
} bTCounterValRegL;
typedef struct {
  u8 TCounterVal_Hi;
} bTCounterValRegH;
/* TestSel1Reg */
typedef struct {
  u8 TstBusBitSel:3;
} bTestSel1Reg;
/* TestSel2Reg */
typedef struct {
  u8 TestBusSel:5, PRBS15:1, PRBS9:1, TstBusFlip:1;
} bTestSel2Reg;
/* TestPinEnReg */
typedef struct {
  u8 TestPinEn:7;
} bTestPinEnReg;
/* TestPinValueReg */
typedef struct {
  u8 TestPinValue:7, UseIO:1;
} bTestPinValueReg;
/* TestBusReg */
typedef struct {
  u8 TestBus;
} bTestBusReg;
/* AutoTestReg */
typedef struct {
  u8 SelfTest:4, :2, BitAlways1:1;
} bAutoTestReg;
/* VersionReg */
typedef struct {
  u8 Version;
} bVersionReg;
/* AnalogTestReg */
typedef struct {
  u8 AnalogSelAux2:4;
  u8 AnalogSelAux1:4;
} bAnalogTestReg;
/* TestDAC1Reg */
typedef struct {
  u8 TestDAC1:6;
} bTestDAC1Reg;
/* TestDAC2Reg */
typedef struct {
  u8 TestDAC2:6;
} bTestDAC2Reg;
/* TestADCReg */
typedef struct {
  u8 ADC_Q:4;
  u8 ADC_I:4;
} bTestADCReg;

// Command Set
#define CMD_IDLE               0
#define CMD_MEM                1
#define CMD_GENERATE_RANDOM_ID 2
#define CMD_CALC_CRC           3
#define CMD_TRANSMIT           4
#define CMD_NO_CMD_CHANGE      7
#define CMD_RECEIVE            8
#define CMD_TRANSCEIVE         12
#define CMD_MFAUTHENT          14
#define CMD_SOFT_RESET         15

#define CAST(T, val) (*(T*)&(val))

#define FIFO_SIZE 64

enum {
  RC522_ANTENNA_OFF,
  RC522_ANTENNA_ON
} AntennaStatus;

typedef struct {
  int8_t spi_id;
  u8     pin_ss;
  u8     pin_rst;
} *rfid_dev;

/* class rc522 */
void rc522_init( rfid_dev dev );
u8   rc522_readreg( rfid_dev dev, u8 addr );
void rc522_writereg( rfid_dev dev, u8 addr, u8 val );
void rc522_reset( rfid_dev dev );
void rc522_sendcmd( rfid_dev dev, u8 cmd,
		    u8 *arg=NULL, size_t szarg=0,
		    u8 *res=NULL, size_t *szres=NULL);
void rc522_antenna( rfid_dev, AntennaStatus );

#ifdef __cplusplus
//}
#endif

#endif//__RC522_H__
