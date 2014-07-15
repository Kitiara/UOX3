#include "uox3.h"
#include "classes.h"
#include "regions.h"
#include "ObjectFactory.h"
#if ACT_SQL == 1
#include "SQLManager.h"
#endif

namespace UOX
{

CMapHandler *MapRegion;

int FileSize( std::string filename )
{
	int retVal = 0;

	std::ifstream readDestination;

	readDestination.open( filename.c_str() );					// let's open it 
	if( !( readDestination.eof() || readDestination.fail() ) )
	{
		readDestination.seekg( 0, std::ios::end );
		retVal = readDestination.tellg();
		readDestination.close();
	}

	return retVal;
}

#if ACT_SQL == 1
void LoadChar(std::vector<UString> dataList)
{
	CChar *x = static_cast<CChar *>(ObjectFactory::getSingleton().CreateBlankObject(OT_CHAR));
	if (x == NULL) 
		return;
	if (!x->Load(dataList))
	{
		x->Cleanup();
		ObjectFactory::getSingleton().DestroyObject(x);
	}
}
void LoadItem(std::vector<UString> dataList)
{
	CItem *x = static_cast<CItem *>(ObjectFactory::getSingleton().CreateBlankObject(OT_ITEM));
	if (x == NULL) 
		return;
	if (!x->Load(dataList))
	{
		x->Cleanup();
		ObjectFactory::getSingleton().DestroyObject(x);
	}
}
void LoadMulti(std::vector<UString> dataList)
{
	CMultiObj *ourHouse = static_cast<CMultiObj *>(ObjectFactory::getSingleton().CreateBlankObject(OT_MULTI));
	if (!ourHouse->Load(dataList))	// if no load, DELETE
	{
		ourHouse->Cleanup();
		ObjectFactory::getSingleton().DestroyObject( ourHouse );
	}
}

void LoadBoat(std::vector<UString> dataList)
{
	CBoatObj *ourBoat = static_cast<CBoatObj *>(ObjectFactory::getSingleton().CreateBlankObject(OT_BOAT));
	if (!ourBoat->Load(dataList)) // if no load, DELETE
	{
		ourBoat->Cleanup();
		ObjectFactory::getSingleton().DestroyObject(ourBoat);
	}
}

void LoadSpawnItem(std::vector<UString> dataList)
{
	CSpawnItem *ourSpawner = static_cast<CSpawnItem *>(ObjectFactory::getSingleton().CreateBlankObject(OT_SPAWNER));
	if (!ourSpawner->Load(dataList)) // if no load, DELETE
	{
		ourSpawner->Cleanup();
		ObjectFactory::getSingleton().DestroyObject(ourSpawner);
	}
}
#else
void LoadChar( std::ifstream& readDestination )
{
	CChar *x = static_cast< CChar * >(ObjectFactory::getSingleton().CreateBlankObject( OT_CHAR ));
	if( x == NULL ) 
		return;
	if( !x->Load( readDestination ) )
	{
		x->Cleanup();
		ObjectFactory::getSingleton().DestroyObject( x );
	}
}
void LoadItem( std::ifstream& readDestination )
{
	CItem *x = static_cast< CItem * >(ObjectFactory::getSingleton().CreateBlankObject( OT_ITEM ));
	if( x == NULL ) 
		return;
	if( !x->Load( readDestination ) )
	{
		x->Cleanup();
		ObjectFactory::getSingleton().DestroyObject( x );
	}
}
void LoadMulti( std::ifstream& readDestination )
{
	CMultiObj *ourHouse = static_cast< CMultiObj * >(ObjectFactory::getSingleton().CreateBlankObject( OT_MULTI ));
	if( !ourHouse->Load( readDestination ) )	// if no load, DELETE
	{
		ourHouse->Cleanup();
		ObjectFactory::getSingleton().DestroyObject( ourHouse );
	}
}

void LoadBoat( std::ifstream& readDestination )
{
	CBoatObj *ourBoat = static_cast< CBoatObj * >(ObjectFactory::getSingleton().CreateBlankObject( OT_BOAT ));
	if( !ourBoat->Load( readDestination ) ) // if no load, DELETE
	{
		ourBoat->Cleanup();
		ObjectFactory::getSingleton().DestroyObject( ourBoat );
	}
}

void LoadSpawnItem( std::ifstream& readDestination )
{
	CSpawnItem *ourSpawner = static_cast< CSpawnItem * >(ObjectFactory::getSingleton().CreateBlankObject( OT_SPAWNER ));
	if( !ourSpawner->Load( readDestination ) ) // if no load, DELETE
	{
		ourSpawner->Cleanup();
		ObjectFactory::getSingleton().DestroyObject( ourSpawner );
	}
}
#endif

//o--------------------------------------------------------------------------o
//|	Function		-	void SaveToDisk( std::ofstream& writeDestination, std::ofstream &houseDestination )
//|	Date			-	Unknown
//|	Developers		-	Abaddon
//|	Organization	-	UOX3 DevTeam
//o--------------------------------------------------------------------------o
//|	Description		-	Save all items and characters inside a subregion
//|								reworked SaveChar from WorldMain to deal with pointer based stuff in region rather than index based stuff in array
//|								Also saves out all data regardless (in preparation for a simple binary save)
//o--------------------------------------------------------------------------o
#if ACT_SQL == 0
void CMapRegion::SaveToDisk(std::ofstream& writeDestination, std::ofstream &houseDestination)
{
	charData.Push();
	for (CChar* charToWrite = charData.First(); !charData.Finished(); charToWrite = charData.Next())
	{
		if (!ValidateObject(charToWrite))
		{
			charData.Remove(charToWrite);
			continue;
		}
#if defined( _MSC_VER )
		#pragma todo( "PlayerHTML Dumping needs to be reimplemented" )
#endif
		if (charToWrite->ShouldSave()) 
			charToWrite->Save(writeDestination);
	}
	charData.Pop();
	itemData.Push();
	for (CItem *itemToWrite = itemData.First(); !itemData.Finished(); itemToWrite = itemData.Next())
	{
		if (!ValidateObject(itemToWrite))
		{
			itemData.Remove(itemToWrite);
			continue;
		}

		if (itemToWrite->ShouldSave())
		{
			switch (itemToWrite->GetObjType())
			{
			case OT_MULTI:
				{
					CMultiObj *iMulti = static_cast<CMultiObj *>(itemToWrite);
					iMulti->Save(houseDestination);
				}
				break;
			case OT_BOAT:
				{
					CBoatObj *iBoat = static_cast<CBoatObj *>(itemToWrite);
					iBoat->Save(houseDestination);
				}
				break;
			default:
				itemToWrite->Save(writeDestination);
				break;
			}				
		}
	}
	itemData.Pop();
}
#else
void CMapRegion::SaveToDB()
{
	UString uStr;
	charData.Push();
	for (CChar* charToWrite = charData.First(); !charData.Finished(); charToWrite = charData.Next())
	{
		if (!ValidateObject(charToWrite))
		{
			charData.Remove(charToWrite);
			continue;
		}

		if (charToWrite->ShouldSave()) 
			uStr += charToWrite->Save();
	}
	charData.Pop();
	itemData.Push();
	for (CItem *itemToWrite = itemData.First(); !itemData.Finished(); itemToWrite = itemData.Next())
	{
		if (!ValidateObject(itemToWrite))
		{
			itemData.Remove(itemToWrite);
			continue;
		}

		if (itemToWrite->ShouldSave())
		{
			switch (itemToWrite->GetObjType())
			{
			case OT_MULTI:
				{
					CMultiObj *iMulti = static_cast<CMultiObj *>(itemToWrite);
					UString curr = iMulti->Save();
					if (!curr.empty())
						uStr += curr;
				}
				break;
			case OT_BOAT:
				{
					CBoatObj *iBoat = static_cast<CBoatObj *>(itemToWrite);
					UString curr = iBoat->Save();
					if (!curr.empty())
						uStr += curr;
				}
				break;
			default:
				uStr += itemToWrite->Save();
				break;
			}				
		}
	}
	itemData.Pop();
	
	// Rest will make the saving process ~50% faster.
	auto eachTable = SQLManager::getSingleton().Simplify(uStr);
	for (auto itr = eachTable.begin(); itr != eachTable.end(); ++itr)
		SQLManager::getSingleton().ExecuteQuery(*itr);
}
#endif
//o--------------------------------------------------------------------------o
//|	Function		-	CDataList< CItem * > * GetItemList( void )
//|	Date			-	Unknown
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Returns the Item DataList for iteration
//o--------------------------------------------------------------------------o
CDataList< CItem * > * CMapRegion::GetItemList( void )
{
	return &itemData;
}

//o--------------------------------------------------------------------------o
//|	Function		-	CDataList< CChar * > * GetCharList( void )
//|	Date			-	Unknown
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Returns the Character DataList for iteration
//o--------------------------------------------------------------------------o
CDataList< CChar * > * CMapRegion::GetCharList( void )
{
	return &charData;
}

//o--------------------------------------------------------------------------o
//|	Function		-	CMapWorld Constructor/Destructor
//|	Date			-	17 October, 2005
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Initializes & Clears the MapWorld data
//o--------------------------------------------------------------------------o
CMapWorld::CMapWorld() : upperArrayX( 0 ), upperArrayY( 0 ), resourceX( 0 ), resourceY( 0 )
{
	mapResources.resize( 1 );	// ALWAYS initialize at least one resource region.
	mapRegions.resize( 0 );
}

CMapWorld::CMapWorld( UI08 worldNum )
{
	MapData_st& mMap	= Map->GetMapData( worldNum );
	upperArrayX			= static_cast<SI16>(mMap.xBlock / MapColSize);
	upperArrayY			= static_cast<SI16>(mMap.yBlock / MapRowSize);
	resourceX			= static_cast<UI16>(mMap.xBlock / cwmWorldState->ServerData()->ResOreArea());
	resourceY			= static_cast<UI16>(mMap.yBlock / cwmWorldState->ServerData()->ResOreArea());

	size_t resourceSize = static_cast<size_t>(resourceX * resourceY);
	if( resourceSize < 1 )	// ALWAYS initialize at least one resource region.
		resourceSize = 1;
	mapResources.resize( resourceSize );

	mapRegions.resize( static_cast<size_t>(upperArrayX * upperArrayY) );
}

CMapWorld::~CMapWorld()
{
	mapResources.resize( 0 );
	mapRegions.resize( 0 );
}

//o--------------------------------------------------------------------------o
//|	Function		-	CMapRegion * GetMapRegion( SI16 xOffset, SI16 yOffset )
//|	Date			-	9/13/2005
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Returns the MapRegion based on its x and y offsets
//o--------------------------------------------------------------------------o
CMapRegion * CMapWorld::GetMapRegion( SI16 xOffset, SI16 yOffset )
{
	CMapRegion *mRegion			= NULL;
	const size_t regionIndex	= static_cast<size_t>((xOffset * upperArrayY) + yOffset);

	if( xOffset >= 0 && xOffset < upperArrayX && yOffset >= 0 && yOffset < upperArrayY )
	{
		if( regionIndex < mapRegions.size() )
			mRegion = &mapRegions[regionIndex];
	}

	return mRegion;
}

//o--------------------------------------------------------------------------o
//|	Function		-	Resource_st& GetResource( SI16 x, SI16 y )
//|	Date			-	9/17/2005
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Returns the Resource region x and y is.
//o--------------------------------------------------------------------------o
MapResource_st& CMapWorld::GetResource( SI16 x, SI16 y )
{
	const UI16 gridX = (x / cwmWorldState->ServerData()->ResOreArea());
	const UI16 gridY = (y / cwmWorldState->ServerData()->ResOreArea());

	size_t resIndex = ((gridX * resourceY) + gridY);

	if( gridX >= resourceX || gridY >= resourceY || resIndex > mapResources.size() )
			resIndex = 0;

	return mapResources[resIndex];
}

//o--------------------------------------------------------------------------o
//|	Function		-	void SaveResources( UI08 worldNum )
//|	Date			-	9/17/2005
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Saves the Resource data to disk
//o--------------------------------------------------------------------------o
void CMapWorld::SaveResources( UI08 worldNum )
{
	char wBuffer[2];
	const std::string resourceFile	= cwmWorldState->ServerData()->Directory( CSDDP_SHARED ) + "resource[" + UString::number( worldNum ) + "].bin";
	std::ofstream toWrite( resourceFile.c_str(), std::ios::out | std::ios::trunc | std::ios::binary );

	if( toWrite )
	{
		for( std::vector< MapResource_st >::const_iterator mIter = mapResources.begin(); mIter != mapResources.end(); ++mIter )
		{
			wBuffer[0] = static_cast<char>((*mIter).oreAmt>>8);
			wBuffer[1] = static_cast<char>((*mIter).oreAmt%256);
			toWrite.write( (const char *)&wBuffer, 2 );

			wBuffer[0] = static_cast<char>((*mIter).logAmt>>8);
			wBuffer[1] = static_cast<char>((*mIter).logAmt%256);
			toWrite.write( (const char *)&wBuffer, 2 );
		}
		toWrite.close();
	}
	else // Can't save resources
		Console.Error( "Failed to open resource.bin for writing" );
}

//o--------------------------------------------------------------------------o
//|	Function		-	void LoadResources( UI08 worldNum )
//|	Date			-	9/17/2005
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Loads the Resource data from the disk
//o--------------------------------------------------------------------------o
void CMapWorld::LoadResources( UI08 worldNum )
{
	const SI16 resOre				= cwmWorldState->ServerData()->ResOre();
	const SI16 resLog				= cwmWorldState->ServerData()->ResLogs();
	const UI32 oreTime				= BuildTimeValue( static_cast<R32>(cwmWorldState->ServerData()->ResOreTime() ));
	const UI32 logTime				= BuildTimeValue( static_cast<R32>(cwmWorldState->ServerData()->ResLogTime()) );
	const std::string resourceFile	= cwmWorldState->ServerData()->Directory( CSDDP_SHARED ) + "resource[" + UString::number( worldNum ) + "].bin";

	char rBuffer[2];
	std::ifstream toRead ( resourceFile.c_str(), std::ios::in | std::ios::binary );

	bool fileExists				= ( toRead.is_open() );

	if( fileExists )
		toRead.seekg( 0, std::ios::beg );

	for( std::vector< MapResource_st >::iterator mIter = mapResources.begin(); mIter != mapResources.end(); ++mIter )
	{
		if( fileExists )
		{
			toRead.read( rBuffer, 2 );
			(*mIter).oreAmt = ( (rBuffer[0]<<8) + rBuffer[1] );

			toRead.read( rBuffer, 2 );
			(*mIter).logAmt = ( (rBuffer[0]<<8) + rBuffer[1] );

			fileExists = toRead.eof();
		}
		else
		{
			(*mIter).oreAmt  = resOre;
			(*mIter).logAmt  = resLog;
		}
		// No need to preserve time.  Do a refresh automatically
		(*mIter).oreTime = oreTime;
		(*mIter).logTime = logTime;
	}
	if( fileExists )
		toRead.close();
}

//o--------------------------------------------------------------------------o
//|	Function		-	CMapHandler Constructor/Destructor
//|	Date			-	23 July, 2000
//|	Programmer		-	Abaddon
//o--------------------------------------------------------------------------o
//|	Purpose			-	Fills and clears the mapWorlds vector.
//o--------------------------------------------------------------------------o
CMapHandler::CMapHandler()
{
	mapWorlds.resize( 0 );
	UI08 numWorlds = Map->MapCount();

	mapWorlds.reserve( numWorlds );
	for( UI08 i = 0; i < numWorlds; ++i )
	{
		mapWorlds.push_back( new CMapWorld( i ) );
	}
}

CMapHandler::~CMapHandler()
{
	for( WORLDLIST_ITERATOR mIter = mapWorlds.begin(); mIter != mapWorlds.end(); ++mIter )
	{
		if( (*mIter) != NULL )
		{
			delete (*mIter);
			(*mIter) = NULL;
		}
	}
	mapWorlds.resize( 0 );
}

//o--------------------------------------------------------------------------o
//|	Function		-	bool ChangeRegion( CItem *nItem, SI16 x, SI16 y, UI08 worldNum )
//|	Date			-	February 1, 2006
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Changes an items region ONLY if he has changed regions.
//o--------------------------------------------------------------------------o
bool CMapHandler::ChangeRegion( CItem *nItem, SI16 x, SI16 y, UI08 worldNum )
{
	if( !ValidateObject( nItem ) )
		return false;

	CMapRegion *curCell = GetMapRegion( nItem );
	CMapRegion *newCell = GetMapRegion( GetGridX( x ), GetGridY( y ), worldNum );

	if( curCell != newCell )
	{
		if( !curCell->GetItemList()->Remove( nItem ) )
		{
#if defined( DEBUG_REGIONS )
			Console.Warning( "Item 0x%X does not exist in MapRegion, remove failed", nItem->GetSerial() );
#endif
		}
		if( !newCell->GetItemList()->Add( nItem ) )
		{
#if defined( DEBUG_REGIONS )
			Console.Warning( "Item 0x%X already exists in MapRegion, add failed", nItem->GetSerial() );
#endif
		}
		return true;
	}
	return false;
}

//o--------------------------------------------------------------------------o
//|	Function		-	bool ChangeRegion( CChar *nChar, SI16 x, SI16 y, UI08 worldNum )
//|	Date			-	February 1, 2006
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Changes a characters region ONLY if he has changed regions.
//o--------------------------------------------------------------------------o
bool CMapHandler::ChangeRegion( CChar *nChar, SI16 x, SI16 y, UI08 worldNum )
{
	if( !ValidateObject( nChar ) )
		return false;

	CMapRegion *curCell = GetMapRegion( nChar );
	CMapRegion *newCell = GetMapRegion( GetGridX( x ), GetGridY( y ), worldNum );

	if( curCell != newCell )
	{
		if( !curCell->GetCharList()->Remove( nChar ) )
		{
#if defined( DEBUG_REGIONS )
			Console.Warning( "Character 0x%X does not exist in MapRegion, remove failed", nChar->GetSerial() );
#endif
		}
		if( !newCell->GetCharList()->Add( nChar ) )
		{
#if defined( DEBUG_REGIONS )
			Console.Warning( "Character 0x%X already exists in MapRegion, add failed", nChar->GetSerial() );
#endif
		}
		return true;
	}
	return false;
}

//o--------------------------------------------------------------------------o
//|	Function		-	bool AddItem( CItem *nItem )
//|	Date			-	23 July, 2000
//|	Programmer		-	Abaddon
//o--------------------------------------------------------------------------o
//|	Purpose			-	Adds nItem to the proper SubRegion
//o--------------------------------------------------------------------------o
bool CMapHandler::AddItem( CItem *nItem )
{
	if( !ValidateObject( nItem ) )
		return false;
	CMapRegion *cell = GetMapRegion( nItem );
	if( !cell->GetItemList()->Add( nItem ) )
	{
#if defined( DEBUG_REGIONS )
		Console.Warning( "Item 0x%X already exists in MapRegion, add failed", nItem->GetSerial() );
#endif
		return false;
	}
	return true;
}

//o--------------------------------------------------------------------------o
//|	Function		-	bool RemoveItem( CItem *nItem )
//|	Date			-	23 July, 2000
//|	Programmer		-	Abaddon
//o--------------------------------------------------------------------------o
//|	Purpose			-	Removes nItem from it's CURRENT SubRegion
//|					-	Do this before adjusting the location
//o--------------------------------------------------------------------------o
bool CMapHandler::RemoveItem( CItem *nItem )
{
	if( !ValidateObject( nItem ) )
		return false;
	CMapRegion *cell = GetMapRegion( nItem );
	if( !cell->GetItemList()->Remove( nItem ) )
	{
#if defined( DEBUG_REGIONS )
		Console.Warning( "Item 0x%X does not exist in MapRegion, remove failed", nItem->GetSerial() );
#endif
		return false;
	}
	return true;
}

//o--------------------------------------------------------------------------o
//|	Function		-	bool AddChar( CChar *toAdd )
//|	Date			-	23 July, 2000
//|	Programmer		-	Abaddon
//o--------------------------------------------------------------------------o
//|	Purpose			-	Adds toAdd to the proper SubRegion
//o--------------------------------------------------------------------------o
bool CMapHandler::AddChar( CChar *toAdd )
{
	if( !ValidateObject( toAdd ) )
		return false;
	CMapRegion *cell = GetMapRegion( toAdd );
	if( !cell->GetCharList()->Add( toAdd ) )
	{
#if defined( DEBUG_REGIONS )
		Console.Warning( "Character 0x%X already exists in MapRegion, add failed", toAdd->GetSerial() );
#endif
		return false;
	}
	return true;
}

//o--------------------------------------------------------------------------o
//|	Function		-	bool RemoveChar( CChar *toRemove )
//|	Date			-	23 July, 2000
//|	Programmer		-	Abaddon
//o--------------------------------------------------------------------------o
//|	Purpose			-	Removes toRemove from it's CURRENT SubRegion
//|					-	Do this before adjusting the location
//o--------------------------------------------------------------------------o
bool CMapHandler::RemoveChar( CChar *toRemove )
{
	if( !ValidateObject( toRemove ) )
		return false;
	CMapRegion *cell = GetMapRegion( toRemove );
	if( !cell->GetCharList()->Remove( toRemove ) )
	{
#if defined( DEBUG_REGIONS )
		Console.Warning( "Character 0x%X does not exist in MapRegion, remove failed", toRemove->GetSerial() );
#endif
		return false;
	}
	return true;
}

//o--------------------------------------------------------------------------o
//|	Function		-	SI16 GetGridX( SI16 x )
//|	Date			-	23 July, 2000
//|	Programmer		-	Abaddon
//o--------------------------------------------------------------------------o
//|	Purpose			-	Find the Column of SubRegion we want based on location
//o--------------------------------------------------------------------------o
SI16 CMapHandler::GetGridX( SI16 x )
{
	return static_cast<SI16>(x / MapColSize);
}

//o--------------------------------------------------------------------------o
//|	Function		-	SI16 GetGridY( SI16 y )
//|	Date			-	23 July, 2000
//|	Programmer		-	Abaddon
//o--------------------------------------------------------------------------o
//|	Purpose			-	Finds the Row of SubRegion we want based on location
//o--------------------------------------------------------------------------o
SI16 CMapHandler::GetGridY( SI16 y )
{
	return static_cast<SI16>(y / MapRowSize);
}

//o--------------------------------------------------------------------------o
//|	Function		-	CMapRegion *GetMapRegion( SI16 xOffset, SI16 yOffset, UI08 worldNumber )
//|	Date			-	23 July, 2000
//|	Programmer		-	Abaddon
//o--------------------------------------------------------------------------o
//|	Purpose			-	Returns the MapRegion based on the offsets
//o--------------------------------------------------------------------------o
CMapRegion *CMapHandler::GetMapRegion( SI16 xOffset, SI16 yOffset, UI08 worldNumber )
{
	CMapRegion * mRegion = mapWorlds[worldNumber]->GetMapRegion( xOffset, yOffset );
	if( mRegion == NULL )
		mRegion = &overFlow;

	return mRegion;
}

//o--------------------------------------------------------------------------o
//|	Function		-	CMapRegion *GetMapRegion( SI16 x, SI16 y )
//|	Date			-	23 July, 2000
//|	Programmer		-	Abaddon
//o--------------------------------------------------------------------------o
//|	Purpose			-	Returns the subregion that x,y is in
//o--------------------------------------------------------------------------o
CMapRegion *CMapHandler::GetMapRegion( CBaseObject *mObj )
{
	return GetMapRegion( GetGridX( mObj->GetX() ), GetGridY( mObj->GetY() ), mObj->WorldNumber() );
}

//o--------------------------------------------------------------------------o
//|	Function		-	CMapWorld *	GetMapWorld( UI08 worldNum )
//|	Date			-	9/13/2005
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Returns the MapWorld object associated with the worldNumber
//o--------------------------------------------------------------------------o
CMapWorld *	CMapHandler::GetMapWorld( UI08 worldNum )
{
	CMapWorld *mWorld = NULL;
	if( worldNum > mapWorlds.size() )
		mWorld = mapWorlds[worldNum];
	return mWorld;
}

//o--------------------------------------------------------------------------o
//|	Function		-	CMapWorld *	GetMapWorld( UI08 worldNum )
//|	Date			-	9/13/2005
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Returns the MapWorld object associated with the worldNumber
//o--------------------------------------------------------------------------o
MapResource_st * CMapHandler::GetResource( SI16 x, SI16 y, UI08 worldNum )
{
	MapResource_st *resData = NULL;
	if( worldNum < mapWorlds.size() )
		resData = &mapWorlds[worldNum]->GetResource( x, y );
	return resData;
}

//o--------------------------------------------------------------------------o
//|	Function		-	REGIONLIST PopulateList( CBaseObject *mObj )
//|	Date			-	Unknown
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Creates a list of nearby MapRegions based on the object provided
//o--------------------------------------------------------------------------o
REGIONLIST CMapHandler::PopulateList( CBaseObject *mObj )
{
	return PopulateList( mObj->GetX(), mObj->GetY(), mObj->WorldNumber() );
}

//o--------------------------------------------------------------------------o
//|	Function		-	REGIONLIST PopulateList( SI16 x, SI16 y, UI08 worldNumber )
//|	Date			-	Unknown
//|	Programmer		-	giwo
//o--------------------------------------------------------------------------o
//|	Purpose			-	Creates a list of nearby MapRegions based on the coordinates provided
//o--------------------------------------------------------------------------o
REGIONLIST CMapHandler::PopulateList( SI16 x, SI16 y, UI08 worldNumber )
{
	REGIONLIST nearbyRegions;
	const SI16 xOffset	= MapRegion->GetGridX( x );
	const SI16 yOffset	= MapRegion->GetGridY( y );
	for( SI08 counter1 = -1; counter1 <= 1; ++counter1 )
	{
		for( SI08 counter2 = -1; counter2 <= 1; ++counter2 )
		{
			CMapRegion *MapArea	= GetMapRegion( xOffset + counter1, yOffset + counter2, worldNumber );
			if( MapArea == NULL )
				continue;
			nearbyRegions.push_back( MapArea );
		}
	}
	return nearbyRegions;
}

//o--------------------------------------------------------------------------o
//|	Function		-	Save()
//|	Date			-	23 July, 2000
//|	Programmer		-	Abaddon
//o--------------------------------------------------------------------------o
//|	Purpose			-	Saves out the MapRegions
//o--------------------------------------------------------------------------o
void CMapHandler::Save( void )
{
	const SI16 AreaX = UpperX / 8;	// we're storing 8x8 grid arrays together
	const SI16 AreaY = UpperY / 8;

	std::istringstream issDel;
	std::string deletesql = "DELETE FROM attributes\nDELETE FROM baseobjects\nDELETE FROM boats\n"
		"DELETE FROM characters\nDELETE FROM creatures\nDELETE FROM houses\nDELETE FROM items\n"
		"DELETE FROM spawn_items";
	issDel.str(deletesql);
	while (std::getline(issDel, deletesql))
		SQLManager::getSingleton().ExecuteQuery(deletesql);

	Console << "Saving Character and Item Map Region data...   ";
	UI32 StartTime = getclock();
	SI32 baseX, baseY;
#if ACT_SQL == 1
	for (SI16 counter1 = 0; counter1 < AreaX; ++counter1)   // move left->right
	{ 
		baseX = counter1 * 8;
		for (SI16 counter2 = 0; counter2 < AreaY; ++counter2) // move up->down
		{
			baseY = counter2 * 8;                            // calculate x grid offset
			for (UI08 xCnt = 0; xCnt < 8; ++xCnt)             // walk through each part of the 8x8 grid, left->right
				for (UI08 yCnt = 0; yCnt < 8; ++yCnt)         // walk the row
					for (WORLDLIST_ITERATOR mIter = mapWorlds.begin(); mIter != mapWorlds.end(); ++mIter)
						if (CMapRegion* mRegion = (*mIter)->GetMapRegion((baseX+xCnt), (baseY+yCnt)))
							mRegion->SaveToDB();
		}
	}

	overFlow.SaveToDB();
#else
	Console.TurnYellow();
	std::ofstream writeDestination, houseDestination;
	int onePercent = 0;
	//const int onePercent			= (int)((float)(UpperX*UpperY*Map->MapCount())/100.0f);
	for(UI08 i = 0; i < Map->MapCount(); ++i )
	{
		MapData_st& mMap = Map->GetMapData( i );
		onePercent += (int)(mMap.xBlock / MapColSize) * (mMap.yBlock / MapRowSize);
	}
	onePercent /= 100.0f;

	const char blockDiscriminator[] = "\n\n---REGION---\n\n";
	UI32 count						= 0;

	Console << "0%";

	std::string basePath = cwmWorldState->ServerData()->Directory( CSDDP_SHARED );
	std::string filename = basePath + "house.wsc";
	
	houseDestination.open( filename.c_str() );

	for( SI16 counter1 = 0; counter1 < AreaX; ++counter1 )	// move left->right
	{
		const SI32 baseX = counter1 * 8;
		for( SI16 counter2 = 0; counter2 < AreaY; ++counter2 )	// move up->down
		{
			const SI32 baseY	= counter2 * 8;								// calculate x grid offset
			filename	= basePath + UString::number( counter1 ) + "." + UString::number( counter2 ) + ".wsc";	// let's name our file
			writeDestination.open( filename.c_str() );

			if( !writeDestination ) 
			{
				Console.Error( "Failed to open %s for writing", filename.c_str() );
				continue;
			}

			for( UI08 xCnt = 0; xCnt < 8; ++xCnt )					// walk through each part of the 8x8 grid, left->right
			{
				for( UI08 yCnt = 0; yCnt < 8; ++yCnt )				// walk the row
				{
					for( WORLDLIST_ITERATOR mIter = mapWorlds.begin(); mIter != mapWorlds.end(); ++mIter )
					{
						++count;
						if( count%onePercent == 0 )
						{
							if( count/onePercent <= 10 )
								Console << "\b\b" << (UI32)(count/onePercent) << "%";
							else if( count/onePercent <= 100 )
								Console << "\b\b\b" << (UI32)(count/onePercent) << "%";
						}
						CMapRegion * mRegion = (*mIter)->GetMapRegion( (baseX+xCnt), (baseY+yCnt) );
						if( mRegion != NULL )
							mRegion->SaveToDisk( writeDestination, houseDestination );

						writeDestination << blockDiscriminator;
					}
				}
			}
			writeDestination.close();
		}
	}
	houseDestination.close();

	filename = basePath + "overflow.wsc";
	writeDestination.open( filename.c_str() );

	if( writeDestination.is_open() )
	{
		overFlow.SaveToDisk( writeDestination, writeDestination );
		writeDestination.close();
	}
	else
	{
		Console.Error( "Failed to open %s for writing", filename.c_str() );
		return;
	}
	
	Console << "\b\b\b\b";
	Console.PrintDone();
#endif

	Console.Print("World saved in %.02fsec\n", ((float)(getclock()-StartTime))/1000.0f);

	for (WORLDLIST_ITERATOR wIter = mapWorlds.begin(); wIter != mapWorlds.end(); ++wIter)
		(*wIter)->SaveResources(wIter-mapWorlds.begin());
}

bool PostLoadFunctor( CBaseObject *a, UI32 &b, void *extraData )
{
	if( ValidateObject( a ) )
	{
		if( !a->isFree() )
			a->PostLoadProcessing();
	}
	return true;
}

//o--------------------------------------------------------------------------o
//|	Function		-	Load()
//|	Date			-	23 July, 2000
//|	Programmer		-	Abaddon
//|	Modified		-
//o--------------------------------------------------------------------------o
//|	Purpose			-	Loads in the MapRegions
//o--------------------------------------------------------------------------o
void CMapHandler::Load( void )
{
	UI32 StartTime = getclock();
#if ACT_SQL == 1
	std::string strsql = "SELECT baseobjects.*, %s %s %s FROM baseobjects %s %s %s WHERE baseobjects.type = %u";
	std::string column[3] = {"", "", ""};
	std::string table[3] = {"", "", ""}; // last is not like the others...
	bool Twice = false;
	for (int objtype = OT_CHAR; objtype < OT_SPAWNER+1;)
	{
		for (int i = 0; i < 3; ++i)
		{
			column[i] = "";
			table[i] = "";
		}

		switch (objtype)
		{
		case OT_CHAR: // Characters (Be aware: NPCs are also characters)
			table[0] = "RIGHT JOIN attributes ON baseobjects.serial = attributes.serial";
			column[0] = "guildfealty,speech,privileges,packitem,guildtitle,hunger,brkpeacechancegain,brkpeacechance,attributes.maxhp,maxmana,maxstam,town,summontimer,maylevitate,"
				"stealth,reserved,region,advanceobject,advraceobject,baseskills,guildnumber,fonttype,towntitle,canrun,canattack,allmove,isnpc,isshop,dead,cantrain,"
				"iswarring,guildtoggle,poisonstrength,willhunger,murdertimer,peacetimer,";

			table[1] = "RIGHT JOIN characters ON baseobjects.serial = characters.serial";
			column[1] = "account,robeserial,originalid,hair,beard,townvote,laston,UNIX_TIMESTAMP(laston),orgname,commandlevel,squelched,deaths,fixedlight,townprivileges,atrophy,skilllocks";

			table[2] = "RIGHT JOIN creatures ON baseobjects.serial "+std::string(Twice ? "" : "!")+"= creatures.serial";
			if (Twice)
				column[2] = ",npcaitype,taming,peaceing,provoing,holdg,split,wanderarea,npcwander,spattack,questtype,questregions,fleeat,reattackat,npcflag,mounted,"
					"stabled,tamedhungerrate,tamedhungerwildchance,foodlist,walkingspeed,runningspeed,fleeingspeed";
			break;
		case OT_MULTI: // Houses (Multis)
		case OT_BOAT: // Boats
			table[2] = "RIGHT JOIN houses ON baseobjects.serial = houses.serial"; // we'll use 3rd like this
			column[2] = ",coowner,banned,lockeditem,maxlockeddown,deedname";
			if (objtype == 4)
			{
				table[2] += " RIGHT JOIN boats ON baseobjects.serial = boats.serial"; // we'll use 3rd like this
				column[2] += ",hold,planks,tiller";
			}
		case OT_SPAWNER: // SpawnItems
			if (objtype == 5)
			{
				table[2] = "RIGHT JOIN spawn_items ON baseobjects.serial = spawn_items.serial"; // we'll use 3rd like this
				column[2] = ",spawn_items.interval,spawnsection,issectionalist";
			}
		case OT_ITEM: // Items
			table[1] = "RIGHT JOIN items ON baseobjects.serial = items.serial";
			column[1] = "gridloc,layer,cont,more,creator,morexyz,glow,glowbc,ammo,ammofx,spells,name2,items.desc,items.type,offspell,amount,weightmax,baseweight,items.maxhp,speed,moveable,priv,value,"
				"restock,ac,rank,sk_made,bools,good,glowtype,racedamage,entrymadefrom";
			break;
		}

		size_t size = strsql.size()-12;
		for (int i = 0; i < 3; ++i)
			size += (strlen(column[i].c_str())+strlen(table[i].c_str())*2);
		size += int(floor(log10(double(objtype)))+1);

		char* sql = (char*)malloc(size+1);
		sprintf_s(sql, size+1, strsql.c_str(), column[0].c_str(), column[1].c_str(), column[2].c_str(), table[0].c_str(), table[1].c_str(), table[2].c_str(), objtype);
		int index = 0;
		if (SQLManager::getSingleton().ExecuteQuery(sql, &index, false))
		{
			int ColumnCount = mysql_num_fields(SQLManager::getSingleton().GetMYSQLResult());
			while (SQLManager::getSingleton().FetchRow(&index))
			{
				std::vector<UString> dataList;
				for(int i = 0; i < ColumnCount; ++i)
				{
					UString value;
					bool EmptyColumn = SQLManager::getSingleton().GetColumn(i, value, &index) == false ? true : false;
					dataList.push_back(EmptyColumn == true ? "" : value);
				}

				switch (objtype)
				{
					case OT_CHAR:
						LoadChar(dataList);
						break;
					case OT_ITEM:
						LoadItem(dataList);
						break;
					case OT_MULTI:
						LoadMulti(dataList);
						break;
					case OT_BOAT:
						LoadBoat(dataList);
						break;
					case OT_SPAWNER:
						LoadSpawnItem(dataList);
						break;
				}
			}
			SQLManager::getSingleton().QueryRelease(false);
		}
		free(sql);
		if (objtype == OT_CHAR && !Twice)
			Twice = true;
		else
			++objtype;
	}
#else
	Console.TurnYellow();
	const SI16 AreaX		= UpperX / 8;	// we're storing 8x8 grid arrays together
	const SI16 AreaY		= UpperY / 8;
//	const int onePercent	= (int)((float)(AreaX*AreaY)/100.0f);
	UI32 count				= 0;
	std::ifstream readDestination;
	Console << "0%";
	std::string basePath	= cwmWorldState->ServerData()->Directory( CSDDP_SHARED );
	std::string filename;

	UI32 runningCount = 0;
	int fileSizes[AreaX][AreaY];

	for( SI16 cx = 0; cx < AreaX; ++cx )
	{
		for( SI16 cy = 0; cy < AreaY; ++cy )
		{
			filename			= basePath + UString::number( cx ) + "." + UString::number( cy ) + ".wsc";	// let's name our file
			fileSizes[cx][cy]	= FileSize( filename );
			runningCount		+= fileSizes[cx][cy];
		}
	}

	if( runningCount == 0 )
		runningCount = 1;

	int runningDone			= 0;
	for( SI16 counter1 = 0; counter1 < AreaX; ++counter1 )	// move left->right
	{
		for( SI16 counter2 = 0; counter2 < AreaY; ++counter2 )	// move up->down
		{
			filename	= basePath + UString::number( counter1 ) + "." + UString::number( counter2 ) + ".wsc";	// let's name our file
			readDestination.open( filename.c_str());					// let's open it 
			readDestination.seekg( 0, std::ios::beg );

			if( readDestination.eof() || readDestination.fail() )
			{
				readDestination.close();
				readDestination.clear();
				continue;
			}

			++count;
			LoadFromDisk( readDestination, runningDone, fileSizes[counter1][counter2], runningCount );

			runningDone		+= fileSizes[counter1][counter2];
			float tempVal	= (float)runningDone / (float)runningCount * 100.0f;
			if( tempVal <= 10 )
				Console << "\b\b" << (UI32)(tempVal) << "%";
			else if( tempVal <= 100 )
				Console << "\b\b\b" << (UI32)(tempVal) << "%";

			readDestination.close();
			readDestination.clear();
		}
	}

	Console.TurnNormal();
	Console << "\b\b\b";
	Console.PrintDone();

	filename	= basePath + "overflow.wsc";
	std::ifstream flowDestination( filename.c_str() );
	LoadFromDisk( flowDestination, -1, -1, -1 );
	flowDestination.close();

	filename	= basePath + "house.wsc";
	std::ifstream houseDestination( filename.c_str() );
	LoadFromDisk( houseDestination, -1, -1, -1 );
	houseDestination.close();
#endif
	UI32 b = 0;
	ObjectFactory::getSingleton().IterateOver(OT_MULTI, b, NULL, &PostLoadFunctor);
	ObjectFactory::getSingleton().IterateOver(OT_ITEM, b, NULL, &PostLoadFunctor);
	ObjectFactory::getSingleton().IterateOver(OT_CHAR, b, NULL, &PostLoadFunctor);

	Console.Print("ASCII world loaded in %.02fsec\n", ((float)(getclock()-StartTime))/1000.0f);

	UI08 i = 0;
	for(WORLDLIST_ITERATOR wIter = mapWorlds.begin(); wIter != mapWorlds.end(); ++wIter)
	{
		(*wIter)->LoadResources( i );
		++i;
	}
}

//o--------------------------------------------------------------------------o
//|	Function		-	LoadFromDisk( std::ifstream& readDestination )
//|	Date			-	23 July, 2000
//|	Programmer		-	Abaddon
//|	Modified		-
//o--------------------------------------------------------------------------o
//|	Purpose			-	Loads in objects from specified file
//o--------------------------------------------------------------------------o
#if ACT_SQL == 0
void CMapHandler::LoadFromDisk( std::ifstream& readDestination, int baseValue, int fileSize, int maxSize )
{
	char line[1024];
	float basePercent	= (float)baseValue / (float)maxSize * 100.0f;
	float targPercent	= (float)(baseValue + fileSize) / (float)maxSize * 100.0f;
	float diffValue		= targPercent - basePercent;

	int updateCount		= 0;
	while( !readDestination.eof() && !readDestination.fail() )
	{
		readDestination.getline( line, 1024 );
		UString sLine( line );
		sLine = sLine.removeComment().stripWhiteSpace();
		if( sLine.substr( 0, 1 ) == "[" )	// in a section
		{
			sLine = sLine.substr( 1, sLine.size() - 2 );
			sLine = sLine.upper().stripWhiteSpace();
			if( sLine == "CHARACTER" )
				LoadChar( readDestination );
			else if( sLine == "ITEM" )
				LoadItem( readDestination );
			else if( sLine == "HOUSE" )
				LoadMulti( readDestination );
			else if( sLine == "BOAT" )
				LoadBoat( readDestination );
			else if( sLine == "SPAWNITEM" )
				LoadSpawnItem( readDestination );

			if( fileSize != -1 && (++updateCount)%20 == 0 )
			{
				float curPos	= readDestination.tellg();
				float tempVal	= basePercent + ( curPos / fileSize * diffValue );
				if( tempVal <= 10 )
					Console << "\b\b" << (UI32)(tempVal) << "%";
				else
					Console << "\b\b\b" << (UI32)(tempVal) << "%";
			}
		}
		else if( sLine == "---REGION---" )	// end of region
			continue;
	}
}
#endif
}
