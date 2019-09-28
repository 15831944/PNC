// PEC.h : PEC DLL ����ͷ�ļ�
//

#pragma once

#include "HashTable.h"
#include "ProcessPartDraw.h"
#include "IPEC.h"

class CPEC : public IPEC
{
private:	//���ݴ洢���ļ�����
	int m_iCurFileIndex;						//��ǰ���ڱ༭���ļ�����
	CXhChar200 m_sFolderPath;					//���ļ���
	CStringArray m_arrPlateFileName;			//�ְ幹������
	CStringArray m_arrAngleFileName;			//�Ǹֹ�������
	CString GetFilePathByIndex(int index);
	CString GetFileNameByIndex(int index);
	BOOL LoadPartFromFile(int iCurFileIndex);
private:	//UI����
	int m_iNo;			//�����༭�����
	GECS m_workCS;		//��������ϵ���༭����ʱʹ��
	WORD m_wDrawMode;	//��ͼģʽ 1.�༭ģʽ��2.NCģʽ��3.���ղ�ͼģʽ
	BYTE m_cPartType;	//��ǰ�༭�Ĺ�������
	HWND m_hWnd;
	I2dDrawing *m_p2dDraw;
	ISolidDraw *m_pSolidDraw;
	ISolidSet *m_pSolidSet;
	ISolidSnap *m_pSolidSnap;
	ISolidOper *m_pSolidOper;
	CSuperHashList<CProcessPart> m_hashProcessPartBySerial;
	CSuperHashList<CProcessPartDraw> m_hashPartDrawBySerial;
	bool (*FirePartModify)(WORD cmdType);
	//
private:
	WORD m_wCurCmdType;	//��ǰ��������
	//��������(�㡢�ߡ�Բ����˨��)
	void FinishViewVertex(IDbEntity* pEnt);
	void FinishViewLine(IDbEntity* pEnt);
	void FinishViewCir(IDbEntity* pEnt);
	void FinishViewLsCir(IDbEntity* pEnt);
	////���롢ɾ��������
	//BOOL FinishInsertPlankVertex(fAtom *pAtom);
	//BOOL FinishDelPartFeature(fAtom *pAtom);
	//��������ͨ�������������͵���
	void RotateAntiClockwisePlate();
	void RotateClockwisePlate();
	void OverturnPart();
	void NewBoltHole();
	void CalPlateWrapProfile(CProcessPlate *pPlate,double angle_lim=60.0);
	void CalPlateWrapProfile();
	void SetSpecWCS();
	void ViewPartFeature();
	void DimSize();
	void DefPlateCutInPt();
	void DrawCuttingTrack();
public:
	CPEC(int iNo);
	~CPEC();
	WORD GetCurCmdType(){return m_wCurCmdType;}
	virtual char GetCurPartType();
	//�ְ����
	virtual void SetMarkPos(double posX,double posY);
	virtual void GetPlateNcUcs(GECS& cs);
	virtual void InitMkRect();
	//UI ��ͼ����
	virtual bool InitDrawEnviornment(HWND hWnd,I2dDrawing* p2dDraw,ISolidDraw* pSolidDraw,
									 ISolidSet* pSolidSet,ISolidSnap* pSolidSnap,ISolidOper* pSolidOper);
	virtual int SetDrawMode(WORD mode);	//1.�༭ģʽ��2.NCģʽ��3.���ղ�ͼģʽ
	virtual int GetDrawMode() {return m_wDrawMode;}
	virtual bool AddProcessPart(CBuffer &processPartBuffer,DWORD key);		//��༭�������ӵ�ǰ������ʾ�Ĺ��չ���
	virtual bool GetProcessPart(CBuffer &partBuffer,int serial=0);
	virtual bool UpdateProcessPart(CBuffer &partBuffer,int serial=0);
	virtual void ClearProcessParts();	//��ձ༭���е�ǰ���չ�����
	virtual void Draw();
	virtual void ReDraw(int serial=0);
	virtual bool SetWorkCS(GECS cs);
	virtual void SetCallBackPartModifiedFunc(bool (*func)(WORD cmdType)){FirePartModify=func;}
	virtual bool SetCallBackProcessPartModified();
	virtual bool SetCallBackOperationFunc();		//Mouse or keyboard input->CWnd->DrawSolid->CWnd->PartsEditor
	virtual int GetSerial(){return m_iNo;}
	virtual void ExecuteCommand(WORD taskType);
	//��������
	virtual void SetDrawBoltMode(BOOL bDisplayOrder){CProcessPartDraw::m_bDispBoltOrder=bDisplayOrder;}
	virtual void SetProcessCardPath(const char* sCardDxfFilePath,BYTE angle0_plate1=0);
	virtual void UpdateJgDrawingPara(const char* para_buf,int buf_len);
	//
	static BOOL GetSysParaFromReg(const char* sEntry,char* sValue);
};
