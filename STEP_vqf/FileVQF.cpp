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
	//�^�C�g��
	data = vqf.GetField('N','A','M','E',&dwSize);
	if(data){
        SetTrackNameSI(pFileMP3, static_cast<CString>((const char*)data));
    }
	//�A�[�e�B�X�g
	data = vqf.GetField('A','U','T','H',&dwSize);
	if(data){
		SetArtistNameSI(pFileMP3, static_cast<CString>((const char*)data));
	}
	//�ۑ���
	//data = vqf.GetField('F','I','L','E',&dwSize);
	//if(data){
	//}
	//���쌠
	data = vqf.GetField('(','c',')',' ',&dwSize);
	if(data){
		SetCopyrightSI(pFileMP3, static_cast<CString>((const char*)data));
	}
    //�R�����g
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
    //�^�C�g��
	strTmp = GetTrackNameSI(pFileMP3);
	vqf.SetField('N','A','M','E',
                (const unsigned char *)(LPCSTR)strTmp,
                strlen(strTmp));
    //�A�[�e�B�X�g
	strTmp = GetArtistNameSI(pFileMP3);
	vqf.SetField('A','U','T','H',
                (const unsigned char *)(LPCSTR)strTmp,
                strlen(strTmp));
    //�ۑ���
	//vqf.SetField('F','I','L','E',
    //              ???,
    //              ???);
    //���쌠
	strTmp = GetCopyrightSI(pFileMP3);
	vqf.SetField('(','c',')',' ',
		        (const unsigned char *)(LPCSTR)strTmp,
                  strlen(strTmp));
    //�R�����g
	strTmp = GetCommentSI(pFileMP3);
	vqf.SetField('C','O','M','T',
                (const unsigned char *)(LPCSTR)strTmp,
                strlen(strTmp));
    return vqf.Save(NULL, GetFullPath(pFileMP3)) == ERROR_SUCCESS;
}
