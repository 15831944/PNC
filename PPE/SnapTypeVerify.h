#pragma once
#include "I_DrawSolid.h"
struct OBJPROVIDER{
	static const BYTE DRAWINGSPACE = 1;
	static const BYTE SOLIDSPACE = 2;
	static const BYTE LINESPACE = 3;
};
class CSnapTypeVerify : public ISnapTypeVerify
{
	DWORD m_dwDrawingSpaceFlag;	//provider=1
	DWORD m_dwSolidSpaceFlag;	//provider=2
	DWORD m_dwLineSpaceFlag;	//provider=3
public:
	CSnapTypeVerify(int provider=OBJPROVIDER::SOLIDSPACE,DWORD dwVerifyFlag=0xffffffff);
	virtual ~CSnapTypeVerify(void){;}
public:
	void  ClearSnapFlag(){m_dwDrawingSpaceFlag=m_dwSolidSpaceFlag=m_dwLineSpaceFlag=0;}
	DWORD SetVerifyFlag(int provider,DWORD dwFlag=0);
	DWORD AddVerifyFlag(int provider,DWORD dwFlag);
	DWORD AddVerifyType(int provider,int iObjClsType);
public:	//ISnapTypeVerify�̳з���
	virtual bool IsValidObjType(char ciProvider,int iObjClsType);
	virtual bool IsValidEntityObjType(int iObjClsType);
	virtual bool IsValidSolidObjType(int iObjClsType);
	virtual bool IsValidGeObjType(int iObjClsType);
	virtual bool IsEnableSnapSpace(int providerSpace);	//1.DRAWINGSPACE;2.SOLIDSPACE;3.LINESPACE
	virtual DWORD GetSnapTypeFlag(char ciProvider);
};

struct SELOBJ{
	BYTE provider;
	BYTE ciTriggerType;	//0.�������ʰȡ;1:�Ҽ���ֹ;2:��Χ������ֹ;3:�س���ȷ���˳�;4:ESC����ֹ
	static const BYTE TRIGGER_LBUTTON	=0x00;
	static const BYTE TRIGGER_RBUTTON	=0x01;
	static const BYTE TRIGGER_PROCESS	=0x02;
	static const BYTE TRIGGER_KEYRETURN =0x03;
	static const BYTE TRIGGER_KEYESCAPE =0x04;
	union{
		struct{
			long idEnt;
			int iEntType;
		};
		struct{
			long hObj;
			int  iObjType;
		};
		struct{
			fAtom* pAtom;
			int iAtomType;
		};
	};
	SELOBJ(DWORD dwhObj=0,DWORD dwObjInfoFlag=0,IDrawing* pDrawing=NULL);
	void InitObj(DWORD dwhObj=0,DWORD dwObjInfoFlag=0,IDrawing* pDrawing=NULL);
	long GetRelaObj(){return m_hRelaObj;}
	__declspec( property(get=GetRelaObj)) long hRelaObj;
private:
	long m_hRelaObj;
	IDrawing* m_pDrawing;
	long GetRelaObjH();
};
