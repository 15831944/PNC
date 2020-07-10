#pragma once

#ifdef PEC_EXPORTS
#define PEC_API __declspec(dllexport)
#else
#define PEC_API __declspec(dllimport)
#endif

#include "I_DrawSolid.h"

//���˼·
//0.ԭ����Ӧ����PlateNC,�����༭��,����װƽ̨�й������ձ༭���ʼ�
//1.����ͨ�ù����༭������ͬʱ���ƶ������(��Ҫָ�ְ�)
//2.����CProcessPart����CLDSPart�༰���ݽ�����׼�еĹ�������&������Ϣ�������Ӧ
//3.����CProcessPartDraw��,���а���CProcessPart��Ϣ,ͬʱ���������ղ�ͼ������Ϣ����������ʾ������Ϣ��NC�ӹ���λ��Ϣ
//4.����IPEC�ӿ��༰��Ӧ��ʵ����
struct PEC_API IPEC{
	//����ģʽ
	static const WORD DRAW_MODE_EDIT		= 1;	//�༭ģʽ
	static const WORD DRAW_MODE_NC			= 2;	//NC����ģʽ(�ְ壺��ʾ���壬�Ǹ֣���ʾAB��)
	static const WORD DRAW_MODE_PROCESSCARD	= 3;	//���ղ�ͼģʽ(��ʾ��ע��Ϣ,�Ǹֻ������ó���ѹ��)
	//��������
	static const WORD CMD_VIEW_PART_FEATURE		= 1;
	static const WORD CMD_SPEC_WCS_ORIGIN		= 2;
	static const WORD CMD_SPEC_WCS_AXIS_X		= 3;
	static const WORD CMD_SPEC_WCS_AXIS_Y		= 4;
	static const WORD CMD_DEL_PART_FEATURE		= 5;
	static const WORD CMD_INSERT_PLATE_VERTEX	= 6;
	static const WORD CMD_CAL_PLATE_HUOQU_POS	= 7;
	static const WORD CMD_ROLL_PLATE_EDGE		= 8;
	static const WORD CMD_BEND_PLATE			= 9;
	static const WORD CMD_DIM_SIZE				= 10;
	static const WORD CMD_DEF_PLATE_CUT_IN_PT	= 11;
	static const WORD CMD_DRAW_CUTTING_TRACK	= 12;
	//
	static const WORD CMD_OVERTURN_PART				= 101;
	static const WORD CMD_ROTATEANTICLOCKWISE_PLATE	= 102;
	static const WORD CMD_ROTATECLOCKWISE_PLATE		= 103;
	static const WORD CMD_NWE_BOLT_HOLE				= 104;
	static const WORD CMD_CAL_PLATE_PROFILE			= 105;
	static const WORD CMD_OTHER						= 10000;

	virtual char GetCurPartType()=0;
	//�ְ����
	virtual void GetPlateNcUcs(GECS& cs)=0;
	//UI ��ͼ����
	virtual bool InitDrawEnviornment(HWND hWnd,I2dDrawing* p2dDraw,ISolidDraw* pSolidDraw,
		ISolidSet* pSolidSet,ISolidSnap* pSolidSnap,ISolidOper* pSolidOper)=0;
	virtual int SetDrawMode(WORD mode)=0;	//1.�༭ģʽ��2.NCģʽ��3.���ղ�ͼģʽ
	virtual int GetDrawMode()=0;
	virtual bool AddProcessPart(CBuffer &processPartBuffer,DWORD key)=0;		//��༭�������ӵ�ǰ������ʾ�Ĺ��չ���
	virtual bool GetProcessPart(CBuffer &partBuffer,int serial=0)=0;
	virtual bool UpdateProcessPart(CBuffer &partBuffer,int serial=0)=0;
	virtual void ClearProcessParts()=0;		//��ձ༭���е�ǰ���չ�����
	virtual void Draw()=0;
	virtual void ReDraw(int serial=0)=0;
	virtual bool SetWorkCS(GECS cs)=0;
	virtual void SetCallBackPartModifiedFunc(bool (*func)(WORD cmdType))=0;
	virtual bool SetCallBackProcessPartModified()=0;
	virtual bool SetCallBackOperationFunc()=0;		//Mouse or keyboard input->CWnd->DrawSolid->CWnd->PartsEditor
	virtual int GetSerial()=0;
	virtual void ExecuteCommand(WORD cmdType)=0;
	//��������
	virtual void SetProcessCardPath(const char* sCardDxfFilePath,BYTE angle0_plate1=0)=0;
	virtual void SetDrawBoltMode(BOOL bDisplayOrder)=0;
	virtual void UpdateJgDrawingPara(const char* para_buf,int buf_len)=0;
};

class PEC_API CPartsEditorFactory{
public:
	static IPEC* CreatePartsEditorInstance();
	static IPEC* PartsEditorFromSerial(long serial);
	static BOOL Destroy(long h);
};