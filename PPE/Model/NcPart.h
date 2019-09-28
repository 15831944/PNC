#pragma once
#include "PPEModel.h"
#include "NcPlate.h"

struct DRILL_BOLT_INFO
{
	int nBoltNum;
	double fHoleD;
	double fMinDist;
	DRILL_BOLT_INFO() { fHoleD = 0; nBoltNum = 0; fMinDist = 0; }
};
class CDrillBolt
{
public:
	static const int DRILL_NULL		= -1;	//����Ҫ���
	static const int DRILL_T2		= 0;	//T2��ͷ
	static const int DRILL_T3		= 1;	//T3��ͷ
	BYTE biMode;	//-1.����Ҫ 0.T2 1.T3
	int nBoltD;
	double fHoleD;
	ATOM_LIST<BOLT_INFO> boltList;
	CDrillBolt() { biMode = 0; fHoleD = 0.0; nBoltD = 0; }
	void OptimizeBoltOrder(f3dPoint& startPos,BYTE ciAlgType =0);
};
struct PLATE_GROUP
{
	DWORD thick;
	char cMaterial;
	CXhChar50 sKey;
	CXhPtrSet<CProcessPlate> plateSet;
};

class CNCPart
{
public:
	//�ְ�NCģʽ
	static const BYTE CUT_MODE		= 0x01;	 //�и�����
	static const BYTE PROCESS_MODE	= 0x02;	 //�崲�ӹ�
	static const BYTE LASER_MODE	= 0x04;	 //���⸴�ϻ�
	//�ְ�NC����ļ�
	static const int PLATE_DXF_FILE = 0x01;  //�ְ�DXF�����ļ�
	static const int PLATE_NC_FILE	= 0x02;	 //�ְ�NC�����ļ�
	static const int PLATE_TXT_FILE = 0x04;	 //�ְ�TXT�����ļ�
	static const int PLATE_CNC_FILE = 0x08;	 //�ְ�CNC�����ļ�
	static const int PLATE_PMZ_FILE = 0x10;	 //�ְ�PMZ�����ļ�
	static const int PLATE_PBJ_FILE = 0x20;  //�ְ�PBJ�����ļ�
	static const int PLATE_TTP_FILE = 0x40;	 //�ְ�TTP�����ļ�
	static const int PLATE_WKF_FILE = 0x80;	 //���Ϸ���WKF�ļ�
	//��˨�Ż������㷨
	static const BYTE ALG_GREEDY	= 1;	//̰���㷨
	static const BYTE ALG_BACKTRACK	= 2;	//�����㷨
	static const BYTE ALG_ANNEAL	= 3;	//�˻��㷨
public:
	static BOOL m_bDisplayLsOrder;		//��ʾ��˨˳�����ڲ���ʹ�ã�
	static BOOL m_bSortHole;			//����˨�������򣨵���PBJ�ļ����п��ƣ�
	static BOOL m_bDeformedProfile;		//���ǻ�������
	static CString m_sExportPartInfoKeyStr;	//�ְ������ϸ
	static double m_fHoleIncrement;		//�׾�����ֵ wht 19-07-25
public:
	//static BOOL GetSysParaFromReg(const char* sEntry, char* sValue);
	static void InitStoreMode(CHashList<CDrillBolt>& hashDrillBoltByD,ARRAY_LIST<double> &holeDList,BOOL bIncSH=TRUE);
	static void RefreshPlateHoles(CProcessPlate *pPlate,BOOL bSortByHoleD=TRUE,BYTE ciAlgType = 0);
	static BOOL IsNeedCreateHoleFile(CProcessPlate *pPlate,BYTE ciHoleProcessType);
	static void DeformedPlateProfile(CProcessPlate *pPlate);
	//�ְ����
	static bool CreatePlateTtpFile(CProcessPlate *pPlate,const char* file_path);
	static void CreatePlateTtpFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static bool CreatePlateDxfFile(CProcessPlate *pPlate,const char* file_path,int dxf_mode);
	static void CreatePlateDxfFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	//��Ӽ��Ϸ��ظְ����(*.wkf)���� wht 19-06-11
	static bool CreatePlateWkfFile(CProcessPlate *pPlate, const char* file_path);
	static void CreatePlateWkfFiles(CPPEModel *pModel, CXhPtrSet<CProcessPlate> &plateSet, const char* work_dir);
#ifdef __PNC_
	static void InitDrillBoltHashTbl(CProcessPlate *pPlate, CHashList<CDrillBolt>& hashDrillBoltByD,
									 BOOL bMergeHole = FALSE, BOOL bIncSpecialHole=TRUE, BOOL bDrillGroupSort=FALSE);
	static void OptimizeBolt(CProcessPlate *pPlate,CHashList<CDrillBolt>& hashDrillBoltByD,
							 BOOL bSortByHoleD=TRUE,BYTE ciAlgType=0,BOOL bMergeHole=FALSE,BOOL bIncSpecialHole=TRUE);
	static void CreatePlatePncDxfFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static bool CreatePlatePbjFile(CProcessPlate *pPlate,const char* file_path);
	static void CreatePlatePbjFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static bool CreatePlatePmzFile(CProcessPlate *pPlate,const char* file_path);
	static bool CreatePlatePmzCheckFile(CProcessPlate *pPlate, const char* file_path);
	static void CreatePlatePmzFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static bool CreatePlateTxtFile(CProcessPlate *pPlate,const char* file_path);
	static void CreatePlateTxtFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static bool CreatePlateNcFile(CProcessPlate *pPlate,const char* file_path);
	static void CreatePlateNcFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static bool CreatePlateCncFile(CProcessPlate *pPlate,const char* file_path);
	static void CreatePlateCncFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir);
	static void CreatePlateFiles(CPPEModel *pModel,CXhPtrSet<CProcessPlate> &plateSet,const char* work_dir,int nFileType);
	static void ImprotPlateCncOrTextFile(CProcessPlate *pPlate,const char* file_path);
#endif
	static void CreateAllPlateFiles(int nFileType);
	//�Ǹֲ���
	static void CreateAngleNcFiles(CPPEModel *pModel,CXhPtrSet<CProcessAngle> &angleSet,const char* drv_path,const char* sPartNoPrefix,const char* work_dir);
	static void CreateAllAngleNcFile(CPPEModel *pModel,const char* drv_path,const char* sPartNoPrefix,const char* work_dir);
	//����PPI�ļ�
	static bool CreatePPIFile(CProcessPart *pPart,const char* file_path);
	static void CreatePPIFiles(CPPEModel *pModel,CXhPtrSet<CProcessPart> &partSet,const char* work_dir);
	static void CreateAllPPIFiles(CPPEModel *pModel,const char* work_dir);
};
extern BOOL GetSysParaFromReg(const char* sEntry, char* sValue);

