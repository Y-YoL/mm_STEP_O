// STEP_reConv.cpp : DLL �p�̏����������̒�`���s���܂��B
//

#include "stdafx.h"
#include "STEP_reConv.h"
#include "STEPlugin.h"

#include "imm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//
//	����!
//
//		���� DLL �� MFC DLL �ɑ΂��ē��I�Ƀ����N�����ꍇ�A
//		MFC ���ŌĂяo����邱�� DLL ����G�N�X�|�[�g���ꂽ
//		�ǂ̊֐����֐��̍ŏ��ɒǉ������ AFX_MANAGE_STATE 
//		�}�N�����܂�ł��Ȃ���΂Ȃ�܂���B
//
//		��:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// �ʏ�֐��̖{�̂͂��̈ʒu�ɂ���܂�
//		}
//
//		���̃}�N�����e�֐��Ɋ܂܂�Ă��邱�ƁAMFC ����
//		�ǂ̌Ăяo�����D�悷�邱�Ƃ͔��ɏd�v�ł��B
//		����͊֐����̍ŏ��̃X�e�[�g�����g�łȂ���΂�
//		��Ȃ����Ƃ��Ӗ����܂��A�R���X�g���N�^�� MFC 
//		DLL ���ւ̌Ăяo�����s���\��������̂ŁA�I�u
//		�W�F�N�g�ϐ��̐錾�����O�łȂ���΂Ȃ�܂���B
//
//		�ڍׂɂ��Ă� MFC �e�N�j�J�� �m�[�g 33 �����
//		58 ���Q�Ƃ��Ă��������B
//

/////////////////////////////////////////////////////////////////////////////
// CSTEP_reConvApp

BEGIN_MESSAGE_MAP(CSTEP_reConvApp, CWinApp)
	//{{AFX_MSG_MAP(CSTEP_reConvApp)
		// ���� - ClassWizard �͂��̈ʒu�Ƀ}�b�s���O�p�̃}�N����ǉ��܂��͍폜���܂��B
		//        ���̈ʒu�ɐ��������R�[�h��ҏW���Ȃ��ł��������B
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSTEP_reConvApp �̍\�z

CSTEP_reConvApp::CSTEP_reConvApp()
{
	// TODO: ���̈ʒu�ɍ\�z�p�̃R�[�h��ǉ����Ă��������B
	// ������ InitInstance �̒��̏d�v�ȏ��������������ׂċL�q���Ă��������B
}

/////////////////////////////////////////////////////////////////////////////
// �B��� CSTEP_reConvApp �I�u�W�F�N�g

CSTEP_reConvApp theApp;

UINT nPluginID;

CString strINI;

// �R�}���hID
UINT nIDReConvHiragana;
UINT nIDReConvRomaji;

void AddConvMenu(HMENU hMenu) {
	InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_SEPARATOR, 0, NULL);
	InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_STRING, nIDReConvHiragana, TEXT("�Ђ炪�Ȃɕϊ�"));
	InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_STRING, nIDReConvRomaji, TEXT("���[�}���ɕϊ�"));
}

STEP_API bool WINAPI STEPInit(UINT pID, LPCTSTR szPluginFolder)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (Initialize() == false)	return false;
	nPluginID = pID;

	// INI�t�@�C���̓ǂݍ���
	strINI = szPluginFolder;
	strINI += "STEP_reConv.ini";

	nIDReConvHiragana = STEPGetCommandID();
	STEPKeyAssign(nIDReConvHiragana, TEXT("�Ђ炪�Ȃɕϊ�"), TEXT("STEP_reConv_KEY_RE_CONV_HIRAGANA"));
	nIDReConvRomaji = STEPGetCommandID();
	STEPKeyAssign(nIDReConvRomaji, TEXT("���[�}���ɕϊ�"), TEXT("STEP_reConv_KEY_RE_CONV_ROMAJI"));

	return true;
}

STEP_API void WINAPI STEPFinalize() {
	Finalize();
}

STEP_API UINT WINAPI STEPGetAPIVersion(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return STEP_API_VERSION;
}

STEP_API LPCTSTR WINAPI STEPGetPluginName(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return TEXT("STEP_reConv");
}

STEP_API LPCTSTR WINAPI STEPGetPluginInfo(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return TEXT("Version 0.03 Copyright (C) 2003-2006 haseta\r\nMS-IME2000/2002�ɂ��Ђ炪��/���[�}���ɕϊ����܂�");
}

STEP_API LPCTSTR WINAPI STEPGetStatusMessage(UINT nID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (nID == nIDReConvHiragana) {
		return TEXT("�I������Ă���͈͂��Ђ炪�Ȃɕϊ����܂�");
	}
	if (nID == nIDReConvRomaji) {
		return TEXT("�I������Ă���͈͂����[�}���ɕϊ����܂�");
	}
	return NULL;
}

STEP_API bool WINAPI STEPOnUpdateCommand(UINT nID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (nID == nIDReConvHiragana) {
		if (STEPIsRangeSelected() || STEPIsCurrentCellEditOK()) return true;
		else return false;
	}
	if (nID == nIDReConvRomaji) {
		if (STEPIsRangeSelected() || STEPIsCurrentCellEditOK()) return true;
		else return false;
	}
	return false;
}

// strcnv.cpp
enum	{CONV_SUJI=1, CONV_ALPHA=2, CONV_KATA=4, CONV_KIGOU=8, CONV_ALL=15};
#ifdef __cplusplus
extern "C" {
#endif

extern	int conv_han2zens(unsigned char *, const unsigned char *, int);
extern	int conv_zen2hans(LPTSTR , LPCTSTR, int);
extern	void conv_kata2hira(unsigned char *);
extern	void conv_kata_erase_dakuon(unsigned char *);
extern	void conv_hira2kata(unsigned char *);
extern	void conv_upper(unsigned char *);
extern	void conv_lower(unsigned char *);
extern	void conv_first_upper(unsigned char *);
//extern	DWORD conv_kan2hira(HWND, unsigned char *, DWORD);
//extern	void conv_romaji(HWND hwnd, unsigned char *str, unsigned char *sRomaji);
#ifdef __cplusplus
}
#endif
CString conv_kan2hira(HWND hWnd, LPCTSTR str)
{
	CString strText = str;
	// MS-IME2000�ł͂��܂����������AATOK16�ł͂��߁B�P��P�ʂł���΂ł���...
	HIMC himc = ::ImmGetContext(hWnd);
	DWORD dwRet = ::ImmGetConversionList(
					 ::GetKeyboardLayout(0), himc,
					 str, NULL, 0,
					 GCL_REVERSECONVERSION);

	// �ǂ݉����i�[�̈���m�� 
	CANDIDATELIST *lpCand;
	lpCand = (CANDIDATELIST *)malloc(dwRet);
	// �ǂ݉������擾
	dwRet = ::ImmGetConversionList(
					 ::GetKeyboardLayout(0), himc,
					 str, lpCand, dwRet,
					 GCL_REVERSECONVERSION);
	if (dwRet > 0 && lpCand->dwCount > 0) {
		char *work = (char*)lpCand + lpCand->dwOffset[0];
		strText = work;

		/*
		for (unsigned int i = 0; i< lpCand->dwCount; i++)
		{
			TRACE("%s", (LPBYTE)lpCand + lpCand->dwOffset[i]);
		}
		*/
	}

	free(lpCand);
	::ImmReleaseContext(hWnd, himc);
	return strText;
}

CString conv_romaji(HWND hwnd, LPCTSTR str)
{
	static const LPCTSTR romaji[] = {
		TEXT("������"), TEXT("KKYA"), TEXT("������"), TEXT("KKYU"), TEXT("������"), TEXT("KYO"),
		TEXT("����"), TEXT("KYA"), TEXT("����"), TEXT("KYU"), TEXT("����"), TEXT("KYO"),
		TEXT("������"), TEXT("SSHA"), TEXT("������"), TEXT("SSHU"), TEXT("������"), TEXT("SSHO"),
		TEXT("����"), TEXT("SHA"), TEXT("����"), TEXT("SHU"), TEXT("����"), TEXT("SHO"),
		TEXT("������"), TEXT("CCHA"), TEXT("������"), TEXT("CCHU"), TEXT("������"), TEXT("CCHO"),
		TEXT("����"), TEXT("CHA"), TEXT("����"), TEXT("CHU"), TEXT("����"), TEXT("CHO"),
		TEXT("���ɂ�"), TEXT("NNYA"), TEXT("���ɂ�"), TEXT("NNYU"), TEXT("���ɂ�"), TEXT("NNYO"),
		TEXT("�ɂ�"), TEXT("NYA"), TEXT("�ɂ�"), TEXT("NYU"), TEXT("�ɂ�"), TEXT("NYO"),
		TEXT("���Ђ�"), TEXT("HHYA"), TEXT("���Ђ�"), TEXT("HHYU"), TEXT("���Ђ�"), TEXT("HHYO"),
		TEXT("�Ђ�"), TEXT("HYA"), TEXT("�Ђ�"), TEXT("HYU"), TEXT("�Ђ�"), TEXT("HYO"),
		TEXT("���݂�"), TEXT("MMYA"), TEXT("���݂�"), TEXT("MMYU"), TEXT("���݂�"), TEXT("MMYO"),
		TEXT("�݂�"), TEXT("MYA"), TEXT("�݂�"), TEXT("MYU"), TEXT("�݂�"), TEXT("MYO"),
		TEXT("�����"), TEXT("RRYA"), TEXT("�����"), TEXT("RRYU"), TEXT("�����"), TEXT("RRYO"),
		TEXT("���"), TEXT("RYA"), TEXT("���"), TEXT("RYU"), TEXT("���"), TEXT("RYO"),
		TEXT("������"), TEXT("GGYA"), TEXT("������"), TEXT("GGYU"), TEXT("������"), TEXT("GGYO"),
		TEXT("����"), TEXT("GYA"), TEXT("����"), TEXT("GYU"), TEXT("����"), TEXT("GYO"),
		TEXT("������"), TEXT("JJA"), TEXT("������"), TEXT("JJU"), TEXT("������"), TEXT("JJO"),
		TEXT("����"), TEXT("JA"), TEXT("����"), TEXT("JU"), TEXT("����"), TEXT("JO"),
		TEXT("���т�"), TEXT("BBYA"), TEXT("���т�"), TEXT("BBYU"), TEXT("���т�"), TEXT("BBYO"),
		TEXT("�т�"), TEXT("BYA"), TEXT("�т�"), TEXT("BYU"), TEXT("�т�"), TEXT("BYO"),
		TEXT("���҂�"), TEXT("PPYA"), TEXT("���҂�"), TEXT("PPYU"), TEXT("���҂�"), TEXT("PPYO"),
		TEXT("�҂�"), TEXT("PYA"), TEXT("�҂�"), TEXT("PYU"), TEXT("�҂�"), TEXT("PYO"),

		TEXT("����"), TEXT("KKA"), TEXT("����"), TEXT("KKI"), TEXT("����"), TEXT("KKU"), TEXT("����"), TEXT("KKE"), TEXT("����"), TEXT("KKO"),
		TEXT("��"), TEXT("KA"), TEXT("��"), TEXT("KI"), TEXT("��"), TEXT("KU"), TEXT("��"), TEXT("KE"), TEXT("��"), TEXT("KO"),
		TEXT("����"), TEXT("SSA"), TEXT("����"), TEXT("SSHI"), TEXT("����"), TEXT("SSU"), TEXT("����"), TEXT("SSE"), TEXT("����"), TEXT("SSO"),
		TEXT("��"), TEXT("SA"), TEXT("��"), TEXT("SHI"), TEXT("��"), TEXT("SU"), TEXT("��"), TEXT("SE"), TEXT("��"), TEXT("SO"),
		TEXT("����"), TEXT("TTA"), TEXT("����"), TEXT("CCHI"), TEXT("����"), TEXT("TTSU"), TEXT("����"), TEXT("TTE"), TEXT("����"), TEXT("TTO"),
		TEXT("��"), TEXT("TA"), TEXT("��"), TEXT("CHI"), TEXT("��"), TEXT("TSU"), TEXT("��"), TEXT("TE"), TEXT("��"), TEXT("TO"),
		TEXT("����"), TEXT("NNA"), TEXT("����"), TEXT("NNI"), TEXT("����"), TEXT("NNU"), TEXT("����"), TEXT("NNE"), TEXT("����"), TEXT("NNO"),
		TEXT("��"), TEXT("NA"), TEXT("��"), TEXT("NI"), TEXT("��"), TEXT("NU"), TEXT("��"), TEXT("NE"), TEXT("��"), TEXT("NO"),
		TEXT("����"), TEXT("HHA"), TEXT("����"), TEXT("HHI"), TEXT("����"), TEXT("FFU"), TEXT("����"), TEXT("HHE"), TEXT("����"), TEXT("HHO"),
		TEXT("��"), TEXT("HA"), TEXT("��"), TEXT("HI"), TEXT("��"), TEXT("FU"), TEXT("��"), TEXT("HE"), TEXT("��"), TEXT("HO"),
		TEXT("����"), TEXT("MMA"), TEXT("����"), TEXT("MMI"), TEXT("����"), TEXT("MMU"), TEXT("����"), TEXT("MME"), TEXT("����"), TEXT("MMO"),
		TEXT("��"), TEXT("MA"), TEXT("��"), TEXT("MI"), TEXT("��"), TEXT("MU"), TEXT("��"), TEXT("ME"), TEXT("��"), TEXT("MO"),
		TEXT("����"), TEXT("YYA"), TEXT("����"), TEXT("YUYU"), TEXT("����"), TEXT("YYO"),
		TEXT("��"), TEXT("YA"), TEXT("��"), TEXT("YU"), TEXT("��"), TEXT("YO"),
		TEXT("����"), TEXT("RRA"), TEXT("����"), TEXT("RRI"), TEXT("����"), TEXT("RRU"), TEXT("����"), TEXT("RRE"), TEXT("����"), TEXT("RRO"),
		TEXT("��"), TEXT("RA"), TEXT("��"), TEXT("RI"), TEXT("��"), TEXT("RU"), TEXT("��"), TEXT("RE"), TEXT("��"), TEXT("RO"),
		TEXT("����"), TEXT("WWA"),
		TEXT("��"), TEXT("WA"),
		TEXT("����"), TEXT("GGA"), TEXT("����"), TEXT("GGI"), TEXT("����"), TEXT("GGU"), TEXT("����"), TEXT("GGE"), TEXT("����"), TEXT("GGO"),
		TEXT("��"), TEXT("GA"), TEXT("��"), TEXT("GI"), TEXT("��"), TEXT("GU"), TEXT("��"), TEXT("GE"), TEXT("��"), TEXT("GO"),
		TEXT("����"), TEXT("ZZA"), TEXT("����"), TEXT("JJI"), TEXT("����"), TEXT("ZZU"), TEXT("����"), TEXT("ZZE"), TEXT("����"), TEXT("ZZO"),
		TEXT("��"), TEXT("ZA"), TEXT("��"), TEXT("JI"), TEXT("��"), TEXT("ZU"), TEXT("��"), TEXT("ZE"), TEXT("��"), TEXT("ZO"),
		TEXT("����"), TEXT("DDA"), TEXT("����"), TEXT("JJI"), TEXT("����"), TEXT("ZZU"), TEXT("����"), TEXT("DDE"), TEXT("����"), TEXT("DDO"),
		TEXT("��"), TEXT("DA"), TEXT("��"), TEXT("JI"), TEXT("��"), TEXT("ZU"), TEXT("��"), TEXT("DE"), TEXT("��"), TEXT("DO"),
		TEXT("����"), TEXT("BBA"), TEXT("����"), TEXT("BBI"), TEXT("����"), TEXT("BBU"), TEXT("����"), TEXT("BBE"), TEXT("����"), TEXT("BBO"),
		TEXT("��"), TEXT("BA"), TEXT("��"), TEXT("BI"), TEXT("��"), TEXT("BU"), TEXT("��"), TEXT("BE"), TEXT("��"), TEXT("BO"),
		TEXT("����"), TEXT("PPA"), TEXT("����"), TEXT("PPI"), TEXT("����"), TEXT("PPU"), TEXT("����"), TEXT("PPE"), TEXT("����"), TEXT("PPO"),
		TEXT("��"), TEXT("PA"), TEXT("��"), TEXT("PI"), TEXT("��"), TEXT("PU"), TEXT("��"), TEXT("PE"), TEXT("��"), TEXT("PO"),

		TEXT("����"), TEXT("AA"), TEXT("����"), TEXT("II"), TEXT("����"), TEXT("UU"), TEXT("����"), TEXT("EE"), TEXT("����"), TEXT("OO"),
		TEXT("��"), TEXT("A"), TEXT("��"), TEXT("I"), TEXT("��"), TEXT("U"), TEXT("��"), TEXT("E"), TEXT("��"), TEXT("O"),
		TEXT("����"), TEXT("OO"), TEXT("����"), TEXT("NN"),
		TEXT("��"), TEXT("O"), TEXT("��"), TEXT("N"),
		TEXT("�["), TEXT(""),
		NULL, NULL,
	};

	CString	strWork, strRep;
	int		nPos;
	int nRomaji = 0;

	strWork = str;
	/*
	while (romaji[nRomaji*2] != NULL) {
		while((nPos = strWork.Find(romaji[nRomaji*2])) != -1) {
			int		nLenOrg = strWork.GetLength();
			int		nLenKey = strlen(romaji[nRomaji*2]);
			strRep = romaji[nRomaji*2+1];
			conv_lower((unsigned char *)strRep.GetBuffer(0));
			strRep.ReleaseBuffer();
			strWork.Format("%s%s%s"), strWork.Left(nPos), strRep, strWork.Right(nLenOrg-(nPos+nLenKey)));
		}
		nRomaji++;
	}
	*/

	strWork = conv_kan2hira(hwnd, strWork);

	nRomaji = 0;
	while (romaji[nRomaji*2] != NULL) {
		while((nPos = strWork.Find(romaji[nRomaji*2])) != -1) {
			int		nLenOrg = strWork.GetLength();
			int		nLenKey = lstrlen(romaji[nRomaji*2]);
			strRep = romaji[nRomaji*2+1];
			conv_lower((unsigned char *)strRep.GetBuffer(0));
			strRep.ReleaseBuffer();
			conv_first_upper((unsigned char *)strRep.GetBuffer(0));
			strRep.ReleaseBuffer();
			strWork.Format(TEXT("%s%s%s"), strWork.Left(nPos), strRep, strWork.Right(nLenOrg-(nPos+nLenKey)));
		}
		nRomaji++;
	}

	conv_zen2hans(strRep.GetBuffer(strWork.GetLength()*2+1), strWork, CONV_ALL);
	strWork.ReleaseBuffer();
	strRep.ReleaseBuffer();
	return strRep;
}

STEP_API bool WINAPI STEPOnCommand(UINT nID, HWND hWnd)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (nID == nIDReConvHiragana) {
		static const auto sMessage = TEXT("MS-IME2002/2000�ȊO�ł͐���ɓ��삵�Ȃ��\��������܂��B\n\n"
									   "�ϊ������s���Ă���낵���ł����H");
		static bool bKan2HiraConfirmIME = false;
		if (bKan2HiraConfirmIME == true || MessageBox(hWnd, sMessage, TEXT("�Ђ炪�Ȃɕϊ�"), MB_YESNO|MB_TOPMOST) == IDYES) {
			bKan2HiraConfirmIME = true;
			int sx, sy, ex, ey;
			if (STEPGetSelectedRange(&sx, &sy, &ex, &ey)) {
				for (int nItem = sy; nItem <= ey; nItem++) {
					if (STEPIsItemFile(nItem) == true) {
						//FILE_INFO info;
						//STEPGetFileInfo(nItem, &info);
						for (int nColumn = sx; nColumn <= ex; nColumn++) {
							CString	strText;
							// �Z���̃e�L�X�g���擾
							strText = conv_kan2hira(hWnd, STEPGetSubItemText(nItem, nColumn));
							STEPChangeSubItemText(nItem, nColumn, conv_kan2hira(hWnd, strText));
						}
					}
				}
			}
		}
		return true;
	}
	if (nID == nIDReConvRomaji) {
		static const auto sMessage = TEXT("MS-IME2002/2000�ȊO�ł͐���ɓ��삵�Ȃ��\��������܂��B\n\n"
									   "�ϊ������s���Ă���낵���ł����H");
		static bool bRomajiConfirmIME = false;
		if (bRomajiConfirmIME == true || MessageBox(hWnd, sMessage, TEXT("���[�}���ɕϊ�"), MB_YESNO|MB_TOPMOST) == IDYES) {
			int sx, sy, ex, ey;
			if (STEPGetSelectedRange(&sx, &sy, &ex, &ey)) {
				for (int nItem = sy; nItem <= ey; nItem++) {
					if (STEPIsItemFile(nItem) == true) {
						//FILE_INFO info;
						//STEPGetFileInfo(nItem, &info);
						for (int nColumn = sx; nColumn <= ex; nColumn++) {
							CString	strText;
							// �Z���̃e�L�X�g���擾
							strText = conv_romaji(hWnd, STEPGetSubItemText(nItem, nColumn));
							STEPChangeSubItemText(nItem, nColumn, conv_kan2hira(hWnd, strText));
						}
					}
				}
			}
		}
		return true;
	}
	return false;
}

STEP_API void WINAPI STEPOnLoadMenu(HMENU hMenu, UINT nType)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	switch (nType) {
	case MENU_FILE_EDIT_OK:
		AddConvMenu(hMenu);
		break;
	}
}

STEP_API void WINAPI STEPOnLoadMainMenu()
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	// ���j���[�ւ̒ǉ�
	HMENU hMenu = STEPGetMenu(MENU_CONV);
	AddConvMenu(hMenu);
}

bool g_bZenHanKigouKana = true; /* STEP 016 */
