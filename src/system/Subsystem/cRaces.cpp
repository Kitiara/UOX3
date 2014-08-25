// Abaddon: I have a VERY set idea of features and plans for races
// Please DO NOT TOUCH THEM, because I will be working on them quite
// solidly, along with EviLDeD, over the next few months.  

#include "uox3.h"
#include "cRaces.h"
#include "cServerDefinitions.h"
#include "ssection.h"
#include "cEffects.h"
#include "classes.h"
#include "CJSEngine.h"
#include "power.h"

namespace UOX
{

cRaces *Races = NULL;

const UI08 MALE = 1;
const UI08 FEMALE = 2;

const RACEREL MAX_ENEMY = -100;
const RACEREL MAX_ALLY  = 100;

const UI32 BIT_REQBEARD   = 0;
const UI32 BIT_NOBEARD    = 1;
const UI32 BIT_PLAYERRACE = 2;
const UI32 BIT_NOHAIR     = 3;

bool cRaces::InvalidRace(RACEID x) const
{
    return (x >= races.size());
}

// PRE:  race is a valid race
// POST: returns pointer to a string
const std::string cRaces::Name(RACEID race) const
{
    return races[race]->Name();
}

CRace * cRaces::Race(RACEID x)
{
    if (InvalidRace(x))
        return NULL;
    return races[x];
}

cRaces::cRaces(void) {}

// PRE:  cRaces has been initialized
// POST: Dynamic memory deleted
cRaces::~cRaces(void)
{
    JSEngine->ReleaseObject(IUE_RACE, this);

    for (size_t i = 0; i < races.size(); ++i)
    {
        delete races[i];
        races[i] = NULL;
    }
    races.clear();
}

void cRaces::DefaultInitCombat(void)
{
    combat.resize(4);
    combat[0].value = 0;
    combat[1].value = 50;
    combat[2].value = 100;
    combat[3].value = 200;
}

// PRE:  races.scp exists
// POST: class loaded and populated, dynamically
void cRaces::load(void)
{
    UI32 i = 0;
    UI32 raceCount = 0;
    bool done = false;

    UString sect;
    UString tag;
    UString data;

    while (!done)
    {
        sect = "RACE " + UString::number(raceCount);
        ScriptSection *tempSect = FileLookup->FindEntry(sect, race_def);
        if (tempSect == NULL)
            done = true;
        else
            ++raceCount;
    }

    for (i = 0; i < raceCount; ++i)
        races.push_back(new CRace(raceCount));

    ScriptSection *CombatMods = FileLookup->FindEntry("COMBAT MODS", race_def);
    if (CombatMods != NULL)
    {
        tag = CombatMods->First();
        if (tag == NULL) // location didn't exist!!!
            DefaultInitCombat();
        else
        {
            if (tag.upper() != "MODCOUNT")
            {
                Console << "MODCOUNT must come before any entries!" << myendl;
                DefaultInitCombat();
            }
            else
            {
                UI32 modifierCount = CombatMods->GrabData().toULong();
                if (modifierCount < 4)
                {
                    Console << "MODCOUNT must be more >= 4, or it uses the defaults!" << myendl;
                    DefaultInitCombat();
                }
                else
                {
                    combat.resize(modifierCount);
                    for (i = 0; i < modifierCount; ++i)
                    {
                        tag = CombatMods->Next();
                        data = CombatMods->GrabData();
                        if (!data.empty())
                            combat[i].value = data.toUByte();
                        else
                            combat[i].value = 100;
                    }
                }
            }
        }
    }
    else
        DefaultInitCombat();

    for (size_t er = 0; er < raceCount; ++er)
        races[er]->Load(er, combat.size());
}

RaceRelate cRaces::Compare(CChar *player1, CChar *player2) const
{
    if (!ValidateObject(player1) || !ValidateObject(player2))
        return RACE_NEUTRAL;
    RACEID r1 = player1->GetRace();
    RACEID r2 = player2->GetRace();
    if (r1 >= races.size() || r2 >= races.size())
        return RACE_NEUTRAL;
    return races[r1]->RaceRelation(r2);
}

// PRE: race1 and race2 are below the maximum number of races
// POST: Returns 0 if no enemy or ally, -1 if enemy, or 1 if ally
RaceRelate cRaces::CompareByRace(RACEID race1, RACEID race2) const
{
    if (race1 >= races.size())      // invalid race?
        return RACE_NEUTRAL;
    else if (race2 >= races.size()) // invalid race?
        return RACE_NEUTRAL;
    else
        return races[race1]->RaceRelation(race2); // enemy race
}

// PRE:  PLAYER s is valid, x is a race index and always = 0 or 1
// POST: PLAYER s belongs to new race x or doesn't change based on restrictions
void cRaces::gate(CChar *s, RACEID x, bool always)
{
    if (!ValidateObject(s))
        return;
    CItem *hairobject = NULL, *beardobject = NULL;

    CRace *pRace = Race(x);
    if (pRace == NULL)
        return;

    CSocket *mSock = s->GetSocket();
    if (!pRace->IsPlayerRace())
    {
        mSock->sysmessage(369);
        return;
    }

    if (s->GetRaceGate() == 65535 || always)
    {
        UI16 hairColor = 0;
        std::map< UI08, std::string > lossMap;

        lossMap[STRENGTH] = "strength";
        lossMap[DEXTERITY] = "speed";
        lossMap[INTELLECT] = "wisdom";
    
        beardobject = s->GetItemAtLayer(IL_FACIALHAIR);
        hairobject    = s->GetItemAtLayer(IL_HAIR);
        if (pRace->GenderRestriction() != 0)
        {
            if (pRace->GenderRestriction() != FEMALE && (s->GetID() == 0x0191 || s->GetID() == 0x025E || s->GetID() == 0x029B || s->GetID() == 0x02EF || s->GetID() == 0x00B8 || s->GetID() == 0x00BA))
            {
                mSock->sysmessage(370);
                return;
            }

            if (pRace->GenderRestriction() != MALE && (s->GetID() == 0x0190 || s->GetID() == 0x025D || s->GetID() == 0x029A || s->GetID() == 0x02EE || s->GetID() == 0x00B7 || s->GetID() == 0x00B9))
            {
                mSock->sysmessage(370);
                return;
            }
        }
        s->SetRaceGate(x);
        s->SetRace(x);

        SI16 stats[3];
        stats[0] = s->ActualStrength();
        stats[1] = s->ActualDexterity();
        stats[2] = s->ActualIntelligence();

        for (UI08 counter = STRENGTH; counter <= INTELLECT; ++counter)
            if (stats[counter-STRENGTH] > pRace->Skill(counter))
            {
                mSock->sysmessage(371, lossMap[counter].c_str());
                stats[counter-STRENGTH] = pRace->Skill(counter);
            }
            else
                stats[counter-STRENGTH] = 0;

        if (stats[0] != 0)
            s->SetStrength(stats[0]);
        if (stats[1] != 0)
            s->SetDexterity(stats[1]);
        if (stats[2] != 0)
            s->SetIntelligence(stats[2]);

        if (ValidateObject(hairobject))
        {
            if (pRace->IsHairRestricted())
            {
                hairColor = (hairobject->GetColour());
                if (pRace->IsValidHair(hairColor))
                {
                    hairColor = RandomHair(x);
                    hairobject->SetColour(hairColor);
                }
            }

            if (pRace->NoHair())
            {
                hairobject->Delete();
                hairobject = NULL;
            }
        }

        if (pRace->RequiresBeard() && (s->GetID() == 0x0190 || s->GetID() == 0x025D) && !ValidateObject(beardobject))
        {
            if (pRace->IsBeardRestricted())
                hairColor = RandomBeard(x);
            else
                hairColor = 0x0480;
            CItem *n = Items->CreateItem(NULL, s, 0x204C, 1, hairColor, OT_ITEM);
            if (n != NULL)
            {
                n->SetDecayable(false);
                n->SetLayer(IL_FACIALHAIR);
                if (n->SetCont(s))
                    beardobject = n;
            }
        }

        if (ValidateObject(beardobject))
        {
            if (pRace->IsBeardRestricted())
            {
                hairColor = beardobject->GetColour();
                if (pRace->IsValidBeard(hairColor))
                {
                    hairColor = RandomBeard(x);
                    beardobject->SetColour(hairColor);
                }
            }

            if (pRace->NoBeard())
            {
                beardobject->Delete();
                beardobject = NULL;
            }
        }

        if (pRace->IsSkinRestricted()) // do we have a limited skin colour range?
        {
            hairColor = s->GetSkin();
            if (pRace->IsValidSkin(hairColor)) // if not in range
            {
                hairColor = RandomSkin(x); // get random skin in range
                s->SetSkin(hairColor);
                s->SetOrgSkin(hairColor);
            }
        }

        s->Teleport();
        Effects->PlayStaticAnimation(s, 0x373A, 0, 15);
        Effects->PlaySound(s, 0x01E9);
    }
    else 
        mSock->sysmessage(372);
}

// PRE:  Race is valid
// POST: Returns whether colour is valid for beard
bool cRaces::beardInRange(COLOUR color, RACEID x) const
{
    if (InvalidRace(x))
        return false;
    return races[x]->IsValidBeard(color);
}

// PRE:  Race is valid
// POST: Returns whether colour is valid for skin
bool cRaces::skinInRange(COLOUR color, RACEID x) const
{
    if (InvalidRace(x))
        return false;
    return races[x]->IsValidSkin(color);
}

// PRE:  Race is valid
// POST: Returns whether colour is valid for hair
bool cRaces::hairInRange(COLOUR color, RACEID x) const
{
    if (InvalidRace(x))
        return false;
    return races[x]->IsValidHair(color);
}

// PRE:  skill is valid, race is valid
// POST: returns skill bonus associated with race
SKILLVAL cRaces::Skill(int skill, RACEID race) const
{ 
    if (InvalidRace(race) || skill >= ALLSKILLS)
        return 0;
    return races[race]->Skill(skill);
}

// PRE:  skill is valid, value is valid, race is valid
// POST: sets race's skill bonus to value
void cRaces::Skill(int skill, int value, RACEID race)
{ 
    if (InvalidRace(race))
        return;
    races[race]->Skill(value, skill);
}

// PRE:  Race is valid
// POST: returns whether race's gender is restricted, and if so, which gender
//       0 - none 1 - male 2 - female
GENDER cRaces::GenderRestrict(RACEID race) const
{ 
    if (InvalidRace(race))
        return MALE;
    return races[race]->GenderRestriction();
}

// PRE:  Race is valid, gender is valid
// POST: Sets race's gender restriction
void cRaces::GenderRestrict(GENDER gender, RACEID race)
{ 
    if (InvalidRace(race))
        return;
    races[race]->GenderRestriction(gender);
}

// PRE:  race is valid
// POST: returns whether race must have beard
bool cRaces::RequireBeard(RACEID race) const
{ 
    if (InvalidRace(race))
        return false;
    return races[race]->RequiresBeard();
}

// PRE:  Race is valid, and value is true or false
// POST: sets whether race requires a beard
void cRaces::RequireBeard(bool value, RACEID race)
{ 
    if (InvalidRace(race))
        return;
    races[race]->RequiresBeard(value); 
}

// PRE:  Race is valid, value is a valid armorclass
// POST: sets the armor class of race
void cRaces::ArmorRestrict(RACEID race, ARMORCLASS value)
{ 
    if (InvalidRace(race))
        return;
    races[race]->ArmourClassRestriction(value); 
}

// PRE:  Race is valid
// POST: Returns armor class of race
ARMORCLASS cRaces::ArmorRestrict(RACEID race) const
{ 
    if (InvalidRace(race))
        return 0;
    return races[race]->ArmourClassRestriction();
}

// PRE:  Race is valid
// POST: returns a valid skin colour for the race
COLOUR cRaces::RandomSkin(RACEID x) const
{ 
    if (InvalidRace(x))
        return 0000;
    return races[x]->RandomSkin();
}

// PRE:  Race is valid
// POST: returns a valid hair colour for the race
COLOUR cRaces::RandomHair(RACEID x) const
{ 
    if (InvalidRace(x))
        return 0000;
    return races[x]->RandomHair();
}

// PRE:  Race is valid
// POST: returns a valid beard colour for the race
COLOUR cRaces::RandomBeard(RACEID x) const
{ 
    if (InvalidRace(x))
        return 0;
    return races[x]->RandomBeard();
}

// PRE:  race is valid
// POST: returns true if race's beard colour is restricted
bool cRaces::beardRestricted(RACEID x) const
{ 
    if (InvalidRace(x))
        return false;
    return races[x]->IsBeardRestricted();
}

// PRE:  race is valid
// POST: returns true if race's hair colour is restricted
bool cRaces::hairRestricted(RACEID x) const
{ 
    if (InvalidRace(x))
        return false;
    return races[x]->IsHairRestricted();
}

// PRE:  race is valid
// POST: returns true if race's skin colour is restricted
bool cRaces::skinRestricted(RACEID x) const
{ 
    if (InvalidRace(x))
        return false;
    return races[x]->IsSkinRestricted();
}

// PRE:  x is valid, skill is valid
// POST: returns chance difference to race x in skill skill
SI32 cRaces::DamageFromSkill(int skill, RACEID x) const
{
    if (InvalidRace(x))
        return 0;
    if (skill >= ALLSKILLS)
        return 0;
    SKILLVAL modifier = races[x]->Skill(skill);
    if (modifier >= static_cast<SKILLVAL>(combat.size()))
        return -(combat[modifier].value);
    else
        return (combat[modifier].value);
    return 0;
}

// PRE:  x is valid, skill is valid
// POST: returns positive/negative fight damage bonus for race x with skill skill
SI32 cRaces::FightPercent(int skill, RACEID x) const
{
    if (InvalidRace(x))
        return 100;
    SKILLVAL modifier = races[x]->Skill(skill);
    int divValue = combat[modifier].value / 10;
    divValue = divValue / 10;
    if (divValue == 0)
        return 100;
    if (modifier >= static_cast<int>(combat.size()))
        return -(int)(100/(R32)divValue);
    else
        return (int)(100/(R32)divValue);
    return 100;
}

// PRE:  race and toSet are valid races, value is a valid relation
// POST: the relation between race and toset is set to value
void cRaces::RacialInfo(RACEID race, RACEID toSet, RaceRelate value)
{
    if (InvalidRace(race))
        return;
    races[race]->RaceRelation(value, toSet);
}

// PRE:  race and enemy are valid
// POST: enemy is race's enemy
void cRaces::RacialEnemy(RACEID race, RACEID enemy)
{
    if (InvalidRace(race))
        return;
    RacialInfo(race, enemy, RACE_ENEMY);
}

// PRE:  race and ally are valid
// POST: ally is race's ally
void cRaces::RacialAlly(RACEID race, RACEID ally)
{
    if (InvalidRace(race))
        return;
    RacialInfo(race, ally, RACE_ALLY);
}

// PRE:  race and neutral are valid
// POST: neutral is neutral to race
void cRaces::RacialNeutral(RACEID race, RACEID neutral)
{
    if (InvalidRace(race))
        return;
    RacialInfo(race, neutral, RACE_NEUTRAL);
}

// PRE:  x is a valid race
// POST: returns race's minimum skill for language
SKILLVAL cRaces::LanguageMin(RACEID x) const
{
    return races[x]->LanguageMin();
}

// PRE:  race and toSetTo is valid
// POST: race's min language requirement is set to toSetTo
void cRaces::LanguageMin(SKILLVAL toSetTo, RACEID race)
{
    if (InvalidRace(race))
        return;
    races[race]->LanguageMin(toSetTo);
}

// PRE:  Race is valid, value is a valid light level
// POST: the light level that race burns at is set to value
void cRaces::LightLevel(RACEID race, LIGHTLEVEL value)
{ 
    if (InvalidRace(race))
        return;
    races[race]->LightLevel(value); 
}

// PRE:  Race is valid
// POST: Returns the light level that race burns at
LIGHTLEVEL cRaces::LightLevel(RACEID race) const
{ 
    if (InvalidRace(race))
        return 0;
    return races[race]->LightLevel(); 
}

// PRE:  Race is valid, value is a valid cold level
// POST: the cold level that race burns at is set to value
void cRaces::ColdLevel(RACEID race, COLDLEVEL value)
{ 
    if (InvalidRace(race))
        return;
    races[race]->ColdLevel(value); 
}

// PRE:  Race is valid
// POST: Returns the cold level that race burns at
COLDLEVEL cRaces::ColdLevel(RACEID race) const
{ 
    if (InvalidRace(race))
        return 0;
    return races[race]->ColdLevel(); 
}

// PRE:  Race is valid, value is a valid heat level
// POST: the light heat that race burns at is set to value
void cRaces::HeatLevel(RACEID race, HEATLEVEL value)
{ 
    if (InvalidRace(race))
        return;
    races[race]->HeatLevel(value); 
}

// PRE:  Race is valid
// POST: Returns the heat level that race burns at
HEATLEVEL cRaces::HeatLevel(RACEID race) const
{ 
    if (InvalidRace(race))
        return 0;
    return races[race]->HeatLevel(); 
}

void cRaces::DoesHunger(RACEID race, bool value)
{ 
    if (InvalidRace(race))
        return;
    races[race]->DoesHunger(value); 
}

bool cRaces::DoesHunger(RACEID race) const
{ 
    if (InvalidRace(race))
        return 0;
    return races[race]->DoesHunger(); 
}

void cRaces::SetHungerRate(RACEID race, UI16 value)
{ 
    if (InvalidRace(race))
        return;
    races[race]->SetHungerRate(value); 
}

UI16 cRaces::GetHungerRate(RACEID race) const
{ 
    if (InvalidRace(race))
        return 0;
    return races[race]->GetHungerRate(); 
}

void cRaces::SetHungerDamage(RACEID race, SI16 value)
{ 
    if (InvalidRace(race))
        return;
    races[race]->SetHungerDamage(value); 
}

SI16 cRaces::GetHungerDamage(RACEID race) const
{ 
    if (InvalidRace(race))
        return 0;
    return races[race]->GetHungerDamage(); 
}

bool cRaces::Affect(RACEID race, WeatherType element) const
{
    bool rValue = false;
    if (!InvalidRace(race))
        rValue = races[race]->AffectedBy(element);
    return rValue;
}

void cRaces::Affect(RACEID race, WeatherType element, bool value)
{
    if (!InvalidRace(race))
        races[race]->AffectedBy(value, element);
}

// PRE:  Race is valid, element is an element of weather
// POST: Returns number of seconds between burns for race from element
SECONDS cRaces::Secs(RACEID race, WeatherType element) const
{
    if (InvalidRace(race))
        return 1;
    return races[race]->WeatherSeconds(element);
}

// PRE:  Race is valid, element is element of weather, value is seconds
// POST: Sets number of seconds between burns for race from element
void cRaces::Secs(RACEID race, WeatherType element, SECONDS value)
{
    if (InvalidRace(race))
        return;
    races[race]->WeatherSeconds(value, element);
}

// PRE:  Race is valid, element is an element of weather
// POST: Returns damage incurred by race from element
SI08 cRaces::Damage(RACEID race, WeatherType element) const
{
    if (InvalidRace(race))
        return 1;
    return races[race]->WeatherDamage(element);
}

// PRE:  race is valid, element is element of weather, damage is > -127 && < 127
// POST: sets damage incurred by race from element
void cRaces::Damage(RACEID race, WeatherType element, SI08 damage)
{
    if (InvalidRace(race))
        return;
    races[race]->WeatherDamage(damage, element);
}

// PRE:  x is valid
// POST: returns light level bonus of race x
LIGHTLEVEL cRaces::VisLevel(RACEID x) const
{
    if (InvalidRace(x))
        return 0;
    return races[x]->NightVision();
}

// PRE:  x is valid
// POST: sets race's light level bonus to bonus
void cRaces::VisLevel(RACEID x, LIGHTLEVEL bonus)
{
    if (InvalidRace(x))
        return;
    races[x]->NightVision(bonus);
}

// PRE:  x is valid
// POST: Returns distance that race can see
RANGE cRaces::VisRange(RACEID x) const
{
    if (InvalidRace(x))
        return 0;
    return races[x]->VisibilityRange();
}

// PRE:  x is valid and range is valid
// POST: sets distance that race can see to range
void cRaces::VisRange(RACEID x, RANGE range)
{
    if (InvalidRace(x))
        return;
    races[x]->VisibilityRange(range);
}

bool cRaces::NoBeard(RACEID x) const
{
    if (InvalidRace(x))
        return false;
    return races[x]->NoBeard();
}

void cRaces::NoBeard(bool value, RACEID race)
{
    if (InvalidRace(race))
        return;
    races[race]->NoBeard(value);
}

void cRaces::debugPrint(RACEID x)
{
    if (InvalidRace(x))
        return;
    Console << "Race ID: " << x << myendl;
    Console << "Race: " << races[x]->Name() << myendl;
    if (races[x]->RequiresBeard()) 
        Console << "Req Beard: Yes" << myendl;
    else 
        Console << "Req Beard: No" << myendl;
    if (races[x]->NoBeard()) 
        Console << "No Beard: Yes" << myendl;
    else 
        Console << "No Beard: No" << myendl;
    if (races[x]->IsPlayerRace()) 
        Console << "Player Race: Yes" << myendl;
    else 
        Console << "Player Race: No" << myendl;
    Console << "Restrict Gender: " << races[x]->GenderRestriction() << myendl;
    Console << "LightLevel: " << races[x]->LightLevel() << myendl;
    Console << "NightVistion: " << races[x]->NightVision() << myendl;
    Console << "ArmorRestrict: " << races[x]->ArmourClassRestriction() << myendl;
    Console << "LangMin: " << races[x]->LanguageMin() << myendl;
    Console << "Vis Distance: " << races[x]->VisibilityRange() << myendl << myendl;
}

void cRaces::debugPrintAll(void)
{
    for (RACEID x = 0; x < races.size(); ++x)
        debugPrint(x);
}

bool cRaces::IsPlayerRace(RACEID race) const
{
    if (InvalidRace(race))
        return false;
    return races[race]->IsPlayerRace();
}

// PRE:  x is a valid race, value is either true or false
// POST: sets if x is a player race or not
void cRaces::IsPlayerRace(RACEID x, bool value)
{
    if (InvalidRace(x))
        return;
    races[x]->IsPlayerRace(value);
}

SKILLVAL CRace::Skill(int skillNum) const
{
    return iSkills[skillNum];
}

const std::string CRace::Name(void) const
{
    return raceName;
}

bool CRace::RequiresBeard(void) const
{
    return bools.test(BIT_REQBEARD);
}

bool CRace::NoBeard(void) const
{
    return bools.test(BIT_NOBEARD);
}

bool CRace::IsPlayerRace(void) const
{
    return bools.test(BIT_PLAYERRACE);
}

bool CRace::NoHair(void) const
{
    return bools.test(BIT_NOHAIR);
}

GENDER CRace::GenderRestriction(void) const
{
    return restrictGender;
}

LIGHTLEVEL CRace::LightLevel(void) const
{
    return lightLevel;
}

COLDLEVEL CRace::ColdLevel(void) const
{
    return coldLevel;
}

HEATLEVEL CRace::HeatLevel(void) const
{
    return heatLevel;
}

LIGHTLEVEL CRace::NightVision(void) const
{
    return nightVision;
}

ARMORCLASS CRace::ArmourClassRestriction(void) const
{
    return armourRestrict;
}

SECONDS CRace::WeatherSeconds(WeatherType iNum) const
{
    return weathSecs[iNum];
}

SI08 CRace::WeatherDamage(WeatherType iNum) const
{
    return weathDamage[iNum];
}

SKILLVAL CRace::LanguageMin(void) const
{
    return languageMin;
}

RANGE CRace::VisibilityRange(void) const
{
    return visDistance;
}

void CRace::Skill(SKILLVAL newValue, int iNum)
{
    iSkills[iNum] = newValue;
}

void CRace::Name(const std::string& newName)
{
    raceName = newName;
}

void CRace::RequiresBeard(bool newValue)
{
    bools.set(BIT_REQBEARD, newValue);
}

void CRace::NoBeard(bool newValue)
{
    bools.set(BIT_NOBEARD, newValue);
}

void CRace::IsPlayerRace(bool newValue)
{
    bools.set(BIT_PLAYERRACE, newValue);
}

void CRace::NoHair(bool newValue)
{
    bools.set(BIT_NOHAIR, newValue);
}

bool CRace::AffectedBy(WeatherType iNum) const
{
    return weatherAffected.test(iNum);
}
void CRace::AffectedBy(bool value, WeatherType iNum)
{
    weatherAffected.set(iNum, value);
}

void CRace::GenderRestriction(GENDER newValue)
{
    restrictGender = newValue;
}

void CRace::LightLevel(LIGHTLEVEL newValue)
{
    lightLevel = newValue;
}

void CRace::ColdLevel(COLDLEVEL newValue)
{
    coldLevel = newValue;
}

void CRace::HeatLevel(HEATLEVEL newValue)
{
    heatLevel = newValue;
}

void CRace::NightVision(LIGHTLEVEL newValue)
{
    nightVision = newValue;
}

void CRace::ArmourClassRestriction(ARMORCLASS newValue)
{
    armourRestrict = newValue;
}

void CRace::WeatherSeconds(SECONDS newValue, WeatherType iNum)
{
    weathSecs[iNum] = newValue;
}

void CRace::WeatherDamage(SI08 newValue, WeatherType iNum)
{
    weathDamage[iNum] = newValue;
}

void CRace::LanguageMin(SKILLVAL newValue)
{
    languageMin = newValue;
}

void CRace::VisibilityRange(RANGE newValue)
{
    visDistance = newValue;
}

UI16 CRace::GetHungerRate(void) const
{
    return hungerRate;
}

void CRace::SetHungerRate(UI16 newValue)
{
    hungerRate = newValue;
}

SI16 CRace::GetHungerDamage(void) const
{
    return hungerDamage;
}

void CRace::SetHungerDamage(SI16 newValue)
{
    hungerDamage = newValue;
}

bool CRace::DoesHunger(void) const
{
    return doesHunger;
}

void CRace::DoesHunger(bool newValue)
{
    doesHunger = newValue;
}

CRace::CRace() : bools(4), visDistance(0), nightVision(0), armourRestrict(0), lightLevel(1), restrictGender(0),
    languageMin(0), poisonResistance(0.0f), magicResistance(0.0f)
{
    memset(&iSkills[0], 0, sizeof(SKILLVAL) * SKILLS);
    memset(&weathDamage[0], 0, sizeof(SI08) * WEATHNUM);
    memset(&weathSecs[0], 60, sizeof(SECONDS) * WEATHNUM);
    
    Skill(100, STRENGTH);
    Skill(100, DEXTERITY);
    Skill(100, INTELLECT);
    HPModifier(0);
    ManaModifier(0);
    StamModifier(0);
    DoesHunger(false);
    SetHungerRate(0);
    SetHungerDamage(0);
}

CRace::CRace(int numRaces) : bools(4), visDistance(0), nightVision(0), armourRestrict(0), lightLevel(1), restrictGender(0),
    languageMin(0), poisonResistance(0.0f), magicResistance(0.0f)
{
    NumEnemyRaces(numRaces);

    memset(&iSkills[0], 0, sizeof(SKILLVAL) * SKILLS);
    memset(&weathDamage[0], 0, sizeof(SI08) * WEATHNUM);
    memset(&weathSecs[0], 0, sizeof(SECONDS) * WEATHNUM);

    Skill(100, STRENGTH);
    Skill(100, DEXTERITY);
    Skill(100, INTELLECT);
    HPModifier(0);
    ManaModifier(0);
    StamModifier(0);
    DoesHunger(false);
    SetHungerRate(0);
    SetHungerDamage(0);
    weatherAffected.reset();
}

void CRace::NumEnemyRaces(int iNum)
{
    racialEnemies.resize(iNum);
}

RaceRelate CRace::RaceRelation(RACEID race) const
{
    return racialEnemies[race];
}

COLOUR CRace::RandomSkin(void) const
{
    if (!IsSkinRestricted())
        return 0;
    size_t sNum = RandomNum(static_cast< size_t >(0), skinColours.size() - 1);
    return (COLOUR)RandomNum(skinColours[sNum].cMin, skinColours[sNum].cMax);
}

COLOUR CRace::RandomHair(void) const
{
    if (!IsHairRestricted())
        return 0;
    size_t sNum = RandomNum(static_cast< size_t >(0), hairColours.size() - 1);
    return (COLOUR)RandomNum(hairColours[sNum].cMin, hairColours[sNum].cMax);
}

COLOUR CRace::RandomBeard(void) const
{
    if (!IsBeardRestricted())
        return 0;
    size_t sNum = RandomNum(static_cast< size_t >(0), beardColours.size() - 1);
    return (COLOUR)RandomNum(beardColours[sNum].cMin, beardColours[sNum].cMax);
}

bool CRace::IsSkinRestricted(void) const
{
    return (!skinColours.empty());
}
bool CRace::IsHairRestricted(void) const
{
    return (!hairColours.empty());
}
bool CRace::IsBeardRestricted(void) const
{
    return (!beardColours.empty());
}

bool CRace::IsValidSkin(COLOUR val) const
{
    if (!IsSkinRestricted())
        return true;

    for (size_t i = 0; i < skinColours.size(); ++i)
        if (val >= skinColours[i].cMin && val <= skinColours[i].cMax)
            return true;

    return false;
}

bool CRace::IsValidHair(COLOUR val) const
{
    if (!IsHairRestricted())
        return true;

    for (size_t i = 0; i < hairColours.size(); ++i)
        if (val >= hairColours[i].cMin && val <= hairColours[i].cMax)
            return true;

    return false;
}

bool CRace::IsValidBeard(COLOUR val) const
{
    if (!IsBeardRestricted())
        return true;

    for (size_t i = 0; i < beardColours.size(); ++i)
        if (val >= beardColours[i].cMin && val <= beardColours[i].cMax)
            return true;

    return false;
}

void CRace::RaceRelation(RaceRelate value, RACEID race)
{
    racialEnemies[race] = value;
}

SI16 CRace::HPModifier(void) const
{
    return HPMod;
}

void CRace::HPModifier(SI16 value)
{
    if (value > -100)
        HPMod = value;
    else
        HPMod = -99;
}

SI16 CRace::ManaModifier(void) const
{
    return ManaMod;
}

void CRace::ManaModifier(SI16 value)
{
    if (value > -100)
        ManaMod = value;
    else
        ManaMod = -99;
}

SI16 CRace::StamModifier(void) const
{
    return StamMod;
}

void CRace::StamModifier(SI16 value)
{
    if (value > -100)
        StamMod = value;
    else
        StamMod = -99;
}

void CRace::Load(size_t sectNum, int modCount)
{
    UString tag;
    UString data;
    UString UTag;
    SI32 raceDiff = 0;
    UString sect = "RACE " + UString::number(sectNum);
    ScriptSection *RacialPart = FileLookup->FindEntry(sect, race_def);

    COLOUR beardMin = 0, skinMin = 0, hairMin = 0;

    for (tag = RacialPart->First(); !RacialPart->AtEnd(); tag = RacialPart->Next())
    {
        UTag = tag.upper();
        data = RacialPart->GrabData();
        switch(tag[0])
        {
            case 'a':
            case 'A':
                if (UTag == "ARMORREST")
                    ArmourClassRestriction(data.toUByte()); // 8 classes, value 0 is all, else it's a bit comparison
                break;
            case 'b':
            case 'B':
                if (UTag == "BEARDMIN")
                    beardMin = data.toUShort();
                else if (UTag == "BEARDMAX")
                    beardColours.push_back(ColourPair(beardMin, data.toUShort()));
                break;
            case 'c':
            case 'C':
                if (UTag == "COLDAFFECT")      // are we affected by cold?
                    AffectedBy(true, COLD);
                else if (UTag == "COLDLEVEL")  // cold level at which to take damage
                    ColdLevel(data.toUShort());
                else if (UTag == "COLDDAMAGE") // how much damage to take from cold
                    WeatherDamage(data.toUShort(), COLD);
                else if (UTag == "COLDSECS")   // how often cold affects in secs
                    WeatherSeconds(data.toUShort(), COLD);
                break;
            case 'd':
            case 'D':
                if (UTag == "DEXCAP")
                    Skill(data.toUShort(), DEXTERITY);
                break;
            case 'g':
            case 'G':
                if (UTag == "GENDER")
                {
                    if (data.upper() == "MALE")
                        GenderRestriction(MALE);
                    else if (data.upper() == "FEMALE")
                        GenderRestriction(FEMALE);
                    else
                        GenderRestriction(MALE);
                }
                break;
            case 'h':
            case 'H':
                if (UTag == "HAIRMIN")
                    hairMin = data.toUShort();
                else if (UTag == "HAIRMAX")
                    hairColours.push_back(ColourPair(hairMin, data.toUShort()));
                else if (UTag == "HEATAFFECT") // are we affected by light?
                    AffectedBy(true, HEAT);
                else if (UTag == "HEATDAMAGE") // how much damage to take from light
                    WeatherDamage(data.toUShort(), HEAT);
                else if (UTag == "HEATLEVEL") // heat level at which to take damage
                    HeatLevel(data.toUShort());
                else if (UTag == "HEATSECS")        // how often light affects in secs
                    WeatherSeconds(data.toUShort(), HEAT);
                else if (UTag == "HPMOD") // how much additional percent of strength are hitpoints
                    HPModifier(data.toShort());
                else if (UTag == "HUNGER") // does race suffer from hunger
                    if (data.sectionCount(",") != 0)
                    {
                        SetHungerRate(static_cast<UI16>(data.section(",", 0, 0).stripWhiteSpace().toUShort()));
                        SetHungerDamage(static_cast<SI16>(data.section(",", 1, 1).stripWhiteSpace().toShort()));
                    }
                    else
                    {
                        SetHungerRate(0);
                        SetHungerDamage(0);
                    }
                    if (GetHungerRate() > 0)
                        DoesHunger(true);
                    else
                        DoesHunger(false);
                break;
            case 'i':
            case 'I':
                if (UTag == "INTCAP")
                    Skill(data.toUShort(), INTELLECT);
                break;
            case 'l':
            case 'L':
                if (UTag == "LIGHTAFFECT")      // are we affected by light?
                    AffectedBy(true, LIGHT);
                else if (UTag == "LIGHTDAMAGE") // how much damage to take from light
                    WeatherDamage(data.toUShort(), LIGHT);
                else if (UTag == "LIGHTLEVEL")  // light level at which to take damage
                    LightLevel(data.toUShort());
                else if (UTag == "LIGHTSECS")   // how often light affects in secs
                    WeatherSeconds(data.toUShort(), LIGHT);

                else if (UTag == "LIGHTNINGAFFECT") // are we affected by light?
                    AffectedBy(true, LIGHTNING);
                else if (UTag == "LIGHTNINGDAMAGE") // how much damage to take from light
                    WeatherDamage(data.toUShort(), LIGHTNING);
                else if (UTag == "LIGHTNINGCHANCE") // how big is the chance to get hit by a lightning
                    WeatherSeconds(data.toUShort(), LIGHTNING);
                else if (UTag == "LANGUAGEMIN")     // set language min 
                    LanguageMin(data.toUShort()); 
                break;
            case 'm':
            case 'M':
                if (UTag == "MAGICRESISTANCE") // magic resistance?
                    MagicResistance(data.toFloat());
                else if (UTag == "MANAMOD")    // how much additional percent of int are mana
                    ManaModifier(data.toShort());
                break;
            case 'n':
            case 'N':
                if (UTag == "NAME")
                    Name(data);
                else if (UTag == "NOBEARD")
                    NoBeard(true);
                else if (UTag == "NIGHTVIS") // night vision level... light bonus
                    NightVision(data.toUByte());
                break;
            case 'p':
            case 'P':
                if (UTag == "PLAYERRACE")            // is it a player race?
                    IsPlayerRace((data.toUByte() != 0));
                else if (UTag == "POISONRESISTANCE") // poison resistance?
                    PoisonResistance(data.toFloat());
                else if (UTag == "PARENTRACE")
                {
                    CRace *pRace = Races->Race(data.toUShort());
                    if (pRace != NULL)
                        (*this) = (*pRace);
                }
                break;
            case 'r':
            case 'R':
                if (UTag == "REQUIREBEARD")
                    RequiresBeard(true);
                else if (UTag == "RAINAFFECT") // are we affected by light?
                    AffectedBy(true, RAIN);
                else if (UTag == "RAINDAMAGE") // how much damage to take from light
                    WeatherDamage(data.toUShort(), RAIN);
                else if (UTag == "RAINSECS")   // how often light affects in secs
                    WeatherSeconds(data.toUShort(), RAIN);
                else if (UTag == "RACERELATION")
                {
                    if (data.sectionCount(" ") != 0)
                        RaceRelation(static_cast<RaceRelate>(data.section(" ", 1, 1).stripWhiteSpace().toByte()), data.section(" ", 0, 0).stripWhiteSpace().toUShort());
                }
                else if (UTag == "RACIALENEMY")
                {
                    raceDiff = data.toLong();
                    if (raceDiff > static_cast<SI32>(racialEnemies.size()))
                        Console << "Error in race " << static_cast< UI32 >(sectNum) << ", invalid enemy race " << raceDiff << myendl;
                    else
                        RaceRelation(RACE_ENEMY, static_cast<RACEID>(raceDiff));
                }
                else if (UTag == "RACIALAID")
                {
                    raceDiff = data.toLong();
                    if (raceDiff > static_cast<SI32>(racialEnemies.size()))
                        Console << "Error in race " << static_cast< UI32 >(sectNum) << ", invalid ally race " <<  raceDiff << myendl;
                    else
                        RaceRelation(RACE_ALLY, static_cast<RACEID>(raceDiff));
                }
                break;
            case 's':
            case 'S':
                if (UTag == "STRCAP")
                    Skill(data.toUShort(), STRENGTH);
                else if (UTag == "SKINMIN")
                    skinMin = data.toUShort();
                else if (UTag == "SKINMAX")
                    skinColours.push_back(ColourPair(skinMin, data.toUShort()));
                else if (UTag == "SNOWAFFECT")  // are we affected by light?
                    AffectedBy(true, SNOW);
                else if (UTag == "SNOWDAMAGE")  // how much damage to take from light
                    WeatherDamage(data.toUShort(), SNOW);
                else if (UTag == "SNOWSECS")    // how often light affects in secs
                    WeatherSeconds(data.toUShort(), SNOW);
                else if (UTag == "STORMAFFECT") // are we affected by storm?
                    AffectedBy(true, STORM);
                else if (UTag == "STORMDAMAGE") // how much damage to take from storm
                    WeatherDamage(data.toUShort(), STORM);
                else if (UTag == "STORMSECS")   // how often storm affects in secs
                    WeatherSeconds(data.toUShort(), STORM);
                else if (UTag == "STAMMOD")     // how much additional percent of int are mana
                    StamModifier(data.toShort());
                break;
            case 'v':
            case 'V':
                if (UTag == "VISRANGE") // set visibility range ... defaults to 18
                    VisibilityRange(data.toByte());
                break;
        }

        for (int iCountA = 0; iCountA < ALLSKILLS; ++iCountA)
        {
            UString skillthing = cwmWorldState->skill[iCountA].name;
            skillthing += "G";
            if (skillthing == tag)
                Skill(data.toUShort(), iCountA);
            else
            {
                skillthing = cwmWorldState->skill[iCountA].name;
                skillthing += "L";
                if (skillthing == tag)
                    Skill(modCount + data.toUShort(), iCountA);
            }
        }
    }
}

CRace::~CRace() {}

R32 CRace::MagicResistance(void) const
{
    return magicResistance;
}
R32 CRace::PoisonResistance(void) const
{
    return poisonResistance;
}

void CRace::MagicResistance(R32 value)
{
    magicResistance = value;
}

void CRace::PoisonResistance(R32 value)
{
    poisonResistance = value;
}

CRace& CRace::operator =(CRace& trgRace)
{
    memcpy(iSkills, trgRace.iSkills, sizeof(SKILLVAL) * SKILLS);
    raceName = trgRace.raceName;

    beardColours.resize(trgRace.beardColours.size());
    for (size_t bCtr = 0; bCtr < beardColours.size(); ++bCtr)
    {
        beardColours[bCtr].cMax = trgRace.beardColours[bCtr].cMax;
        beardColours[bCtr].cMin = trgRace.beardColours[bCtr].cMin;
    }

    hairColours.resize(trgRace.hairColours.size());
    for (size_t hCtr = 0; hCtr < hairColours.size(); ++hCtr)
    {
        hairColours[hCtr].cMax = trgRace.hairColours[hCtr].cMax;
        hairColours[hCtr].cMin = trgRace.hairColours[hCtr].cMin;
    }

    skinColours.resize(trgRace.skinColours.size());
    for (size_t sCtr = 0; sCtr < skinColours.size(); ++sCtr)
    {
        skinColours[sCtr].cMax = trgRace.skinColours[sCtr].cMax;
        skinColours[sCtr].cMin = trgRace.skinColours[sCtr].cMin;
    }

    bools = trgRace.bools;
    restrictGender = trgRace.restrictGender;

    racialEnemies.resize(trgRace.racialEnemies.size());
    for (size_t rCtr = 0; rCtr < racialEnemies.size(); ++rCtr)
        racialEnemies[rCtr] = trgRace.racialEnemies[rCtr];

    lightLevel = trgRace.lightLevel;
    nightVision = trgRace.nightVision;
    armourRestrict = trgRace.armourRestrict;

    memcpy(weathSecs, trgRace.weathDamage, sizeof(SECONDS) * WEATHNUM);
    memcpy(weathSecs, trgRace.weathDamage, sizeof(SI08) * WEATHNUM);

    languageMin = trgRace.languageMin;
    visDistance = trgRace.visDistance;
    poisonResistance = trgRace.poisonResistance;
    magicResistance = trgRace.magicResistance;

    return (*this);
}

size_t cRaces::Count(void) const
{
    return races.size();
}

}
