// Id3tagv2.cpp: CId3tagv2 クラスのインプリメンテーション
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
//#include "mp3infp_res/resource.h"		// メイン シンボル
#include "GlobalCommand.h"
//#include "ClassID.h"
#include "ID3v2/Id3tagv1.h"
#include "ID3v2/Id3tagv2.h"

#include <vector>

static const unsigned long ID3V2_PADDING_SIZE = 0x0800;
//static const unsigned char SCMPX_GENRE_NULL = 247;
//static const unsigned char WINAMP_GENRE_NULL = 255;
const int CId3tagv2::ID3V2CHARENCODE_ISO_8859_1	= 0;
const int CId3tagv2::ID3V2CHARENCODE_UTF_16		= 1;
const int CId3tagv2::ID3V2CHARENCODE_UTF_16BE	= 2;
const int CId3tagv2::ID3V2CHARENCODE_UTF_8		= 3;

// 2004-09-24 UTF-16はリトルエンディアンで書き込む
//#define UTF16_BIGENDIAN

// STEP
extern bool		bOptId3v2OldComment;	// mp3infp2.43以降では不要
extern bool		bOptId3v2UNICODE;		// mp3infp2.46以降ではBool型から変更されたのでSTEPでも詳細を設定できるように
extern bool		bOptNotUnSyncAlways;
extern bool		bOptUnSyncNew;

namespace {
	// UNICODE文字列のBE/LEを変換する(len=文字数)
	void UTF16toUTF16BE(WCHAR *str, int len)
	{
		int i; for (i = 0; i<len; i++)
		{
			str[i] = (str[i] << 8) | (str[i] >> 8);
		}
	}

	CString readUtf16String(unsigned char* first, unsigned char* last, bool isBe)
	{
		//UTF-16
		const auto size = std::distance(first, last) / 2;
		auto text = reinterpret_cast<LPWSTR>(first);
		if (isBe) {
			UTF16toUTF16BE(text, size);
		}

		auto len = wcsnlen(text, size);
		if (len <= 0) {
			return TEXT("");
		}

		return static_cast<CString>(CStringW(text, len));
	}

	CString readUtf16String(unsigned char* first, unsigned char* last)
	{
		if (first >= last) {
			return TEXT("");
		}

		auto it = first;
		if ((std::distance(it, last) >= 2) && (memcmp(it, "\xff\xfe", 2) == 0))
		{
			// UTF-16
			std::advance(it, 2);
			return readUtf16String(it, last, false);
		}
		else if ((std::distance(it, last) >= 2) && (memcmp(it, "\xfe\xff", 2) == 0))
		{
			// UTF-16(BE) -> UTF-16
			std::advance(it, 2);
			return readUtf16String(it, last, true);
		}

		return TEXT("");
	}

	CString readUtf8String(unsigned char* first, unsigned char* last)
	{
		if (first >= last) {
			return TEXT("");
		}

		//UTF-8 -> UTF-16
		auto src = reinterpret_cast<LPCSTR>(first);
		auto srcLength = std::distance(first, last);
		const int size = MultiByteToWideChar(CP_UTF8, 0, src, srcLength, NULL, 0);
		std::vector<WCHAR> buf(size);
		MultiByteToWideChar(CP_UTF8, 0, src, srcLength, buf.data(), buf.size());

		auto len = wcsnlen(buf.data(), buf.size());
		if (len <= 0) {
			return TEXT("");
		}

		return static_cast<CString>(CStringW(buf.data(), len));
	}

	CString readCStringA(unsigned char* first, unsigned char* last)
	{
		if (first >= last) {
			return "";
		}

		// 終端の\0を取り除く　2002-09-16
		const auto size = std::distance(first, last);
		auto text = reinterpret_cast<LPCSTR>(first);
		auto len = strnlen(text, size);
		if (len == 0)
		{
			return "";
		}

		return static_cast<CString>(CStringA(text, len));
	}

	/// <summary>
	/// 説明文を読み飛ばす(unicode)
	/// </summary>
	/// <returns>読み飛ばした後のIndex</returns>
	int skipUnicodeDescip(int offset, const unsigned char* data, std::size_t size)
	{
		for (int i = offset; i < size - 1; i += 2)
		{
			if ((data[i] == '\0') && (data[i + 1] == '\0'))
			{
				return i + 2;
			}
		}

		return size;
	}

	/// <summary>
	/// 説明文を読み飛ばす(MBCS)
	/// </summary>
	/// <returns>読み飛ばした後のIndex</returns>
	int skipMbcsDescip(int offset, const unsigned char* data, std::size_t size)
	{
		for (int i = offset; i < size; i++)
		{
			if (data[i] == '\0')
			{
				return i + 1;
			}
		}

		return size;
	}
}
//////////////////////////////////////////////////////////////////////
// 構築/消滅
//////////////////////////////////////////////////////////////////////

CId3tagv2::CId3tagv2()
{
	m_encode = ID3V2CHARENCODE_ISO_8859_1;
	m_bUnSynchronization = TRUE;
//	m_wDefaultId3TagVersion = 0x0300;
	Release();
}

CId3tagv2::~CId3tagv2()
{
}

void CId3tagv2::Release()
{
	m_bEnable = FALSE;
	memset(&m_head,0,sizeof(m_head));
	memcpy(m_head.id3des,"ID3",3);

	m_wVer = 0x0300;
	m_frames.clear();
}

void CId3tagv2::SetEncDefault(const char *szDefaultEnc)
{
	m_strDefaultEnc = szDefaultEnc;
}

CString CId3tagv2::GetId3String(const char szId[])
{
	decltype(m_frames)::iterator p;
	DWORD dwId;
	unsigned char *data;
	switch(szId[0]){
	case 'T':	//テキスト情報フレーム
		memcpy(&dwId,szId,sizeof(dwId));
		p = m_frames.find(dwId);
		if((p == m_frames.end()) || !p->second.GetSize())
		{
			break;
		}
		data = p->second.GetData();
		switch (data[0]) {
		case 0x01:
			return readUtf16String(data + 1, data + p->second.GetSize());
		case 0x02:
			//UTF-16(BE) -> UTF-16
			return readUtf16String(data + 1, data + p->second.GetSize(), true);
		case 0x03:
			//UTF-8 -> USC-2(Unicode)
			return readUtf8String(data + 1, data + p->second.GetSize());
		case 0x00:
			return readCStringA(data + 1, data + p->second.GetSize());
		}
		break;
	case 'W':	//URLリンクフレームx
		memcpy(&dwId,szId,sizeof(dwId));
		p = m_frames.find(dwId);
		if((p == m_frames.end()) || !p->second.GetSize())
		{
			return "";
		}
		data = p->second.GetData();
		if( (p->second.GetSize() >= 4) && (memcmp(data,"\x01\xff\xfe",3) == 0) )
		{
			auto i = skipUnicodeDescip(3, data, p->second.GetSize());
			return readUtf16String(data + i, data + p->second.GetSize());
		}
		else if( (p->second.GetSize() >= 4) && (memcmp(data,"\x01\xfe\xff",3) == 0) )
		{
			auto i = skipUnicodeDescip(3, data, p->second.GetSize());
			return readUtf16String(data + i, data + p->second.GetSize());
		}
		else if( (p->second.GetSize() >= 1) && (data[0] == 0x02) )
		{
			auto i = skipUnicodeDescip(1, data, p->second.GetSize());
			return readUtf16String(data + i, data + p->second.GetSize(), true);
		}
		else if( (p->second.GetSize() >= 1) && (data[0] == 0x03) )
		{
			auto i = skipMbcsDescip(1, data, p->second.GetSize());
			return readUtf8String(data + i, data + p->second.GetSize());
		}
		else if((p->second.GetSize() >= 2) && (data[0] == 0))
		{
			auto i = skipMbcsDescip(1, data, p->second.GetSize());
			return readCStringA(data + i, data + p->second.GetSize());
		}
		break;
	case 'C':
		if(strcmp(szId,"COMM") != 0)
		{
			break;
		}
		memcpy(&dwId,szId,sizeof(dwId));
		p = m_frames.find(dwId);
		if((p == m_frames.end()) || !p->second.GetSize())
		{
			return "";
		}
		data = p->second.GetData();
		if( (p->second.GetSize() >= (1+3/*Language*/+4/*BOM 0 0*/+2/*BOM*/)) &&
			(data[0] == 1) &&
			(memcmp(&data[1+3/*Language*/],"\xff\xfe",2) == 0))
		{
			auto i = skipUnicodeDescip(3 + 3/*Language*/ - 2/* STEP */, data, p->second.GetSize());
			return readUtf16String(data + i, data + p->second.GetSize());
		}
		else if( (p->second.GetSize() >= (1+3/*Language*/+4/*BOM 0 0*/+2/*BOM*/)) &&
			(data[0] == 1) &&
			(memcmp(&data[1+3/*Language*/],"\xfe\xff",2) == 0))
		{
			auto i = skipUnicodeDescip(3 + 3/*Language*/ - 2/* STEP */, data, p->second.GetSize());
			return readUtf16String(data + i, data + p->second.GetSize());
		}
		/* STEP */else if( (p->second.GetSize() >= (1+3/*Language*/+4/*BOM 0 0*/+2/*BOM*/)) &&
			(data[0] == 1) &&
			(memcmp(&data[1+3/*Language*/],"\x00\x00",2) == 0) )
		{
			//説明文を読み飛ばす
			auto i = 1+3;
			if(i >= p->second.GetSize())
			{
				break;//本文がない場合
			}
			i += 2;
			return readUtf16String(data + i, data + p->second.GetSize());
		}
		else if( (p->second.GetSize() >= (1+3/*Language*/+1/*0*/)) &&
			(data[0] == 2) )
		{
			auto i = skipUnicodeDescip(1 + 3/*Language*/, data, p->second.GetSize());
			return readUtf16String(data + i, data + p->second.GetSize(), true);
		}
		else if( (p->second.GetSize() >= (1+3/*Language*/+1/*0*/)) &&
			(data[0] == 3) )
		{
			auto i = skipMbcsDescip(1 + 3/*Language*/, data, p->second.GetSize());
			return readUtf8String(data + i, data + p->second.GetSize());
		}
		else if((p->second.GetSize() >= 2+3) && (data[0] == 0))
		{
			auto i = skipMbcsDescip(1 + 3/*Language*/, data, p->second.GetSize());
			return readCStringA(data + i, data + p->second.GetSize());
		}
		break;
	}
	return "";
}

void CId3tagv2::SetId3String(const char szId[], LPCSTR szString, const char *szDescription)
{
	return SetId3String(szId, CStringW(szString), szDescription);
}
void CId3tagv2::SetId3String(const char szId[], LPCWSTR szString, const char *szDescription)
{
	multimap<DWORD,CId3Frame>::iterator p;
	CId3Frame *pFrame;
	DWORD dwId;
	memcpy(&dwId, szId, sizeof(dwId));

	//Loadしたファイルにフレームがなかった場合
	const auto length = wcslen(szString);
	if(length == 0)
	{
		m_frames.erase(dwId);	//消す(あれば)
		return;
	}
	
	std::vector<std::uint8_t> data;
	switch(szId[0]){
	case 'T':	//テキスト情報フレーム
		switch (m_encode) {
		case ID3V2CHARENCODE_ISO_8859_1:
		default:	// ISO-8859-1
			{
				CStringA text = szString;
				data.resize(strlen(text) + 2);
				data[0] = 0;	//encoding
				strcpy_s((char *)&data[1], data.size() - 1, text);
			}
			break;
		case ID3V2CHARENCODE_UTF_16:
			// UTF-16
			data.resize((length * sizeof(WCHAR)) + 3);
#ifndef UTF16_BIGENDIAN
			data[0] = 1;	//encoding
			data[1] = 0xff;	//BOM
			data[2] = 0xfe;
			memcpy(&data[3], szString, length * sizeof(WCHAR));
#else
			data[0] = 1;	//encoding
			data[1] = 0xfe;	//BOM
			data[2] = 0xff;
			memcpy(&data[3], szString, length * sizeof(WCHAR));
			UTF16toUTF16BE((WCHAR*)&data[1], length);
#endif
			break;
		case ID3V2CHARENCODE_UTF_16BE:	// UTF-16BE
			// UTF-16 -> UTF-16BE
			data.resize((length * sizeof(WCHAR)) + 1);
			data[0] = 0x02;	//encoding
			memcpy(&data[1], szString, length * sizeof(WCHAR));
			UTF16toUTF16BE((WCHAR*)&data[1], length);
			break;
		case ID3V2CHARENCODE_UTF_8:	// UTF-8
			// UNICODE -> UTF-8
			data.resize(WideCharToMultiByte(CP_UTF8, 0, szString, -1, NULL, 0, NULL, NULL) + 1);
			data[0] = 3;	//encoding
			WideCharToMultiByte(CP_UTF8, 0, szString, -1, (char *)&data[1], data.size() - 1, NULL, NULL);
			break;
		}
		p = m_frames.find(dwId);
		if((p == m_frames.end()) || !p->second.GetSize())
		{
			//Loadしたファイルにフレームがなかった場合
			CId3Frame frame;
			frame.SetId(dwId);
			frame.SetFlags(0);
			frame.SetData(data.data(), data.size());
			m_frames.insert(pair<DWORD,CId3Frame>(frame.GetId(),frame));
		}
		else
		{
			pFrame = &p->second;
			pFrame->SetData(data.data(), data.size());
		}
		break;
	case 'W':	//URLリンクフレームx
		switch(m_encode){
		case ID3V2CHARENCODE_ISO_8859_1:
		default:	// ISO-8859-1
			{
				CStringA text = szString;
				data.resize(strlen(text) + 3);
				data[0] = 0;	//encoding
				data[1] = 0;	//説明文(省略)
				strcpy_s((char *)&data[2], data.size() - 2, text);
			}
			break;
		case ID3V2CHARENCODE_UTF_16:
			// UTF-16
			data.resize((length * sizeof(WCHAR)) + 7);
#ifndef UTF16_BIGENDIAN
			data[0] = 1;	//encoding
			data[1] = 0xff;	//BOM
			data[2] = 0xfe;
			data[3] = 0;	//説明文(省略)
			data[4] = 0;
			data[5] = 0xff;	//BOM
			data[6] = 0xfe;
			memcpy(&data[7], szString, length * sizeof(WCHAR));
#else	// ビックエンディアン
			data[0] = 1;	//encoding
			data[1] = 0xfe;	//BOM
			data[2] = 0xff;
			data[3] = 0;	//説明文(省略)
			data[4] = 0;
			data[5] = 0xfe;	//BOM
			data[6] = 0xff;
			memcpy(&data[7], szString, length * sizeof(WCHAR));
			UTF16toUTF16BE((LPWSTR)&data[7], length);
#endif
			break;
		case ID3V2CHARENCODE_UTF_16BE:
			// UNICODE -> UTF-16BE
			data.resize((length * sizeof(WCHAR)) + 3);
			data[0] = 2;	//encoding
			data[1] = 0;	//説明文(省略)
			data[2] = 0;
			memcpy(&data[3], szString, length * sizeof(WCHAR));
			UTF16toUTF16BE((LPWSTR)&data[3], length);
			break;
		case ID3V2CHARENCODE_UTF_8:
			// UNICODE -> UTF-8
			data.resize(WideCharToMultiByte(CP_UTF8, 0, szString, -1, NULL, 0, NULL, NULL) + 2);
			data[0] = 3;	//encoding
			data[1] = 0;	//説明文(省略)
			WideCharToMultiByte(CP_UTF8, 0, szString, -1, (char *)&data[2], data.size() - 2, NULL, NULL);
			break;
		}
		p = m_frames.find(dwId);
		if((p == m_frames.end()) || !p->second.GetSize())
		{
			//Loadしたファイルにフレームがなかった場合
			CId3Frame frame;
			frame.SetId(dwId);
			frame.SetFlags(0);
			frame.SetData(data.data(), data.size());
			m_frames.insert(pair<DWORD,CId3Frame>(frame.GetId(),frame));
		}
		else
		{
			pFrame = &p->second;
			pFrame->SetData(data.data(), data.size());
		}
		break;
	case 'C':
		if(strcmp(szId,"COMM") != 0)
		{
			break;
		}
		switch(m_encode){
		case ID3V2CHARENCODE_ISO_8859_1:
		default:	// ISO-8859-1
			{
				CStringA text = szString;
				data.resize(strlen(text) + 1 + 5);
				data[0] = 0;	//encoding
				data[1] = 'e';	//Language
				data[2] = 'n';
				data[3] = 'g';
				data[4] = 0;	//説明文(省略)
				strcpy_s((char *)&data[5], data.size() - 5, text);
			}
			break;
		case ID3V2CHARENCODE_UTF_16:
			// UTF-16
			data.resize((length * sizeof(WCHAR)) + 10);
#ifndef UTF16_BIGENDIAN
			data[0] = 1;	//encoding
			data[1] = 'e';	//Language
			data[2] = 'n';
			data[3] = 'g';
			data[4] = 0xff;	//BOM
			data[5] = 0xfe;
			data[6] = 0;	//説明文(省略)
			data[7] = 0;
			data[8] = 0xff;	//BOM
			data[9] = 0xfe;
			memcpy(&data[10], szString, length * sizeof(WCHAR));
#else	// ビッグエンディアン
			data[0] = 1;	//encoding
			data[1] = 'e';	//Language
			data[2] = 'n';
			data[3] = 'g';
			data[4] = 0xfe;	//BOM
			data[5] = 0xff;
			data[6] = 0;	//説明文(省略)
			data[7] = 0;
			data[8] = 0xfe;	//BOM
			data[9] = 0xff;
			memcpy(&data[10], szString, length * sizeof(WCHAR));
			UTF16toUTF16BE((LPWSTR)&data[10], length);
#endif
			break;
		case ID3V2CHARENCODE_UTF_16BE:
			// UNICODE -> UTF-16BE
			data.resize((length * sizeof(WCHAR)) + 6);
			data[0] = 2;	//encoding
			data[1] = 'e';	//Language
			data[2] = 'n';
			data[3] = 'g';
			data[4] = 0;	//説明文(省略)
			data[5] = 0;	//説明文(省略)
			memcpy(&data[6], szString, length * sizeof(WCHAR));
			UTF16toUTF16BE((LPWSTR)&data[10], length);
			break;
		case ID3V2CHARENCODE_UTF_8:
			// UNICODE -> UTF-8
			data.resize(WideCharToMultiByte(CP_UTF8, 0, szString, -1, NULL, 0, NULL, NULL) + 5);
			data[0] = 3;	//encoding
			data[1] = 'e';	//Language
			data[2] = 'n';
			data[3] = 'g';
			data[4] = 0;	//説明文(省略)
			WideCharToMultiByte(CP_UTF8, 0, szString, -1, (char *)&data[5], data.size() - 5, NULL, NULL);
			break;
			
		}
		p = m_frames.find(dwId);
		if((p == m_frames.end()) || !p->second.GetSize())
		{
			CId3Frame frame;
			frame.SetId(dwId);
			frame.SetFlags(0);
			frame.SetData(data.data(), data.size());
			m_frames.insert(pair<DWORD,CId3Frame>(frame.GetId(),frame));
		}
		else
		{
			pFrame = &p->second;
			pFrame->SetData(data.data(), data.size());
		}
		break;
	}
	return;
}

DWORD CId3tagv2::GetTotalFrameSize()
{
	DWORD dwSize = 0;
	multimap<DWORD,CId3Frame>::iterator p;

	p = m_frames.begin();
	while(p != m_frames.end())
	{
		CId3Frame *pFrame = &p->second;
		if(m_wVer == 0x0200)
		{
			dwSize += pFrame->GetSize() + 6;
		}
		else
		{
			dwSize += pFrame->GetSize() + 10;
		}

		p++;
	}
	return dwSize;
}

void CId3tagv2::v23IDtov22ID(char *v23ID,char *v22ID)
{
	//v2.3からv2.2へフレームIDを変換
	if(memcmp(v23ID,"TIT2",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TT2",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TRCK",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TRK",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TPE1",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TP1",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TALB",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TAL",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TDRC",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TYE",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TYER",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TYE",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TCON",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TCO",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"COMM",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"COM",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TCOM",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TCM",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TOPE",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TOA",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TCOP",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TCR",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"WXXX",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"WXX",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TSSE",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TSS",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TENC",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TEN",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"APIC",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"PIC",sizeof(v22ID));
	}
	/* STEP start */
	else if(memcmp(v23ID,"TBPM",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TBP",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TPOS",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TPA",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TDAT",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TDA",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TDLY",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TDY",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TFLT",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TFT",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TIME",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TIM",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TKEY",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TKE",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TLAN",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TLA",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TLEN",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TLE",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TMED",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TMT",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TOFN",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TOF",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TOLY",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TOL",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TORY",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TOR",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TOAL",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TOT",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TPE2",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TP2",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TPE3",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TP3",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TPE4",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TP4",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TPUB",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TPB",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TSRC",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TRC",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TRDA",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TRD",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TSIZ",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TSI",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TIT1",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TT1",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TIT3",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TT3",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TEXT",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TXT",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TXXX",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TXX",sizeof(v22ID));
	}
	else if(memcmp(v23ID,"TPUB",sizeof(v23ID)) == 0)
	{
		memcpy(v22ID,"TPB",sizeof(v22ID));
	}
	/* STEP end */
	else
	{
		// 不明
		memcpy(v22ID,"XXX",sizeof(v22ID));
	}
}

CString CId3tagv2::GetTitle()
{
	//タイトル
	return GetId3String("TIT2");
}

void CId3tagv2::SetTitle(LPCTSTR title)
{
	//タイトル
	SetId3String("TIT2",title);
}

CString CId3tagv2::GetTrackNo()
{
	//トラック番号
	return GetId3String("TRCK");
}

void CId3tagv2::SetTrackNo(LPCTSTR szTrackNo)
{
	//トラック番号
	SetId3String("TRCK",szTrackNo);
}

CString CId3tagv2::GetDiskNo()
{
	//ディスク番号
	return GetId3String("TPOS");
}

void CId3tagv2::SetDiskNo(LPCTSTR szTrackNo)
{
	//ディスク番号
	SetId3String("TPOS",szTrackNo);
}

CString CId3tagv2::GetArtist()
{
	//アーティスト
	return GetId3String("TPE1");
}

void CId3tagv2::SetArtist(LPCTSTR artist)
{
	//アーティスト
	SetId3String("TPE1",artist);
}

CString CId3tagv2::GetAlbum()
{
	//アルバム
	return GetId3String("TALB");
}

void CId3tagv2::SetAlbum(LPCTSTR album)
{
	//アルバム
	SetId3String("TALB",album);
}

CString CId3tagv2::GetYear()
{
	//西暦
	if(m_wVer < 0x0400)
	{
		return GetId3String("TYER");
	}
	else
	{
		return GetId3String("TDRC");
	}
}

void CId3tagv2::SetYear(LPCTSTR year)
{
	//西暦
	if(m_wVer < 0x0400)
	{
		SetId3String("TDRC", TEXT(""));
		SetId3String("TYER",year);
	}
	else
	{
		SetId3String("TDRC",year);
		SetId3String("TYER", TEXT(""));
	}
}

CString CId3tagv2::GetGenre()
{
	//ジャンル
	CStringA strGenre = static_cast<CStringA>(GetId3String("TCON"));
	unsigned char *data = (unsigned char *)(LPCSTR )strGenre;
	//最初の()を読み飛ばす処理	Fix 2001-05-20
	while(1)
	{
		if(strGenre.GetLength() &&
			!IsDBCSLeadByte(strGenre[0]) &&
			(strGenre[0] == '(') )	//頭にカッコを検出
		{
			if((strGenre.GetLength() > 1) &&
				!IsDBCSLeadByte(strGenre[1]) &&
				(strGenre[1] == '(') )
			{
				strGenre.Delete(0);	//先頭の'('を削除
				break;
			}
			int index = strGenre.Find(')');	
			if(0 <= index)
			{
				auto strGenreFromCode = strGenre;
				strGenre.Delete(0,index+1);	//)'以前を削除
				if (strGenre.GetLength() == 0) { // 空になってしまったらジャンルコードから変換を試みる
					BOOL m_bScmpxGenre = TRUE; // m_bScmpxGenreが削除されたのでここで定義
					CId3tagv1 id3tagv1(m_bScmpxGenre);
					strGenreFromCode.Delete(index);
					strGenreFromCode.Delete(0);
					strGenreFromCode = id3tagv1.GenreNum2String(atoi(strGenreFromCode));
					if (strGenreFromCode.GetLength() > 0) {
						return static_cast<CString>(strGenreFromCode);
					}
				}
				continue;	//Fix 2001-10-24
			}
		}
		break;
	}
	return static_cast<CString>(strGenre);
}

void CId3tagv2::SetGenre(LPCTSTR szGenre)
{
	//ジャンル
	CString strGenre;
/*	CId3tagv1 id3tagv1(m_bScmpxGenre);
	long genre = id3tagv1.GenreString2Num(szGenre);
	if((genre != -1) &&
		!((m_bScmpxGenre && (SCMPX_GENRE_NULL == genre)) || (!m_bScmpxGenre && (WINAMP_GENRE_NULL == genre)))
		)
	{
		strGenre.Format("(%d)",genre);
	}
	2003-01-25 iTunesがジャンルコードへの参照を正しく読めないため、ジャンルコードを入れないように修正
*/
	// 2004-05-16 "("で始まる場合は先頭に"("を追加
	if(lstrlen(szGenre) &&
		!IsDBCSLeadByte(szGenre[0]) &&
		(szGenre[0] == '(') )	//頭にカッコを検出
	{
		strGenre += "(";
	}
	strGenre += szGenre;

	SetId3String("TCON",strGenre);
}

CString CId3tagv2::GetComment()
{
	//コメント
	return GetId3String("COMM");
}

void CId3tagv2::SetComment(LPCTSTR comment)
{
	//コメント
	SetId3String("COMM",comment);
}

CString CId3tagv2::GetWriter()
{
	//作詞
	return GetId3String("TEXT");
}

void CId3tagv2::SetWriter(LPCTSTR writer)
{
	//作詞
	SetId3String("TEXT",writer);
}

CString CId3tagv2::GetComposer()
{
	//作曲
	return GetId3String("TCOM");
}

void CId3tagv2::SetComposer(LPCTSTR composer)
{
	//作曲
	SetId3String("TCOM",composer);
}

CString CId3tagv2::GetAlbmArtist()
{
	//Albm.アーティスト
	return GetId3String("TPE2");
}

void CId3tagv2::SetAlbmArtist(LPCTSTR albmArtist)
{
	//Albm.アーティスト
	SetId3String("TPE2",albmArtist);
}

CString CId3tagv2::GetOrigArtist()
{
	//Orig.アーティスト
	return GetId3String("TOPE");
}

void CId3tagv2::SetOrigArtist(LPCTSTR origArtist)
{
	//Orig.アーティスト
	SetId3String("TOPE",origArtist);
}

CString CId3tagv2::GetCopyright()
{
	//著作権
	return GetId3String("TCOP");
}

void CId3tagv2::SetCopyright(LPCTSTR copyright)
{
	//著作権
	SetId3String("TCOP",copyright);
}

CString CId3tagv2::GetUrl()
{
	//URL
	return GetId3String("WXXX");
}

void CId3tagv2::SetUrl(LPCTSTR url)
{
	//URL
	SetId3String("WXXX",url);
}

CString CId3tagv2::GetEncoder()
{
	//エンコーダー
	return GetId3String("TSSE");
}

void CId3tagv2::SetEncoder(LPCTSTR encoder)
{
	//エンコーダー
	SetId3String("TSSE",encoder);
}

CString CId3tagv2::GetEncodest()
{
	//エンコードした人または組織
	return GetId3String("TENC");
}

void CId3tagv2::SetEncodest(LPCTSTR encoder)
{
	//エンコードした人または組織
	SetId3String("TENC",encoder);
}

CString CId3tagv2::GetEngineer()
{
	//エンジニア（出版）
	return GetId3String("TPUB");
}

void CId3tagv2::SetEngineer(LPCTSTR engineer)
{
	//エンコードした人または組織
	SetId3String("TPUB",engineer);
}

DWORD CId3tagv2::DecodeUnSynchronization(unsigned char *data,DWORD dwSize)
{
	DWORD dwDecodeSize = 0;
	unsigned char *writePtr = data;
	BOOL bHitFF = FALSE;

	for(DWORD i=0; i<dwSize; i++)
	{
		if(data[i] == 0xff)
		{
			bHitFF = TRUE;
		}
		else
		{
			if(bHitFF && (data[i] == 0x00))
			{
				bHitFF = FALSE;
				continue;
			}
			bHitFF = FALSE;
		}
		writePtr[dwDecodeSize] = data[i];
		dwDecodeSize++;
	}
	return dwDecodeSize;
}

DWORD CId3tagv2::EncodeUnSynchronization(unsigned char *srcData,DWORD dwSize,unsigned char *dstData)
{
	DWORD dwDecodeSize = 0;
	unsigned char *writePtr = dstData;
	BOOL bHitFF = FALSE;

	for(DWORD i=0; i<dwSize; i++)
	{
		if(bHitFF && (((srcData[i]&0xe0) == 0xe0) || (srcData[i] == 0x00)) )
		{
			writePtr[dwDecodeSize] = 0x00;
			dwDecodeSize++;
		}
		if(srcData[i] == 0xff)
		{
			bHitFF = TRUE;
		}
		else
		{
			bHitFF = FALSE;
		}
		writePtr[dwDecodeSize] = srcData[i];
		dwDecodeSize++;
	}
	return dwDecodeSize;
}

DWORD CId3tagv2::ExtractV2Size(const unsigned char size[4])
{
	return (((DWORD )(size[0]&0x7f)<<21) | ((DWORD )(size[1]&0x7f)<<14) | ((DWORD )(size[2]&0x7f)<<7) | (DWORD )(size[3]&0x7f));
}

void CId3tagv2::MakeV2Size(DWORD dwSize,unsigned char size[4])
{
	size[3] = dwSize & 0x7f;
	size[2] = (dwSize>>7) & 0x7f;
	size[1] = (dwSize>>14) & 0x7f;
	size[0] = (dwSize>>21) & 0x7f;
}

DWORD CId3tagv2::Load(LPCTSTR szFileName)
{
	DWORD	dwWin32errorCode = ERROR_SUCCESS;
	Release();
	HANDLE hFile = CreateFile(
				szFileName,
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,	//ファイルをオープンします。指定ファイルが存在していない場合、関数は失敗します。
				FILE_ATTRIBUTE_NORMAL,
				NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		dwWin32errorCode = GetLastError();
		return dwWin32errorCode;
	}

	//ファイルをオープン
	ID3HEAD head;
	memcpy(&head,&m_head,sizeof(m_head));
	//ID3V2ヘッダを読み込む
	DWORD dwRet;
	if(!ReadFile(hFile,&head,sizeof(head),&dwRet,NULL) || (dwRet != sizeof(head)))
	{
		CloseHandle(hFile);
		return -1;
	}

	//ID3v2を確認
	if(memcmp(head.id3des,"ID3",3) != 0)
	{
		CloseHandle(hFile);
		return -1;
	}
	
	//バージョン
	WORD ver = (head.ver[0]<<8) | head.ver[1];
	if((ver != 0x0200) &&
		(ver != 0x0300) &&
		(ver != 0x0301) &&
		(ver != 0x0400) )
	{
		// ID3v2.4より大きいバージョンはサポートしない
		CloseHandle(hFile);
		return -1;
	}
	m_wVer = ver;
	if(m_wVer == 0x0301)
	{
		m_wVer = 0x0400;
	}

	//Id3tagサイズ
	DWORD dwId3Size = ExtractV2Size(head.size);

	//全フレームの読込
	unsigned char *buf = (unsigned char *)malloc(dwId3Size);
	if(!buf)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		return dwWin32errorCode;
	}
	if(!ReadFile(hFile,buf,dwId3Size,&dwRet,NULL) || (dwRet != dwId3Size))
	{
		free(buf);
		CloseHandle(hFile);
		return -1;
	}

	//非同期化の解除
	if(head.flag & 0x80)
	{
		dwId3Size = DecodeUnSynchronization(buf,dwId3Size);
		m_bUnSynchronization = TRUE;
	}
	else
	{
		m_bUnSynchronization = FALSE;
	}

	DWORD dwRemainSize = dwId3Size;
	//拡張ヘッダを読み飛ばす(ID3v2.2には存在しない)
	if((ver != 0x0200) && (head.flag & 0x40))
	{
		dwRemainSize -= ExtractV2Size(buf) + 4;
	}
	head.flag &= ~0x40;	//解除

	m_encode = ID3V2CHARENCODE_ISO_8859_1;
	while(dwRemainSize)
	{
		CId3Frame frame;
		DWORD dwReadSize;
		if(m_wVer < 0x0300)
		{
			dwReadSize = frame.LoadFrame2_2(buf+(dwId3Size-dwRemainSize),dwRemainSize);
		}
		else if(m_wVer < 0x0400)
		{
			dwReadSize = frame.LoadFrame2_3(buf+(dwId3Size-dwRemainSize),dwRemainSize);
		}
		else
		{
			dwReadSize = frame.LoadFrame2_4(buf+(dwId3Size-dwRemainSize),dwRemainSize);
		}
		if(!dwReadSize)
			break;
		unsigned char *data = frame.GetData();
		if(frame.GetSize() && data)
		{
			switch(data[0]){
			case 0:
			default:	// ISO-8859-1
				break;
			case 1:		// UTF-16
				if(data[0] > ID3V2CHARENCODE_ISO_8859_1)
				{
					m_encode = ID3V2CHARENCODE_UTF_16;
				}
				break;
			case 2:		// UTF-16BE
				if(data[0] > ID3V2CHARENCODE_UTF_16)
				{
					m_encode = ID3V2CHARENCODE_UTF_16BE;
				}
				break;
			case 3:		// UTF-8
				if(data[0] > ID3V2CHARENCODE_UTF_16BE)
				{
					m_encode = ID3V2CHARENCODE_UTF_8;
				}
				break;
			}
		}
		m_frames.insert(pair<DWORD,CId3Frame>(frame.GetId(),frame));
		dwRemainSize -= dwReadSize;
	}

	free(buf);
	CloseHandle(hFile);

//	if(m_wVer < 0x0300)	//v2.2はv2.3へ変換して保存する
//	{
//		head.ver[0] = 0x03;
//		head.ver[1] = 0x00;
//		m_wVer = 0x0300;
//	}

	memcpy(&m_head,&head,sizeof(m_head));
	m_bEnable = TRUE;

	return dwWin32errorCode;
}

DWORD CId3tagv2::Save(LPCTSTR szFileName)
{
	DWORD	dwWin32errorCode = ERROR_SUCCESS;
	HANDLE hFile;
	BOOL bInsPadding = FALSE;
	DWORD	dwWritten = 0;
	TCHAR szTempPath[MAX_PATH];
	TCHAR szTempFile[MAX_PATH];
	
	if(!m_bEnable)
	{
		return -1;
	}

	// エンコード指定$2/$3が使えるのはv2.4以降
	if(m_wVer < 0x0400)
	{
		if(	(m_encode != ID3V2CHARENCODE_ISO_8859_1) &&
			(m_encode != ID3V2CHARENCODE_UTF_16) )
		{
			// ISO-8859-1に自動設定
			m_encode = ID3V2CHARENCODE_ISO_8859_1;
		}
	}

	DWORD dwTotalFrameSize = GetTotalFrameSize();
	//フレーム情報を書き出す準備
	unsigned char *framedata = (unsigned char *)malloc(dwTotalFrameSize);
	if(!framedata)
	{
		dwWin32errorCode = GetLastError();
		return dwWin32errorCode;
	}
	DWORD dwFrameDataPtr = 0;
	multimap<DWORD,CId3Frame>::iterator p;
	p = m_frames.begin();
	while(p != m_frames.end())
	{
		CId3Frame *pFrame = &p->second;
		DWORD dwSize = pFrame->GetSize();
		DWORD id = pFrame->GetId();
		//サイズが0のときはフレームを書き込まない
		if(p->second.GetSize() < 2)
		{
			p++;
			continue;
		}
		WORD flags = pFrame->GetFlags();
		WORD flagsBe = ((flags<<8)|(flags>>8));
		unsigned char *data = pFrame->GetData();
		if(m_wVer == 0x0200)
		{
			unsigned char size[3];
			size[2] = dwSize & 0xff;
			size[1] = (dwSize>>8) & 0xff;
			size[0] = (dwSize>>16) & 0xff;
			// id3v2.2
			char v22id[3];
			//v2.3からv2.2へフレームIDを変換
			v23IDtov22ID((char *)&id,v22id);
			memcpy(&framedata[dwFrameDataPtr],&v22id,sizeof(v22id));
			dwFrameDataPtr += sizeof(v22id);
			memcpy(&framedata[dwFrameDataPtr],size,sizeof(size));
			dwFrameDataPtr += sizeof(size);
			memcpy(&framedata[dwFrameDataPtr],data,dwSize);
			dwFrameDataPtr += dwSize;
		}
		else if(m_wVer == 0x0300)
		{
			unsigned char size[4];
			size[3] = dwSize & 0xff;
			size[2] = (dwSize>>8) & 0xff;
			size[1] = (dwSize>>16) & 0xff;
			size[0] = (dwSize>>24) & 0xff;
			// id3v2.3-
			memcpy(&framedata[dwFrameDataPtr],&id,sizeof(id));
			dwFrameDataPtr += sizeof(id);
			memcpy(&framedata[dwFrameDataPtr],size,sizeof(size));
			dwFrameDataPtr += sizeof(size);
			memcpy(&framedata[dwFrameDataPtr],&flagsBe,sizeof(flagsBe));
			dwFrameDataPtr += sizeof(flagsBe);
			memcpy(&framedata[dwFrameDataPtr],data,dwSize);
			dwFrameDataPtr += dwSize;
		}
		else
		{
			unsigned char size[4];
			MakeV2Size(dwSize,size);
			// 長さ制限
			dwSize &= 0x0fffffff;
			// id3v2.4
			memcpy(&framedata[dwFrameDataPtr],&id,sizeof(id));
			dwFrameDataPtr += sizeof(id);
			memcpy(&framedata[dwFrameDataPtr],size,sizeof(size));
			dwFrameDataPtr += sizeof(size);
			memcpy(&framedata[dwFrameDataPtr],&flagsBe,sizeof(flagsBe));
			dwFrameDataPtr += sizeof(flagsBe);
			memcpy(&framedata[dwFrameDataPtr],data,dwSize);
			dwFrameDataPtr += dwSize;
		}
		p++;
	}
//	ASSERT(dwFrameDataPtr == dwTotalFrameSize);
	//非同期化
	if(m_bUnSynchronization && !bOptNotUnSyncAlways /* STEP 003,006 */)
	{
		unsigned char *encData = (unsigned char *)malloc(dwTotalFrameSize*2);
		DWORD dwEncodeSize = EncodeUnSynchronization(framedata,dwTotalFrameSize,encData);
//		if(dwEncodeSize != dwTotalFrameSize)
//	2004-03-20 プロパティに非同期化スイッチをつけたため、非同期化フラグは必ず立てることにした
//	(非同期化ONが保存できない様子はユーザからみておかしな動きに見えるため)
/* STEP では非同期化は必要なときのみ行うので、サイズでフラグのON/OFFを決定する */
		if(dwEncodeSize != dwTotalFrameSize)
		{
			//非同期化フラグをセット
			m_head.flag |= 0x80;
			dwTotalFrameSize = dwEncodeSize;
		}
		free(framedata);
		framedata = encData;
	}
	else
	{
		//非同期化フラグを解除
		m_head.flag &= ~0x80;
	}

	//Id3tagサイズ
	DWORD dwId3Size = ExtractV2Size(m_head.size);

	if(dwTotalFrameSize > dwId3Size)
	{
		//[パディング領域を挿入]
		DWORD dwPaddingSize = dwTotalFrameSize - dwId3Size + ID3V2_PADDING_SIZE;
		dwId3Size += dwPaddingSize;

		//==================元ファイルをメモリに保存==================
		hFile = CreateFile(
					szFileName,
					GENERIC_READ,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,	//指定ファイルが存在していない場合、関数は失敗します。
					FILE_ATTRIBUTE_NORMAL,
					NULL);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			dwWin32errorCode = GetLastError();
			free(framedata);
			return dwWin32errorCode;
		}

		DWORD dwDataSize = GetFileSize(hFile,NULL);
		//バッファメモリの確保
		char *pRawData = (char *)malloc(dwDataSize);
		if(!pRawData)
		{
			dwWin32errorCode = GetLastError();
			CloseHandle(hFile);
			free(framedata);
			return dwWin32errorCode;
		}
		//raw dataの読み出し
		if(!ReadFile(hFile,pRawData,dwDataSize,&dwWritten,NULL))
		{
			dwWin32errorCode = GetLastError();
			CloseHandle(hFile);
			free(pRawData);
			free(framedata);
			return dwWin32errorCode;
		}
		CloseHandle(hFile);

		//==================テンポラリを作成==================
		//テンポラリ名を取得
		lstrcpy(szTempPath,szFileName);
		cutFileName(szTempPath);
		if(!GetTempFileName(szTempPath, TEXT("tms"), 0, szTempFile))
		{
			dwWin32errorCode = GetLastError();
			free(pRawData);
			DeleteFile(szTempFile);
			free(framedata);
			return dwWin32errorCode;
		}
		//ファイルオープン(指定ファイルがすでに存在している場合、そのファイルは上書きされます。)
		hFile = CreateFile(szTempFile,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			dwWin32errorCode = GetLastError();
			free(pRawData);
			DeleteFile(szTempFile);
			free(framedata);
			return dwWin32errorCode;
		}
		//おまじない
		if(SetFilePointer(hFile,0,NULL,FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		{
			dwWin32errorCode = GetLastError();
			CloseHandle(hFile);
			free(pRawData);
			DeleteFile(szTempFile);
			free(framedata);
			return dwWin32errorCode;
		}
		//パディング領域をスキップ
		if(SetFilePointer(hFile,dwPaddingSize,NULL,FILE_BEGIN) == INVALID_SET_FILE_POINTER)
		{
			dwWin32errorCode = GetLastError();
			CloseHandle(hFile);
			free(pRawData);
			DeleteFile(szTempFile);
			free(framedata);
			return dwWin32errorCode;
		}
		//移動先にコピー
		if(WriteFile(hFile,pRawData,dwDataSize,&dwWritten,NULL) == 0)
		{
			dwWin32errorCode = GetLastError();
			CloseHandle(hFile);
			free(pRawData);
			DeleteFile(szTempFile);
			free(framedata);
			return dwWin32errorCode;
		}

		free(pRawData);

		bInsPadding = TRUE;
	}
	else
	{
		//ファイルをオープン
		hFile = CreateFile(
					szFileName,
					GENERIC_READ|GENERIC_WRITE,
					FILE_SHARE_READ,
					NULL,
					OPEN_EXISTING,	//ファイルをオープンします。指定ファイルが存在していない場合、関数は失敗します。
					FILE_ATTRIBUTE_NORMAL,
					NULL);
		if(hFile == INVALID_HANDLE_VALUE)
		{
			dwWin32errorCode = GetLastError();
			free(framedata);
			return dwWin32errorCode;
		}
	}

	//ID3v2ヘッダを書き込む
	if(SetFilePointer(hFile,0,NULL,FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		DeleteFile(szTempFile);
		free(framedata);
		return dwWin32errorCode;
	}
	ID3HEAD head;
	m_head.ver[0] = (m_wVer & 0xff00) >> 8;
	m_head.ver[1] = (m_wVer & 0x00ff);
	memcpy(&head,&m_head,sizeof(m_head));
	MakeV2Size(dwId3Size,head.size);
	// 長さ制限
	dwId3Size &= 0x0fffffff;
	if(WriteFile(hFile,&head,sizeof(head),&dwWritten,NULL) == 0)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		DeleteFile(szTempFile);
		free(framedata);
		return dwWin32errorCode;
	}
	memcpy(&m_head,&head,sizeof(head));

	//フレーム情報をファイルに書き出す
	if(SetFilePointer(hFile,10,NULL,FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		DeleteFile(szTempFile);
		free(framedata);
		return dwWin32errorCode;
	}
	if(!WriteFile(hFile,framedata,dwTotalFrameSize,&dwWritten,NULL))
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		if(bInsPadding)
		{
			DeleteFile(szTempFile);
		}
		free(framedata);
		return dwWin32errorCode;
	}
	free(framedata);

	//パディング領域を0でパディング
	int i; for(i=0; i<(dwId3Size - dwTotalFrameSize); i++)
	{
		DWORD dwRet;
		unsigned char pad = 0x00;
		if(!WriteFile(hFile,&pad,1,&dwRet,NULL))
		{
			dwWin32errorCode = GetLastError();
			CloseHandle(hFile);
			if(bInsPadding)
			{
				DeleteFile(szTempFile);
			}
			return dwWin32errorCode;
		}
	}

	CloseHandle(hFile);

	if(bInsPadding)
	{
		//オリジナルファイルを退避(リネーム)
		TCHAR szPreFile[MAX_PATH];
		if(!GetTempFileName(szTempPath, TEXT("tms"), 0, szPreFile))
		{
			dwWin32errorCode = GetLastError();
			DeleteFile(szTempFile);
			return dwWin32errorCode;
		}
		DeleteFile(szPreFile);//手抜き(^^;
		if(!MoveFile(szFileName,szPreFile))
		{
			dwWin32errorCode = GetLastError();
			DeleteFile(szTempFile);
			return dwWin32errorCode;
		}

		//完成品をリネーム
		if(!MoveFile(szTempFile,szFileName))
		{
			dwWin32errorCode = GetLastError();
			MoveFile(szPreFile,szFileName);
			DeleteFile(szTempFile);
			return dwWin32errorCode;
		}

		//オリジナルを削除
		DeleteFile(szPreFile);
	}

	return dwWin32errorCode;
}

DWORD CId3tagv2::DelTag(LPCTSTR szFileName)
{
	DWORD	dwWin32errorCode = ERROR_SUCCESS;
	DWORD	dwWritten = 0;
	TCHAR szTempPath[MAX_PATH];
	TCHAR szTempFile[MAX_PATH];

	if(!m_bEnable)
	{
		return -1;
	}

	//==================元ファイルをメモリに保存==================
	HANDLE hFile = CreateFile(
							szFileName,
							GENERIC_READ,
							FILE_SHARE_READ,
							NULL,
							OPEN_EXISTING,	//指定ファイルが存在していない場合、関数は失敗します。
							FILE_ATTRIBUTE_NORMAL,
							NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		dwWin32errorCode = GetLastError();
		return dwWin32errorCode;
	}

	DWORD dwDataSize = GetFileSize(hFile,NULL);
	DWORD dwId3v2Size = ExtractV2Size(m_head.size) + 10;
	if(dwDataSize < dwId3v2Size)
	{
		return -1;
	}
	dwDataSize -= dwId3v2Size;
	SetFilePointer(hFile,dwId3v2Size,NULL,FILE_BEGIN);

	//バッファメモリの確保
	char *pRawData = (char *)malloc(dwDataSize);
	if(!pRawData)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		return dwWin32errorCode;
	}
	//raw dataの読み出し
	if(!ReadFile(hFile,pRawData,dwDataSize,&dwWritten,NULL))
	{
		dwWin32errorCode = GetLastError();
		free(pRawData);
		CloseHandle(hFile);
		return dwWin32errorCode;
	}
	CloseHandle(hFile);

	//==================テンポラリを作成==================
	//テンポラリ名を取得
	lstrcpy(szTempPath,szFileName);
	cutFileName(szTempPath);
	if(!GetTempFileName(szTempPath, TEXT("tms"), 0, szTempFile))
	{
		dwWin32errorCode = GetLastError();
		free(pRawData);
		DeleteFile(szTempFile);
		return dwWin32errorCode;
	}
	//ファイルオープン(指定ファイルがすでに存在している場合、そのファイルは上書きされます。)
	hFile = CreateFile(szTempFile,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		dwWin32errorCode = GetLastError();
		free(pRawData);
		DeleteFile(szTempFile);
		return dwWin32errorCode;
	}
	//おまじない
	if(SetFilePointer(hFile,0,NULL,FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		free(pRawData);
		DeleteFile(szTempFile);
		return dwWin32errorCode;
	}
	//移動先にコピー
	if(WriteFile(hFile,pRawData,dwDataSize,&dwWritten,NULL) == 0)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		free(pRawData);
		return dwWin32errorCode;
	}
	free(pRawData);
	CloseHandle(hFile);

	//オリジナルファイルを退避(リネーム)
	TCHAR szPreFile[MAX_PATH];
	if(!GetTempFileName(szTempPath, TEXT("tms"), 0, szPreFile))
	{
		dwWin32errorCode = GetLastError();
		DeleteFile(szTempFile);
		return dwWin32errorCode;
	}
	DeleteFile(szPreFile);//手抜き(^^;
	if(!MoveFile(szFileName,szPreFile))
	{
		dwWin32errorCode = GetLastError();
		DeleteFile(szTempFile);
		return dwWin32errorCode;
	}

	//完成品をリネーム
	if(!MoveFile(szTempFile,szFileName))
	{
		dwWin32errorCode = GetLastError();
		MoveFile(szPreFile,szFileName);
		DeleteFile(szTempFile);
		return dwWin32errorCode;
	}

	//オリジナルを削除
	DeleteFile(szPreFile);
	
	Release();
	
	return dwWin32errorCode;
}

DWORD CId3tagv2::MakeTag(LPCTSTR szFileName)
{
	DWORD	dwWin32errorCode = ERROR_SUCCESS;
	if(m_bEnable)
	{
		return -1;
	}

	//デフォルト情報
	TCHAR szDefaultName[MAX_PATH];
	lstrcpy(szDefaultName, getFileName(CString(szFileName)));
	SetId3String("TIT2",szDefaultName);
	SetId3String("TSSE",m_strDefaultEnc);
	
	DWORD dwTotalFrameSize = GetTotalFrameSize();
	//[パディング領域を挿入]
	//DWORD dwPaddingSize = ID3V2_PADDING_SIZE;
	DWORD dwPaddingSize = ID3V2_PADDING_SIZE - dwTotalFrameSize;

	//フレーム情報を書き出す準備
	unsigned char *framedata = (unsigned char *)malloc(dwTotalFrameSize);
	if(!framedata)
	{
		dwWin32errorCode = GetLastError();
		return dwWin32errorCode;
	}
	DWORD dwFrameDataPtr = 0;
	multimap<DWORD,CId3Frame>::iterator p;
	p = m_frames.begin();
	while(p != m_frames.end())
	{
		CId3Frame *pFrame = &p->second;
		DWORD dwSize = pFrame->GetSize();
		DWORD id = pFrame->GetId();
		WORD flags = pFrame->GetFlags();
		WORD flagsBe = ((flags<<8)|(flags>>8));
		unsigned char *data = pFrame->GetData();
		if(m_wVer == 0x0200)
		{
			unsigned char size[3];
			size[2] = dwSize & 0xff;
			size[1] = (dwSize>>8) & 0xff;
			size[0] = (dwSize>>16) & 0xff;
			// id3v2.2
			char v22id[3];
			//v2.3からv2.2へフレームIDを変換
			v23IDtov22ID((char *)&id,v22id);
			memcpy(&framedata[dwFrameDataPtr],&v22id,sizeof(v22id));
			dwFrameDataPtr += sizeof(v22id);
			memcpy(&framedata[dwFrameDataPtr],size,sizeof(size));
			dwFrameDataPtr += sizeof(size);
			memcpy(&framedata[dwFrameDataPtr],data,dwSize);
			dwFrameDataPtr += dwSize;
		}
		else
		{
			// ID3v2.3-
			unsigned char size[4];
			size[3] = dwSize & 0xff;
			size[2] = (dwSize>>8) & 0xff;
			size[1] = (dwSize>>16) & 0xff;
			size[0] = (dwSize>>24) & 0xff;
			memcpy(&framedata[dwFrameDataPtr],&id,sizeof(id));
			dwFrameDataPtr += sizeof(id);
			memcpy(&framedata[dwFrameDataPtr],size,sizeof(size));
			dwFrameDataPtr += sizeof(size);
			memcpy(&framedata[dwFrameDataPtr],&flagsBe,sizeof(flagsBe));
			dwFrameDataPtr += sizeof(flagsBe);
			memcpy(&framedata[dwFrameDataPtr],data,dwSize);
			dwFrameDataPtr += dwSize;
		}
		p++;
	}
	ASSERT(dwFrameDataPtr == dwTotalFrameSize);
	//非同期化
//	if(m_bUnSynchronization) /* STEP 006 */
	if(!bOptUnSyncNew /* STEP 006 */)
	{
		unsigned char *encData = (unsigned char *)malloc(dwTotalFrameSize*2);
		DWORD dwEncodeSize = EncodeUnSynchronization(framedata,dwTotalFrameSize,encData);
//		if(dwEncodeSize != dwTotalFrameSize)
//	2004-09-24 プロパティに非同期化スイッチをつけたため、非同期化フラグは必ず立てることにした
//	(非同期化ONが保存できない様子はユーザからみておかしな動きに見えるため)
/* STEP では非同期化は必要なときのみ行うので、サイズでフラグのON/OFFを決定する */
		if(dwEncodeSize != dwTotalFrameSize)
		{
			//非同期化フラグをセット
			m_head.flag |= 0x80;
			dwTotalFrameSize = dwEncodeSize;
		}
		free(framedata);
		framedata = encData;
	}

	//==================元ファイルをメモリに保存==================
	HANDLE hFile = CreateFile(
				szFileName,
				GENERIC_READ,
				FILE_SHARE_READ,
				NULL,
				OPEN_EXISTING,	//指定ファイルが存在していない場合、関数は失敗します。
				FILE_ATTRIBUTE_NORMAL,
				NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		dwWin32errorCode = GetLastError();
		free(framedata);
		return dwWin32errorCode;
	}

	DWORD dwDataSize = GetFileSize(hFile,NULL);
	//バッファメモリの確保
	char *pRawData = (char *)malloc(dwDataSize);
	if(!pRawData)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		free(framedata);
		return dwWin32errorCode;
	}
	//raw dataの読み出し
	DWORD dwWritten;
	if(!ReadFile(hFile,pRawData,dwDataSize,&dwWritten,NULL))
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		free(pRawData);
		free(framedata);
		return dwWin32errorCode;
	}
	CloseHandle(hFile);

	//==================テンポラリを作成==================
	//テンポラリ名を取得
	TCHAR szTempPath[MAX_PATH];
	TCHAR szTempFile[MAX_PATH];
	lstrcpy(szTempPath, szFileName);
	cutFileName(szTempPath);
	if(!GetTempFileName(szTempPath, TEXT("tms"), 0, szTempFile))
	{
		dwWin32errorCode = GetLastError();
		free(pRawData);
		DeleteFile(szTempFile);
		free(framedata);
		return dwWin32errorCode;
	}
	//ファイルオープン(指定ファイルがすでに存在している場合、そのファイルは上書きされます。)
	hFile = CreateFile(szTempFile,GENERIC_WRITE|GENERIC_READ,FILE_SHARE_READ,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile == INVALID_HANDLE_VALUE)
	{
		dwWin32errorCode = GetLastError();
		free(pRawData);
		DeleteFile(szTempFile);
		free(framedata);
		return dwWin32errorCode;
	}
	//おまじない
	if(SetFilePointer(hFile,0,NULL,FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		free(pRawData);
		DeleteFile(szTempFile);
		free(framedata);
		return dwWin32errorCode;
	}
	//パディング領域をスキップ
	if(SetFilePointer(hFile,dwPaddingSize + dwTotalFrameSize + 10,NULL,FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		free(pRawData);
		DeleteFile(szTempFile);
		free(framedata);
		return dwWin32errorCode;
	}
	//移動先にコピー
	if(WriteFile(hFile,pRawData,dwDataSize,&dwWritten,NULL) == 0)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		free(pRawData);
		free(framedata);
		return dwWin32errorCode;
	}
	free(pRawData);

	if(SetFilePointer(hFile,0,NULL,FILE_BEGIN) == INVALID_SET_FILE_POINTER)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		DeleteFile(szTempFile);
		free(framedata);
		return dwWin32errorCode;
	}
	ID3HEAD head;
	m_head.ver[0] = (m_wVer & 0xff00) >> 8;
	m_head.ver[1] = (m_wVer & 0x00ff);
	memcpy(&head,&m_head,sizeof(m_head));
	MakeV2Size(dwPaddingSize+dwTotalFrameSize,head.size);
	if(WriteFile(hFile,&head,sizeof(head),&dwWritten,NULL) == 0)
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		DeleteFile(szTempFile);
		free(framedata);
		return dwWin32errorCode;
	}
	memcpy(&m_head,&head,sizeof(head));

	//フレーム情報をファイルに書き出す
	if(!WriteFile(hFile,framedata,dwTotalFrameSize,&dwWritten,NULL))
	{
		dwWin32errorCode = GetLastError();
		CloseHandle(hFile);
		DeleteFile(szTempFile);
		free(framedata);
		return dwWin32errorCode;
	}
	free(framedata);

	//パディング領域を0でパディング
	int i; for(i=0; i<dwPaddingSize; i++)
	{
		DWORD dwRet;
		unsigned char pad = 0x00;
		if(!WriteFile(hFile,&pad,1,&dwRet,NULL))
		{
			dwWin32errorCode = GetLastError();
			CloseHandle(hFile);
			DeleteFile(szTempFile);
			return dwWin32errorCode;
		}
	}
	CloseHandle(hFile);

	//オリジナルファイルを退避(リネーム)
	TCHAR szPreFile[MAX_PATH];
	if(!GetTempFileName(szTempPath, TEXT("tms"), 0, szPreFile))
	{
		dwWin32errorCode = GetLastError();
		DeleteFile(szTempFile);
		return dwWin32errorCode;
	}
	DeleteFile(szPreFile);//手抜き(^^;
	if(!MoveFile(szFileName,szPreFile))
	{
		dwWin32errorCode = GetLastError();
		DeleteFile(szTempFile);
		return dwWin32errorCode;
	}

	//完成品をリネーム
	if(!MoveFile(szTempFile,szFileName))
	{
		dwWin32errorCode = GetLastError();
		MoveFile(szPreFile,szFileName);
		DeleteFile(szTempFile);
		return dwWin32errorCode;
	}

	//オリジナルを削除
	DeleteFile(szPreFile);

	m_bEnable = TRUE;

	return dwWin32errorCode;
}
