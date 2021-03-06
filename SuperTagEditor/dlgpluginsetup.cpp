// CDlgPluginSetup.cpp : インプリメンテーション ファイル
//

#include "stdafx.h"
#include "supertageditor.h"
#include "DlgPluginSetup.h"

#include "STEP_api.h"
#include "Plugin.h"

#include "Registry.h"

extern CPlugin plugins;

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifndef ListView_SetCheckState
#define ListView_SetCheckState(hwndLV, i, fCheck)	ListView_SetItemState(hwndLV, i, INDEXTOSTATEIMAGEMASK((fCheck)+1), LVIS_STATEIMAGEMASK)
#endif

#define ListView_GetSelectedItem(listCtrl)			listCtrl.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED | LVIS_FOCUSED)

/////////////////////////////////////////////////////////////////////////////
// CDlgPluginSetup ダイアログ


CDlgPluginSetup::CDlgPluginSetup(CWnd* pParent /*=NULL*/)
	: CDialog(CDlgPluginSetup::IDD, pParent)
{
	//{{AFX_DATA_INIT(CDlgPluginSetup)
	m_strPluginInfo = _T("");
	//}}AFX_DATA_INIT
}


void CDlgPluginSetup::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CDlgPluginSetup)
	DDX_Control(pDX, IDC_LIST_PLUGIN, m_listPlugin);
	DDX_Text(pDX, IDC_EDIT_INFO, m_strPluginInfo);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CDlgPluginSetup, CDialog)
	//{{AFX_MSG_MAP(CDlgPluginSetup)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_PLUGIN, OnItemchangedListPlugin)
	ON_BN_CLICKED(IDC_BT_SETUP, OnBtSetup)
	ON_BN_CLICKED(IDC_BT_INSTALL, OnBtInstall)
	ON_BN_CLICKED(IDC_BT_UP, OnBtUp)
	ON_BN_CLICKED(IDC_BT_DOWN, OnBtDown)
	ON_BN_CLICKED(IDC_BT_UNINSTALL, OnBtUninstall)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDlgPluginSetup メッセージ ハンドラ

BOOL CDlgPluginSetup::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// TODO: この位置に初期化の補足処理を追加してください
	DWORD	dwStyle;
	dwStyle = m_listPlugin.SendMessage(LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
	dwStyle |= LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT;
	m_listPlugin.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);

	RECT	rect;
	m_listPlugin.GetClientRect(&rect);

	m_listPlugin.InsertColumn(1, TEXT("プラグイン"), LVCFMT_LEFT, rect.right-rect.left-16, -1);
	m_listPlugin.DeleteAllItems();					// クリア

	for (int nIndex=0;nIndex<plugins.arPlugins.GetSize();nIndex++) {
		PSTEPlugin pPlugin = (PSTEPlugin)plugins.arPlugins.GetAt(nIndex);
		m_listPlugin.InsertItem(nIndex, pPlugin->sPluginName);
		m_listPlugin.SetItemData(nIndex, (DWORD)pPlugin);
		ListView_SetCheckState(m_listPlugin.GetSafeHwnd(), nIndex, pPlugin->bUse ? TRUE : FALSE);
	}
	
	return TRUE;  // コントロールにフォーカスを設定しないとき、戻り値は TRUE となります
	              // 例外: OCX プロパティ ページの戻り値は FALSE となります
}

void CDlgPluginSetup::OnItemchangedListPlugin(NMHDR* pNMHDR, LRESULT* pResult) 
{
	NM_LISTVIEW* pNMListView = (NM_LISTVIEW*)pNMHDR;
	// TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
	int nIndex =  ListView_GetSelectedItem(m_listPlugin);
	if (nIndex < 0) {
		GetDlgItem(IDC_BT_SETUP)->EnableWindow(FALSE);
		GetDlgItem(IDC_BT_UP)->EnableWindow(FALSE);
		GetDlgItem(IDC_BT_DOWN)->EnableWindow(FALSE);
		GetDlgItem(IDC_BT_UNINSTALL)->EnableWindow(FALSE);
		return;
	}
	GetDlgItem(IDC_BT_UNINSTALL)->EnableWindow(TRUE);

	PSTEPlugin pPlugin = (PSTEPlugin)m_listPlugin.GetItemData(nIndex);
	m_strPluginInfo = pPlugin->sFileName;
	if (pPlugin->STEPGetPluginInfo != NULL) {
		m_strPluginInfo += "\r\n";
		m_strPluginInfo += pPlugin->STEPGetPluginInfo();
	}

	if (pPlugin->STEPShowOptionDialog == NULL) {
		GetDlgItem(IDC_BT_SETUP)->EnableWindow(FALSE);
	} else {
		GetDlgItem(IDC_BT_SETUP)->EnableWindow(TRUE);
	}
	if (nIndex == 0) {
		GetDlgItem(IDC_BT_UP)->EnableWindow(FALSE);
	} else {
		GetDlgItem(IDC_BT_UP)->EnableWindow(TRUE);
	}
	if (nIndex == m_listPlugin.GetItemCount()-1) {
		GetDlgItem(IDC_BT_DOWN)->EnableWindow(FALSE);
	} else {
		GetDlgItem(IDC_BT_DOWN)->EnableWindow(TRUE);
	}

	UpdateData(FALSE);
	*pResult = 0;
}

void CDlgPluginSetup::OnBtSetup() 
{
	// TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
	int nIndex =  ListView_GetSelectedItem(m_listPlugin);
	if (nIndex < 0) return;

	PSTEPlugin pPlugin = (PSTEPlugin)m_listPlugin.GetItemData(nIndex);
	if (pPlugin->STEPShowOptionDialog != NULL) {
		pPlugin->STEPShowOptionDialog(GetSafeHwnd());
	}
}

extern "C" STEP_API void WINAPI STEPUpdateCellInfo(void);
void CDlgPluginSetup::OnBtInstall() 
{
	// TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
	TCHAR szFilter[] = TEXT("STEプラグイン (*.ste)|*.ste|全て (*.*)|*.*||");
	CFileDialog dialog(TRUE, TEXT("ste"), NULL, 0, szFilter, this);
	if (dialog.DoModal() == IDOK) {
		CString strPluginFile = dialog.GetPathName();
		extern PSTEPlugin STEPluginLoadFile(LPCTSTR);
		PSTEPlugin pPlugin = STEPluginLoadFile(strPluginFile);
		if (pPlugin == NULL) {
			MessageBox(TEXT("選択されたプラグインは使用できません。"), TEXT("プラグインのインストール"), MB_ICONSTOP|MB_OK|MB_TOPMOST);
			return;
		}
		pPlugin->bUse = true;
		int nIndex = m_listPlugin.GetItemCount();
		m_listPlugin.InsertItem(nIndex, pPlugin->sPluginName);
		m_listPlugin.SetItemData(nIndex, (DWORD)pPlugin);
		ListView_SetCheckState(m_listPlugin.GetSafeHwnd(), nIndex, pPlugin->bUse ? TRUE : FALSE);

		STEPUpdateCellInfo();
	}
}

void CDlgPluginSetup::OnBtUp() 
{
	// TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
	int nIndex =  ListView_GetSelectedItem(m_listPlugin);
	PSTEPlugin pPlugin = (PSTEPlugin)m_listPlugin.GetItemData(nIndex);
	nIndex--;
	m_listPlugin.InsertItem(nIndex, pPlugin->sPluginName);
	m_listPlugin.SetItemData(nIndex, (DWORD)pPlugin);
	ListView_SetCheckState(m_listPlugin.GetSafeHwnd(), nIndex, pPlugin->bUse ? TRUE : FALSE);
	m_listPlugin.DeleteItem(nIndex+2);
	m_listPlugin.SetItemState(nIndex, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
}

void CDlgPluginSetup::OnBtDown() 
{
	// TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
	int nIndex =  ListView_GetSelectedItem(m_listPlugin);
	PSTEPlugin pPlugin = (PSTEPlugin)m_listPlugin.GetItemData(nIndex);
	nIndex += 2;
	m_listPlugin.InsertItem(nIndex, pPlugin->sPluginName);
	m_listPlugin.SetItemData(nIndex, (DWORD)pPlugin);
	ListView_SetCheckState(m_listPlugin.GetSafeHwnd(), nIndex, pPlugin->bUse ? TRUE : FALSE);
	m_listPlugin.DeleteItem(nIndex-2);
	m_listPlugin.SetItemState(nIndex-1, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);	
}

void CDlgPluginSetup::OnOK() 
{
	auto pApp = (CSuperTagEditorApp*)AfxGetApp();

	auto strINI = pApp->MakeFileName(TEXT("Plugin"), TEXT("ini"));
	Profile_Initialize(strINI.c_str(), FALSE);

	for (int nIndex = 0; nIndex < m_listPlugin.GetItemCount(); nIndex++) {
		auto pPlugin = (PSTEPlugin)m_listPlugin.GetItemData(nIndex);
		pPlugin->bUse = ListView_GetCheckState(m_listPlugin.GetSafeHwnd(), nIndex) ? true : false;
		// 相対パスに変換
		TCHAR buff[_MAX_PATH] = {};
		if (!PathRelativePathTo(buff, strINI.c_str(), 0, pPlugin->sFileName, 0)) {
			// 変換なし
			lstrcpy(buff, pPlugin->sFileName);
		}

		CString strSection;
		strSection.Format(TEXT("Load%03d"), nIndex);
		Profile_WriteString(strSection, TEXT("Path"), buff, strINI.c_str());
		Profile_WriteString(strSection, TEXT("Use"), pPlugin->bUse ? TEXT("1") : TEXT("0"), strINI.c_str());
	}

	Profile_Flush(strINI.c_str());
	Profile_Free();
	CDialog::OnOK();
}

void CDlgPluginSetup::OnBtUninstall() 
{
	// TODO: この位置にコントロール通知ハンドラ用のコードを追加してください
	if (MessageBox(TEXT("選択されているプラグインをアンインストールしますか？\n※ファイルは削除されません。"), TEXT("アンインストール"), MB_YESNO|MB_TOPMOST) == IDYES) {
		int nIndex =  ListView_GetSelectedItem(m_listPlugin);
		PSTEPlugin pPlugin = (PSTEPlugin)m_listPlugin.GetItemData(nIndex);
		pPlugin->bUse = false;
		m_listPlugin.DeleteItem(nIndex);
	}
}
