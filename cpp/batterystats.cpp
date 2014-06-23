// cpp.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "iostream"
#include "sstream"
#include "time.h"
#include "vector"
#include "iostream"
#include "Windows.h"
#include "fstream"
#include "Shlobj.h"
#include "stdio.h"
#include "WbemCli.h"
#include "sqlite3.h"

using namespace std;

//get the users home directory
wstring GetHomeDirectory(){
		PWSTR path;
		SHGetKnownFolderPath(FOLDERID_Profile, 0, NULL, &path);
		return path;
}

void WriteDefaultConfiguration(char *defaulttime){
	ofstream config_file;
	config_file.open("config.ini", ios::out);
	config_file.write(defaulttime, strlen(defaulttime));
	config_file.close();
}

int ConvertToMilliseconds(int interval){
	return ((interval * 60) * 1000);
}

int ReadConfigurationFile(){
	char defaulttime[7] = "10";
	char line[7];
	ifstream config_file;
	config_file.open("config.ini",ios::in);
	if(!config_file.is_open()){
		WriteDefaultConfiguration(defaulttime);
		config_file.open("config.ini",ios::in);
		if(!config_file.is_open()){
			return ConvertToMilliseconds(atoi(defaulttime));
		}
	}
	config_file.get(line, 10);
	int interval;
	interval = atoi(line);
	int interval_milli = ConvertToMilliseconds(interval);
	return interval_milli;
}

sqlite3* CreateDatabase(){
	sqlite3 *db;
	char *err = 0;

	int rc = sqlite3_open("battery_stats.sqlite", &db);

	if(rc){
		fprintf(stderr, "cant open database %s\n", sqlite3_errmsg(db));
	}

	char *sql = "CREATE TABLE IF NOT EXISTS stat(" \
			"date DATETIME NOT NULL," \
			"battery_percentage INT NOT NULL," \
			"ac_connected INT NOT NULL);";

	sqlite3_stmt *statement;
	if(sqlite3_prepare_v2(db, sql, -1, &statement, 0) == SQLITE_OK){
		sqlite3_step(statement);
	}
	sqlite3_close(db);

	return db;
}

int _tmain(int argc, _TCHAR* argv[]){  
	
	//hide console window
	HWND hWnd = GetConsoleWindow();
	ShowWindow(hWnd, SW_HIDE);
	int interval_time = ReadConfigurationFile();
	sqlite3 *db = CreateDatabase();

	while(true){
		SYSTEM_POWER_STATUS status;
		::GetSystemPowerStatus(&status);//get battery life
		int per = status.BatteryLifePercent;
		//get time
		time_t rawtime;
		struct tm* printtime;
		time(&rawtime);
		printtime = localtime(&rawtime);
		string timenow = asctime(printtime); 
		//create structure to hold percentage as a string
		char strPer[4]; 
		//copy int percentage to string
		itoa(per, strPer, 10);
		//open textfile to write to
		ofstream batteryStats;
		batteryStats.open(GetHomeDirectory() +L"/battery_stats.txt", ios::app);
		int AC = (int) status.ACLineStatus;
		string acconnected;
		if(AC){
			acconnected = "AC connected";
		}else{
			acconnected = "AC disconnected";
		}

		if(!batteryStats.is_open()){
			cout<< "Error";
		}else{
			batteryStats << "\n" << timenow <<strPer << "\t" << acconnected;
		}

		sqlite3_stmt *statement;
		char *sql = "INSERT INTO stat (date, battery_percentage, ac_connected) VALUES(?, ?, ?);";
		if(sqlite3_prepare_v2(db, sql, strlen(sql), &statement, 0) == SQLITE_OK){
			sqlite3_bind_text(statement, 1, asctime(printtime),strlen(asctime(printtime)), 0);
			sqlite3_bind_int(statement, 2, per);
			sqlite3_bind_int(statement, 3, AC);

			sqlite3_step(statement);
			sqlite3_finalize(statement);
		} else {
		fprintf(stderr, "cant update database %s\n", sqlite3_errmsg(db));
		}

		batteryStats.close();
		//sleep for specified time
		Sleep(interval_time);
	}	
}





