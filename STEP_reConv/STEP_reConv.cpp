// STEP_reConv.cpp : DLL 用の初期化処理の定義を行います。
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
//	メモ!
//
//		この DLL が MFC DLL に対して動的にリンクされる場合、
//		MFC 内で呼び出されるこの DLL からエクスポートされた
//		どの関数も関数の最初に追加される AFX_MANAGE_STATE 
//		マクロを含んでいなければなりません。
//
//		例:
//
//		extern "C" BOOL PASCAL EXPORT ExportedFunction()
//		{
//			AFX_MANAGE_STATE(AfxGetStaticModuleState());
//			// 通常関数の本体はこの位置にあります
//		}
//
//		このマクロが各関数に含まれていること、MFC 内の
//		どの呼び出しより優先することは非常に重要です。
//		これは関数内の最初のステートメントでなければな
//		らないことを意味します、コンストラクタが MFC 
//		DLL 内への呼び出しを行う可能性があるので、オブ
//		ジェクト変数の宣言よりも前でなければなりません。
//
//		詳細については MFC テクニカル ノート 33 および
//		58 を参照してください。
//

/////////////////////////////////////////////////////////////////////////////
// CSTEP_reConvApp

BEGIN_MESSAGE_MAP(CSTEP_reConvApp, CWinApp)
	//{{AFX_MSG_MAP(CSTEP_reConvApp)
		// メモ - ClassWizard はこの位置にマッピング用のマクロを追加または削除します。
		//        この位置に生成されるコードを編集しないでください。
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSTEP_reConvApp の構築

CSTEP_reConvApp::CSTEP_reConvApp()
{
	// TODO: この位置に構築用のコードを追加してください。
	// ここに InitInstance の中の重要な初期化処理をすべて記述してください。
}

/////////////////////////////////////////////////////////////////////////////
// 唯一の CSTEP_reConvApp オブジェクト

CSTEP_reConvApp theApp;

UINT nPluginID;

CString strINI;

// コマンドID
UINT nIDReConvHiragana;
UINT nIDReConvRomaji;

void AddConvMenu(HMENU hMenu) {
	InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_SEPARATOR, 0, NULL);
	InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_STRING, nIDReConvHiragana, TEXT("ひらがなに変換"));
	InsertMenu(hMenu, MF_BYPOSITION, MF_BYPOSITION | MFT_STRING, nIDReConvRomaji, TEXT("ローマ字に変換"));
}

STEP_API bool WINAPI STEPInit(UINT pID, LPCTSTR szPluginFolder)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (Initialize() == false)	return false;
	nPluginID = pID;

	// INIファイルの読み込み
	strINI = szPluginFolder;
	strINI += "STEP_reConv.ini";

	nIDReConvHiragana = STEPGetCommandID();
	STEPKeyAssign(nIDReConvHiragana, TEXT("ひらがなに変換"), TEXT("STEP_reConv_KEY_RE_CONV_HIRAGANA"));
	nIDReConvRomaji = STEPGetCommandID();
	STEPKeyAssign(nIDReConvRomaji, TEXT("ローマ字に変換"), TEXT("STEP_reConv_KEY_RE_CONV_ROMAJI"));

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
	return TEXT("Version 0.03 Copyright (C) 2003-2006 haseta\r\nMS-IME2000/2002によりひらがな/ローマ字に変換します");
}

STEP_API LPCTSTR WINAPI STEPGetStatusMessage(UINT nID)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	if (nID == nIDReConvHiragana) {
		return TEXT("選択されている範囲をひらがなに変換します");
	}
	if (nID == nIDReConvRomaji) {
		return TEXT("選択されている範囲をローマ字に変換します");
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
	// MS-IME2000ではうまく動いたが、ATOK16ではだめ。単語単位であればできる...
	HIMC himc = ::ImmGetContext(hWnd);
	DWORD dwRet = ::ImmGetConversionList(
					 ::GetKeyboardLayout(0), himc,
					 str, NULL, 0,
					 GCL_REVERSECONVERSION);

	// 読み仮名格納領域を確保 
	CANDIDATELIST *lpCand;
	lpCand = (CANDIDATELIST *)malloc(dwRet);
	// 読み仮名を取得
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
		TEXT("っきゃ"), TEXT("KKYA"), TEXT("っきゅ"), TEXT("KKYU"), TEXT("っきょ"), TEXT("KYO"),
		TEXT("きゃ"), TEXT("KYA"), TEXT("きゅ"), TEXT("KYU"), TEXT("きょ"), TEXT("KYO"),
		TEXT("っしゃ"), TEXT("SSHA"), TEXT("っしゅ"), TEXT("SSHU"), TEXT("っしょ"), TEXT("SSHO"),
		TEXT("しゃ"), TEXT("SHA"), TEXT("しゅ"), TEXT("SHU"), TEXT("しょ"), TEXT("SHO"),
		TEXT("っちゃ"), TEXT("CCHA"), TEXT("っちゅ"), TEXT("CCHU"), TEXT("っちょ"), TEXT("CCHO"),
		TEXT("ちゃ"), TEXT("CHA"), TEXT("ちゅ"), TEXT("CHU"), TEXT("ちょ"), TEXT("CHO"),
		TEXT("っにゃ"), TEXT("NNYA"), TEXT("っにゅ"), TEXT("NNYU"), TEXT("っにょ"), TEXT("NNYO"),
		TEXT("にゃ"), TEXT("NYA"), TEXT("にゅ"), TEXT("NYU"), TEXT("にょ"), TEXT("NYO"),
		TEXT("っひゃ"), TEXT("HHYA"), TEXT("っひゅ"), TEXT("HHYU"), TEXT("っひょ"), TEXT("HHYO"),
		TEXT("ひゃ"), TEXT("HYA"), TEXT("ひゅ"), TEXT("HYU"), TEXT("ひょ"), TEXT("HYO"),
		TEXT("っみゃ"), TEXT("MMYA"), TEXT("っみゅ"), TEXT("MMYU"), TEXT("っみょ"), TEXT("MMYO"),
		TEXT("みゃ"), TEXT("MYA"), TEXT("みゅ"), TEXT("MYU"), TEXT("みょ"), TEXT("MYO"),
		TEXT("っりゃ"), TEXT("RRYA"), TEXT("っりゅ"), TEXT("RRYU"), TEXT("っりょ"), TEXT("RRYO"),
		TEXT("りゃ"), TEXT("RYA"), TEXT("りゅ"), TEXT("RYU"), TEXT("りょ"), TEXT("RYO"),
		TEXT("っぎゃ"), TEXT("GGYA"), TEXT("っぎゅ"), TEXT("GGYU"), TEXT("っぎょ"), TEXT("GGYO"),
		TEXT("ぎゃ"), TEXT("GYA"), TEXT("ぎゅ"), TEXT("GYU"), TEXT("ぎょ"), TEXT("GYO"),
		TEXT("っじゃ"), TEXT("JJA"), TEXT("っじゅ"), TEXT("JJU"), TEXT("っじょ"), TEXT("JJO"),
		TEXT("じゃ"), TEXT("JA"), TEXT("じゅ"), TEXT("JU"), TEXT("じょ"), TEXT("JO"),
		TEXT("っびゃ"), TEXT("BBYA"), TEXT("っびゅ"), TEXT("BBYU"), TEXT("っびょ"), TEXT("BBYO"),
		TEXT("びゃ"), TEXT("BYA"), TEXT("びゅ"), TEXT("BYU"), TEXT("びょ"), TEXT("BYO"),
		TEXT("っぴゃ"), TEXT("PPYA"), TEXT("っぴゅ"), TEXT("PPYU"), TEXT("っぴょ"), TEXT("PPYO"),
		TEXT("ぴゃ"), TEXT("PYA"), TEXT("ぴゅ"), TEXT("PYU"), TEXT("ぴょ"), TEXT("PYO"),

		TEXT("っか"), TEXT("KKA"), TEXT("っき"), TEXT("KKI"), TEXT("っく"), TEXT("KKU"), TEXT("っけ"), TEXT("KKE"), TEXT("っこ"), TEXT("KKO"),
		TEXT("か"), TEXT("KA"), TEXT("き"), TEXT("KI"), TEXT("く"), TEXT("KU"), TEXT("け"), TEXT("KE"), TEXT("こ"), TEXT("KO"),
		TEXT("っさ"), TEXT("SSA"), TEXT("っし"), TEXT("SSHI"), TEXT("っす"), TEXT("SSU"), TEXT("っせ"), TEXT("SSE"), TEXT("っそ"), TEXT("SSO"),
		TEXT("さ"), TEXT("SA"), TEXT("し"), TEXT("SHI"), TEXT("す"), TEXT("SU"), TEXT("せ"), TEXT("SE"), TEXT("そ"), TEXT("SO"),
		TEXT("った"), TEXT("TTA"), TEXT("っち"), TEXT("CCHI"), TEXT("っつ"), TEXT("TTSU"), TEXT("って"), TEXT("TTE"), TEXT("っと"), TEXT("TTO"),
		TEXT("た"), TEXT("TA"), TEXT("ち"), TEXT("CHI"), TEXT("つ"), TEXT("TSU"), TEXT("て"), TEXT("TE"), TEXT("と"), TEXT("TO"),
		TEXT("っな"), TEXT("NNA"), TEXT("っに"), TEXT("NNI"), TEXT("っぬ"), TEXT("NNU"), TEXT("っね"), TEXT("NNE"), TEXT("っの"), TEXT("NNO"),
		TEXT("な"), TEXT("NA"), TEXT("に"), TEXT("NI"), TEXT("ぬ"), TEXT("NU"), TEXT("ね"), TEXT("NE"), TEXT("の"), TEXT("NO"),
		TEXT("っは"), TEXT("HHA"), TEXT("っひ"), TEXT("HHI"), TEXT("っふ"), TEXT("FFU"), TEXT("っへ"), TEXT("HHE"), TEXT("っほ"), TEXT("HHO"),
		TEXT("は"), TEXT("HA"), TEXT("ひ"), TEXT("HI"), TEXT("ふ"), TEXT("FU"), TEXT("へ"), TEXT("HE"), TEXT("ほ"), TEXT("HO"),
		TEXT("っま"), TEXT("MMA"), TEXT("っみ"), TEXT("MMI"), TEXT("っむ"), TEXT("MMU"), TEXT("っめ"), TEXT("MME"), TEXT("っも"), TEXT("MMO"),
		TEXT("ま"), TEXT("MA"), TEXT("み"), TEXT("MI"), TEXT("む"), TEXT("MU"), TEXT("め"), TEXT("ME"), TEXT("も"), TEXT("MO"),
		TEXT("っや"), TEXT("YYA"), TEXT("っゆ"), TEXT("YUYU"), TEXT("っよ"), TEXT("YYO"),
		TEXT("や"), TEXT("YA"), TEXT("ゆ"), TEXT("YU"), TEXT("よ"), TEXT("YO"),
		TEXT("っら"), TEXT("RRA"), TEXT("っり"), TEXT("RRI"), TEXT("っる"), TEXT("RRU"), TEXT("っれ"), TEXT("RRE"), TEXT("っろ"), TEXT("RRO"),
		TEXT("ら"), TEXT("RA"), TEXT("り"), TEXT("RI"), TEXT("る"), TEXT("RU"), TEXT("れ"), TEXT("RE"), TEXT("ろ"), TEXT("RO"),
		TEXT("っわ"), TEXT("WWA"),
		TEXT("わ"), TEXT("WA"),
		TEXT("っが"), TEXT("GGA"), TEXT("っぎ"), TEXT("GGI"), TEXT("っぐ"), TEXT("GGU"), TEXT("っげ"), TEXT("GGE"), TEXT("っご"), TEXT("GGO"),
		TEXT("が"), TEXT("GA"), TEXT("ぎ"), TEXT("GI"), TEXT("ぐ"), TEXT("GU"), TEXT("げ"), TEXT("GE"), TEXT("ご"), TEXT("GO"),
		TEXT("っざ"), TEXT("ZZA"), TEXT("っじ"), TEXT("JJI"), TEXT("っず"), TEXT("ZZU"), TEXT("っぜ"), TEXT("ZZE"), TEXT("っぞ"), TEXT("ZZO"),
		TEXT("ざ"), TEXT("ZA"), TEXT("じ"), TEXT("JI"), TEXT("ず"), TEXT("ZU"), TEXT("ぜ"), TEXT("ZE"), TEXT("ぞ"), TEXT("ZO"),
		TEXT("っだ"), TEXT("DDA"), TEXT("っぢ"), TEXT("JJI"), TEXT("っづ"), TEXT("ZZU"), TEXT("っで"), TEXT("DDE"), TEXT("っど"), TEXT("DDO"),
		TEXT("だ"), TEXT("DA"), TEXT("ぢ"), TEXT("JI"), TEXT("づ"), TEXT("ZU"), TEXT("で"), TEXT("DE"), TEXT("ど"), TEXT("DO"),
		TEXT("っば"), TEXT("BBA"), TEXT("っび"), TEXT("BBI"), TEXT("っぶ"), TEXT("BBU"), TEXT("っべ"), TEXT("BBE"), TEXT("っぼ"), TEXT("BBO"),
		TEXT("ば"), TEXT("BA"), TEXT("び"), TEXT("BI"), TEXT("ぶ"), TEXT("BU"), TEXT("べ"), TEXT("BE"), TEXT("ぼ"), TEXT("BO"),
		TEXT("っぱ"), TEXT("PPA"), TEXT("っぴ"), TEXT("PPI"), TEXT("っぷ"), TEXT("PPU"), TEXT("っぺ"), TEXT("PPE"), TEXT("っぽ"), TEXT("PPO"),
		TEXT("ぱ"), TEXT("PA"), TEXT("ぴ"), TEXT("PI"), TEXT("ぷ"), TEXT("PU"), TEXT("ぺ"), TEXT("PE"), TEXT("ぽ"), TEXT("PO"),

		TEXT("っあ"), TEXT("AA"), TEXT("っい"), TEXT("II"), TEXT("っう"), TEXT("UU"), TEXT("っえ"), TEXT("EE"), TEXT("っお"), TEXT("OO"),
		TEXT("あ"), TEXT("A"), TEXT("い"), TEXT("I"), TEXT("う"), TEXT("U"), TEXT("え"), TEXT("E"), TEXT("お"), TEXT("O"),
		TEXT("っを"), TEXT("OO"), TEXT("っん"), TEXT("NN"),
		TEXT("を"), TEXT("O"), TEXT("ん"), TEXT("N"),
		TEXT("ー"), TEXT(""),
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
		static const auto sMessage = TEXT("MS-IME2002/2000以外では正常に動作しない可能性があります。\n\n"
									   "変換を実行してもよろしいですか？");
		static bool bKan2HiraConfirmIME = false;
		if (bKan2HiraConfirmIME == true || MessageBox(hWnd, sMessage, TEXT("ひらがなに変換"), MB_YESNO|MB_TOPMOST) == IDYES) {
			bKan2HiraConfirmIME = true;
			int sx, sy, ex, ey;
			if (STEPGetSelectedRange(&sx, &sy, &ex, &ey)) {
				for (int nItem = sy; nItem <= ey; nItem++) {
					if (STEPIsItemFile(nItem) == true) {
						//FILE_INFO info;
						//STEPGetFileInfo(nItem, &info);
						for (int nColumn = sx; nColumn <= ex; nColumn++) {
							CString	strText;
							// セルのテキストを取得
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
		static const auto sMessage = TEXT("MS-IME2002/2000以外では正常に動作しない可能性があります。\n\n"
									   "変換を実行してもよろしいですか？");
		static bool bRomajiConfirmIME = false;
		if (bRomajiConfirmIME == true || MessageBox(hWnd, sMessage, TEXT("ローマ字に変換"), MB_YESNO|MB_TOPMOST) == IDYES) {
			int sx, sy, ex, ey;
			if (STEPGetSelectedRange(&sx, &sy, &ex, &ey)) {
				for (int nItem = sy; nItem <= ey; nItem++) {
					if (STEPIsItemFile(nItem) == true) {
						//FILE_INFO info;
						//STEPGetFileInfo(nItem, &info);
						for (int nColumn = sx; nColumn <= ex; nColumn++) {
							CString	strText;
							// セルのテキストを取得
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
	// メニューへの追加
	HMENU hMenu = STEPGetMenu(MENU_CONV);
	AddConvMenu(hMenu);
}

bool g_bZenHanKigouKana = true; /* STEP 016 */
