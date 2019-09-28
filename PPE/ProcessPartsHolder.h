#pragma once

#include "Buffer.h"
#include "ProcessPart.h"
#include "I_DrawSolid.h"
#include "IPEC.h"

class CProcessPartsHolder
{
private:
	int m_iCurFileIndex;						//��ǰ���ڱ༭���ļ�����
	BYTE m_cPartType;							//��ǰ���ڱ༭�Ĺ�������
	CString m_sFolderPath;						//���ļ���
	CProcessPlate m_xPlate;
	CProcessAngle m_xAngle;
	CStringArray m_arrPlateFileName;			//�ְ幹������
	CStringArray m_arrAngleFileName;			//�Ǹֹ�������
	CString GetFilePathByIndex(int curIndex);
	CString GetCurFileNameByIndex(int index);
	BOOL UpdateCurPartFromFile(int iCurFileIndex);
public:
	CProcessPartsHolder();
	~CProcessPartsHolder();
	//���ò���
	void Init(CString sFolderPath);
	void JumpToFirstPart();
	void JumpToPrePart();
	void JumpToNextPart();
	void JumpToLastPart();
	void JumpToSpecPart(char *sPartFileName);
	CString GetCurFileName();
	BYTE GetCurPartType() { return m_cPartType; }
	void SaveCurPartInfoToFile();
	CXhChar200 AddPart(CProcessPart *pPart);
	//�ְ����
	void CreateAllPlateDxfFile();
	void CreateAllPlateTtpFile();
	//�Ǹֲ���
	void CreateAllAngleNcFile(char* drv_path,char* sPartNoPrefix);
	//
	CStringArray& GetPlateFileNameArr() { return m_arrPlateFileName; }
	CStringArray& GetAngleFileNameArr() { return m_arrAngleFileName; }
	//
	CProcessPart* GetCurProcessPart();
	bool UpdatePartInfoToEditor();
	void EscapeCurEditPart();
};

extern CProcessPartsHolder g_partInfoHodler;
extern ISolidDraw *g_pSolidDraw;
extern ISolidSet *g_pSolidSet;
extern ISolidSnap *g_pSolidSnap;
extern ISolidOper *g_pSolidOper;
extern I2dDrawing *g_p2dDraw;
extern IPEC* g_pPartEditor;