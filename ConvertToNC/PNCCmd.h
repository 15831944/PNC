#pragma once
#ifndef __PNC_MENU_FUNC_H_
#define __PNC_MENU_FUNC_H_
#include "PNCModel.h"
//�ӹ�
void SmartExtractPlate();	//������ȡ�����Ϣ
void SmartExtractPlate(CPNCModel *pModel);
void ManualExtractPlate();	//�ֶ���ȡ�ְ���Ϣ
void ManualExtractPlate(CPNCModel *pModel);
void EnvGeneralSet();		//ϵͳ����
#ifndef __UBOM_ONLY_
void SendPartEditor();		//�༭�ְ���Ϣ
void LayoutPlates();		//�Զ��Ű�
void InsertMKRect();		//�����ӡ��
void DrawProfileByTxtFile();//ͨ����ȡTxt�ļ���������
void ExplodeText();			//�����ı�(�ı�����ת��Ϊ�����)
#endif
#if defined(__UBOM_) || defined(__UBOM_ONLY_)
void RevisionPartProcess();	//У�󹹼�������Ϣ
#endif
#endif