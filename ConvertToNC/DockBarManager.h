#pragma once
#include "AcUiDialogPanel.h"
#include "PartListDlg.h"


#ifdef __SUPPORT_DOCK_UI_
extern CAcUiDialogPanel *g_pPartListDockBar;	//��ͣ������
#endif
#ifndef __UBOM_ONLY_
CPartListDlg* GetPartListDlgPtr();
#endif
void DestoryPartListDockBar();
BOOL IsShowDisplayPartListDockBar();
void DisplayPartListDockBar();
void HidePartListDockBar();
