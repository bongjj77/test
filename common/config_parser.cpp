//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================
#include <string.h>
#include "config_parser.h"

#pragma pack(push, 1)
typedef struct _CFG_ITEM 
{
	char name[512];
	char value[512];
} CFG_ITEM, *PCFG_ITEM;
#pragma pack(pop)

ConfigParser* ConfigParser::_pInstance = 0;
bool ConfigParser::_destoryed = false;

//====================================================================================================
// Constructor
//====================================================================================================
ConfigParser::ConfigParser()
{
	if (!_pInstance)
		_config_list.clear();
}

//====================================================================================================
// Destructor
//====================================================================================================
ConfigParser::~ConfigParser()
{
	if (_pInstance)
		UnloadFile();

	_pInstance = 0;
	_destoryed = true;
}

//====================================================================================================
// GetInstance
//====================================================================================================
ConfigParser& ConfigParser::GetInstance()
{
	if (!_pInstance)
	{
		if (_destoryed)
		{
			CreateInstance();
#ifdef _USE_MEM_EXCEPTION_
			try 
			{
				new (_pInstance) ConfigParser;
			}
			catch (std::exception& e)
			{
				LOG_WRITE(("%s():%d %s", __FUNCTION__, __LINE__, e.what()));
			}
#else
			//new (_pInstance) ConfigParser;
			new ConfigParser;
#endif
			atexit(KillInstance);
			_destoryed = false;
		}
		
		if (!_pInstance)
		{
			CreateInstance();
		}
	}
	return *_pInstance;
}

//====================================================================================================
// CreateInstance
//====================================================================================================
void ConfigParser::CreateInstance()
{
	static ConfigParser _instance;
	_pInstance = &_instance;
}

//====================================================================================================
// KillInstance
//====================================================================================================
void ConfigParser::KillInstance()
{
	//throw std::runtime_error("Dead Reference Detected");
	_pInstance->~ConfigParser();
}


//====================================================================================================
// IsSkipChar
//====================================================================================================
static bool IsSkipChar(char Value)
{
	if( Value=='\t' || Value=='\r' || Value=='\n' || Value==' ' )
	{
		return true;
	}
	return false;
}

//====================================================================================================
// GetTokenStart
//====================================================================================================
static char *  GetTokenStart(char *  pStr)
{
	char * 	pt_ch = nullptr;

	for(pt_ch=pStr; *pt_ch != '\0' && IsSkipChar(*pt_ch); pt_ch++) ;
	if( *pt_ch == '\0' ) { return nullptr; }

	return pt_ch;
}

//====================================================================================================
// GetTokenEnd
//====================================================================================================
static char *  GetTokenEnd(char *  pStr)
{
	char * 	pt_ch = nullptr;

	for(pt_ch=pStr; *pt_ch != '\0' && !IsSkipChar(*pt_ch); pt_ch++) ;

	return pt_ch;
}

//====================================================================================================
// SplitAndParseLine
//====================================================================================================
static bool SplitAndParseLine(char *  pLine /*IN*/, char ** ppName /*OUT*/, char ** ppValue /*OUT*/)
{
	char * 	pt_ch = nullptr;
	char * 	pt_name = nullptr;
	char * 	pt_value = nullptr;

	// 파라미터 체크
	if( !pLine || !ppName || !ppValue ) { return false; }

	// 주석처리
	if( (pt_ch = strchr(pLine, ';')) != nullptr ) { pt_ch = nullptr; }

	// 아이템이름
	pt_name = GetTokenStart(pLine); if( !pt_name ) { return false; }
	pt_ch = GetTokenEnd(pt_name+1); if( !pt_ch || *pt_ch == '\0' ) { return false; }
	*pt_ch = '\0';

	// 아이템값
	pt_value = GetTokenStart(pt_ch+1); if( !pt_value ) { return false; }
	pt_ch = GetTokenEnd(pt_value+1); *pt_ch = '\0';

	// 설정
	*ppName = pt_name;
	*ppValue = pt_value;

	return true;
}

//====================================================================================================
// LoadFile
//====================================================================================================
bool ConfigParser::LoadFile(std::string strFilePath)
{
	FILE	*fp = nullptr;
	char	buf[1024];

	// 파라미터 체크
	if(strFilePath.empty() == true) { return false; }

	// 이전 설정값 리스트 삭제
    if( _config_list.size() ) { UnloadFile(); }

	// 파일 오픈
	fp = fopen(strFilePath.c_str(), "r");
	if( !fp ) { return false; }

	// 파싱!
	while(fgets(buf, sizeof(buf), fp))
	{
		char * 		pt_name = nullptr;
		char * 		pt_value = nullptr;
		PCFG_ITEM	pt_item = nullptr;

		// 이름과 값 얻기
		if( !SplitAndParseLine(buf, &pt_name, &pt_value) ) { continue; }

		// 할당
		pt_item = (PCFG_ITEM)malloc(sizeof(CFG_ITEM));
		strcpy(pt_item->name, pt_name);
		strcpy(pt_item->value, pt_value);
		_config_list.push_back(pt_item);
	}

	// 파일 닫기
	fclose(fp); 
	fp = nullptr;

	_file_path = strFilePath;
	
	return true;
}

//====================================================================================================
// UnloadFile
//====================================================================================================
void ConfigParser::UnloadFile()
{	 
    PCFG_ITEM pt_item = nullptr;
	for(auto item=_config_list.begin(); item!=_config_list.end(); ++item)
	{
        pt_item = (PCFG_ITEM)*item;
        if( pt_item )
        {
		    free(pt_item);
            pt_item = nullptr;
        }
	}
	_config_list.clear();
}

//====================================================================================================
// QueryValue
//====================================================================================================
bool ConfigParser::QueryValue(const char *  pName /*IN*/, char *  pValue /*OUT*/)
{
	// 파라미터 체크
	if( !pName || !pValue ) { return false; }

	// 초기화
	pValue[0] = '\0';

	// 검색
	for(auto item=_config_list.begin(); item!=_config_list.end(); ++item)
	{
		PCFG_ITEM	pt_item = (PCFG_ITEM)*item;	

		// 체크
		if( !strcmp(pt_item->name, pName) )
		{
			strcpy(pValue, pt_item->value);
			return true;
		}
	}

	return false;
}

//====================================================================================================
// GetValue
//====================================================================================================
char *  ConfigParser::GetValue(const char *  pName)
{
	// 파라미터 체크
	if( !pName ) { return nullptr; }

	// 검색
	for(auto item=_config_list.begin(); item!=_config_list.end(); ++item)
	{
		PCFG_ITEM	pt_item = (PCFG_ITEM)*item;	

		// 체크
		if( !strcmp(pt_item->name, pName) )
		{
			return pt_item->value;
		}
	}

	return nullptr;
}


//====================================================================================================
// Reload
//====================================================================================================
bool ConfigParser::Reload()
{
	UnloadFile();


	return LoadFile(_file_path); 
}



