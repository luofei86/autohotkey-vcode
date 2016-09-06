#include "stdafx.h"
#include <iostream>
#include <stdio.h>
#include "cdef.h"
//#include "windows.h"
#include "HttpClient.h"
#include <Wincrypt.h>

//��ȡ�ļ���CRC����ֵ
unsigned long Crc32Table[256];
int Get_CRC(CString &csData, DWORD dwSize)//��ȡcrc32ֵ
{
	ULONG  crc(0xffffffff);
	int len;
	unsigned char* buffer;
	len = dwSize;
	buffer = (unsigned char*)(LPCTSTR)csData;
	while(len--)
		crc = (crc >> 8) ^ Crc32Table[(crc & 0xFF) ^ *buffer++];
	return crc^0xffffffff;
}
void MakeTable()//��̬����CRC32�����
{
	int i,j;
	unsigned long crc;
	for (i = 0; i < 256; i++)
	{
		crc = i;
		for (j = 0; j < 8; j++)
		{
			if (crc & 1)
				crc = (crc >> 1) ^ 0xEDB88320;
			else
				crc >>= 1;
		}
		Crc32Table[i] = crc;
	}
}
CString GetFileCrc(const CString& strFileName) 
{
	HANDLE hFile = {NULL};
	DWORD dwSize, bytes_read;
    
	MakeTable();//�������

	hFile = CreateFile(strFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	dwSize = GetFileSize(hFile, NULL);

	CString csData(' ', dwSize);
	ReadFile(hFile, csData.GetBuffer(dwSize), dwSize, &bytes_read, NULL);
	CloseHandle(hFile);
	csData.ReleaseBuffer();

	int nCRC = Get_CRC(csData, dwSize);
	char ch[20] = {0};
	itoa(nCRC, ch, 16); 
	USES_CONVERSION;
	CString strFileCrcValue = A2W(ch);
	//����8λ��ǰ�油0
	int len = strFileCrcValue.GetLength();
	if(len < 8)
	{
		int miss = 8 - len;
		for(int i = 0; i < miss; ++i)
		{
			strFileCrcValue = CString(L"0") + strFileCrcValue;
		}
	}

	return strFileCrcValue.MakeUpper();
}

BOOL GetBytesMD5(const BYTE* lpBuffer, DWORD lpNumberOfBytes, CString& strMD5)
{
	HCRYPTPROV hProv=NULL;
	if(CryptAcquireContext(&hProv,NULL,NULL,PROV_RSA_FULL,CRYPT_VERIFYCONTEXT)==FALSE)       //���CSP��һ����Կ�����ľ��
	{
		return FALSE;
	}
	HCRYPTPROV hHash=NULL;
	//��ʼ������������hash������������һ����CSP��hash������صľ��������������������    CryptHashData���á�
	if(CryptCreateHash(hProv,CALG_MD5,0,0,&hHash)==FALSE)
	{
		return FALSE;
	}
	if(CryptHashData(hHash,lpBuffer,lpNumberOfBytes,0)==FALSE)      //hash�ļ�  
	{
		return FALSE;
	}
	BYTE *pbHash;
	DWORD dwHashLen=sizeof(DWORD);
	if (!CryptGetHashParam(hHash,HP_HASHVAL,NULL,&dwHashLen,0)) //��Ҳ��֪��ΪʲôҪ����������CryptGetHashParam������ǲ��յ�msdn       
	{
 		return FALSE;
	}
	pbHash=(byte*)malloc(dwHashLen);
	if(CryptGetHashParam(hHash,HP_HASHVAL,pbHash,&dwHashLen,0))//���md5ֵ 
	{
		for(DWORD i=0;i<dwHashLen;i++)         //���md5ֵ 
		{
   			TCHAR str[3]={0};
  			//CString strFilePartM=_T("");
   			_stprintf(str,_T("%02x"),pbHash[i]);
			strMD5 += str;
		}
	} 
 
	//�ƺ���
	if(CryptDestroyHash(hHash)==FALSE)          //����hash����  
	{
		return FALSE;
	}
	if(CryptReleaseContext(hProv,0)==FALSE)
	{
		return FALSE;
	}
	return TRUE;
}

BOOL GetFileMd5(CString strDllPath, CString& strFileMd5)  
{  
	HANDLE hFile=CreateFile(strDllPath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,NULL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)                                        //���CreateFile����ʧ��  
	{   
		//��ʾCreateFile����ʧ�ܣ����������š�visual studio�п��ڡ����ߡ�>��������ҡ������ô���ŵõ�������Ϣ��  
		CloseHandle(hFile);
		return FALSE;
	}

	DWORD dwFileSize=GetFileSize(hFile,0);    //��ȡ�ļ��Ĵ�С
	if (dwFileSize==0xFFFFFFFF)               //�����ȡ�ļ���Сʧ��  
	{
		return FALSE;
	}
	byte* lpReadFileBuffer=new byte[dwFileSize];
	DWORD lpReadNumberOfBytes;
	if (ReadFile(hFile,lpReadFileBuffer,dwFileSize,&lpReadNumberOfBytes,NULL)==0)        //��ȡ�ļ�  
	{
		return FALSE;
	}

	BOOL bRet = GetBytesMD5(lpReadFileBuffer, lpReadNumberOfBytes, strFileMd5);

	delete[] lpReadFileBuffer;
	CloseHandle(hFile);          //�ر��ļ����
	return bRet;
}  

// MBCS�ַ���ת����UTF-8
CString MBCS2Utf8(const CString& szMBCS)
{
	if (szMBCS.IsEmpty())
		return "";
	// ��������ת����CP_ACP��ת����CP_UTF8
	int nLength = MultiByteToWideChar(CP_ACP, 0, szMBCS, -1, NULL, NULL);   // ��ȡ���������ȣ��ٷ����ڴ�
	WCHAR *tch = new WCHAR[nLength];

	nLength = MultiByteToWideChar(CP_ACP, 0, szMBCS, -1, tch, nLength);     // ��MBCSת����Unicode

	int nUTF8len = WideCharToMultiByte(CP_UTF8, 0, tch, nLength, 0, 0, 0, 0);   // ��ȡUTF-8���볤��
	char *utf8_string = new char[nUTF8len];
	WideCharToMultiByte(CP_UTF8, 0, tch, nLength, utf8_string, nUTF8len, 0, 0); //ת����UTF-8����	

	delete[] tch;
	CString strResult = utf8_string;
	delete[] utf8_string;
	return strResult;
}

BOOL GetStringMD5(CString str, CString& strMD5)
{
	return GetBytesMD5((BYTE*)(LPCTSTR)str, str.GetLength(), strMD5);
}

bool CheckDll(int softid, const CString& strSoftKey, const CString& strDllPath, const CString& strCheckKey)
{
	//��ȡdll�ļ���md5ֵ
	CString strDllMD5;
	GetFileMd5(strDllPath, strDllMD5);
	//��ȡdll�ļ���crcУ��ֵ
	CString strDllCrc = GetFileCrc(strDllPath);
	//��ȡһ��guid
	CString strGuid;
	GUID guid;
	CoCreateGuid(&guid);
	strGuid.AppendFormat(_T("%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x"), 
		guid.Data1,
		guid.Data2,
		guid.Data3,
		guid.Data4[0], guid.Data4[1],
		guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5],
		guid.Data4[6], guid.Data4[7]);
	strGuid = strGuid.MakeUpper();

	CString strSoftID;
	strSoftID.Format("%d", softid);
	//���������id��Ӧ��dllУ��key���ڿ����ߺ�̨-�ҵ����������Բ�ĵ���
	
	CString strYuanShiInfo = strSoftID + strCheckKey + strGuid + strDllMD5.MakeUpper() + strDllCrc.MakeUpper();
	CString strLocalInfo;
	GetStringMD5(strYuanShiInfo, strLocalInfo);

	char serverInfo[50];
	uu_CheckApiSignA(softid, strSoftKey, strGuid, strDllMD5, strDllCrc, serverInfo);
	CString strServerInfo = serverInfo;
	return strServerInfo.Compare(strLocalInfo) ? false : true;
}

//strResult����Ϊ���������ص��ַ�����softidΪ���id��codeIDΪͼƬid��checkKeyΪ�����У��key
CString CheckResult(const CString& strResult, int softid, int codeID, const CString& checkKey)
{
	//���������ص��Ǵ������
	if(_ttol(strResult) < 0)
	{
		return strResult;
	}
	//��ͼƬ�������У�飬��ֹdll���滻
	else
	{
		int iPos = strResult.Find('_');
		//���������������ص�У����
		CString strServerKey = strResult.Left(iPos);
		CString strCodeResult = strResult.Right(strResult.GetLength() - iPos - 1);
		//���ؼ���У����
		CString strCodeID;
		strCodeID.Format("%d", codeID);
		CString strSoftID;
		strSoftID.Format("%d", softid);
		CString strLocalInfo = strSoftID + checkKey + strCodeID + strCodeResult.MakeUpper();
		//��У��֮���Ƚ�MBCS�ַ���ת��ΪUTF-8����
		CString strLocalInfoUtf8 = MBCS2Utf8(strLocalInfo);
		CString strLocalKey;
		GetStringMD5(strLocalInfoUtf8, strLocalKey);
		strLocalKey = strLocalKey.MakeUpper();
		//�����У��ͨ��
		if(strLocalKey.CompareNoCase(strServerKey) == 0)
		{
			return strCodeResult;
		}
		//У�鲻ͨ��
		else
		{
			return "���У�鲻��ȷ";
		}
	}
}

int _tmain(int argc, _TCHAR* argv[])
{
	TCHAR exeFullPath[MAX_PATH]; // MAX_PATH��WINDEF.h�ж����ˣ�����260
	memset(exeFullPath,0,MAX_PATH);
	::GetModuleFileName(NULL,exeFullPath,MAX_PATH);
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	::_tsplitpath_s( exeFullPath, drive, _MAX_DRIVE, dir, _MAX_DIR, NULL, 0, NULL, 0 );

	CString strDllPath(drive);
	strDllPath += dir;
	strDllPath += "UUWiseHelper.dll";

	int softID=2097;
    char *softKEY="b7ee76f547e34516bc30f6eb6c67c7db";
	CString strCheckKey = "32F1C86B-E64C-4EAF-8BC1-C142570008BC";
	if(!CheckDll(softID, softKEY, strDllPath, strCheckKey))
	{
		printf("Dll�ļ�У��ʧ�ܡ�");
		system("pause");
		//return 0;
	}
	else
	{
		printf("Dll�ļ�У��ɹ���");
	}

	char *userName,*userPassword;
    int loginStatus,codeID,score;
	char recoResult[50] = {0};

    userName="UserName";		//�û���
    userPassword="PassWord";	//����

    printf("��ʼ������uu_setSoftInfoA\n");

	uu_setSoftInfoA(softID,softKEY);
    printf("\n���õ�¼����uu_loginA\n");
    loginStatus=uu_loginA(userName,userPassword);

    if(loginStatus>0)
	{
        score=uu_getScoreA(userName,userPassword);
        printf("��ϲ������¼�ɹ��������û�IDΪ��%d,���ʻ���ʣ�����Ϊ��%d\n",loginStatus,score);
        printf("��ʼ����ʶ����,�����ĵȴ����ؽ������\n");

        codeID=uu_recognizeByCodeTypeAndPathA("c:\\1.jpg", 8001, recoResult);
        if(codeID > 0)
		{
			CString strCodeResult = CheckResult(recoResult, softID, codeID, strCheckKey);
            printf("ʶ�����,ͼƬIDΪ��%d,ʶ����Ϊ��%s",codeID,strCodeResult);
        }
		else
		{
            printf("ʶ����ִ���,���صĴ������Ϊ��%d,resutErrorCode:%s\n", codeID, recoResult);
        }
    }
	else
	{
        printf("�Բ��𣬵�¼ʧ�ܣ��������Ϊ��%d\n",loginStatus);
    }

	system("pause");
    return 0;
}

