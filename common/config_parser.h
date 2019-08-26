//====================================================================================================
//  Created by Bong Jaejong
//  Email : bongjj77@gmail.com
//====================================================================================================

#pragma once

#include <list>
#include <string>
 
#define GET_CONFIG_VALUE(n) (ConfigParser::GetInstance().GetValue(n)) ? (ConfigParser::GetInstance().GetValue(n)) : n##_VALUE

//====================================================================================================
// ConfigParser
//====================================================================================================
class ConfigParser  
{
public:
	static ConfigParser& GetInstance();

protected:
	ConfigParser();
	ConfigParser(const ConfigParser&) {}
	~ConfigParser();

private:
	static void CreateInstance();
	static void KillInstance();

public:
	bool LoadFile(std::string strFilePath);
	void UnloadFile();
	bool Reload(); 
	
	bool QueryValue(const char *  pName /*IN*/, char *  pValue /*OUT*/);
	char *GetValue(const char *  pName);

private:
	static ConfigParser * _pInstance;
	static bool _destoryed;

	std::list<void *> _config_list;
	std::string _file_path; 
};