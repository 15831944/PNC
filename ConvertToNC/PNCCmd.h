#pragma once
#ifndef __PNC_MENU_FUNC_H_
#define __PNC_MENU_FUNC_H_
#include "PNCModel.h"
//�ӹ�
void SmartExtractPlate();	//������ȡ�����Ϣ
void SmartExtractPlate(CPNCModel *pModel);
void ManualExtractPlate();	//�ֶ���ȡ�ְ���Ϣ
#ifdef __PNC_
void SendPartEditor();		//�༭�ְ���Ϣ
void LayoutPlates();		//�Զ��Ű�
void EnvGeneralSet();		//ϵͳ����
void InsertMKRect();		//�����ӡ��
#endif
void RevisionPartProcess();	//У�󹹼�������Ϣ
void DrawProfileByTxtFile();//ͨ����ȡTxt�ļ���������
#endif