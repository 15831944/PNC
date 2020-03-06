#pragma once
#include "f_ent.h"
#include "XhCharString.h"

struct BASIC_INFO{
	char m_cMat;
	char m_cQuality;			//�����ȼ�
	int m_nThick;
	int m_nNum;
	long m_idCadEntNum;
	CXhChar16 m_sPartNo;
	CXhChar100 m_sPrjCode;		//���̱��
	CXhChar100 m_sPrjName;		//��������
	CXhChar50 m_sTaType;		//����
	CXhChar50 m_sTaAlias;		//����
	CXhChar50 m_sTaStampNo;		//��ӡ��
	CXhChar200 m_sBoltStr;		//��˨�ַ���
	CXhChar100 m_sTaskNo;		//�����
	BASIC_INFO() { m_nThick = m_nNum = 0; m_cMat = 0; m_cQuality = 0; m_idCadEntNum = 0; }
};
struct BOLT_HOLE{
	double d;			//��˨����ֱ����M20��˨ʱd=20
	BYTE ciSymbolType;	//0.ͼ��;1.Բ��
	float increment;	//��˨�ױ���˨����ֱ���������϶ֵ
	float posX,posY;	//
	BOLT_HOLE() {
		posX = posY = increment = ciSymbolType = 0; d = 0;
	}
};

struct ISymbolRecognizer {
	virtual bool IsHuoquLine(GELINE* pLine, DWORD cbFilterFlag = 0) = 0;
};