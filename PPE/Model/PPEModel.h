#pragma once
#include "HashTable.h"
#include "XhCharString.h"
#include "ProcessPart.h"

struct ISysPara
{
	const static BYTE TYPE_FLAME_CUT	= 0;	//�����и�
	const static BYTE TYPE_PLASMA_CUT	= 1;	//�������и�
	virtual double GetCutInLineLen(double fThick,BYTE cType=-1)=0;
	virtual double GetCutOutLineLen(double fThick,BYTE cType=-1)=0;
	virtual int GetCutEnlargedSpaceLen(BYTE cType=-1)=0;
	virtual int GetCutInitPosFarOrg(BYTE cType=-1)=0;
	virtual int GetCutPosInInitPos(BYTE cType=-1)=0;
	virtual BOOL IsCutSpecialHole(BYTE cType=-1)=0;
};

class CPPEModel
{
private:
	CXhChar500 m_sFolderPath;	//�ļ���·��
	CSuperHashStrList<CProcessPart> m_hashPartByPartNo;
	CHashStrList<CXhChar500> m_hashFilePathByPartNo;
public:
	CXhChar50 m_sVersion;		//�汾��
	CXhChar100 m_sCompanyName;	//��Ƶ�λ
	CXhChar100 m_sPrjCode;		//���̱��
	CXhChar100 m_sPrjName;		//��������
	CXhChar50 m_sTaType;		//����
	CXhChar50 m_sTaAlias;		//����
	CXhChar50 m_sTaStampNo;		//��ӡ��
	CXhChar16 m_sOperator;		//����Ա���Ʊ��ˣ�
	CXhChar16 m_sAuditor;		//�����
	CXhChar16 m_sCritic;		//������
public:
	static ILog2File* log2file;
	static ILog2File* Log2File();//��Զ�᷵��һ����Чָ��,��ֻ��log2file!=NULLʱ,�Ż�������¼������־
	static ISysPara* sysPara;
public:
	CPPEModel(void);
	~CPPEModel(void);
	void (*DisplayProcess)(int percent,char *sTitle);	//������ʾ�ص�����
	BOOL FromBuffer(CBuffer &buffer);
	void ToBuffer(CBuffer &buffer,const char *file_version);
	BOOL InitModelByFolderPath(const char *folder_path);
	void InitPlateMcsAndCutPt(bool bSaveToFile=false);	//��ʼ���ְ�ӹ�����ϵ�������
	void Empty();
	CXhChar500 GetFolderPath(){return m_sFolderPath;}
	//
	CXhChar500 GetPartFilePath(const char *sPartNo);
	bool SavePartToFile(CProcessPart *pPart);
	CProcessPart* AddPart(const char *sPartNo,BYTE cType,const char *sFilePath=NULL);
	BOOL DeletePart(const char *sPartNo);
	CProcessPart* FromPartNo(const char *sPartNo){return m_hashPartByPartNo.GetValue(sPartNo);}
	CProcessPart* EnumPartFirst(){return m_hashPartByPartNo.GetFirst();}
	CProcessPart* EnumPartNext(){return m_hashPartByPartNo.GetNext();}
	DWORD PartCount(){return m_hashPartByPartNo.GetNodeNum();}
	void ModifyPartNo(const char* sOldPartNo,const char* sNewPartNo);
	BOOL IsAllDeformedProfile();
	//�õ����򼯺�(�Ǹּ��Ϻ͸ְ弯��)
	void GetSortedAngleSetAndPlateSet(CXhPtrSet<CProcessAngle> &angleSet,CXhPtrSet<CProcessPlate> &plateSet);
	void ReadPrjTowerInfoFromCfgFile(const char* cfg_file_path);
};
extern CPPEModel model;
