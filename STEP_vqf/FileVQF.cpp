#include "stdafx.h"
#include "STEPlugin.h"
#include "Vqf.h"

bool LoadAttributeFileVQF(FILE_INFO *pFileMP3);
bool WriteAttributeFileVQF(FILE_INFO *pFileMP3);

bool LoadAttributeFileVQF(FILE_INFO *pFileMP3)
{
    CVqf vqf;
    if(vqf.Load(GetFullPath(pFileMP3)) != ERROR_SUCCESS){
        return false;
    }
	DWORD dwSize;
	unsigned char *data;
	//タイトル
	data = vqf.GetField('N','A','M','E',&dwSize);
	if(data){
        SetTrackNameSI(pFileMP3, static_cast<CString>((const char*)data));
    }
	//アーティスト
	data = vqf.GetField('A','U','T','H',&dwSize);
	if(data){
		SetArtistNameSI(pFileMP3, static_cast<CString>((const char*)data));
	}
	//保存名
	//data = vqf.GetField('F','I','L','E',&dwSize);
	//if(data){
	//}
	//著作権
	data = vqf.GetField('(','c',')',' ',&dwSize);
	if(data){
		SetCopyrightSI(pFileMP3, static_cast<CString>((const char*)data));
	}
    //コメント
	data = vqf.GetField('C','O','M','T',&dwSize);
	if(data){
		SetCommentSI(pFileMP3, static_cast<CString>((const char*)data));
	}

	SetPlayTime(pFileMP3, vqf.GetTime());
	SetAudioFormat(pFileMP3, vqf.GetFormatString());

    return true;
}

bool WriteAttributeFileVQF(FILE_INFO *pFileMP3)
{
    CVqf vqf;
    if(vqf.Load(GetFullPath(pFileMP3)) != ERROR_SUCCESS){
        return false;
    }
    CStringA strTmp;
    //タイトル
	strTmp = GetTrackNameSI(pFileMP3);
	vqf.SetField('N','A','M','E',
                (const unsigned char *)(LPCSTR)strTmp,
                strlen(strTmp));
    //アーティスト
	strTmp = GetArtistNameSI(pFileMP3);
	vqf.SetField('A','U','T','H',
                (const unsigned char *)(LPCSTR)strTmp,
                strlen(strTmp));
    //保存名
	//vqf.SetField('F','I','L','E',
    //              ???,
    //              ???);
    //著作権
	strTmp = GetCopyrightSI(pFileMP3);
	vqf.SetField('(','c',')',' ',
		        (const unsigned char *)(LPCSTR)strTmp,
                  strlen(strTmp));
    //コメント
	strTmp = GetCommentSI(pFileMP3);
	vqf.SetField('C','O','M','T',
                (const unsigned char *)(LPCSTR)strTmp,
                strlen(strTmp));
    return vqf.Save(NULL, GetFullPath(pFileMP3)) == ERROR_SUCCESS;
}
