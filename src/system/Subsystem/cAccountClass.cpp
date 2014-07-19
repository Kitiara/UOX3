//o--------------------------------------------------------------------------o
//|	File			-	cAccountClass.cpp
//|	Date			-	12/6/2002 4:16:33 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//o--------------------------------------------------------------------------o
//|	Description		-	Seeing as I had screwed up accounts so bad, I started from
//|							scratch. The concept of how they work has been reevaluated
//|							and will be changed to suit a more understandable direction.
//|							See xRFC0004 for complete specification and implementaion
//|							information.
//o--------------------------------------------------------------------------o
#include "uox3.h"
#include "cVersionClass.h"
#include "SQLManager.h"
#include <openssl\sha.h>

namespace UOX
{

cAccountClass *Accounts;

const UI08 CHARACTERCOUNT = 7;

//o--------------------------------------------------------------------------o
//|	Function		-	Class Construction and Desctruction
//|	Date			-	12/6/2002 4:18:58 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Classic class construction and destruction
//|									
//|	Modification	-	Added the member to handle the stream to packet packing
//|							symantics. I put it here because from most levels of 
//|							object the pointer to accounts data is fairly consistant
//|							and readily available
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
cAccountClass::cAccountClass()
{
	m_wHighestAccount = 0x0000;
	I = m_mapUsernameIDMap.end();
	actbInvalid.wAccountIndex = AB_INVALID_ID;
}
//
cAccountClass::~cAccountClass()
{

}
//

void cAccountClass::WriteAccountSection( CAccountBlock& actbTemp, std::fstream& fsOut )
{
	fsOut << "SECTION ACCOUNT " << std::dec << actbTemp.wAccountIndex << std::endl;
	fsOut << "{" << std::endl;
	fsOut << "NAME " << actbTemp.sUsername << std::endl;
	fsOut << "PASS " << actbTemp.sPassword << std::endl;
	fsOut << "FLAGS 0x" << std::hex << actbTemp.wFlags.to_ulong() << std::dec << std::endl;
	fsOut << "PATH " << UString::replaceSlash(actbTemp.sPath) << std::endl;
	fsOut << "TIMEBAN 0x" << std::hex << actbTemp.wTimeBan << std::dec << std::endl;
	fsOut << "LASTIP " << (int)((actbTemp.dwLastIP&0xFF000000)>>24) << "." << (int)((actbTemp.dwLastIP&0x00FF0000)>>16) << "." << (int)((actbTemp.dwLastIP&0x0000FF00)>>8) << "." << (int)((actbTemp.dwLastIP&0x000000FF)%256) << std::endl;
	fsOut << "CONTACT " << (actbTemp.sContact.length()?actbTemp.sContact:"NA") << std::endl;
	for( UI08 ii = 0; ii < CHARACTERCOUNT; ++ii )
	{
		SERIAL charSer			= INVALIDSERIAL;
		std::string charName	= "UNKNOWN";
		if( actbTemp.lpCharacters[ii] != NULL )
		{
			charSer		= actbTemp.dwCharacters[ii];
			charName	= actbTemp.lpCharacters[ii]->GetName();
		}
		fsOut << "CHARACTER-" << std::dec << (ii+1) << " 0x" << std::hex << charSer << std::dec;
		fsOut << " [" << charName.c_str() << "]" << std::endl;
	}
	fsOut << "}" << std::endl << std::endl;

}


//o--------------------------------------------------------------------------o
//|	Function		-	UI16 cAccountClass::AddAccount(std::string sUsername, std::string sPassword, std::string sContact)
//|						UI16 cAccountClass::AddAccount(std::string sUsername, std::string sPassword, std::string sContact,UI16 wAttributes)
//|	Date			-	12/12/2002 11:15:20 PM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	This function creates in memory as well as on disk a new
//|							account that will allow access to the server from external
//|							clients. The basic goal of this function is to create a
//|							new account record with no characters. Because this function
//|							may be called from in game a hard copy will be made as well
//|							instead of waiting for a save to make the changes perminent.
//|									
//|							sUsername:   Name of the account
//|							sPassword:   Password of the account
//|							sContact:    Historically this is the valid email address 
//|							             for this account
//|							wAttributes: What attributes should this account start with
//o--------------------------------------------------------------------------o
//|	Returns			-	[UI16] Containing the account number of new accounts or 0
//o--------------------------------------------------------------------------o
//|	NOTE			-	This function does NOT add this account to the accounts
//|							map. This function ONLY creates the directories, and entries
//|							required to load these new accounts. The idea was not to 
//|							have to rely on the accounts in memory, so that accounts
//|							that are added, are added immediatly to the hard copy.
//|	NOTE			-	For any new accounts to be loaded into the internal accounts
//|							map structure, accounts must be reloaded!
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
UI16 cAccountClass::AddAccount(std::string sUsername, std::string sPassword, std::string sContact,UI16 wAttributes)
{
	// First were going to make sure that the needed fields are sent in with at least data
	if( sUsername.length() < 4 || sPassword.length() < 4 )
	{
		// Username, and or password must both be 4 characters or more in length
		Console.Log("ERROR: Unable to create account for username '%s' with password of '%s'. Username/Password to short","accounts.log",sUsername.c_str(),sPassword.c_str());
		return 0x0000;
	}
	// Next thing were going to do is make sure there isn't a duplicate username.
	if( isUser( sUsername ) )
	{
		// This username is already on the list.
		return 0x0000;
	}
	// If we get here then were going to create a new account block, and create all the needed directories and files.
	CAccountBlock actbTemp;
	actbTemp.reset();

	// Build as much of the account block that we can right now.
	actbTemp.sUsername	= sUsername;
	actbTemp.sPassword	= sPassword;
	actbTemp.sContact	= sContact;
	actbTemp.wFlags		= wAttributes;
	actbTemp.wTimeBan	= 0;
	actbTemp.dwLastIP	= 0;
	for(UI08 i = 0; i < CHARACTERCOUNT; ++i)
	{
		actbTemp.lpCharacters[i] = NULL;
		actbTemp.dwCharacters[i] = INVALIDSERIAL;
	}

    unsigned char digest[SHA_DIGEST_LENGTH];
    SHA_CTX ctx;
    SHA1_Init(&ctx);
    SHA1_Update(&ctx, (sUsername+":"+sPassword).c_str(), strlen((sUsername+":"+sPassword).c_str()));
    SHA1_Final(digest, &ctx);
 
    char sha1hash[SHA_DIGEST_LENGTH*2+1];
    for (int i = 0; i < SHA_DIGEST_LENGTH; i++)
        sprintf(&sha1hash[i*2], "%02x", (unsigned int)digest[i]);

	std::stringstream sql;
	sql << "INSERT INTO accounts (id, username, sha_pass_hash, email, flags, last_ip) VALUES ('";
	sql << m_wHighestAccount+1 << sUsername << "', '" << sha1hash << "', '" << sContact << "', '" << actbTemp.wFlags.to_ulong() << "', '";
	sql << (int)((actbTemp.dwLastIP&0xFF000000)>>24) << "." << (int)((actbTemp.dwLastIP&0x00FF0000)>>16) << ".";
	sql << (int)((actbTemp.dwLastIP&0x0000FF00)>>8) << "." << (int)((actbTemp.dwLastIP&0x000000FF)%256) << "')";

	SQLManager::getSingleton().ExecuteQuery(sql.str(), NULL, false);
	SQLManager::getSingleton().QueryRelease(false);

	// Ok might be a good thing to add this account to the map(s) now.
	m_mapUsernameIDMap[actbTemp.wAccountIndex]=actbTemp;
	m_mapUsernameMap[actbTemp.sUsername]=&m_mapUsernameIDMap[actbTemp.wAccountIndex];
	// Return to the calling function
	return (UI16)m_mapUsernameIDMap.size();
}


//o--------------------------------------------------------------------------o
//|	Function		-	bool cAccountClass::isUser(string sUsername)
//|	Date			-	12/16/2002 12:09:13 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	The only function that this function member servers is to 
//|							return a response based on the existance of the specified
//|							username.
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
bool cAccountClass::isUser(std::string sUsername)
{
	MAPUSERNAME_ITERATOR I	= m_mapUsernameMap.find( sUsername );
	return( I != m_mapUsernameMap.end() );
}

//o--------------------------------------------------------------------------o
//|	Function		-	UI16 cAccountClass::size()
//|	Date			-	12/17/2002 3:35:31 PM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Returns the current count of accounts that are currently
//|							stored in the account map. 
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
size_t cAccountClass::size()
{
	return m_mapUsernameMap.size();
}

//o--------------------------------------------------------------------------o
//|	Function		-	UI16 cAccountClass::Load()
//|	Date			-	12/17/2002 4:00:47 PM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Load the internal accounts structure from the accounts.adm
//|							file. Due to the nature of this function we will empty the
//|							previous contents of the account map, and reload them.
//|									
//|	Modification	-	1/20/2003 - Forgot to put in place the lookup to see if 
//|									accounts need to be updated to the v3 format. Should only
//|									need to check the first line of the accounts.adm file.
//|									NOTE: Do not remove this line if you wish to convert 
//|									properly. Without the first line this function will assume
//|									that the accounts file is a v2 format file(UOX3 v0.97.0).
//|									
//|	Modification	-	1/20/2003 - Added support for the DTL libs, and create a
//|									general SQL query for MSAccess, MSSQL, and mySQL
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
bool cAccountClass::FinaliseBlock(CAccountBlock& toFinalise)
{
	for(int i = 0; i < CHARACTERCOUNT; ++i)
	{
		if(toFinalise.dwCharacters[i] != INVALIDSERIAL)
		{
			toFinalise.lpCharacters[i] = calcCharObjFromSer(toFinalise.dwCharacters[i]);
			if(toFinalise.lpCharacters[i] != NULL)
				toFinalise.lpCharacters[i]->SetAccount(toFinalise);
		}
	}
	m_mapUsernameIDMap[toFinalise.wAccountIndex] = toFinalise;
	m_mapUsernameMap[toFinalise.sUsername] = &m_mapUsernameIDMap[toFinalise.wAccountIndex];
	return true;
}

bool cAccountClass::LoadFromDB(UI16& numLoaded)
{
	int index = 0;
	bool result = SQLManager::getSingleton().ExecuteQuery("SELECT * FROM accounts", &index, false);

	if (result)
	{
		CAccountBlock actB;
		numLoaded = 0;
		UI08 CharacterLimit = 0;
		int ColumnCount = mysql_num_fields(SQLManager::getSingleton().GetMYSQLResult());
		while (SQLManager::getSingleton().FetchRow(&index))
		{
			std::string AccountID = "";
			for(int i = 0; i < ColumnCount; ++i)
			{
				UString value;
				if(!SQLManager::getSingleton().GetColumn(i, value, &index))
				{
					Console.Warning("SQLManager: Error retrieving column %i on record %i.", i, index);
					if (i == 1 || i == 2)
					{
						Console.Warning("SQLManager: Account with no %s! [%i], skipping...", i == 1 ? "username" : "sha_pass_hash", index);
						break;
					}
				}
				else
				{
					switch(i)
					{
					case 0:
						FinaliseBlock(actB); // Get the accountblock
						actB.reset(); // We have to reset first especially while reloading accounts this is very important.
						actB.dbRetrieved = true;
						CharacterLimit = -1;
						AccountID = value;
						actB.wAccountIndex = value.toUShort();
						break;
					case 1:
						actB.sUsername = value.stripWhiteSpace();
						break;
					case 2:
						actB.sPassword = value.stripWhiteSpace();
						break;
					case 3:
						actB.sContact = value.stripWhiteSpace();
						break;
					case 4:
						{
							int currflags = value.toUInt();
							for (int flags = AB_FLAGS_ALL; flags > -1; --flags)
								if (currflags >= pow(2.0, flags))
								{
									actB.wFlags.set(flags, true);
									currflags -= pow(2.0, flags);
								}

								int index2 = 0;							
								if (SQLManager::getSingleton().ExecuteQuery("SELECT serial FROM characters WHERE account = "+AccountID, &index2, false))
								{
									while(SQLManager::getSingleton().FetchRow(&index2))
									{
										++CharacterLimit;
										if (CharacterLimit == CHARACTERCOUNT)
										{
											Console.Warning("SQLManager: Too many characters on account.");
											break;
										}

										UString value2;
										if(!SQLManager::getSingleton().GetColumn(0, value2, &index2))
										{
											Console.Warning("SQLManager: Error retrieving column 0 on record %i.", index2);
											actB.dwCharacters[CharacterLimit] = INVALIDSERIAL;
										}
										else
											actB.dwCharacters[CharacterLimit] = value2.toULong();
									}
									SQLManager::getSingleton().QueryRelease(false);
								}
						}
						break;
					case 5:
						actB.dwLastIP = calcserial(value.section(".", 0, 0).toByte(), value.section(".", 1, 1).toByte(), value.section(".", 2, 2).toByte(), value.section(".", 3, 3).toByte());
						++numLoaded;
						FinaliseBlock(actB);
						break;
					}
				}
			}
		}
		SQLManager::getSingleton().QueryRelease(false);
	}

	return result;
}

UI16 cAccountClass::Load(void)
{
	UI16 retVal = 0;
	if (LoadFromDB(retVal))
		return retVal;
	return 0;
}

//o--------------------------------------------------------------------------o
//|	Function		-	bool cAccountClass::TransCharacter(UI16 wSAccountID,UI16 wSSlot,UI16 wDAccountID)
//|	Date			-	1/7/2003 5:39:12 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Transfer a character from a known slot on a specified 
//|							account, to a new account and slot.
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
bool cAccountClass::TransCharacter(UI16 wSAccountID,UI16 wSSlot,UI16 wDAccountID)
{
	// Abaddon please leave these declarations alone. When we do this VC autocompetion doesn't work cause this isn't shoved on the stack with type info
	MAPUSERNAMEID_ITERATOR I = m_mapUsernameIDMap.find(wSAccountID);
	//
	if( I==m_mapUsernameIDMap.end() )
	{
		// This ID was not found.
		return false;
	}
	// Get the account block for this ID
	CAccountBlock& actbID	= I->second;
	// Ok now we need to get the matching username map 
	MAPUSERNAME_ITERATOR J	= m_mapUsernameMap.find(actbID.sUsername);
	if( J==m_mapUsernameMap.end() )
	{
		// This ID was not found.
		return false;
	}
	// Get the account block for this username.

	// ok we will check to see if there is a valid char in source slot, if not we will return
	if( actbID.dwCharacters[wSSlot]==INVALIDSERIAL )
		return false;
	//
	MAPUSERNAMEID_ITERATOR II = m_mapUsernameIDMap.find( wDAccountID );
	if( II == m_mapUsernameIDMap.end() )
	{
		// This ID was not found.
		return false;
	}
	// Get the account block for this ID
	CAccountBlock& actbIID	= II->second;
	// Ok now we need to get the matching username map 
	MAPUSERNAME_ITERATOR JJ	= m_mapUsernameMap.find(actbIID.sUsername);
	if( JJ==m_mapUsernameMap.end() )
	{
		// This ID was not found.
		return false;
	}
	// Get the account block for this username.

	// ok at this point I/II = SourceID, and DestID and J/JJ SourceName/DestName
	return AddCharacter(wDAccountID, wSAccountID, wSSlot, actbID, true);
}

//o--------------------------------------------------------------------------o
//|	Function		-	bool cAccountClass::AddCharacter(UI16 accountid, CChar *object)
//|						bool cAccountClass::AddCharacter(UI16 accountid, void *object)
//|	Date			-	12/18/2002 2:09:04 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Add a character object to the in memory storage. 
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
bool cAccountClass::AddCharacter(UI16 wAccountID, CChar *lpObject)
{
	return AddCharacter(wAccountID, NULL, NULL, lpObject->GetSerial(), lpObject);
}

bool cAccountClass::AddCharacter(UI16 wDAccountID, UI16 wSAccountID, UI16 wSSlot, CAccountBlock& actbTemp, bool transaction)
{
	if (transaction)
		SQLManager::getSingleton().BeginTransaction();
	bool res = AddCharacter(wDAccountID, wSAccountID, wSSlot, actbTemp.dwCharacters[wSSlot], actbTemp.lpCharacters[wSSlot], transaction);
	if (transaction)
		SQLManager::getSingleton().FinaliseTransaction(transaction);
	return res;
}

bool cAccountClass::AddCharacter(UI16 wAccountID, UI16 wSAccountID, UI16 wSSlot, UI32 dwCharacterID, CChar *lpObject, bool transaction)
{
	if (lpObject == NULL) // Make sure that the lpObject pointer is valid
		return false;

	// Ok we need to do is get see if this account id exists
	MAPUSERNAMEID_ITERATOR IdItr = m_mapUsernameIDMap.find(wAccountID);
	if (IdItr == m_mapUsernameIDMap.end())
		return false; // This ID was not found.

	CAccountBlock& actbID = IdItr->second; // Get the account block for this ID
	// Ok now we need to get the matching username map 
	MAPUSERNAME_ITERATOR NameItr = m_mapUsernameMap.find(actbID.sUsername);
	if (NameItr == m_mapUsernameMap.end())
		return false; // This ID was not found.
	
	CAccountBlock& actbName = (*NameItr->second); // Get the account block for this username.
	// Ok now that we have both of our account blocks we can update them. We will use the first empty slot for this character
	bool bExit = false;
	for (UI08 i = 0; i < CHARACTERCOUNT; ++i)
		if (actbID.dwCharacters[i] != lpObject->GetSerial() && actbName.dwCharacters[i] != lpObject->GetSerial() &&
			actbID.dwCharacters[i] == INVALIDSERIAL && actbName.dwCharacters[i] == INVALIDSERIAL)
		{
			// Ok this slot is empty, we will stach this character in this slot
			actbID.lpCharacters[i] = lpObject;
			actbID.dwCharacters[i] = dwCharacterID;
			actbName.lpCharacters[i] = lpObject;
			actbName.dwCharacters[i] = dwCharacterID;
			bExit = true; // we do not need to continue throught this loop anymore.
			break; 
		}

	// If we were successfull then we return true
	if (bExit)
	{
		m_mapUsernameIDMap[actbID.wAccountIndex] = actbID; // Make sure to put the values back into the maps corrected.

		if (wSAccountID != NULL && wSSlot != NULL)
		{
			transaction = true;
			DelCharacter(wSAccountID, wSSlot, true, transaction);

			std::stringstream sql;
			sql << "UPDATE characters SET account = '" << wAccountID << "' WHERE account ='" << wAccountID << "' and serial = '" << lpObject->GetSerial() << "'";
			SQLManager::getSingleton().ExecuteQuery(sql.str(), NULL, transaction);
		}
		lpObject->SetAccountNum(wAccountID);

		return true;
	}
	return false;
}
//o--------------------------------------------------------------------------o
//|	Function		-	bool cAccountClass::clear(void)
//|	Date			-	12/18/2002 2:24:07 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	This function is used to clear out all entries contained
//|						in both the Username, and UsernameID map structures.
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
bool cAccountClass::clear(void)
{
	// First we should check to make sure that we can even use the objects, or they are not already cleared
	try
	{
		if( m_mapUsernameMap.empty() || m_mapUsernameIDMap.empty() )
		{
			return false;
		}
		// ok clear the map
		m_mapUsernameIDMap.clear();
		m_mapUsernameMap.clear();
		m_wHighestAccount=0x0000;
	}
	catch( ... )
	{
		return false;
	}
	return true;
}

//o--------------------------------------------------------------------------o
//|	Function		-	bool cAccountClass::DelAccount(std::string sUsername)
//|						bool cAccountClass::DelAccount(UI16 wAccountID)
//|	Date			-	12/18/2002 2:56:34 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Remove an account from the internal account storage map.
//|						At this point this function will only remove the account
//|						block from the accounts.adm and not the physical files
//|						on the storage medium.
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
bool cAccountClass::DelAccount(std::string sUsername)
{
	// Ok were just going to get the ID number for this account and make the ID function do all the work
	MAPUSERNAME_ITERATOR I = m_mapUsernameMap.find(sUsername);
	if( I==m_mapUsernameMap.end() )
		return false;
	CAccountBlock& actbTemp = (*I->second);
	// Ok lets do the work now
	return DelAccount( actbTemp.wAccountIndex );
}
//o--------------------------------------------------------------------------o
bool cAccountClass::DelAccount(UI16 wAccountID)
{
	// Ok we need to get the ID block again as it wasn't passed in
	MAPUSERNAMEID_ITERATOR I = m_mapUsernameIDMap.find(wAccountID);
	if( I==m_mapUsernameIDMap.end() )
		return false;
	CAccountBlock& actbID=I->second;
	// Now we need to get the matching username map entry
	MAPUSERNAME_ITERATOR J = m_mapUsernameMap.find(actbID.sUsername);
	if( J==m_mapUsernameMap.end() )
		return false;

	std::stringstream sql;
	sql << "DELETE FROM accounts WHERE id = '" << wAccountID << "'\n";
	sql << "DELETE FROM attributes WHERE serial IN (SELECT serial FROM characters WHERE account = '" << wAccountID << "')\n";
	sql << "DELETE FROM baseobjects WHERE type = '1' and serial IN (SELECT serial FROM characters WHERE account = '" << wAccountID << "')\n";
	sql << "DELETE FROM characters WHERE account = '" << wAccountID << "')";

	std::istringstream iss;
	iss.str(sql.str());
	std::string line;
	while (std::getline(iss, line))
		SQLManager::getSingleton().ExecuteQuery(line);

	SQLManager::getSingleton().FinaliseTransaction(true);

	// Ok we have both the map iterators pointing to the right place. Erase these entries
	m_mapUsernameIDMap.erase(I);
	m_mapUsernameMap.erase(J);
	// Just a means to show that an accounts has been removed when looking at the directories
	char szDirName[40];
	char szDirPath[MAX_PATH];
	memset(szDirName,0x00,sizeof(char)*40);
	memset(szDirPath,0x00,sizeof(char)*MAX_PATH);
	// Ok copy only the username portion to this so we can build a correct rename path
	strcpy(szDirName,&actbID.sPath[actbID.sPath.length()-actbID.sUsername.length()-1]);
	strncpy(szDirPath,actbID.sPath.c_str(),actbID.sPath.length()-actbID.sUsername.length()-1);
	std::string sNewDir(szDirPath);
	sNewDir += "_";
	sNewDir += szDirName;
	rename(actbID.sPath.c_str(),sNewDir.c_str());
	return true;
}

//o--------------------------------------------------------------------------o
//|	Function		-	bool cAccountClass::DelCharacter(UI16 wAccountID, int nSlot)
//|	Date			-	12/19/2002 12:45:10 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Remove a character from the accounts map. Currently this
//|						character object resource is not freed. The Character ID, 
//|						and CChar object holders will be set to -1, and NULL 
//|						respectivly. As a consolation the characters will be for
//|						the interim listed in the orphan.adm file. This file will
//|						contain simply the username, and character ID that was 
//|						removed. For those that are wondering why this might be.
//|						Until the system itself deletes the character and items
//|						at least we will have a listing of orphaned character in
//|						game that a GM can use to locate, or even assign to another
//|						account.
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
bool cAccountClass::DelCharacter(UI16 wAccountID, UI08 nSlot, bool switching, bool transaction)
{
	// Do the simple here, save us some work
	if (nSlot > CHARACTERCOUNT)
		return false;
	// ok were going to need to get the respective blocked from the maps
	MAPUSERNAMEID_ITERATOR IdItr = m_mapUsernameIDMap.find(wAccountID);
	if (IdItr == m_mapUsernameIDMap.end())
		return false;

	CAccountBlock& actbID = IdItr->second;
	// Ok now that we have the ID Map block we can get the Username Block
	MAPUSERNAME_ITERATOR NameItr = m_mapUsernameMap.find(actbID.sUsername);
	if (NameItr == m_mapUsernameMap.end())
		return false;

	CAccountBlock& actbName = (*NameItr->second);
	// Check to see if this record has been flagged changed
	if (actbID.bChanged)
	{
		IdItr->second.bChanged = false;
		return false;
	}
	// We have both blocks now. We should validate the slot, and make the changes
	if (actbID.dwCharacters[nSlot] == INVALIDSERIAL || actbName.dwCharacters[nSlot] == INVALIDSERIAL)
		return false;

	UI32 serial = actbID.dwCharacters[nSlot];
	// Ok there is something in this slot so we should remove it.
	actbID.dwCharacters[nSlot] = actbName.dwCharacters[nSlot] = INVALIDSERIAL;
	actbID.lpCharacters[nSlot] = actbName.lpCharacters[nSlot] = NULL;
	// need to reorder the accounts or have to change the addchar code to ignore invalid serials(earier here)
	CAccountBlock actbScratch;
	actbScratch.reset();
	actbScratch = actbID;
	int j=0;
	// Dont mind this loop, becuase we needed a copy of the data too we need to invalidate actbScracthes pointers, and indexs
	for (UI08 i = 0; i < CHARACTERCOUNT; ++i)
		if (actbID.dwCharacters[i] != INVALIDSERIAL && actbID.lpCharacters[i] != NULL)
		{
			// OK we keep this entry 
			actbScratch.dwCharacters[j]=actbID.dwCharacters[j];
			actbScratch.lpCharacters[j]=actbID.lpCharacters[j];
			j += 1;
		}

	// Fill the rest with standard empty values.
	for (UI08 i = j; i < CHARACTERCOUNT; ++i)
	{
		actbScratch.dwCharacters[i]=INVALIDSERIAL;
		actbScratch.lpCharacters[i]=NULL;
	}
	// Now copy back out the info to the structures
	for (UI08 i = 0; i < CHARACTERCOUNT; ++i)
	{
		actbID.dwCharacters[i] = actbName.dwCharacters[i] = actbScratch.dwCharacters[i];
		actbID.lpCharacters[i] = actbName.lpCharacters[i] = actbScratch.lpCharacters[i];
	}
	actbID.bChanged = actbName.bChanged = true;
	// Now we have to put the values back into the maps
	try
	{
		if (!switching)
		{
			std::stringstream sql;
			sql << "DELETE FROM characters WHERE account ='" << wAccountID << "' and serial = '" << serial << "'\n";
			sql << "DELETE FROM baseobjects WHERE type ='1' and serial = '" << serial << "'\n";
			sql << "DELETE FROM attributes WHERE serial = '" << serial << "'";
			printf("%s\n", sql.str().c_str());
			std::istringstream iss;
			iss.str(sql.str());
			std::string line;
			while (std::getline(iss, line))
				SQLManager::getSingleton().ExecuteQuery(line, NULL, transaction);

			if (transaction == false)
				SQLManager::getSingleton().QueryRelease(transaction);
		}
	}
	catch( ... )
	{
		return false;
	}
	return true;
}

//o--------------------------------------------------------------------------o
//|	Function		-	CAccountBlock& GetAccountByName( std::string sUsername )
//|	Date			-	12/19/2002 2:16:37 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Search the map for an account but the username that was
//|						specified.
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
CAccountBlock& cAccountClass::GetAccountByName( std::string sUsername )
{
	// Ok now we need to get the map blocks for this account.
	MAPUSERNAME_ITERATOR I = m_mapUsernameMap.find(sUsername);
	if( I != m_mapUsernameMap.end() )
	{
		CAccountBlock &actbName = (*I->second);
		// Get the block from the ID map, so we can check that they are the same.
		MAPUSERNAMEID_ITERATOR J = m_mapUsernameIDMap.find( actbName.wAccountIndex );
		if( J != m_mapUsernameIDMap.end() )
		{
			CAccountBlock& actbID = J->second;
			// Check to see that these both are equal where it counts.
			if( actbID.sUsername == actbName.sUsername || actbID.sPassword == actbName.sPassword )
				return actbName;
		}
	}
	return actbInvalid;
}

//o--------------------------------------------------------------------------o
//|	Function		-	CAccountBlock& cAccountClass::GetAccountByName( UI16 wAccountID )
//|	Date			-	12/19/2002 2:17:31 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Search the map for an account but the username that was
//|						specified.
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
CAccountBlock& cAccountClass::GetAccountByID( UI16 wAccountID )
{
	// Ok now we need to get the map blocks for this account.
	MAPUSERNAMEID_ITERATOR I = m_mapUsernameIDMap.find(wAccountID);
	if( I != m_mapUsernameIDMap.end() )
	{
		CAccountBlock &actbID = I->second;
		// Get the block from the ID map, so we can check that they are the same.
		MAPUSERNAME_ITERATOR J = m_mapUsernameMap.find( actbID.sUsername );
		if( J != m_mapUsernameMap.end() )
		{
			CAccountBlock& actbName = (*J->second);
			// Check to see that these both are equal where it counts.
			if( actbID.sUsername == actbName.sUsername || actbID.sPassword == actbName.sPassword )
				return actbID;
		}
	}
	return actbInvalid;
}

//o--------------------------------------------------------------------------o
//|	Function		-	UI16 cAccountClass::Save(void)
//|	Date			-	12/19/2002 2:45:39 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Save the contents of the internal structures out to a flat
//|							file where it can be loaded later, and archived for later
//|							use should a crash occur and the userfile is damaged. At
//|							this stage this function will only write out the accounts.adm
//|							file, even though the access.adm will still eventually
//|							be used for server sharing.
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
UI16 cAccountClass::Save()
{
	for (MAPUSERNAMEID_ITERATOR itr = m_mapUsernameIDMap.begin(); itr != m_mapUsernameIDMap.end(); ++itr)
	{
		CAccountBlock& actbID = itr->second; // Get a usable structure for this iterator
		// Ok we are going to load up each username block from that map too for checking
		MAPUSERNAME_ITERATOR NameItr = m_mapUsernameMap.find(actbID.sUsername);
		CAccountBlock& actbName = (*NameItr->second);
		// Check to make sure at least that the username and passwords match
		if (actbID.sUsername != actbName.sUsername || actbID.sPassword != actbName.sPassword )
		{
			// there was an error between blocks
			Console.Error("Save(): Mismatch %s - %s", actbID.sUsername.c_str(), actbName.sUsername.c_str());
			continue;
		}
		std::stringstream sql;
		sql << "UPDATE accounts SET username = '" << actbID.sUsername;
		sql << "', sha_pass_hash = '" << actbID.sPassword;
		sql << "', email = " << (actbID.sContact.length() ? ("'"+actbID.sContact+"'") : "NULL");
		sql << ", flags = '" << actbID.wFlags.to_ulong();
		sql << "', last_ip = '" << (int)((actbID.dwLastIP&0xFF000000)>>24) << "." << (int)((actbID.dwLastIP&0x00FF0000)>>16) 
			<< "." << (int)((actbID.dwLastIP&0x0000FF00)>>8) << "." << (int)((actbID.dwLastIP&0x000000FF)%256) << "' ";
		sql << "WHERE id = '" << actbID.wAccountIndex << "'\n";
		for (int i = 0; i < CHARACTERCOUNT; ++i)
		{
			SERIAL charSer = INVALIDSERIAL;
			std::string charName = "UNKNOWN";
			if (actbID.lpCharacters[i] != NULL)
			{
				charSer = actbID.dwCharacters[i];
				charName = actbID.lpCharacters[i]->GetName();
			}

			if (charSer != INVALIDSERIAL)
				sql << "UPDATE characters SET name = '" << charName << "' WHERE serial = '" << charSer << "'\n";
		}

		std::istringstream iss;
		iss.str(sql.str());
		std::string line;
		while (std::getline(iss, line))
			SQLManager::getSingleton().ExecuteQuery(line, NULL, false);
	}
	return m_mapUsernameIDMap.size();
}

//o--------------------------------------------------------------------------o
//|	Function		-	MAPUSERNAME_ITERATOR cAccountClass::Begin(void)
//|	Date			-	1/14/2003 5:47:28 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Returns the First iterator in a set. If there is no record
//|							then END() will be returned. This function will set the 
//|							internal Iterator to the first record or will indicate
//|							end().
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
MAPUSERNAMEID_ITERATOR& cAccountClass::begin( void )
{
	I = m_mapUsernameIDMap.begin();
	return I;
}

//o--------------------------------------------------------------------------o
//|	Function		-	MAPUSERNAME_ITERATOR cAccountClass::End(void)
//|	Date			-	1/14/2003 5:48:32 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Returns the Last iterator in a set. This function forces
//|						the internal iterator to one past the last record. It
//|						will also return End() as a result.
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
MAPUSERNAMEID_ITERATOR& cAccountClass::end(void)
{
	I = m_mapUsernameIDMap.end();
	return I;
}

//o--------------------------------------------------------------------------o
//|	Function		-	MAPUSERNAMEID_ITERATOR& cAccountClass::Last(void)
//|	Date			-	1/14/2003 6:03:22 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	For those that need a means to get the last valid record
//|						in the container, without having to process the Iterator
//|						and back it up one record. By default the map will return
//|						end() when there is no record to return for us;
//|									
//|						NOTE: This will update the current internal iterator to 
//|						the last record as well.
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
MAPUSERNAMEID_ITERATOR& cAccountClass::last(void)
{
	I = m_mapUsernameIDMap.end();
	--I;
	return I;
}

//o--------------------------------------------------------------------------o
//|	Function		-	cAccountClass& cAccountClass::operator++()
//|						cAccountClass& cAccountClass::operator--(int)
//|	Date			-	1/14/2003 5:33:13 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Use this operators to control the internal iterator that
//|						points into the UsernameID map. This will work only on 
//|						this map. It will be assumed that the UsernameMap will 
//|						be matching this exactally. 
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
cAccountClass& cAccountClass::operator++()
{
	// just increment I, the rest will be handled internally
	I++;
	return (*this);
}
//
cAccountClass& cAccountClass::operator--(int)
{	
	I--;
	return (*this);
}

//o--------------------------------------------------------------------------o
//|	Function		-	void cAccountClass::AccountsHeader(fstream &fsOut)
//|	Date			-	12/19/2002 2:56:36 AM
//|	Developers		-	EviLDeD
//|	Organization	-	UOX3 DevTeam
//|	Status			-	Currently under development
//o--------------------------------------------------------------------------o
//|	Description		-	Writes the Accounts.Adm header to the specified output
//|						stream.
//o--------------------------------------------------------------------------o
//| Modifications	-	
//o--------------------------------------------------------------------------o
void cAccountClass::WriteAccountsHeader(std::fstream &fsOut)
{
	fsOut << "//AV3.0" << "-UV" << CVersionClass::GetVersion() << "-BD" << CVersionClass::GetBuild() << "-DS" << time(NULL) << "-ED" << CVersionClass::GetRealBuild() << std::endl;
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
	fsOut << "//accounts.adm[TEXT] : UOX3 uses this file for shared accounts access between servers" << std::endl;
	fsOut << "//" << std::endl;
	fsOut << "//   Format: " << std::endl;
	fsOut << "//      SECTION ACCOUNT 0" << std::endl;
	fsOut << "//      {" << std::endl;
	fsOut << "//         NAME username" << std::endl;
	fsOut << "//         PASS password" << std::endl;
	fsOut << "//         FLAGS 0x0000" << std::endl;
	fsOut << "//         PATH c:/uox3/Accounts/path2userdata/" << std::endl;
	fsOut << "//         TIMEBAN 0" << std::endl;
	fsOut << "//         LASTIP 127.0.0.1" << std::endl;
	fsOut << "//         CONTACT NONE" << std::endl;
	fsOut << "//         CHARACTER-1 0xffffffff" << std::endl;
	fsOut << "//         CHARACTER-2 0xffffffff" << std::endl;
	fsOut << "//         CHARACTER-3 0xffffffff" << std::endl;
	fsOut << "//         CHARACTER-4 0xffffffff" << std::endl;
	fsOut << "//         CHARACTER-5 0xffffffff" << std::endl;
	fsOut << "//         CHARACTER-6 0xffffffff" << std::endl;
	fsOut << "//      }" << std::endl;
	fsOut << "//" << std::endl;
	fsOut << "//   FLAGS: " << std::endl;
	fsOut << "//      Bit:  0x0001) Banned           0x0002) Suspended        0x0004) Public           0x0008) Currently Logged In" << std::endl;
	fsOut << "//            0x0010) Char-1 Blocked   0x0020) Char-2 Blocked   0x0040) Char-3 Blocked   0x0080) Char-4 Blocked" << std::endl;
	fsOut << "//            0x0100) Char-5 Blocked   0x0200) Char-6 Blocked   0x0400) Unused           0x0800) Unused" << std::endl;
	fsOut << "//            0x1000) Unused           0x2000) Seer             0x4000) Counselor        0x8000) GM Account" << std::endl;
	fsOut << "//" << std::endl;
	fsOut << "//   TIMEBAN: " << std::endl;
	fsOut << "//      This would be the end date of a timed ban." << std::endl;
	fsOut << "//" << std::endl;
	fsOut << "//   CONTACT: " << std::endl;
	fsOut << "//      Usually this is the email address, but can be used as a comment or ICQ" << std::endl;
	fsOut << "//" << std::endl;
	fsOut << "//   LASTIP: " << std::endl;
	fsOut << "//      The last IP this account was used from." << std::endl;
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
}
//o--------------------------------------------------------------------------o
//|	Description		-	Writes the Access.Adm header to the specified output
//|									stream.
//o--------------------------------------------------------------------------o
void cAccountClass::WriteAccessHeader(std::fstream &fsOut)
{
	fsOut << "//SA3.0" << "-UV" << CVersionClass::GetVersion() << "-BD" << CVersionClass::GetBuild() << "-DS" << time(NULL) << "-ED" << CVersionClass::GetRealBuild() << std::endl;
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
	fsOut << "//access.adm[TEXT] : UOX3 uses this file for shared accounts access between servers" << std::endl;
	fsOut << "// " << std::endl;
	fsOut << "//   Format: " << std::endl;
	fsOut << "//      SECTION ACCESS 0" << std::endl;
	fsOut << "//      {" << std::endl;
	fsOut << "//         NAME username" << std::endl;
	fsOut << "//         PASS password" << std::endl;
	fsOut << "//         PATH c:/uox3/Accounts/path2userdata/" << std::endl;
	fsOut << "//         FLAGS 0x0000" << std::endl;
	fsOut << "//         CONTACT NONE" << std::endl;
	fsOut << "//      }" << std::endl;
	fsOut << "//" << std::endl;
	fsOut << "//   FLAGS: " << std::endl;
	fsOut << "//      Bit:  0x0001) Banned           0x0002) Suspended        0x0004) Public           0x0008) Currently Logged In" << std::endl;
	fsOut << "//            0x0010) Char-1 Blocked   0x0020) Char-2 Blocked   0x0040) Char-3 Blocked   0x0080) Char-4 Blocked" << std::endl;
	fsOut << "//            0x0100) Char-5 Blocked   0x0200) Char-6 Blocked   0x0400) Unused           0x0800) Unused" << std::endl;
	fsOut << "//            0x1000) Unused           0x2000) Seer             0x4000) Counselor        0x8000) GM Account" << std::endl;
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
}
//o--------------------------------------------------------------------------o
//|	Description		-	Writes the Orphan.Adm header to the specified output
//|									stream.
//o--------------------------------------------------------------------------o
void cAccountClass::WriteOrphanHeader(std::fstream &fsOut)
{
	fsOut << "//OI3.0" << "-UV" << CVersionClass::GetVersion() << "-BD" << CVersionClass::GetBuild() << "-DS" << time(NULL) << "-ED" << CVersionClass::GetRealBuild() << "\n";
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
	fsOut << "// Orphans.Adm " << std::endl;
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
	fsOut << "// UOX3 uses this file to store any characters that have been removed from" << std::endl;
	fsOut << "// an account. They are stored here so there is some history of user deltions" << std::endl;
	fsOut << "// of their characters. At best the co-ordinate may be available." << std::endl;
	fsOut << "// Name, X/Y/Z values if available will be displayed to ease locating trouble" << std::endl;
	fsOut << "// users, and where they deleted their characters last." << std::endl;
	fsOut << "// " << std::endl;
	fsOut << "// The format is as follows:" << std::endl;
	fsOut << "// " << std::endl;
	fsOut << "//    username=ID,CharacterName,X,Y,Z" << std::endl;
	fsOut << "// " << std::endl;
	fsOut << "// NOTE: CharacterName, and Coordinates may not be available." << std::endl;
	fsOut << "//------------------------------------------------------------------------------\n";
}
//o--------------------------------------------------------------------------o
//|	Description		-	Writes the Username.uad header to the specified output
//|									stream.
//o--------------------------------------------------------------------------o
void cAccountClass::WriteUADHeader( std::fstream &fsOut, CAccountBlock& actbTemp )
{
	fsOut << "//AI3.0" << "-UV" << CVersionClass::GetVersion() << "-BD" << CVersionClass::GetBuild() << "-DS" << time(NULL) << "-ED" << CVersionClass::GetRealBuild() << std::endl;
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
	fsOut << "// UAD Path: " << actbTemp.sPath << std::endl;
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
	fsOut << "// UOX3 uses this file to store any extra administration info\n";
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
	fsOut << "ID " << actbTemp.wAccountIndex << std::endl;
	fsOut << "NAME " << actbTemp.sUsername << std::endl;
	fsOut << "PASS " << actbTemp.sPassword << std::endl;
	fsOut << "BANTIME " << std::hex << "0x" << actbTemp.wTimeBan << std::dec << std::endl;
	fsOut << "LASTIP " << (int)((actbTemp.dwLastIP&0xFF000000)>>24) << "." << (int)((actbTemp.dwLastIP&0x00FF0000)>>16) << "." << (int)((actbTemp.dwLastIP&0x0000FF00)>>8) << "." << (int)((actbTemp.dwLastIP&0x000000FF)%256) << std::endl;
	fsOut << "CONTACT " << actbTemp.sContact << std::endl;
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
}
//o--------------------------------------------------------------------------o
//|	Description		-	Writes the Username.uad header to the specified output
//|									stream.
//o--------------------------------------------------------------------------o
void cAccountClass::WriteImportHeader(std::fstream &fsOut)
{
	fsOut << "//AIMP3.0" << "-UV" << CVersionClass::GetVersion() << "-BD" << CVersionClass::GetBuild() << "-DS" << time(NULL) << "-ED" << CVersionClass::GetRealBuild() << std::endl;
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
	fsOut << "// UOX3 uses this file to store new accounts that are to be imported on the next" << std::endl;
	fsOut << "// time the server does a world save, or world load." << std::endl;
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
	fsOut << "// FORMAT: " << std::endl;
	fsOut << "//" << std::endl;
	fsOut << "//    USER=username,password,flags,contact" << std::endl;
	fsOut << "//" << std::endl;
	fsOut << "// WHERE: username   - Username of the accounts to create." << std::endl;
	fsOut << "//        password   - Password of the account to create." << std::endl;
	fsOut << "//        flags      - See accounts.adm for correct flag values." << std::endl;
	fsOut << "//        contact    - Usually this is the email address, but can be used as a comment or ICQ" << std::endl;
	fsOut << "//" << std::endl;
	fsOut << "// NOTE: Flags, and contact values are not required, defaults will be used." << std::endl;
	fsOut << "// NOTE: Please ensure you press ENTER after your last line for proper loading." << std::endl;
	fsOut << "//------------------------------------------------------------------------------" << std::endl;
}

}
