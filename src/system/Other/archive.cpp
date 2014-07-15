#include "uox3.h"
#include "regions.h"
#if UOX_PLATFORM != PLATFORM_WIN32
	#include <dirent.h>
#else
	#include <direct.h>
#endif

#if ACT_SQL == 1
#include "SQLManager.h"
#endif

namespace UOX
{

const SI32 BUFFERSIZE = 16384;

bool fileCopy( std::string sourceFile, std::string targetFile )
{
	bool rvalue = false;
	if( !sourceFile.empty() && !targetFile.empty() )
	{
		std::fstream source;
		std::fstream target;
		source.open( sourceFile.c_str(), std::ios_base::in | std::ios_base::binary );
		target.open( targetFile.c_str(), std::ios_base::out | std::ios_base::binary );
		if( target.is_open() && source.is_open() )
		{
			rvalue = true;
			char buffer[BUFFERSIZE];
			while( !source.eof() )
			{
				source.read( buffer, BUFFERSIZE );
				std::streamsize amtRead = source.gcount();
				if( amtRead > 0 )
				{
					target.write( buffer, amtRead );
				}
			}
		}
		// We put these out of the if scope, since one may be open.  It doesn't hurt closing a stream not open
		source.close();
		target.close();
	}
	return rvalue;
}

//o---------------------------------------------------------------------------o
//|   Function    :  void fileArchive( void )
//|   Date        :  11th April, 2002
//|   Programmer  :  duckhead
//o---------------------------------------------------------------------------o
//|   Purpose     :  Makes a backup copy of a file in the shared directory.
//|                  puts the copy in the backup directory
//o---------------------------------------------------------------------------o
static void backupFile( std::string filename, std::string backupDir )
{
	UString			to( backupDir );
	to				= to.fixDirectory();
	to				+= filename;
	UString from	= cwmWorldState->ServerData()->Directory( CSDDP_SHARED ) + filename;

	fileCopy( from, to );
}

//o---------------------------------------------------------------------------o
//|   Function    :  void fileArchive( void )
//|   Date        :  24th September, 2001
//|   Programmer  :  Abaddon (rewritten for 0.95)
//o---------------------------------------------------------------------------o
//|   Purpose     :  Makes a backup copy of the current world state
//o---------------------------------------------------------------------------o
void fileArchive( void )
{
	Console << "Beginning backup... ";
	time_t mytime;
	time( &mytime );
	tm *ptime = localtime( &mytime );			// get local time
	const char *timenow = asctime( ptime );		// convert it to a string

	char timebuffer[256];
	strcpy( timebuffer, timenow );

	// overwriting the characters that aren't allowed in paths
	for( UI32 a = 0; a < strlen( timebuffer ); ++a )
	{
		if( isspace( timebuffer[a] ) || timebuffer[a] == ':' )
			timebuffer[a]='-';
	}

	std::string backupRoot	= cwmWorldState->ServerData()->Directory( CSDDP_BACKUP );
	backupRoot				+= timebuffer;
#if ACT_SQL == 0
	int makeResult = _mkdir( backupRoot.c_str(), 0777 );

	if( makeResult != 0 )
		Console << "Cannot create backup directory, please check available disk space" << myendl;
	else
	{
		Console << "NOTICE: Accounts not backed up. Archiving will change. Sorry for the trouble." << myendl;

		backupFile( "house.wsc", backupRoot );

		// effect backups
		backupFile( "effects.wsc", backupRoot );

		const SI16 AreaX = UpperX / 8;	// we're storing 8x8 grid arrays together
		const SI16 AreaY = UpperY / 8;
		char backupPath[MAX_PATH + 1];
		char filename1[MAX_PATH];

		sprintf( backupPath, "%s%s/", cwmWorldState->ServerData()->Directory( CSDDP_SHARED ).c_str(), timebuffer );

		for( SI16 counter1 = 0; counter1 < AreaX; ++counter1 )	// move left->right
		{
			for( SI16 counter2 = 0; counter2 < AreaY; ++counter2 )	// move up->down
			{
				sprintf( filename1, "%i.%i.wsc", counter1, counter2 );
				backupFile( filename1, backupRoot );
			}
		}
		backupFile( "overflow.wsc", backupRoot );
		backupFile( "jails.wsc", backupRoot );
		backupFile( "guilds.wsc", backupRoot );
		backupFile( "regions.wsc", backupRoot );
	}
#else
	{
		std::stringstream BackupQuery;
		std::string db = SQLManager::getSingleton().GetDatabase();
		int index[2];
		if (SQLManager::getSingleton().ExecuteQuery("SHOW TABLES", &index[0], false))
		{
			int tables = mysql_num_fields(SQLManager::getSingleton().GetMYSQLResult());
			while (SQLManager::getSingleton().FetchRow(&index[0])) // First, get all table names of the db
			{
				UString value[2];
				if (SQLManager::getSingleton().GetColumn(0, value[0], &index[0]))
				{
					if (SQLManager::getSingleton().ExecuteQuery("SHOW CREATE TABLE "+db+"."+value[0], &index[1], false))
					{
						if (SQLManager::getSingleton().GetColumn(1, value[1], &index[1])) // Second, show how to create the table
							BackupQuery << value[1] << ";\n";
						SQLManager::getSingleton().QueryRelease(false);
					}
					
					std::istringstream iss;
					iss.str(value[1]);
					value[1].clear();

					if (SQLManager::getSingleton().ExecuteQuery("SELECT COUNT(*) FROM "+value[0], &index[1], false))
					{
						SQLManager::getSingleton().GetColumn(0, value[1], &index[1]); // Third, check if the table contains data
						SQLManager::getSingleton().QueryRelease(false);
					}
					if (value[1].compare("0") != 0) // It contains data
					{
						value[1].clear();

						std::stringstream first;
						first << "INSERT INTO `" << value[0] << "` (";
						int columnnames = 0;
						if (SQLManager::getSingleton().ExecuteQuery("SELECT `COLUMN_NAME` FROM information_schema.COLUMNS WHERE `TABLE_SCHEMA`=\""+db+"\" and `TABLE_NAME`=\""+value[0]+"\"", &index[1], false))
						{
							while (SQLManager::getSingleton().FetchRow(&index[1]))
							{
								if (columnnames != 0)
									first << ", ";
								if (SQLManager::getSingleton().GetColumn(0, value[1], &index[1])) // Fourth, get all column names of the table
									first << "`" << value[1] << "`";
								
								++columnnames;
								value[1].clear();
							}

							SQLManager::getSingleton().QueryRelease(false);
						}

						std::vector<bool> nullable;

						std::string line;
						while (std::getline(iss, line))
						{
							if (line.find(",") != std::string::npos)
								if (line.find("NOT NULL") != std::string::npos)
									nullable.push_back(false);
								else
									nullable.push_back(true);
						}

						first << ") VALUES ";

						int MaxQuerySize = 300; // will affect size of the backup
						int CurrQuerySize = MaxQuerySize;
						if (SQLManager::getSingleton().ExecuteQuery("SELECT * FROM "+value[0], &index[1], false))
						{
							bool started = false;
							int columns = mysql_num_fields(SQLManager::getSingleton().GetMYSQLResult());
							BackupQuery << first.str();
							while (SQLManager::getSingleton().FetchRow(&index[1])) // Fifth, get all datas
							{
								if (CurrQuerySize == 0)
								{
									BackupQuery << ");\n" << first.str();
									started = false;
									CurrQuerySize = MaxQuerySize;
								}
								if (started)
									BackupQuery << "),";

								BackupQuery << "(";
								for (int j = 0; j < columns; ++j)
								{
									started = true;
									SQLManager::getSingleton().GetColumn(j, value[1], &index[1]);
									if (value[1].empty())
										BackupQuery << (nullable[j] ? "NULL" : "\"\"");
									else
									{
										std::string search[2] = {"'", "\\"};
										for (int k = 0; k < 2; ++k)
											if (value[1].find(search[k]) != std::string::npos)
												value[1].replace(value[1].find(search[k]), search[k].length(), search[k]+search[k]);

										BackupQuery << "'" << value[1] << "'";
									}
									value[1].clear();

									if (j != columns-1)
										BackupQuery << ",";
								}
								--CurrQuerySize;
							}
							BackupQuery << ");\n";

							SQLManager::getSingleton().QueryRelease(false);
						}
					}
				}
			}
			SQLManager::getSingleton().QueryRelease(false);
		}
		
		std::ofstream sqlfile (backupRoot+db+".sql");
		sqlfile << BackupQuery.str();
		sqlfile.close();
	}

#endif
	Console << "Finished backup" << myendl;
}

}
