#include "uox3.h"
#include "jail.h"
#include "cServerDefinitions.h"
#include "ssection.h"
#include "scriptc.h"
#include "SQLManager.h"

namespace UOX
{

JailSystem *JailSys;

JailCell::~JailCell()
{
	for( size_t i = 0; i < playersInJail.size(); ++i )
	{
		delete playersInJail[i];
	}
	playersInJail.resize( 0 );
}

bool JailCell::IsEmpty( void ) const		
{ 
	return playersInJail.empty(); 
}
size_t JailCell::JailedPlayers( void ) const 
{ 
	return playersInJail.size(); 
}
SI16 JailCell::X( void ) const				
{ 
	return x; 
}
SI16 JailCell::Y( void ) const				
{ 
	return y; 
}
SI08 JailCell::Z( void ) const				
{ 
	return z; 
}
UI08 JailCell::World( void ) const
{
	return world;
}
void JailCell::X( SI16 nVal )				
{ 
	x = nVal; 
}
void JailCell::Y( SI16 nVal )				
{ 
	y = nVal; 
}
void JailCell::Z( SI08 nVal )				
{ 
	z = nVal; 
}
void JailCell::World( UI08 nVal )
{
	world = nVal;
}
void JailCell::AddOccupant( CChar *pAdd, SI32 secsFromNow ) 
{ 
	if( !ValidateObject( pAdd ) )
		return;
	JailOccupant *toAdd = new JailOccupant; 
	time( &(toAdd->releaseTime) ); 
	toAdd->releaseTime += secsFromNow; 
	toAdd->pSerial = pAdd->GetSerial();
	toAdd->x = pAdd->GetX();
	toAdd->y = pAdd->GetY();
	toAdd->z = pAdd->GetZ();
	toAdd->world = pAdd->WorldNumber();
	pAdd->SetLocation( x, y, z, world );
	SendMapChange( pAdd->WorldNumber(), pAdd->GetSocket(), false );
	playersInJail.push_back( toAdd );
}

void JailCell::AddOccupant( JailOccupant *toAdd )
{
	playersInJail.push_back( toAdd );
}

void JailCell::EraseOccupant( size_t occupantID )
{
	if( occupantID >= playersInJail.size() )
		return;
	delete playersInJail[occupantID];
	playersInJail.erase( playersInJail.begin() + occupantID );
}
JailOccupant *JailCell::Occupant( size_t occupantID ) 
{ 
	if( occupantID >= playersInJail.size() )
		return NULL;
	return playersInJail[occupantID];
}
void JailCell::PeriodicCheck( void )
{
	time_t now;
	time( &now );
	for( size_t i = playersInJail.size() - 1; i >= 0 && i < playersInJail.size(); --i )
	{
		if( difftime( now, playersInJail[i]->releaseTime ) >= 0 )
		{	// time to release them
			CChar *toRelease = calcCharObjFromSer( playersInJail[i]->pSerial );
			if( !ValidateObject( toRelease ) )
				EraseOccupant( i );
			else
			{
				toRelease->SetLocation( playersInJail[i]->x, playersInJail[i]->y, playersInJail[i]->z, playersInJail[i]->world );
				toRelease->SetCell( -1 );
				EraseOccupant( i );
			}
		}
	}
}

UString JailCell::WriteData(size_t cellNumber)
{
	std::stringstream Str;
	bool Started = false;
	for (std::vector<JailOccupant *>::const_iterator itr = playersInJail.begin(); itr != playersInJail.end(); ++itr)
		if (*itr != NULL)
		{
			if (Started)
				Str << ",";
			else
			{
				Str << "\nINSERT INTO jail VALUES ";
				Started = true;
			}

			Str << "('" << (*itr)->pSerial << "', ";
			Str << "'" << int((UI08)cellNumber) << "', ";
			Str << "'" << (*itr)->x << "', ";
			Str << "'" << (*itr)->y << "', ";
			Str << "'" << int((SI08)(SI16)(*itr)->z) << "', ";
			Str << "'" << int((SI08)((*itr)->world == NULL ? 0 : (UI08)(*itr)->world)) << "', ";

			if ((*itr)->releaseTime == 0)
				Str << "'0000-00-00 00:00:00')";
			else
				Str << "FROM_UNIXTIME(" << (*itr)->releaseTime << ")";
		}
	return Str.str();
}

JailSystem::JailSystem()
{
	jails.resize( 0 );
}
JailSystem::~JailSystem()
{
	jails.clear();
}
void JailSystem::ReadSetup( void )
{
	ScriptSection *Regions = FileLookup->FindEntry( "JAILS", regions_def );
	if( Regions == NULL )
	{
		jails.resize( 10 );
		jails[0].X( 5276 );		jails[0].Y( 1164 );		jails[0].Z( 0 );
		jails[1].X( 5286 );		jails[1].Y( 1164 );		jails[1].Z( 0 );
		jails[2].X( 5296 );		jails[2].Y( 1164 );		jails[2].Z( 0 );
		jails[3].X( 5306 );		jails[3].Y( 1164 );		jails[3].Z( 0 );
		jails[4].X( 5276 );		jails[4].Y( 1174 );		jails[4].Z( 0 );
		jails[5].X( 5286 );		jails[5].Y( 1174 );		jails[5].Z( 0 );
		jails[6].X( 5296 );		jails[6].Y( 1174 );		jails[6].Z( 0 );
		jails[7].X( 5306 );		jails[7].Y( 1174 );		jails[7].Z( 0 );
		jails[8].X( 5283 );		jails[8].Y( 1184 );		jails[8].Z( 0 );
		jails[9].X( 5304 );		jails[9].Y( 1184 );		jails[9].Z( 0 );
	}
	else
	{
		JailCell toAdd;
		UString tag;
		UString data;
		for( tag = Regions->First(); !Regions->AtEnd(); tag = Regions->Next() )
		{
			if( tag.empty() )
				continue;
			data = Regions->GrabData();
			switch( (tag.data()[0]) )
			{
				case 'X':	toAdd.X( data.toShort() );	break;
				case 'Y':	toAdd.Y( data.toShort() );	break;
				case 'Z':	
							toAdd.Z( data.toByte() );
							jails.push_back( toAdd );
							break;
			}
		}
	}
}
void JailSystem::ReadData( void )
{
	int index;
	if (SQLManager::getSingleton().ExecuteQuery("SELECT serial, cell, oldx, oldy, oldz, world, UNIX_TIMESTAMP(`release`) FROM jail", &index, false))
	{
		int ColumnCount = mysql_num_fields(SQLManager::getSingleton().GetMYSQLResult());
		while (SQLManager::getSingleton().FetchRow(&index))
		{
			JailOccupant toPush;
			UI08 cellNumber = 0;
			bool EmptyColumn = false;
			for(int i = 0; i < ColumnCount; ++i)
			{
				UString value;
				EmptyColumn = SQLManager::getSingleton().GetColumn(i, value, &index) == false ? true : false;
				if (EmptyColumn && (i == 0 || i == 1))
					break;
				else
				{
					switch(i)
					{
					case 0: // jail.serial
						toPush.pSerial = value.toULong();
						break;
					case 1: // jail.cell
						cellNumber = value.toUByte();
						break;
					case 2: // jail.oldx
						toPush.x = value.toShort();
						break;
					case 3: // jail.oldy
						toPush.y = value.toShort();
						break;
					case 4: // jail.oldz
						toPush.z = value.toByte();
						break;
					case 5: // jail.world
						toPush.world = value.toByte();
						break;
					case 6: // jail.release
						toPush.releaseTime = value.toLong();
						break;
					}
				}
			}
			if (!EmptyColumn)
				if (cellNumber < jails.size())
					jails[cellNumber].AddOccupant( &toPush );
				else
					jails[RandomNum(static_cast< size_t >(0), jails.size() - 1)].AddOccupant(&toPush);
		}
		SQLManager::getSingleton().QueryRelease(false);
	}
}
void JailSystem::WriteData( void )
{
	UString uStr = "DELETE FROM jail";
	for (size_t jCtr = 0; jCtr < jails.size(); ++jCtr)
		uStr += jails[jCtr].WriteData(jCtr);

	std::istringstream iss;
	iss.str(uStr);
	uStr.clear();
	while (std::getline(iss, uStr))
		SQLManager::getSingleton().ExecuteQuery(uStr);
}
void JailSystem::PeriodicCheck( void )
{
	std::vector< JailCell >::iterator jIter;
	for( jIter = jails.begin(); jIter != jails.end(); ++jIter )
		(*jIter).PeriodicCheck();
}
void JailSystem::ReleasePlayer( CChar *toRelease )
{
	if( !ValidateObject( toRelease ) )
		return;
	SI08 cellNum = toRelease->GetCell();
	if( cellNum < 0 || cellNum >= static_cast<SI08>(jails.size()) )
		return;
	for( size_t iCounter = 0; iCounter < jails[cellNum].JailedPlayers(); ++iCounter )
	{
		JailOccupant *mOccupant = jails[cellNum].Occupant( iCounter );
		if( mOccupant == NULL )
			continue;
		if( mOccupant->pSerial == toRelease->GetSerial() )
		{
			toRelease->SetLocation( mOccupant->x, mOccupant->y, mOccupant->z, mOccupant->world );
			SendMapChange( mOccupant->world, toRelease->GetSocket(), false );
			toRelease->SetCell( -1 );
			jails[cellNum].EraseOccupant( iCounter );
			return;
		}
	}
}
bool JailSystem::JailPlayer( CChar *toJail, SI32 numSecsToJail )
{
	if( jails.empty() || toJail == NULL )
		return false;
	size_t minCell = 0;
	for( size_t i = 0; i < jails.size(); ++i )
	{
		if( jails[i].IsEmpty() )
		{
			minCell = i;
			break;
		}
		else if( jails[i].JailedPlayers() < jails[minCell].JailedPlayers() )
			minCell = i;
	}

	jails[minCell].AddOccupant( toJail, numSecsToJail );
	toJail->SetCell( minCell );
	return true;
}

}
