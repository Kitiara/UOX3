//o--------------------------------------------------------------------------o
//| File         - cBaseobject.cpp
//| Date         - 7/26/2000
//| Developers   - Abaddon/EviLDeD
//| Organization - UOX3 DevTeam
//| Status       - Currently under development
//o--------------------------------------------------------------------------o
//| Description  - Version History
//|
//|                1.0        Abaddon        26th July, 2000
//|                Initial implementation.  Most, if not all, common between
//|                items and chars is now implemented here    Functions declared
//|                as const where possible
//|
//|                1.1        EviLDeD        Unknown
//|                Significant fraction of things moved to CBO to support 
//|                future functionality
//|
//|                1.2        Abaddon        August 27th, 2000
//|                Addition of basic script trigger stuff. Function documentation 
//|                finished for all functions
//o--------------------------------------------------------------------------o
#include "uox3.h"
#include "power.h"
#include "CJSMapping.h"
#include "cScript.h"
#include "network.h"
#include "ObjectFactory.h"
#include "weight.h"
#include <bitset>

namespace UOX
{

const UI32 BIT_FREE       = 0;
const UI32 BIT_DELETED    = 1;
const UI32 BIT_POSTLOADED = 2;
const UI32 BIT_SPAWNED    = 3;
const UI32 BIT_SAVE       = 4;
const UI32 BIT_DISABLED   = 5;
const UI32 BIT_WIPEABLE   = 6;


//o--------------------------------------------------------------------------
//| Function   - CBaseObject destructor
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//o--------------------------------------------------------------------------
//| Purpose    - This function is does basically what the name implies
//o--------------------------------------------------------------------------
CBaseObject::~CBaseObject()
{
    if (multis != NULL)
        RemoveFromMulti(false);
    // Delete all tags.
    tags.clear();
}

const ObjectType DEFBASE_OBJTYPE   = OT_CBO;
const RACEID DEFBASE_RACE          = 0;
const SI16 DEFBASE_X               = 100;
const SI16 DEFBASE_Y               = 100;
const SI08 DEFBASE_Z               = 0;
const UI16 DEFBASE_ID              = 1;
const UI16 DEFBASE_COLOUR          = 0;
const UI08 DEFBASE_DIR             = 0;
const SERIAL DEFBASE_SERIAL        = INVALIDSERIAL;
CMultiObj* DEFBASE_MULTIS          = NULL;
const SERIAL DEFBASE_SPAWNSER      = INVALIDSERIAL;
const SERIAL DEFBASE_OWNER         = INVALIDSERIAL;
const UI08 DEFBASE_WORLD           = 0;
const SI16 DEFBASE_STR             = 0;
const SI16 DEFBASE_DEX             = 0;
const SI16 DEFBASE_INT             = 0;
const SI16 DEFBASE_HP              = 1;
const VisibleTypes DEFBASE_VISIBLE = VT_VISIBLE;
const SI16 DEFBASE_HIDAMAGE        = 0;
const SI16 DEFBASE_LODAMAGE        = 0;
const SI32 DEFBASE_WEIGHT          = 0;
const SI16 DEFBASE_MANA            = 1;
const SI16 DEFBASE_STAMINA         = 1;
const UI16 DEFBASE_SCPTRIG         = 0;
const SI16 DEFBASE_STR2            = 0;
const SI16 DEFBASE_DEX2            = 0;
const SI16 DEFBASE_INT2            = 0;
const SI32 DEFBASE_FP              = -1;
const UI08 DEFBASE_POISONED        = 0;
const SI16 DEFBASE_CARVE           = -1;
const SI16 DEFBASE_KARMA           = 0;
const SI16 DEFBASE_FAME            = 0;
const SI16 DEFBASE_KILLS           = 0;
const UI16 DEFBASE_RESIST          = 0;

//o--------------------------------------------------------------------------o
//| Function   - CBaseObject constructor
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//o--------------------------------------------------------------------------o
//| Purpose    - This function is does basically what the name implies
//o--------------------------------------------------------------------------o
CBaseObject::CBaseObject(void) : objType(DEFBASE_OBJTYPE), race(DEFBASE_RACE), x(DEFBASE_X), y(DEFBASE_Y),
z(DEFBASE_Z), id(DEFBASE_ID), colour(DEFBASE_COLOUR), dir(DEFBASE_DIR), serial(DEFBASE_SERIAL), 
multis(DEFBASE_MULTIS), spawnserial(DEFBASE_SPAWNSER), owner(DEFBASE_OWNER), worldNumber(DEFBASE_WORLD),
strength(DEFBASE_STR), dexterity(DEFBASE_DEX), intelligence(DEFBASE_INT), hitpoints(DEFBASE_HP),
visible(DEFBASE_VISIBLE), hidamage(DEFBASE_HIDAMAGE), lodamage(DEFBASE_LODAMAGE), weight(DEFBASE_WEIGHT),
mana(DEFBASE_MANA), stamina(DEFBASE_STAMINA), scriptTrig(DEFBASE_SCPTRIG), st2(DEFBASE_STR2), dx2(DEFBASE_DEX2),
in2(DEFBASE_INT2), FilePosition(DEFBASE_FP), poisoned(DEFBASE_POISONED), carve(DEFBASE_CARVE), oldLocX(0), oldLocY(0),
oldLocZ(0), oldTargLocX(0), oldTargLocY(0),
fame(DEFBASE_FAME), karma(DEFBASE_KARMA), kills(DEFBASE_KILLS)
{
    objSettings.reset();
    name.reserve(MAX_NAME);
    title.reserve(MAX_TITLE);
    if (cwmWorldState != NULL && cwmWorldState->GetLoaded())
        SetPostLoaded(true);
    ShouldSave(true);
    memset(&resistances[0], DEFBASE_RESIST, sizeof(UI16) * WEATHNUM);
}


//o--------------------------------------------------------------------------o
//| Function     - SI32 CBaseObject::GetNumTags(void) const
//| Date         -
//| Developers   -
//| Organization - UOX3 DevTeam
//| Status       - Currently under development
//o--------------------------------------------------------------------------o
//| Description  -
//o--------------------------------------------------------------------------o
size_t CBaseObject::GetNumTags(void) const 
{
    return tags.size();
}

//o--------------------------------------------------------------------------o
//| Function     - jsval CBaseObject::GetTag(std::string tagname) const 
//| Date         -
//| Developers   -
//| Organization - UOX3 DevTeam
//| Status       - Currently under development
//o--------------------------------------------------------------------------o
//| Description  -
//o--------------------------------------------------------------------------o
TAGMAPOBJECT CBaseObject::GetTag(std::string tagname) const 
{
    TAGMAPOBJECT localObject;
    localObject.m_ObjectType = TAGMAP_TYPE_INT;
    localObject.m_IntValue = 0;
    localObject.m_Destroy = FALSE;
    localObject.m_StringValue = "";
    TAGMAP2_CITERATOR CI = tags.find(tagname);
    if (CI != tags.end())
        localObject = CI->second;

    return localObject;
}

//o--------------------------------------------------------------------------o
//| Function      - void CBaseObject::SetTag(std::string tagname, TAGMAPOBJECT tagval) 
//| Date          - Unknown / Feb 3, 2005
//| Developers    - Unknown / EviLDeD
//| Organization  - UOX3 DevTeam
//| Status        - Currently under development
//o--------------------------------------------------------------------------o
//| Description   -
//o--------------------------------------------------------------------------o
//| Modifications - Updated the function to use the internal tagmap object
//|                 instead of using some stored jsval in a context that
//|                 may or may not change when reloaded.
//o--------------------------------------------------------------------------o
void CBaseObject::SetTag(std::string tagname, TAGMAPOBJECT tagval) 
{
    TAGMAP2_ITERATOR I = tags.find(tagname);
    if (I != tags.end())
    {
        // Check to see if this object needs to be destroyed
        if (I->second.m_Destroy || tagval.m_Destroy)
        {
            tags.erase(I);
            return;
        }
        // Change the tag's TAGMAPOBJECT value. NOTE this will also change type should type be changed
        else if (tagval.m_ObjectType == TAGMAP_TYPE_STRING)
        {
            I->second.m_Destroy = FALSE;
            I->second.m_ObjectType = tagval.m_ObjectType;
            I->second.m_StringValue = tagval.m_StringValue;
            // Just because it seemed like a waste to leave it unused. I put the length of the string in the int member
            I->second.m_IntValue = tagval.m_StringValue.length();
        }
        else
        {
            I->second.m_Destroy = FALSE;
            I->second.m_ObjectType = tagval.m_ObjectType;
            I->second.m_StringValue = "";
            I->second.m_IntValue = tagval.m_IntValue;
        }
    }
    else if (!tagval.m_Destroy) // We need to create a TAGMAPOBJECT and initialize and store into the tagmap
        tags[tagname] = tagval;
}

SI16 CBaseObject::GetOldTargLocX(void) const
{
    return oldTargLocX;
}

SI16 CBaseObject::GetOldTargLocY(void) const
{
    return oldTargLocY;
}

void CBaseObject::SetOldTargLocX(SI16 newValue)
{
    oldTargLocX = newValue;
}

void CBaseObject::SetOldTargLocY(SI16 newValue)
{
    oldTargLocY = newValue;
}

//o--------------------------------------------------------------------------
//| Function   - SI16 GetX()
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns X location of object
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetX(void) const
{
    return x;
}

//o--------------------------------------------------------------------------
//| Function   - SI16 GetY()
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns Y location of object
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetY(void) const
{
    return y;
}

//o--------------------------------------------------------------------------
//| Function   - SI08 GetZ()
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns Z location of object
//o--------------------------------------------------------------------------
SI08 CBaseObject::GetZ(void) const
{
    return z;
}

//o--------------------------------------------------------------------------
//| Function   - SetX(SI16 newValue)
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the new X value
//o--------------------------------------------------------------------------
void CBaseObject::SetX(SI16 newValue)
{
    oldLocX = x;
    x        = newValue;
    Dirty(UT_LOCATION);
}

//o--------------------------------------------------------------------------
//| Function   - SetY(SI16 newValue)
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the new Y value
//o--------------------------------------------------------------------------
void CBaseObject::SetY(SI16 newValue)
{
    oldLocY = y;
    y        = newValue;
    Dirty(UT_LOCATION);
}

//o--------------------------------------------------------------------------
//| Function   - SetZ(SI08 newValue)
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the new Z value
//o--------------------------------------------------------------------------
void CBaseObject::SetZ(SI08 newValue)
{
    oldLocZ = z;
    z        = newValue;
    Dirty(UT_LOCATION);
}

void CBaseObject::WalkXY(SI16 newX, SI16 newY)
{
    oldLocX = x;
    oldLocY = y;
    x = newX;
    y = newY;
}

//o---------------------------------------------------------------------------o
//|   Function    -  UI16 Resist()
//|   Date        -  19. Mar, 2006
//| Programmer - Grimson
//o---------------------------------------------------------------------------o
//| Purpose    - Set and Get the damage resist values
//o---------------------------------------------------------------------------o
void CBaseObject::SetResist(UI16 newValue, WeatherType damage)
{
    resistances[damage] = newValue;
}

UI16 CBaseObject::GetResist(WeatherType damage) const
{
    return resistances[damage];
}

//o--------------------------------------------------------------------------
//| Function   - UI16 GetID()
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the ID of the object
//o--------------------------------------------------------------------------
UI16 CBaseObject::GetID(void) const
{
    return id;
}

//o--------------------------------------------------------------------------
//| Function   - UI16 GetColour()
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the colour of the object
//o--------------------------------------------------------------------------
UI16 CBaseObject::GetColour(void) const
{
    return colour;
}

//o--------------------------------------------------------------------------
//| Function   - UI08 GetID(UI08 part)
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns part of the ID
//o--------------------------------------------------------------------------
UI08 CBaseObject::GetID(UI08 part) const
{
    UI08 rvalue = static_cast<UI08>(id>>8);
    if (part == 2)
        rvalue = static_cast<UI08>(id%256);
    return rvalue;
}

//o--------------------------------------------------------------------------
//| Function   - UI08 GetColour(UI08 part)
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns part of the colour
//o--------------------------------------------------------------------------
UI08 CBaseObject::GetColour(UI08 part) const
{
    UI08 rvalue = static_cast<UI08>(colour>>8);
    if (part == 2)
        rvalue = static_cast<UI08>(colour%256);
    return rvalue;
}

//o--------------------------------------------------------------------------
//| Function   - SetID(UI16 newValue)
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the ID
//o--------------------------------------------------------------------------
void CBaseObject::SetID(UI16 newValue)
{
    CBaseObject *checkCont = NULL;
    if (isPostLoaded() && CanBeObjType(OT_ITEM))
        checkCont = (static_cast<CItem *>(this))->GetCont();

    if (ValidateObject(checkCont))
        Weight->subtractItemWeight(checkCont, static_cast<CItem *>(this));

    id = newValue;

    if (ValidateObject(checkCont))
        Weight->addItemWeight(checkCont, static_cast<CItem *>(this));

    Dirty(UT_HIDE);
}

//o--------------------------------------------------------------------------
//| Function   - SetColour(UI16 newValue)
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the colour to newValue
//o--------------------------------------------------------------------------
void CBaseObject::SetColour(UI16 newValue)
{
    colour = newValue;
    Dirty(UT_UPDATE);
}

//o--------------------------------------------------------------------------
//| Function   - SetID(UI08 newValue, UI08 part)
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets part of the ID
//o--------------------------------------------------------------------------
void CBaseObject::SetID(UI08 newValue, UI08 part)
{
    if (part <= 2 && part > 0)
    {
        UI08 parts[2];
        parts[0] = static_cast<UI08>(id>>8);
        parts[1] = static_cast<UI08>(id%256);
        parts[part-1] = newValue;
        SetID(static_cast<UI16>((parts[0]<<8) + parts[1]));
    }
    else
        Console << "Invalid part requested on SetID" << myendl;
}

//o---------------------------------------------------------------------------o
//| Function   - SI32 Weight(void) const
//| Date       - Unknown
//| Programmer - Abaddon
//o---------------------------------------------------------------------------o
//| Purpose    - Weight of the CHARACTER
//o---------------------------------------------------------------------------o
SI32 CBaseObject::GetWeight(void) const
{
    return weight;
}

//o--------------------------------------------------------------------------
//| Function   - CBaseObject *GetMultiObj()
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns multi object we're within
//o--------------------------------------------------------------------------
CMultiObj *CBaseObject::GetMultiObj(void) const
{
    return multis;
}

SERIAL CBaseObject::GetMulti(void) const
{
    SERIAL multiSer = INVALIDSERIAL;
    if (ValidateObject(multis))
        multiSer = multis->GetSerial();

    return multiSer;
}

//o--------------------------------------------------------------------------
//| Function   - SERIAL GetSerial()
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns serial of the item
//o--------------------------------------------------------------------------
SERIAL CBaseObject::GetSerial(void) const
{
    return serial;
}

//o--------------------------------------------------------------------------
//| Function   - SERIAL GetSpawn()
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns SERIAL of thing that spawned it
//o--------------------------------------------------------------------------
SERIAL CBaseObject::GetSpawn(void) const
{
    return spawnserial;
}

//o--------------------------------------------------------------------------
//| Function   - CItem *GetSpawnObj()
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns thing that spawned us - cannot be a character!
//o--------------------------------------------------------------------------;
CSpawnItem *CBaseObject::GetSpawnObj(void) const
{
    CSpawnItem *ourSpawner = static_cast<CSpawnItem *>(calcItemObjFromSer(spawnserial));
    if (ValidateObject(ourSpawner) && ourSpawner->GetObjType() == OT_SPAWNER)
        return ourSpawner;
    return NULL;
}

//o--------------------------------------------------------------------------
//| Function   - SERIAL GetOwner()
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns SERIAL of thing that owns us
//o--------------------------------------------------------------------------
SERIAL CBaseObject::GetOwner(void) const
{
    return owner;
}

//o--------------------------------------------------------------------------
//| Function   - CBaseObject *GetOwnerObj()
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns thing that owns us
//o--------------------------------------------------------------------------
CChar *CBaseObject::GetOwnerObj(void) const
{
    return calcCharObjFromSer(owner);
}

//o--------------------------------------------------------------------------
//| Function   - SetMulti(SERIAL newSerial)
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets serial of item we're within
//o--------------------------------------------------------------------------
void CBaseObject::SetMulti(SERIAL newSerial, bool fireTrigger)
{
    RemoveFromMulti(fireTrigger);
    if (newSerial >= BASEITEMSERIAL)
    {
        CMultiObj *newMulti = calcMultiFromSer(newSerial);
        if (ValidateObject(newMulti))
        {
            multis = newMulti;
            AddToMulti(fireTrigger);
        }
        else
            multis = NULL;
    }
}

//o--------------------------------------------------------------------------
//| Function   - SetSerial(SERIAL newSerial)
//| Date       - 26 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets serial of item we are
//o--------------------------------------------------------------------------
void CBaseObject::SetSerial(SERIAL newSerial)
{
    if (GetSerial() != INVALIDSERIAL)
        ObjectFactory::getSingleton().UnregisterObject(this);
    serial = newSerial;
    if (newSerial != INVALIDSERIAL)
        ObjectFactory::getSingleton().RegisterObject(this, newSerial);
}


//o--------------------------------------------------------------------------
//| Function   - DumpBody(ofstream &outStream) const
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Dumps out body information of the object
//|              This is tag/data pairing information
//o--------------------------------------------------------------------------
std::stringstream CBaseObject::DumpBody() const
{
    std::stringstream Str;
    Str << "INSERT INTO baseobjects VALUES (";
    Str << "'" << objType << "', ";
    Str << "'" << serial << "', ";
    Str << "'" << id << "', ";
    Str << "'" << colour << "', ";
    Str << "'" << int((UI08)(SI16)dir) << "', ";
    if (ValidateObject(multis))
    {
        size_t size = Str.str().size();
        try
        {
            std::stringstream Strserial;
            if (multis->GetSerial() == DEFBASE_SERIAL)
                Str << "NULL, ";
            else
                Str << "'" << multis->GetSerial() << "', ";
        }
        catch(...)
        {
            if (size == Str.str().size())
                Str << "NULL, ";
            Console << "EXCEPTION: CBaseObject::DumpBody(" << name << "[" << serial << "]) - 'MultiID' points to invalid memory." << myendl;
        }
    }
    else
        Str << "NULL, ";

    if (spawnserial == DEFBASE_SPAWNSER)
        Str << "NULL, ";
    else
        Str << "'" << spawnserial << "', ";

    if (owner == DEFBASE_OWNER)
        Str << "NULL, ";
    else
        Str << "'" << owner << "', ";

    if (std::string("#").compare(name) == 0 || name.empty())
        Str << "NULL, ";
    else
        Str << "\"" << name << "\", ";

    Str << "'" << x << "," << y << "," << (SI16)z << "," << (SI16)worldNumber << "', ";

    if (title.empty())
        Str << "NULL, ";
    else
        Str << "\"" << title << "\", ";

    //=========== BUG (= For Characters the dex+str+int malis get saved and get rebuilt on next serverstartup = increasing malis)
    SI16 temp_st2, temp_dx2, temp_in2;
    temp_st2 = st2;
    temp_dx2 = dx2;
    temp_in2 = in2;

    if (objType == OT_CHAR)
    {
        CChar *myChar = (CChar*)(this);

        // For every equipped item 
        // We need to reduce Str2+Dex2+Int2
        for (CItem *myItem = myChar->FirstItem(); !myChar->FinishedItems(); myItem = myChar->NextItem())
            if (ValidateObject(myItem))
            {
                temp_st2 -= myItem->GetStrength2();
                temp_dx2 -= myItem->GetDexterity2();
                temp_in2 -= myItem->GetIntelligence2();
            }
    }
    //=========== BUGFIX END (by Dark-Storm)

    Str << "'" << weight << "', ";
    Str << "'" << mana << "', ";
    Str << "'" << stamina << "', ";
    Str << "'" << dexterity << ", " << temp_dx2 << "', ";
    Str << "'" << intelligence << ", " << temp_in2 << "', ";
    Str << "'" << strength << ", " << temp_st2 << "', ";
    Str << "'" << hitpoints << "', ";
    Str << "'" << race << "', ";
    Str << "'" << int((SI08)(SI16)visible) << "', ";
    Str << "'" << (isDisabled() ? 1 : 0) << "', ";
    Str << "'" << lodamage << "," << hidamage << "', ";
    Str << "'" << int((UI08)(SI16)poisoned) << "', ";
    Str << "'" << GetCarve() << "', ";

    Str << "'";
    for (UI08 resist = 1; resist < WEATHNUM; ++resist)
    {
        UI16 res = GetResist((WeatherType)resist);
        Str << (res > 9 ? "" : "0") << res;
        if (resist != WEATHNUM-1)
            Str << ",";
    }

    Str << "', '" << scriptTrig << "', ";
    Str << "'" << GetFame() << "," << GetKarma() << "," << GetKills() << "', ";

    if (tags.empty())
        Str << "NULL";
    else
    {
        Str << "'";
        for (TAGMAP2_CITERATOR CI = tags.begin(); CI != tags.end(); ++CI)
        {
            Str << CI->first << ",";
            if (CI->second.m_ObjectType != TAGMAP_TYPE_STRING)
                Str << ((SI32)CI->second.m_IntValue) << ",";
            else
                Str << "," << CI->second.m_StringValue;
        }
        Str << "'";
    }
    Str << ")\n";
    return Str;
}

//o--------------------------------------------------------------------------
//| Function   - RACEID GetRace()
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the associated race of the object
//o--------------------------------------------------------------------------
RACEID CBaseObject::GetRace(void) const
{
    return race;
}

//o--------------------------------------------------------------------------
//| Function   - SetRace(RACEID newRace)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the objects Race property
//o--------------------------------------------------------------------------
void CBaseObject::SetRace(RACEID newValue)
{
    race = newValue;
}

//o--------------------------------------------------------------------------
//| Function   - std::string GetName()
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the name of the object
//o--------------------------------------------------------------------------
std::string CBaseObject::GetName(void) const
{
    return name;
}

//o--------------------------------------------------------------------------
//| Function   - SetName(std::string newName)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the name of the object
//o--------------------------------------------------------------------------
void CBaseObject::SetName(std::string newName)
{
    name = newName.substr(0, MAX_NAME - 1);
    Dirty(UT_UPDATE);
}

//o--------------------------------------------------------------------------
//| Function   - SI16 GetStrength()
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the strength of the object
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetStrength(void) const
{
    return strength;
}

//o--------------------------------------------------------------------------
//| Function   - SI16 GetDexterity()
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the dexterity of the object
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetDexterity(void) const
{
    return dexterity;
}

//o--------------------------------------------------------------------------
//| Function   - SI16 GetIntelligence()
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the intelligence of the object
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetIntelligence(void) const
{
    return intelligence;
}

//o--------------------------------------------------------------------------
//| Function   - SI16 GetHP()
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the hitpoints of the object
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetHP(void) const
{
    return hitpoints;
}

//o--------------------------------------------------------------------------
//| Function   - SetStrength(SI16 newValue)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the strength of the obhect
//o--------------------------------------------------------------------------
void CBaseObject::SetStrength(SI16 newValue)
{
    strength = newValue;
}

//o--------------------------------------------------------------------------
//| Function   - SetDexterity(SI16 newValue)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the dexterity of the object
//o--------------------------------------------------------------------------
void CBaseObject::SetDexterity(SI16 newValue)
{
    dexterity = newValue;
}

//o--------------------------------------------------------------------------
//| Function   - SetIntelligence(SI16 newValue)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the intelligence of the object
//o--------------------------------------------------------------------------
void CBaseObject::SetIntelligence(SI16 newValue)
{
    intelligence = newValue;
}

//o--------------------------------------------------------------------------
//| Function   - SetHP(SI16 newValue)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the hitpoints of the object
//o--------------------------------------------------------------------------
void CBaseObject::SetHP(SI16 newValue)
{
    hitpoints = newValue;
}

//o--------------------------------------------------------------------------
//| Function   - IncHP(SI16 amtToChange)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Increments the hitpoints of the object
//o--------------------------------------------------------------------------
void CBaseObject::IncHP(SI16 amtToChange)
{
    SetHP(hitpoints + amtToChange);
}

//o--------------------------------------------------------------------------
//| Function   - SetDir(UI08 newDir)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the direction of the object
//o--------------------------------------------------------------------------
void CBaseObject::SetDir(UI08 newDir)
{
    dir = newDir;
    Dirty(UT_UPDATE);
}

//o--------------------------------------------------------------------------
//| Function   - UI08 GetDir()
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the direction of the object
//o--------------------------------------------------------------------------
UI08 CBaseObject::GetDir(void) const
{
    return dir;
}

//o--------------------------------------------------------------------------
//| Function   - SetVisible(VisibleTypes newValue)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the visibility property of the object
//|              Generally it is
//|               0 = Visible
//|               1 = Temporary Hidden (Skill, Item visible to owner)
//|               2 = Invisible (Magic Invis)
//|               3 = Permanent Hidden (GM Hide)
//o--------------------------------------------------------------------------
void CBaseObject::SetVisible(VisibleTypes newValue)
{
    visible = newValue;
    Dirty(UT_HIDE);
}

//o--------------------------------------------------------------------------
//| Function   - char GetVisible()
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the visibility property of the object
//o--------------------------------------------------------------------------
VisibleTypes CBaseObject::GetVisible(void) const
{
    return visible;
}

//o--------------------------------------------------------------------------
//| Function   - ObjcetType GetObjType()
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns an ObjectType that indicates the item's type
//o--------------------------------------------------------------------------
ObjectType CBaseObject::GetObjType(void) const
{
    return objType;
}

//o--------------------------------------------------------------------------
//| Function   - bool CanBeObjType()
//| Date       - 24 June, 2004
//| Programmer - Maarc
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Indicates whether an object can behave as a
//|              particular type
//o--------------------------------------------------------------------------
bool CBaseObject::CanBeObjType(ObjectType toCompare) const
{
    if (toCompare == OT_CBO)
        return true;
    return false;
}

//o--------------------------------------------------------------------------
//| Function   - RemoveFromMulti(bool fireTrigger)
//| Date       - 15 December, 2001
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Removes object from a multi, selectively firing the trigger
//o--------------------------------------------------------------------------
void CBaseObject::RemoveFromMulti(bool fireTrigger)
{
    if (ValidateObject(multis))
        if (multis->CanBeObjType(OT_MULTI))
        {
            multis->RemoveFromMulti(this);
            if (fireTrigger)
            {
                cScript *onLeaving = JSMapping->GetScript(GetScriptTrigger());
                if (onLeaving != NULL)
                    onLeaving->OnLeaving(multis, this);
            }
        }
        else
            Console.Error("Object of type %i with serial 0x%X has a bad multi setting of %i", GetObjType(), serial, multis->GetSerial());
}

//o--------------------------------------------------------------------------
//| Function   - AddToMulti(void)
//| Date       - 15 December, 2001
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Adds object to multi
//o--------------------------------------------------------------------------
void CBaseObject::AddToMulti(bool fireTrigger)
{
    if (CanBeObjType(OT_MULTI))
    {
        multis = NULL;
        return;
    }

    if (ValidateObject(multis))
        if (multis->CanBeObjType(OT_MULTI))
        {
            multis->AddToMulti(this);
            if (fireTrigger)
            {
                cScript *onEntrance = JSMapping->GetScript(GetScriptTrigger());
                if (onEntrance != NULL)
                    onEntrance->OnEntrance(multis, this);
            }
        }
        else
            Console.Error("Object of type %i with serial 0x%X has a bad multi setting of %X", GetObjType(), serial, multis->GetSerial());
}

//o--------------------------------------------------------------------------
//| Function   - SetMulti(CBaseObject *newMulti)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the object's multi to newMulti
//o--------------------------------------------------------------------------
void CBaseObject::SetMulti(CMultiObj *newMulti, bool fireTrigger)
{
    RemoveFromMulti(fireTrigger);
    multis = newMulti;
    AddToMulti(fireTrigger);
}

//o--------------------------------------------------------------------------
//| Function   - SetSpawn(CBaseObject *newSpawn)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the object's spawner to newSpawn
//o--------------------------------------------------------------------------
void CBaseObject::SetSpawn(SERIAL newSpawn)
{
    CSpawnItem *ourSpawner = GetSpawnObj();
    if (ourSpawner != NULL)
        ourSpawner->spawnedList.Remove(this);
    spawnserial = newSpawn;
    if (newSpawn != INVALIDSERIAL)
    {
        ourSpawner = GetSpawnObj();
        if (ourSpawner != NULL)
            ourSpawner->spawnedList.Add(this);
    }
}

//o--------------------------------------------------------------------------
//| Function   - SetOwner(CBaseObject *newOwner)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the object's owner to newOwner
//o--------------------------------------------------------------------------
void CBaseObject::SetOwner(CChar *newOwner)
{
    RemoveSelfFromOwner();
    if (ValidateObject(newOwner))
        owner = newOwner->GetSerial();
    else
        owner = INVALIDSERIAL;
    AddSelfToOwner();
}

//o--------------------------------------------------------------------------
//| Function   - UI08 GetSerial(UI08 part)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns part of a serial #
//o--------------------------------------------------------------------------
UI08 CBaseObject::GetSerial(UI08 part) const
{
    switch(part)
    {
        case 1: return static_cast<UI08>(serial>>24);
        case 2: return static_cast<UI08>(serial>>16);
        case 3: return static_cast<UI08>(serial>>8);
        case 4: return static_cast<UI08>(serial%256);
    }
    return 0;
}

//o--------------------------------------------------------------------------
//| Function   - UCHAR GetSpawn(UCHAR part)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns part of the item's spawner serial
//o--------------------------------------------------------------------------
UI08 CBaseObject::GetSpawn(UI08 part) const
{
    switch(part)
    {
        case 1: return static_cast<UI08>(spawnserial>>24);
        case 2: return static_cast<UI08>(spawnserial >> 16);
        case 3: return static_cast<UI08>(spawnserial >> 8);
        case 4: return static_cast<UI08>(spawnserial % 256);
    }
}

//o--------------------------------------------------------------------------
//| Function   - SI16 GetHiDamage(void)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the high amount of damage that it can do
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetHiDamage(void) const
{
    return hidamage;
}

//o--------------------------------------------------------------------------
//| Function   - SI16 GetLoDamage(void)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the low amount of damage that it can do
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetLoDamage(void) const
{
    return lodamage;
}

//o--------------------------------------------------------------------------
//| Function   - SetHiDamage(SI16 newValue)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the object's high damage value
//o--------------------------------------------------------------------------
void CBaseObject::SetHiDamage(SI16 newValue)
{
    hidamage = newValue;
}

//o--------------------------------------------------------------------------
//| Function   - SetLoDamage(SI16 newValue)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the object's low damage value
//o--------------------------------------------------------------------------
void CBaseObject::SetLoDamage(SI16 newValue)
{
    lodamage = newValue;
}

//o--------------------------------------------------------------------------
//| Function   - SI32 SetFilePosition(LONG filepos)
//| Date       - Unknown
//| Programmer - EviLDeD
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the object's file position
//o--------------------------------------------------------------------------
SI32 CBaseObject::SetFilePosition(SI32 filepos)
{
    FilePosition = filepos;
    return FilePosition;
}

//o--------------------------------------------------------------------------
//| Function   - SI32 GetFilePosition(void)
//| Date       - Unknown
//| Programmer - EviLDeD
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the object's file position
//o--------------------------------------------------------------------------
SI32 CBaseObject::GetFilePosition(void) const
{
    return FilePosition;
}

//o--------------------------------------------------------------------------
//| Function   - SI16 GetStamina(void)
//| Date       - unknown
//| Programmer - EviLDeD
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the object's stamina
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetStamina(void) const
{
    return stamina;
}

//o--------------------------------------------------------------------------
//| Function   - GetStamina(SI16 stam)
//| Date       - unknown
//| Programmer - EviLDeD
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the object's stamina
//o--------------------------------------------------------------------------
void CBaseObject::SetStamina(SI16 stam)
{
    stamina = stam;
}

//o--------------------------------------------------------------------------
//| Function   - SI16 GetMana(void)
//| Date       - unknown
//| Programmer - EviLDeD
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the object's mana
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetMana(void) const 
{
    return mana;
}

//o--------------------------------------------------------------------------
//| Function   - SetMana(SI16 mn)
//| Date       - unknown
//| Programmer - EviLDeD
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the object's mana
//o--------------------------------------------------------------------------
void CBaseObject::SetMana(SI16 mn)
{
    mana = mn;
}

//o--------------------------------------------------------------------------
//| Function   - std::string GetTitle(void)
//| Date       - unknown
//| Programmer - EviLDeD
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the object's title
//o--------------------------------------------------------------------------
std::string CBaseObject::GetTitle(void) const
{
    return title;
}

//o--------------------------------------------------------------------------
//| Function   - SetTitle(std::string newtitle)
//| Date       - unknown
//| Programmer - EviLDeD
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the object's title
//o--------------------------------------------------------------------------
void CBaseObject::SetTitle(std::string newtitle)
{
    title = newtitle.substr(0, MAX_TITLE - 1);
}

//o--------------------------------------------------------------------------
//| Function   - UI16 GetScriptTrigger(void)
//| Date       - August 27th, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the object's script trigger value
//o--------------------------------------------------------------------------
UI16 CBaseObject::GetScriptTrigger(void) const
{
    return scriptTrig;
}

//o--------------------------------------------------------------------------
//| Function   - SetScriptTrigger(UI16 newValue)
//| Date       - August 27th, 2000
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the object's script trigger value
//o--------------------------------------------------------------------------
void CBaseObject::SetScriptTrigger(UI16 newValue)
{
    scriptTrig = newValue;
}

//o--------------------------------------------------------------------------
//| Function   - point3 GetLocation(void)
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns a point3 structure pointing to the object's 
//|              current location
//o--------------------------------------------------------------------------
point3 CBaseObject::GetLocation(void) const
{
    return point3(x, y, z);
}

//o--------------------------------------------------------------------------
//| Function   - SI16 Strength2(void)
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the second strength var associated with the 
//|              object.  For chars, it's the bonuses 
//|              (via armour and such)
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetStrength2(void) const
{
    return st2;
}

//o--------------------------------------------------------------------------
//| Function   - SI16 Dexterity2(void)
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the second dexterity var associated with the 
//|              object.  For chars, it's the bonuses 
//|              (via armour and such)
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetDexterity2(void) const
{
    return dx2;
}

//o--------------------------------------------------------------------------
//| Function   - SI16 Intelligence2(void)
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the second intelligence var associated with
//|              the object.  For chars, it's the bonuses 
//|              (via armour and such)
//o--------------------------------------------------------------------------
SI16 CBaseObject::GetIntelligence2(void) const
{
    return in2;
}

//o--------------------------------------------------------------------------
//| Function   - void Strength2(SI16 nVal)
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the second strength var associated with the
//|              object.  For chars, it's the bonuses 
//|              (via armour and such)
//o--------------------------------------------------------------------------
void CBaseObject::SetStrength2(SI16 nVal)
{
    st2 = nVal;
}

//o--------------------------------------------------------------------------
//| Function   - void Dexterity2(SI16 nVal)
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the second dexterity var associated with the 
//|              object.  For chars, it's the bonuses 
//|              (via armour and such)
//o--------------------------------------------------------------------------
void CBaseObject::SetDexterity2(SI16 nVal)
{
    dx2 = nVal;
}

//o--------------------------------------------------------------------------
//| Function   - void Intelligence2(SI16 nVal)
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the second intelligence var associated with
//|              the object.  For chars, it's the bonuses 
//|              (via armour and such)
//o--------------------------------------------------------------------------
void CBaseObject::SetIntelligence2(SI16 nVal)
{
    in2 = nVal;
}

//o--------------------------------------------------------------------------
//| Function   - void IncStrength(SI16 toInc = 1)
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Increments the object's strength value
//o--------------------------------------------------------------------------
void CBaseObject::IncStrength(SI16 toInc)
{
    SetStrength(strength + toInc);
}

//o--------------------------------------------------------------------------
//| Function   - void IncDexterity(SI16 toInc = 1)
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Increments the object's dexterity value
//o--------------------------------------------------------------------------
void CBaseObject::IncDexterity(SI16 toInc)
{
    SetDexterity(dexterity + toInc);
}

//o--------------------------------------------------------------------------
//| Function   - void IncIntelligence(SI16 toInc = 1)
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Increments the object's intelligence value
//o--------------------------------------------------------------------------
void CBaseObject::IncIntelligence(SI16 toInc)
{
    SetIntelligence(intelligence + toInc);
}

//o--------------------------------------------------------------------------
//| Function   - bool Load(std::ifstream &inStream)
//| Date       - 28 July, 2000
//| Programmer - Abaddon
//| Modified   - Zippy (1/9/02) no longer needs mode
//o--------------------------------------------------------------------------
//| Purpose    - Loads object from disk based on mode
//o--------------------------------------------------------------------------
void ReadWorldTagData(std::ifstream &inStream, UString &tag, UString &data);

bool CBaseObject::Load(std::vector<UString> dataList)
{
    HandleLine(dataList);
    return LoadRemnants();
}
//o--------------------------------------------------------------------------
//| Function   - bool HandleLine(UString &tag, UString &data)
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Used to handle world lines.  Returns true if the tag
//|              is known.  If known, internal information updated
//|              and load routine continues to next tag.  Otherwise,
//|              passed up inheritance tree (if any)
//o--------------------------------------------------------------------------
void CBaseObject::HandleLine(std::vector<UString> dataList)
{
    for (std::vector<UString>::iterator itr = dataList.begin(); itr != dataList.end(); ++itr)
    {
        switch (itr-dataList.begin())
        {
        case 0: // baseobjects.type
            if (itr->empty())
            {
                Console.Error("There is a problem with a baseobject data, type with null isn't a valid value.");
                itr = dataList.end();
            }
            break;
        case 1: // baseobjects.serial
            serial = itr->toULong();
            break;
        case 2: // baseobjects.id
            id = itr->toUShort();
            break;
        case 3: // baseobjects.colour
            colour = itr->toUShort();
            break;
        case 4: // baseobjects.direction
            dir = itr->toUByte();
            break;
        case 5: // baseobjects.multiid
            multis = itr->empty() ? NULL : (CMultiObj *)itr->toULong();
            break;
        case 6: // baseobjects.spawnerid
            spawnserial = itr->empty() ? DEFBASE_SPAWNSER : itr->toULong();
            break;
        case 7: // baseobjects.ownerid
            owner = itr->empty() ? DEFBASE_OWNER : itr->toULong();
            break;
        case 8: // baseobjects.name
            name = itr->empty() ? "#" : itr->substr(0, MAX_NAME);
            break;
        case 9: // baseobjects.location
            x = DEFBASE_X;
            y = DEFBASE_Y;
            z = DEFBASE_Z;
            worldNumber = DEFBASE_WORLD;
            if (!itr->empty())
                for (int i = 0; i < itr->sectionCount(",")+1; ++i)
                {
                    UString uStr = itr->section(",", i, i).stripWhiteSpace();
                    if (!uStr.empty())
                        if (i == 0)
                            x = uStr.toShort();
                        else if (i == 1)
                            y = uStr.toShort();
                        else if (i == 2)
                            z = uStr.toByte();
                        else if (i == 3)
                            worldNumber = uStr.toUByte();
                }
            break;
        case 10: // baseobjects.title
            title = itr->empty() ? "" : itr->substr(0, MAX_TITLE);
            break;
        case 11: // baseobjects.weight
            SetWeight(itr->toLong());
            break;
        case 12: // baseobjects.mana
            mana = itr->toShort();
            break;
        case 13: // baseobjects.stamina
            stamina = itr->toShort();
            break;
        case 14: // baseobjects.dexterity
            if (itr->empty())
                dexterity = DEFBASE_DEX;
            else if (itr->sectionCount(",") != 0)
            {
                dexterity = itr->section(",", 0, 0).stripWhiteSpace().toShort();
                dx2 = itr->section(",", 1, 1).stripWhiteSpace().empty() ? DEFBASE_DEX2 : itr->section(",", 1, 1).stripWhiteSpace().toShort();
            }
            else
                dexterity = itr->toShort();
            break;
        case 15: // baseobjects.intelligence
            if (itr->empty())
                intelligence = DEFBASE_INT;
            else if (itr->sectionCount(",") != 0)
            {
                intelligence = itr->section(",", 0, 0).stripWhiteSpace().toShort();
                in2 = itr->section(",", 1, 1).stripWhiteSpace().empty() ? DEFBASE_INT2 : itr->section(",", 1, 1).stripWhiteSpace().toShort();
            }
            else
                intelligence = itr->toShort();
            break;
        case 16: // baseobjects.strength
            if (itr->empty())
                strength = DEFBASE_STR;
            else if (itr->sectionCount(",") != 0)
            {
                strength = itr->section(",", 0, 0).stripWhiteSpace().toShort();
                st2 = itr->section(",", 1, 1).stripWhiteSpace().empty() ? DEFBASE_STR2 : itr->section(",", 1, 1).stripWhiteSpace().toShort();
            }
            else
                strength = itr->toShort();
            break;
        case 17: // baseobjects.hitpoints
            hitpoints = itr->toShort();
            break;
        case 18: // baseobjects.race
            race = itr->toUShort();
            break;
        case 19: // baseobjects.visible
            visible    = (VisibleTypes)itr->toByte();
            break;
        case 20: // baseobjects.disabled
            SetDisabled(itr->toUByte() == 1);
            break;
        case 21: // baseobjects.damage
            lodamage = DEFBASE_LODAMAGE;
            hidamage = DEFBASE_HIDAMAGE;
            if (!itr->empty())
                if (itr->sectionCount(",") != 0)
                {
                    if (!itr->section(",", 0, 0).stripWhiteSpace().empty())
                        lodamage = itr->section(",", 0, 0).stripWhiteSpace().toShort();

                    if (!itr->section(",", 1, 1).stripWhiteSpace().empty())
                        hidamage = itr->section(",", 1, 1).stripWhiteSpace().toShort();
                }
            break;
        case 22: // baseobjects.poisoned
            poisoned = itr->toUByte();
            break;
        case 23: // baseobjects.carve
            carve = itr->toShort();
            break;
        case 24: // baseobjects.defense
            {
                UI16 res[WEATHNUM-1] = {0,0,0,0,0,0,0,0,0,0};
                int numSections = 0;
                if (!itr->empty())
                {
                    numSections = itr->sectionCount(",");
                    for (int i = 0; i < numSections+1; ++i)
                    {
                        UString uStr = itr->section(",", i, i);
                        if (!uStr.empty())
                            res[i] = uStr.stripWhiteSpace().toUShort();
                    }

                    if (numSections == 0)
                        res[0] = itr->toUShort();
                }

                for (int resist = 0; resist < WEATHNUM-1; ++resist)
                    SetResist(res[resist], (WeatherType)(resist + 1));
            }
            break;
        case 25: // baseobjects.scptrig
            scriptTrig = itr->toUShort();
            break;
        case 26: // baseobjects.reputation
            {
                SI16 newvalue[3] = {DEFBASE_FAME, DEFBASE_KARMA, DEFBASE_KILLS};
                if (!itr->empty())
                    for (int i = 0; i < itr->sectionCount(",")+1; ++i)
                    {
                        UString uStr = itr->section(",", i, i).stripWhiteSpace();
                        if (!uStr.empty())
                            newvalue[i] = uStr.toShort();
                    }
            
                SetFame(newvalue[0]);
                SetKarma(newvalue[0]);
                SetKills(newvalue[0]);
            }
            break;
        case 27: // baseobjects.alltagprops (TAGNAME, TAGVAL, TAGVALS)
            if (!itr->empty())
            {
                int count = 0;
                std::string TagName = "";
                for (int i = 0; i < itr->sectionCount(",")+1; ++i)
                {
                    ++count;
                    UString uStr = itr->section(",", i, i).stripWhiteSpace();
                    switch (count)
                    {
                    case 1: // tagname
                        if (uStr.empty())
                        {
                            i += 2;
                            TagName.clear();
                            count = 0;
                        }
                        else
                            TagName = uStr.c_str();
                        break;
                    case 2: // tagval
                        if (!uStr.empty())
                        {
                            TAGMAPOBJECT tagvalObject;
                            tagvalObject.m_ObjectType = TAGMAP_TYPE_INT;
                            tagvalObject.m_IntValue = uStr.toULong();
                            tagvalObject.m_Destroy = FALSE;
                            tagvalObject.m_StringValue = "";
                            SetTag(TagName, tagvalObject);
                            ++i;
                            TagName.clear();
                            count = 0;
                        }
                        break;
                    case 3: // tagvals
                        if (!uStr.empty())
                        {
                            TAGMAPOBJECT tagvalObject;
                            tagvalObject.m_ObjectType = TAGMAP_TYPE_STRING;
                            tagvalObject.m_IntValue = uStr.length();
                            tagvalObject.m_Destroy = FALSE;
                            tagvalObject.m_StringValue = uStr;
                            SetTag(TagName, tagvalObject);
                        }
                        TagName.clear();
                        count = 0;
                        break;
                    }
                }
            }
            break;
        }
    }
}
//o--------------------------------------------------------------------------
//| Function   - PostLoadProcessing()
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Used to setup any pointers that may need adjustment
//|              following the loading of the world
//o--------------------------------------------------------------------------
void CBaseObject::PostLoadProcessing(void)
{
    SERIAL tmpSerial = INVALIDSERIAL;
    if (multis != NULL)
    {
        tmpSerial = (SERIAL)multis;
        multis = NULL;
        SetMulti(tmpSerial, false);
    }

    if (spawnserial != INVALIDSERIAL)
    {
        tmpSerial = spawnserial;
        spawnserial = INVALIDSERIAL;
        SetSpawn(tmpSerial);
    }

    if (owner != INVALIDSERIAL) // To repopulate the petlist of the owner
    {
        tmpSerial = owner;
        owner = INVALIDSERIAL;
        SetOwner(calcCharObjFromSer(tmpSerial));
    }

    oldLocX = x;
    oldLocY = y;
    oldLocZ = z;
}

//o--------------------------------------------------------------------------
//| Function   - UI08 WorldNumber(void)
//| Date       - 26th September, 2001
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Returns the world number that the object belongs in
//o--------------------------------------------------------------------------
UI08 CBaseObject::WorldNumber(void) const
{
    return worldNumber;
}

//o--------------------------------------------------------------------------
//| Function   - WorldNumber(UI08 value)
//| Date       - 26th September, 2001
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Sets the world number that the object belongs in
//o--------------------------------------------------------------------------
void CBaseObject::WorldNumber(UI08 value)
{
    worldNumber = value;
    Dirty(UT_LOCATION);
}

//o---------------------------------------------------------------------------o
//| Function   - UI08 Poisoned()
//| Date       - Unknown
//| Programmer - Unknown
//o---------------------------------------------------------------------------o
//| Purpose    - Object is poisoned
//o---------------------------------------------------------------------------o
UI08 CBaseObject::GetPoisoned(void) const
{
    return poisoned;
}

void CBaseObject::SetPoisoned(UI08 newValue)
{
    poisoned = newValue;
}

//o---------------------------------------------------------------------------o
//| Function   - SI16 Carve()
//| Date       - Unknown
//| Programmer - Abaddon
//o---------------------------------------------------------------------------o
//| Purpose    - Characters carve value for carve.dfn
//o---------------------------------------------------------------------------o
SI16 CBaseObject::GetCarve(void) const
{
    return carve;
}

void CBaseObject::SetCarve(SI16 newValue)
{
    carve = newValue;
}

bool CBaseObject::isFree(void) const
{
    return objSettings.test(BIT_FREE);
}

bool CBaseObject::isDeleted(void) const
{
    return objSettings.test(BIT_DELETED);
}

bool CBaseObject::isPostLoaded(void) const
{
    return objSettings.test(BIT_POSTLOADED);
}

bool CBaseObject::isSpawned(void) const
{
    return objSettings.test(BIT_SPAWNED);
}

bool CBaseObject::ShouldSave(void) const
{
    return objSettings.test(BIT_SAVE);
}

bool CBaseObject::isDisabled(void) const
{
    return objSettings.test(BIT_DISABLED);
}

void CBaseObject::SetFree(bool newVal)
{
    objSettings.set(BIT_FREE, newVal);
}

void CBaseObject::SetDeleted(bool newVal)
{
    objSettings.set(BIT_DELETED, newVal);
}

void CBaseObject::SetPostLoaded(bool newVal)
{
    objSettings.set(BIT_POSTLOADED, newVal);
}

void CBaseObject::SetSpawned(bool newVal)
{
    objSettings.set(BIT_SPAWNED, newVal);
}

void CBaseObject::ShouldSave(bool newVal)
{
    objSettings.set(BIT_SAVE, newVal);
}

void CBaseObject::SetDisabled(bool newVal)
{
    objSettings.set(BIT_DISABLED, newVal);
}

//o---------------------------------------------------------------------------o
//| Function   - bool Cleanup()
//| Date       - 11/6/2003
//| Programmer - giwo
//o---------------------------------------------------------------------------o
//| Purpose    - Cleans up after the object
//o---------------------------------------------------------------------------o
void CBaseObject::Cleanup(void)
{
    SetX(7000);
    SetY(7000);
    SetZ(0);

    UI16 scpNum = GetScriptTrigger();
    cScript *tScript = JSMapping->GetScript(scpNum);
    if (tScript != NULL)
        tScript->OnDelete(this);

    QUEUEMAP_ITERATOR toFind = cwmWorldState->refreshQueue.find(this);
    if (toFind != cwmWorldState->refreshQueue.end())
        cwmWorldState->refreshQueue.erase(toFind);

    if (ValidateObject(multis))
        SetMulti(INVALIDSERIAL, false);

    for (CSocket *iSock = Network->FirstSocket(); !Network->FinishedSockets(); iSock = Network->NextSocket())
        if (iSock != NULL)
            if (iSock->TempObj() != NULL && iSock->TempObj() == this)
                iSock->TempObj(NULL);
}

//o---------------------------------------------------------------------------o
//| Function    : void Dirty(void) const
//| Date        : 25 July, 2003
//| Programmer  : Maarc
//o---------------------------------------------------------------------------o
//| Purpose     : Forces the object onto the global refresh queue
//o---------------------------------------------------------------------------o
void CBaseObject::Dirty(UpdateTypes updateType)
{
    if (isPostLoaded())
        ++(cwmWorldState->refreshQueue[this]);
}

void CBaseObject::CopyData(CBaseObject *target)
{
    target->SetTitle(GetTitle());
    target->SetRace(GetRace());
    target->SetName(GetName());
    target->SetStrength(GetStrength());
    target->SetDexterity(GetDexterity());
    target->SetIntelligence(GetIntelligence());
    target->SetHP(GetHP());
    target->SetDir(GetDir());
    target->SetVisible(GetVisible());
    target->SetMana(GetMana());
    target->SetStamina(GetStamina());
    target->SetLocation(this);
    target->SetID(GetID());
    target->SetColour(GetColour());
    target->SetHiDamage(GetHiDamage());
    target->SetLoDamage(GetLoDamage());

    for (UI08 resist = 0; resist < WEATHNUM; ++resist)
        target->SetResist(GetResist((WeatherType)resist), (WeatherType)resist);

    target->SetStrength2(GetStrength2());
    target->SetDexterity2(GetDexterity2());
    target->SetIntelligence2(GetIntelligence2());
    target->SetPoisoned(GetPoisoned());
    target->SetWeight(GetWeight());
    target->SetKarma(karma);
    target->SetFame(fame);
    target->SetKills(kills);
    target->SetWipeable(isWipeable());
}

point3 CBaseObject::GetOldLocation(void)
{
    return point3(oldLocX, oldLocY, oldLocZ);
}

//o--------------------------------------------------------------------------o
//| Function   - SI16 Karma()
//| Date       - unknown
//| Programmer - EviLDeD
//| Modified   -
//o--------------------------------------------------------------------------o
//| Purpose    - The object's karma
//o--------------------------------------------------------------------------o
SI16 CBaseObject::GetKarma(void) const
{
    return karma;
}

void CBaseObject::SetKarma(SI16 value)
{
    karma = value;
}

//o--------------------------------------------------------------------------o
//| Function   - SI16 Fame()
//| Date       - unknown
//| Programmer - EviLDeD
//| Modified   -
//o--------------------------------------------------------------------------o
//| Purpose    - The object's fame
//o--------------------------------------------------------------------------o
SI16 CBaseObject::GetFame(void) const
{
    return fame;
}

void CBaseObject::SetFame(SI16 value)
{
    fame = value;
}

//o--------------------------------------------------------------------------o
//| Function   - SI16 Kills()
//| Date       - unknown
//| Programmer - EviLDeD
//| Modified   -
//o--------------------------------------------------------------------------o
//| Purpose    - The object's kills
//o--------------------------------------------------------------------------o
SI16 CBaseObject::GetKills(void) const
{
    return kills;
}

void CBaseObject::SetKills(SI16 value)
{
    kills = value;
}

//o--------------------------------------------------------------------------
//| Function   - bool isWipeable()
//| Date       - Unknown
//| Programmer - Abaddon
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - object is wipeable
//o--------------------------------------------------------------------------
bool CBaseObject::isWipeable(void) const
{
    return objSettings.test(BIT_WIPEABLE);
}

void CBaseObject::SetWipeable(bool newValue)
{
    objSettings.set(BIT_WIPEABLE, newValue);
}

}
