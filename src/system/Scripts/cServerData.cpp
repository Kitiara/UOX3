#include "uox3.h"
#include "scriptc.h"
#include "ssection.h"
#include "SQLManager.h"
#include <typeinfo>

#if UOX_PLATFORM != PLATFORM_WIN32
    #include <dirent.h>
#else
    #include <direct.h>
#endif

struct isEqual
{
    isEqual(UOX::UString targetuStr) : _targetuStr(targetuStr){};
    bool operator () (std::pair<UOX::UString, UOX::UString>& obj)
    {
        return obj.first == _targetuStr;
    };
    UOX::UString _targetuStr;
};

namespace UOX
{
#define MAX_TRACKINGTARGETS 128
#define SKILLTOTALCAP       7000
#define SKILLCAP            1000
#define STATCAP             325

const UI32 BIT_ANNOUNCESAVES          = 0;
const UI32 BIT_ANNOUNCEJOINPART       = 1;
const UI32 BIT_SERVERBACKUP           = 2;
const UI32 BIT_SHOOTONANIMALBACK      = 3;
const UI32 BIT_NPCTRAINING            = 4;
const UI32 BIT_LOOTDECAYSONCORPSE     = 5;
const UI32 BIT_GUARDSENABLED          = 6;
const UI32 BIT_PLAYDEATHANIMATION     = 7;
const UI32 BIT_AMBIENTFOOTSTEPS       = 8;
const UI32 BIT_INTERNALACCOUNTS       = 9;
const UI32 BIT_SHOWOFFLINEPCS         = 10;
const UI32 BIT_ROGUESTATUS            = 11;
const UI32 BIT_SNOOPISCRIME           = 12;
const UI32 BIT_PERSECUTIONSTATUS      = 13;
const UI32 BIT_SELLBYNAME             = 14;
const UI32 BIT_TRADESYSSTATUS         = 15;
const UI32 BIT_RANKSYSSTATUS          = 16;
const UI32 BIT_CUTSCROLLREQ           = 17;
const UI32 BIT_SHOWHITMESSAGE         = 18;
const UI32 BIT_ESCORTSTATUS           = 19;
const UI32 BIT_MONSTERSVSANIMALS      = 20;
const UI32 BIT_PETHUNGEROFFLINE       = 21;
const UI32 BIT_HIDEWHILEMOUNTED       = 22;
const UI32 BIT_OVERLOADPACKETS        = 23;
const UI32 BIT_ARMORAFFECTMANAREGEN   = 24;
const UI32 BIT_ANIMALSGUARDED         = 25;
const UI32 BIT_ADVANCEDPATHFIND       = 26;
const UI32 BIT_LOOTINGISCRIME         = 27;
const UI32 BIT_BASICTOOLTIPSONLY      = 28;
const UI32 BIT_GLOBALITEMDECAY        = 29;
const UI32 BIT_SCRIPTITEMSDECAYABLE   = 30;
const UI32 BIT_BASEITEMSDECAYABLE     = 31;
const UI32 BIT_ITEMDECAYINHOUSES      = 32;
const UI32 BIT_PAPERDOLLGUILDBUTTON   = 33;
const UI32 BIT_ATTSPEEDFROMSTAMINA    = 34;
const UI32 BIT_SHOWDAMAGENUMBERS      = 35;
const UI32 BIT_SERVERUSINGHSMULTIS    = 36;
const UI32 BIT_SERVERUSINGHSTILES     = 37;
const UI32 BIT_EXTENDEDSTARTINGSTATS  = 38;
const UI32 BIT_EXTENDEDSTARTINGSKILLS = 39;
const UI32 BIT_MAP0ISUOPWRAPPED       = 40;
const UI32 BIT_MAP1ISUOPWRAPPED       = 41;
const UI32 BIT_MAP2ISUOPWRAPPED       = 42;
const UI32 BIT_MAP3ISUOPWRAPPED       = 43;
const UI32 BIT_MAP4ISUOPWRAPPED       = 44;
const UI32 BIT_MAP5ISUOPWRAPPED       = 45;

void CServerData::ResetDefaults(void)
{
    resettingDefaults = true;

    SystemTimer(tSERVER_POTION, 10);

    ServerMoon(0, 0);
    ServerMoon(1, 0);
    WorldLightCurrentLevel(0);
    ServerTimeDay(0);
    ServerTimeHours(0);
    ServerTimeMinutes(0);
    ServerTimeSeconds(0);
    ServerTimeAMPM(0);
    
    // load defaults values
    ServerSkillCap(1000); // Same thing like ServerSkillTotalCap ??

    BuyThreshold(2000);

    for (int i = 0; i < 6; i++)
        MapIsUOPWrapped(i, false);

    resettingDefaults = false;
}

CServerData::CServerData(void)
{
    ResetDefaults();
}

CServerData::~CServerData() {}

void CServerData::RefreshIPs(void)
{
    struct hostent *lpHostEntry = NULL;

    std::vector<physicalServer>::iterator slIter;
    for (slIter = serverList.begin(); slIter != serverList.end(); ++slIter)
        if (!slIter->getDomain().empty())
        {
            lpHostEntry = gethostbyname(slIter->getDomain().c_str());
            if (lpHostEntry != NULL)
            {
                struct in_addr *pinaddr;
                pinaddr = ((struct in_addr*)lpHostEntry->h_addr);
                slIter->setIP(inet_ntoa(*pinaddr));
            }
        }
}

void CServerData::AddServer(UString data)
{
    physicalServer toAdd;
    if (data.sectionCount(";") == 2)
    {
        struct hostent *lpHostEntry = NULL;
        UString sname = data.section(";", 0, 0).stripWhiteSpace();
        UString sip = data.section(";", 1, 1).stripWhiteSpace();
        UString sport = data.section(";", 2, 2).stripWhiteSpace();

        toAdd.setName(sname);
        // Ok look up the data here see if its a number
        bool bDomain = true;
        bool canContinue = true;
        if ((lpHostEntry = gethostbyname(sip.c_str())) == NULL)
        {
            // this was not a domain name so check for IP address
            if ((lpHostEntry = gethostbyaddr(sip.c_str(), sip.size(), AF_INET)) == NULL)
                Console.Warning("Failed to translate %s. This shard will not show up on the shard listing!", sip.c_str()); // We get here it wasn't a valid IP either.
            bDomain = false;
        }

        if (bDomain) // Store the domain name for later then seeing as its a valid one
            toAdd.setDomain(sip);
        else // this was a valid ip address so we will use an ip instead so clear the domain string.
            toAdd.setDomain("");

        // Ok now the server itself uses the ip so we need to store that :) Means we only need to look thisip once 
        struct in_addr *pinaddr;
        pinaddr = ((struct in_addr*)lpHostEntry->h_addr);
        toAdd.setIP(inet_ntoa(*pinaddr));
        toAdd.setPort(sport.toUShort());
        serverList.push_back(toAdd);
    }
    else
        Console.Warning("Malformend serverlist entry: %s. This shard will not show up on the shard listing!", data.c_str());
}

std::string CServerData::ServerName(int id) const
{
    if (!serverList.empty() || id < serverList.size())
        return serverList[id].getName();
    return "Default UOX3 Server";
}

UI16 CServerData::ServerPort(int id) const
{
    if (!serverList.empty() || id < serverList.size())
        return serverList[id].getPort();
    return 2593;
}

void CServerData::ServerConsoleLog(UI08 setting)
{
    consolelogenabled = setting;
}

UI08 CServerData::ServerConsoleLogStatus(void) const
{
    return consolelogenabled;
}

void CServerData::ServerCrashProtection(UI08 setting)
{
    crashprotectionenabled = (setting > 5 && !(setting < 1)) ? 1 : setting;
}

UI08 CServerData::ServerCrashProtectionStatus(void) const
{
    return crashprotectionenabled;
}

void CServerData::ServerCommandPrefix(char cmdvalue)
{
    commandprefix = cmdvalue;
}

char CServerData::ServerCommandPrefix(void) const
{
    return commandprefix;
}

void CServerData::ServerAnnounceSaves(bool newVal)
{
    boolVals.set(BIT_ANNOUNCESAVES, newVal);
}

bool CServerData::ServerAnnounceSavesStatus(void) const
{
    return boolVals.test(BIT_ANNOUNCESAVES);
}

void CServerData::ServerJoinPartAnnouncements(bool newVal)
{
    boolVals.set(BIT_ANNOUNCEJOINPART, newVal);
}

bool CServerData::ServerJoinPartAnnouncementsStatus(void) const
{
    return boolVals.test(BIT_ANNOUNCEJOINPART);
}

void CServerData::ServerBackups(bool newVal)
{
    boolVals.set(BIT_SERVERBACKUP, newVal);
}

bool CServerData::ServerBackupStatus(void) const
{
    return boolVals.test(BIT_SERVERBACKUP);
}

void CServerData::ServerSavesTimer(UI32 timer)
{
    serversavestimer = timer;
    if (timer < 180) // 3 minutes is the lowest value you can set saves for
        serversavestimer = 300; // 5 minutes default
}

UI32 CServerData::ServerSavesTimerStatus(void) const
{
    return serversavestimer;
}

void CServerData::ServerSkillTotalCap(UI16 cap)
{
    skilltotalcap = cap;
    if (cap < 1) // Default is on second loop sleeping
        skilltotalcap = SKILLTOTALCAP;
}

UI16 CServerData::ServerSkillTotalCapStatus(void) const
{
    return skilltotalcap;
}

void CServerData::ServerSkillCap(UI16 cap)
{
    skillcap = cap;
    if (cap < 1) // Default is on second loop sleeping
        skillcap = SKILLCAP;
}

UI16 CServerData::ServerSkillCapStatus(void) const
{
    return skillcap;
}


void CServerData::ServerSkillDelay(UI08 skdelay)
{
    skilldelay = skdelay;
}

UI08 CServerData::ServerSkillDelayStatus(void) const
{
    return skilldelay;
}

void CServerData::ServerStatCap(UI16 cap)
{
    statcap = cap;
    if (cap < 1) // Default is on second loop sleeping
        statcap = STATCAP;
}

UI16 CServerData::ServerStatCapStatus(void) const
{
    return statcap;
}

void CServerData::MaxStealthMovement(SI16 value)
{
    maxstealthmovement = value;
}

SI16 CServerData::MaxStealthMovement(void) const
{
    return maxstealthmovement;
}

void CServerData::MaxStaminaMovement(SI16 value)
{
    maxstaminamovement = value;
}

SI16 CServerData::MaxStaminaMovement(void) const
{
    return maxstaminamovement;
}

TIMERVAL CServerData::BuildSystemTimeValue(cSD_TID timerID) const
{
    return BuildTimeValue(static_cast<R32>(SystemTimer(timerID)));
}

void CServerData::SystemTimer(cSD_TID timerid, UI16 value)
{
    serverTimers[timerid] = value;
}

UI16 CServerData::SystemTimer(cSD_TID timerid) const
{
    return serverTimers[timerid];
}

std::string CServerData::Directory(CSDDirectoryPaths dp)
{
    std::string rvalue;
    if (dp != CSDDP_COUNT)
        rvalue = serverDirectories[dp];
    return rvalue;
}

void CServerData::Directory(CSDDirectoryPaths dp, std::string value)
{
    if (dp == CSDDP_COUNT)
        return;

    std::string verboseDirectory;
    switch(dp)
    {
        case CSDDP_ROOT:
            verboseDirectory = "Root directory";
            break;
        case CSDDP_DATA:
            verboseDirectory = "Data directory";
            break;
        case CSDDP_DEFS:
            verboseDirectory = "DFNs directory";
            break;
        case CSDDP_SCRIPTS:
            verboseDirectory = "Scripts directory";
            break;
        case CSDDP_BACKUP:
            verboseDirectory = "Backup directory";
            break;
        case CSDDP_MSGBOARD:
            verboseDirectory = "Messageboard directory";
            break;
        case CSDDP_LOGS:
            verboseDirectory = "Logs directory";
            break;
        case CSDDP_DICTIONARIES:
            verboseDirectory = "Dictionary directory";
            break;
        case CSDDP_BOOKS:
            verboseDirectory = "Books directory";
            break;
        case CSDDP_COUNT:
        default:
            verboseDirectory = "Unknown directory";
            break;
    };
    // First, let's normalize the path name and fix common errors
    UString sText(value);
    // remove all trailing and leading spaces...
    sText = sText.stripWhiteSpace();
    if (sText.empty())
    {
        Console.Error(" %s directory is blank, set in uox.ini", verboseDirectory.c_str());
        Shutdown(FATAL_UOX3_DIR_NOT_FOUND);
        return;
    }

    // Make sure we're terminated with a directory separator
    // Just incase it's not set in the .ini
    // and convert the windows backward slashes to forward slashes

    sText = sText.fixDirectory();

    bool error = false;
    if (!resettingDefaults)
    {
        char curWorkingDir[MAX_PATH];
        GetModuleFileName(NULL, curWorkingDir, MAX_PATH);
        std::string::size_type pos = std::string(curWorkingDir).find_last_of("\\");

        int iResult = _chdir(sText.c_str());
        if (iResult != 0)
            error = true;
        else
            _chdir(std::string(curWorkingDir).substr(0, pos).c_str()); // move back to where we were
    }

    if (error)
    {
            Console.Error("%s %s does not exist", verboseDirectory.c_str(), sText.c_str());
            Shutdown(FATAL_UOX3_DIR_NOT_FOUND);
    }
    else
    {
        // There was a check to see if text was empty, to set to "./".  However, if text was blank, we bailed out in the
        // beginning of the routine
        serverDirectories[dp] = value;
    }
}

bool CServerData::ShootOnAnimalBack(void) const
{
    return boolVals.test(BIT_SHOOTONANIMALBACK);
}

void CServerData::ShootOnAnimalBack(bool newVal)
{
    boolVals.set(BIT_SHOOTONANIMALBACK, newVal);
}

bool CServerData::NPCTrainingStatus(void) const
{
    return boolVals.test(BIT_NPCTRAINING);
}

void CServerData::NPCTrainingStatus(bool newVal)
{
    boolVals.set(BIT_NPCTRAINING, newVal);
}

//o--------------------------------------------------------------------------o
//| Function/Class - void CServerData::dumpPaths(void)
//| Date           - 02/26/2002
//| Developer(s)   - EviLDeD
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Description    - Needed a function that would dump out the paths. If you
//|                  add any paths to the server please make sure that you
//|                  place exports to the console here to please so it can
//|                  belooked up if needed. 
//o--------------------------------------------------------------------------o
//| Returns        - N/A
//o--------------------------------------------------------------------------o
void CServerData::dumpPaths(void)
{
    Console.PrintSectionBegin();
    Console << "PathDump: \n";
    Console << "    Root      : " << Directory(CSDDP_ROOT) << "\n";
    Console << "    Mul(Data) : " << Directory(CSDDP_DATA) << "\n";
    Console << "    DFN(Defs) : " << Directory(CSDDP_DEFS) << "\n";
    Console << "    JScript   : " << Directory(CSDDP_SCRIPTS) << "\n";
    Console << "    MSGBoards : " << Directory(CSDDP_MSGBOARD) << "\n";
    Console << "    Books     : " << Directory(CSDDP_BOOKS) << "\n";
    Console << "    Backups   : " << Directory(CSDDP_BACKUP) << "\n";
    Console << "    Logs      : " << Directory(CSDDP_LOGS) << "\n";
    Console.PrintSectionBegin();
}

void CServerData::CorpseLootDecay(bool newVal)
{
    boolVals.set(BIT_LOOTDECAYSONCORPSE, newVal);
}

bool CServerData::CorpseLootDecay(void) const
{
    return boolVals.test(BIT_LOOTDECAYSONCORPSE);
}

void CServerData::GuardStatus(bool newVal)
{
    boolVals.set(BIT_GUARDSENABLED, newVal);
}

bool CServerData::GuardsStatus(void) const
{
    return boolVals.test(BIT_GUARDSENABLED);
}

void CServerData::DeathAnimationStatus(bool newVal)
{
    boolVals.set(BIT_PLAYDEATHANIMATION, newVal);
}

bool CServerData::DeathAnimationStatus(void) const
{
    return boolVals.test(BIT_PLAYDEATHANIMATION);
}

void CServerData::WorldAmbientSounds(SI16 value)
{
    if (value < 0)
        ambientsounds = 0;
    else
        ambientsounds = value;
}

SI16 CServerData::WorldAmbientSounds(void) const
{
    return ambientsounds;
}

void CServerData::AmbientFootsteps(bool newVal)
{
    boolVals.set(BIT_AMBIENTFOOTSTEPS, newVal);
}

bool CServerData::AmbientFootsteps(void) const
{
    return boolVals.test(BIT_AMBIENTFOOTSTEPS);
}

void CServerData::InternalAccountStatus(bool newVal)
{
    boolVals.set(BIT_INTERNALACCOUNTS, newVal);
}

bool CServerData::InternalAccountStatus(void) const
{
    return boolVals.test(BIT_INTERNALACCOUNTS);
}

void CServerData::ShowOfflinePCs(bool newVal)
{
    boolVals.set(BIT_SHOWOFFLINEPCS, newVal);
}

bool CServerData::ShowOfflinePCs(void) const
{
    return boolVals.test(BIT_SHOWOFFLINEPCS);
}

void CServerData::RogueStatus(bool newVal)
{
    boolVals.set(BIT_ROGUESTATUS, newVal);
}

bool CServerData::RogueStatus(void) const
{
    return boolVals.test(BIT_ROGUESTATUS);
}

void CServerData::SnoopIsCrime(bool newVal)
{
    boolVals.set(BIT_SNOOPISCRIME, newVal);
}
bool CServerData::SnoopIsCrime(void) const
{
    return boolVals.test(BIT_SNOOPISCRIME);
}

void CServerData::ExtendedStartingStats(bool newVal)
{
    boolVals.set(BIT_EXTENDEDSTARTINGSTATS, newVal);
}

bool CServerData::ExtendedStartingStats(void) const
{
    return boolVals.test(BIT_EXTENDEDSTARTINGSTATS);
}

void CServerData::ExtendedStartingSkills(bool newVal)
{
    boolVals.set(BIT_EXTENDEDSTARTINGSKILLS, newVal);
}

bool CServerData::ExtendedStartingSkills(void) const
{
    return boolVals.test(BIT_EXTENDEDSTARTINGSKILLS);
}

void CServerData::PlayerPersecutionStatus(bool newVal)
{
    boolVals.set(BIT_PERSECUTIONSTATUS, newVal);
}

bool CServerData::PlayerPersecutionStatus(void) const
{
    return boolVals.test(BIT_PERSECUTIONSTATUS);
}

void CServerData::SellByNameStatus(bool newVal)
{
    boolVals.set(BIT_SELLBYNAME, newVal);
}

bool CServerData::SellByNameStatus(void) const
{
    return boolVals.test(BIT_SELLBYNAME);
}

void CServerData::SellMaxItemsStatus(SI16 value)
{
    sellmaxitems = value;
}

SI16 CServerData::SellMaxItemsStatus(void) const
{
    return sellmaxitems;
}

void CServerData::TradeSystemStatus(bool newVal)
{
    boolVals.set(BIT_TRADESYSSTATUS, newVal);
}

bool CServerData::TradeSystemStatus(void) const
{
    return boolVals.test(BIT_TRADESYSSTATUS);
}

void CServerData::RankSystemStatus(bool newVal)
{
    boolVals.set(BIT_RANKSYSSTATUS, newVal);
}

bool CServerData::RankSystemStatus(void) const
{
    return boolVals.test(BIT_RANKSYSSTATUS);
}

void CServerData::CutScrollRequirementStatus(bool newVal)
{
    boolVals.set(BIT_CUTSCROLLREQ, newVal);
}

bool CServerData::CutScrollRequirementStatus(void) const
{
    return boolVals.test(BIT_CUTSCROLLREQ);
}

void CServerData::CheckItemsSpeed(R64 value)
{
    if (value < 0.0)
        checkitems = 0.0;
    else
        checkitems = value;
}

R64 CServerData::CheckItemsSpeed(void) const
{
    return checkitems;
}

void CServerData::CheckBoatSpeed(R64 value)
{
    if (value < 0.0)
        checkboats = 0.0;
    else
        checkboats = value;
}

R64 CServerData::CheckBoatSpeed(void) const
{
    return checkboats;
}

void CServerData::CheckNpcAISpeed(R64 value)
{
    if (value < 0.0)
        checknpcai = 0.0;
    else
        checknpcai = value;
}

R64 CServerData::CheckNpcAISpeed(void) const
{
    return checknpcai;
}

void CServerData::CheckSpawnRegionSpeed(R64 value)
{
    if (value < 0.0)
        checkspawnregions = 0.0;
    else
        checkspawnregions = value;
}

R64 CServerData::CheckSpawnRegionSpeed(void) const
{
    return checkspawnregions;
}

void CServerData::MsgBoardPostingLevel(UI08 value)
{
    msgpostinglevel = value;
}

UI08 CServerData::MsgBoardPostingLevel(void) const
{
    return msgpostinglevel;
}

void CServerData::MsgBoardPostRemovalLevel(UI08 value)
{
    msgremovallevel = value;
}

UI08 CServerData::MsgBoardPostRemovalLevel(void) const
{
    return msgremovallevel;
}

void CServerData::MineCheck(UI08 value)
{
    minecheck = value;
}

UI08 CServerData::MineCheck(void) const
{
    return minecheck;
}

void CServerData::CombatDisplayHitMessage(bool newVal)
{
    boolVals.set(BIT_SHOWHITMESSAGE, newVal);
}

bool CServerData::CombatDisplayHitMessage(void) const
{
    return boolVals.test(BIT_SHOWHITMESSAGE);
}

void CServerData::CombatDisplayDamageNumbers(bool newVal)
{
    boolVals.set(BIT_SHOWDAMAGENUMBERS, newVal);
}

bool CServerData::CombatDisplayDamageNumbers(void) const
{
    return boolVals.test(BIT_SHOWDAMAGENUMBERS);
}

void CServerData::CombatAttackSpeedFromStamina(bool newVal)
{
    boolVals.set(BIT_ATTSPEEDFROMSTAMINA, newVal);
}

bool CServerData::CombatAttackSpeedFromStamina(void) const
{
    return boolVals.test(BIT_ATTSPEEDFROMSTAMINA);
}

void CServerData::CombatNPCDamageRate(SI16 value)
{
    combatnpcdamagerate = value;
}

void CServerData::ServerUsingHSMultis(bool newVal)
{
    boolVals.set(BIT_SERVERUSINGHSMULTIS, newVal);
}

bool CServerData::ServerUsingHSMultis(void) const
{
    return boolVals.test(BIT_SERVERUSINGHSMULTIS);
}

void CServerData::ServerUsingHSTiles(bool newVal)
{
    boolVals.set(BIT_SERVERUSINGHSTILES, newVal);
}

bool CServerData::ServerUsingHSTiles(void) const
{
    return boolVals.test(BIT_SERVERUSINGHSTILES);
}

SI16 CServerData::CombatNPCDamageRate(void) const
{
    return combatnpcdamagerate;
}

void CServerData::CombatAttackStamina(SI16 value)
{
    combatattackstamina = value;
}

SI16 CServerData::CombatAttackStamina(void) const
{
    return combatattackstamina;
}

UI08 CServerData::SkillLevel(void) const
{
    return skilllevel;
}

void CServerData::SkillLevel(UI08 value)
{
    skilllevel = value;
}

void CServerData::EscortsEnabled(bool newVal)
{
    boolVals.set(BIT_ESCORTSTATUS, newVal);
}

bool CServerData::EscortsEnabled(void) const
{
    return boolVals.test(BIT_ESCORTSTATUS);
}

void CServerData::BasicTooltipsOnly(bool newVal)
{
    boolVals.set(BIT_BASICTOOLTIPSONLY, newVal);
}

bool CServerData::BasicTooltipsOnly(void) const
{
    return boolVals.test(BIT_BASICTOOLTIPSONLY);
}

//o--------------------------------------------------------------------------o
//| Function/Class - bool GlobalItemDecay()
//| Date           - 2/07/2010
//| Developer(s)   - Xuri
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Purpose        - Toggles on or off decay on global scale
//o--------------------------------------------------------------------------o
void CServerData::GlobalItemDecay(bool newVal)
{
    boolVals.set(BIT_GLOBALITEMDECAY, newVal);
}

bool CServerData::GlobalItemDecay(void) const
{
    return boolVals.test(BIT_GLOBALITEMDECAY);
}

//o--------------------------------------------------------------------------o
//| Function/Class  - bool ScriptItemsDecayable()
//| Date            - 2/07/2010
//| Developer(s)    - Xuri
//| Company/Team    - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Purpose         - Toggles default decay for items added through scripts
//o--------------------------------------------------------------------------o
void CServerData::ScriptItemsDecayable(bool newVal)
{
    boolVals.set(BIT_SCRIPTITEMSDECAYABLE, newVal);
}

bool CServerData::ScriptItemsDecayable(void) const
{
    return boolVals.test(BIT_SCRIPTITEMSDECAYABLE);
}

//o--------------------------------------------------------------------------o
//| Function/Class - bool BaseItemsDecayable()
//| Date           - 2/07/2010
//| Developer(s)   - Xuri
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Purpose        - Toggles default decay for base items added
//o--------------------------------------------------------------------------o
void CServerData::BaseItemsDecayable(bool newVal)
{
    boolVals.set(BIT_BASEITEMSDECAYABLE, newVal);
}

bool CServerData::BaseItemsDecayable(void) const
{
    return boolVals.test(BIT_BASEITEMSDECAYABLE);
}

//o--------------------------------------------------------------------------o
//| Function/Class - bool ItemDecayInHouses()
//| Date           - 2/07/2010
//| Developer(s)   - Xuri
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Purpose        - Toggles default decay for non-locked down items in houses
//o--------------------------------------------------------------------------o
void CServerData::ItemDecayInHouses(bool newVal)
{
    boolVals.set(BIT_ITEMDECAYINHOUSES, newVal);
}

bool CServerData::ItemDecayInHouses(void) const
{
    return boolVals.test(BIT_ITEMDECAYINHOUSES);
}

void CServerData::PaperdollGuildButton(bool newVal)
{
    boolVals.set(BIT_PAPERDOLLGUILDBUTTON, newVal);
}

bool CServerData::PaperdollGuildButton(void) const
{
    return boolVals.test(BIT_PAPERDOLLGUILDBUTTON);
}

void CServerData::CombatMonstersVsAnimals(bool newVal)
{
    boolVals.set(BIT_MONSTERSVSANIMALS, newVal);
}

bool CServerData::CombatMonstersVsAnimals(void) const
{
    return boolVals.test(BIT_MONSTERSVSANIMALS);
}

void CServerData::CombatAnimalsAttackChance(UI08 value)
{
    if (value > 100)
        value = 100;
    combatanimalattackchance = value;
}

UI08 CServerData::CombatAnimalsAttackChance(void) const
{
    return combatanimalattackchance;
}

void CServerData::HungerDamage(SI16 value)
{
    hungerdamage = value;
}

SI16 CServerData::HungerDamage(void) const
{
    return hungerdamage;
}

void CServerData::PetOfflineTimeout(UI16 value)
{
    petOfflineTimeout = value;
}

UI16 CServerData::PetOfflineTimeout(void) const
{
    return petOfflineTimeout;
}

void CServerData::PetHungerOffline(bool newVal)
{
    boolVals.set(BIT_PETHUNGEROFFLINE, newVal);
}

bool CServerData::PetHungerOffline(void) const
{
    return boolVals.test(BIT_PETHUNGEROFFLINE);
}

void CServerData::BuyThreshold(SI16 value)
{
    buyThreshold = value;
}

SI16 CServerData::BuyThreshold(void) const
{
    return buyThreshold;
}

//o--------------------------------------------------------------------------o
//| Function/Class - bool CharHideWhileMounted()
//| Date           - 09/22/2002
//| Developer(s)   - EviLDeD
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Description    - Toggle characters ability to hide whilst mounted
//o--------------------------------------------------------------------------o
void CServerData::CharHideWhileMounted(bool newVal)
{
    boolVals.set(BIT_HIDEWHILEMOUNTED, newVal);
}

bool CServerData::CharHideWhileMounted(void) const
{
    return boolVals.test(BIT_HIDEWHILEMOUNTED);
}

//o--------------------------------------------------------------------------o
//| Function/Class - R32 WeightPerStr()
//| Date           - 3/12/2003
//| Developer(s)   - Zane
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Purpose        - Amount of Weight one can hold per point of STR
//o--------------------------------------------------------------------------o
R32 CServerData::WeightPerStr(void) const
{
    return weightPerSTR;
}

void CServerData::WeightPerStr(R32 newVal)
{
    weightPerSTR = newVal;
}

//o--------------------------------------------------------------------------o
//| Function/Class - bool ServerOverloadPackets()
//| Date           - 11/20/2005
//| Developer(s)   - giwo
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Purpose        - Enables / Disables Packet handling in JS
//o--------------------------------------------------------------------------o
bool CServerData::ServerOverloadPackets(void) const
{
    return boolVals.test(BIT_OVERLOADPACKETS);
}

void CServerData::ServerOverloadPackets(bool newVal)
{
    boolVals.set(BIT_OVERLOADPACKETS, newVal);
}

//o--------------------------------------------------------------------------o
//| Function/Class - bool ArmorAffectManaRegen()
//| Date           - 3/20/2005
//| Developer(s)   - giwo
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Purpose        - Toggles whether or not armor affects mana regeneration
//o--------------------------------------------------------------------------o
bool CServerData::ArmorAffectManaRegen(void) const
{
    return boolVals.test(BIT_ARMORAFFECTMANAREGEN);
}

void CServerData::ArmorAffectManaRegen(bool newVal)
{
    boolVals.set(BIT_ARMORAFFECTMANAREGEN, newVal);
}

//o--------------------------------------------------------------------------o
//| Function/Class - bool AdvancedPathfinding()
//| Date           - 7/16/2005
//| Developer(s)   - giwo
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Purpose        - Toggles whether or not we use the A* Pathfinding routine
//o--------------------------------------------------------------------------o
void CServerData::AdvancedPathfinding(bool newVal)
{
    boolVals.set(BIT_ADVANCEDPATHFIND, newVal);
}

bool CServerData::AdvancedPathfinding(void) const
{
    return boolVals.test(BIT_ADVANCEDPATHFIND);
}

//o--------------------------------------------------------------------------o
//| Function/Class - bool MapIsUOPWrapped()
//| Date           - 3/14/2012
//| Developer(s)   - Xuri
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Purpose        - Toggles whether or not mapfiles are uop-wrapped
//o--------------------------------------------------------------------------o
void CServerData::MapIsUOPWrapped(UI08 mapNum, bool newVal)
{
    switch(mapNum)
    {
    case 0: boolVals.set(BIT_MAP0ISUOPWRAPPED, newVal); break;
    case 1: boolVals.set(BIT_MAP1ISUOPWRAPPED, newVal); break;
    case 2: boolVals.set(BIT_MAP2ISUOPWRAPPED, newVal); break;
    case 3: boolVals.set(BIT_MAP3ISUOPWRAPPED, newVal); break;
    case 4: boolVals.set(BIT_MAP4ISUOPWRAPPED, newVal); break;
    case 5: boolVals.set(BIT_MAP5ISUOPWRAPPED, newVal); break;
    default:
        break;
    }
}

bool CServerData::MapIsUOPWrapped(UI08 mapNum) const
{
    switch(mapNum)
    {
    case 0:  return boolVals.test(BIT_MAP0ISUOPWRAPPED);
    case 1:  return boolVals.test(BIT_MAP1ISUOPWRAPPED);
    case 2:  return boolVals.test(BIT_MAP2ISUOPWRAPPED);
    case 3:  return boolVals.test(BIT_MAP3ISUOPWRAPPED);
    case 4:  return boolVals.test(BIT_MAP4ISUOPWRAPPED);
    case 5:  return boolVals.test(BIT_MAP5ISUOPWRAPPED);
    default: return false;
    }
}

//o--------------------------------------------------------------------------o
//| Function/Class - bool LootingIsCrime()
//| Date           - 4/09/2007
//| Developer(s)   - grimson
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Purpose        - Toggles whether or not looting of corpses can be a crime
//o--------------------------------------------------------------------------o
void CServerData::LootingIsCrime(bool newVal)
{
    boolVals.set(BIT_LOOTINGISCRIME, newVal);
}

bool CServerData::LootingIsCrime(void) const
{
    return boolVals.test(BIT_LOOTINGISCRIME);
}

void CServerData::BackupRatio(SI16 value)
{
    backupRatio = value;
}

SI16 CServerData::BackupRatio(void) const
{
    return backupRatio;
}

void CServerData::CombatMaxRange(SI16 value)
{
    combatmaxrange = value;
}

SI16 CServerData::CombatMaxRange(void) const
{
    return combatmaxrange;
}

void CServerData::CombatArcherRange(SI16 value)
{
    combatarcherrange = value;
}

SI16 CServerData::CombatArcherRange(void) const
{
    return combatarcherrange;
}

void CServerData::CombatMaxSpellRange(SI16 value)
{
    combatmaxspellrange = value;
}

SI16 CServerData::CombatMaxSpellRange(void) const
{
    return combatmaxspellrange;
}

void CServerData::CombatExplodeDelay(UI16 value)
{
    combatExplodeDelay = value;
}

UI16 CServerData::CombatExplodeDelay(void) const
{
    return combatExplodeDelay;
}

void CServerData::CombatAnimalsGuarded(bool newVal)
{
    boolVals.set(BIT_ANIMALSGUARDED, newVal);
}

bool CServerData::CombatAnimalsGuarded(void) const
{
    return boolVals.test(BIT_ANIMALSGUARDED);
}

void CServerData::CombatNPCBaseFleeAt(SI16 value)
{
    combatnpcbasefleeat = value;
}

SI16 CServerData::CombatNPCBaseFleeAt(void) const
{
    return combatnpcbasefleeat;
}

void CServerData::CombatNPCBaseReattackAt(SI16 value)
{
    combatnpcbasereattackat = value;
}

SI16 CServerData::CombatNPCBaseReattackAt(void) const
{
    return combatnpcbasereattackat;
}

void CServerData::NPCWalkingSpeed(R32 value)
{
    npcWalkingSpeed = value;
}

R32 CServerData::NPCWalkingSpeed(void) const
{
    return npcWalkingSpeed;
}

void CServerData::NPCRunningSpeed(R32 value)
{
    npcRunningSpeed = value;
}

R32 CServerData::NPCRunningSpeed(void) const
{
    return npcRunningSpeed;
}

void CServerData::NPCFleeingSpeed(R32 value)
{
    npcFleeingSpeed = value;
}

R32 CServerData::NPCFleeingSpeed(void) const
{
    return npcFleeingSpeed;
}

void CServerData::TitleColour(UI16 value)
{
    titleColour = value;
}

UI16 CServerData::TitleColour(void) const
{
    return titleColour;
}

void CServerData::LeftTextColour(UI16 value)
{
    leftTextColour = value;
}

UI16 CServerData::LeftTextColour(void) const
{
    return leftTextColour;
}

void CServerData::RightTextColour(UI16 value)
{
    rightTextColour = value;
}

UI16 CServerData::RightTextColour(void) const
{
    return rightTextColour;
}

void CServerData::ButtonCancel(UI16 value)
{
    buttonCancel = value;
}

UI16 CServerData::ButtonCancel(void) const
{
    return buttonCancel;
}

void CServerData::ButtonLeft(UI16 value)
{
    buttonLeft = value;
}

UI16 CServerData::ButtonLeft(void) const
{
    return buttonLeft;
}

void CServerData::ButtonRight(UI16 value)
{
    buttonRight = value;
}

UI16 CServerData::ButtonRight(void) const
{
    return buttonRight;
}

void CServerData::BackgroundPic(UI16 value)
{
    backgroundPic = value;
}

UI16 CServerData::BackgroundPic(void) const
{
    return backgroundPic;
}

void CServerData::TownNumSecsPollOpen(UI32 value)
{
    numSecsPollOpen = value;
}

UI32 CServerData::TownNumSecsPollOpen(void) const
{
    return numSecsPollOpen;
}

void CServerData::TownNumSecsAsMayor(UI32 value)
{
    numSecsAsMayor = value;
}

UI32 CServerData::TownNumSecsAsMayor(void) const
{
    return numSecsAsMayor;
}

void CServerData::TownTaxPeriod(UI32 value)
{
    taxPeriod = value;
}

UI32 CServerData::TownTaxPeriod(void) const
{
    return taxPeriod;
}

void CServerData::TownGuardPayment(UI32 value)
{
    guardPayment = value;
}

UI32 CServerData::TownGuardPayment(void) const
{
    return guardPayment;
}

void CServerData::RepMaxKills(UI16 value)
{
    maxmurdersallowed = value;
}

UI16 CServerData::RepMaxKills(void) const
{
    return maxmurdersallowed;
}

void CServerData::ResLogs(SI16 value)
{
    logsperarea = value;
}

SI16 CServerData::ResLogs(void) const
{
    return logsperarea;
}

void CServerData::ResLogTime(UI16 value)
{
    logsrespawntimer = value;
}

UI16 CServerData::ResLogTime(void) const
{
    return logsrespawntimer;
}

void CServerData::ResLogArea(UI16 value)
{
    if (value < 10)
        value = 10;
    logsrespawnarea = value;
}

UI16 CServerData::ResLogArea(void) const
{
    return logsrespawnarea;
}

void CServerData::ResOre(SI16 value)
{
    oreperarea = value;
}

SI16 CServerData::ResOre(void) const
{
    return oreperarea;
}

void CServerData::ResOreTime(UI16 value)
{
    orerespawntimer = value;
}

UI16 CServerData::ResOreTime(void) const
{
    return orerespawntimer;
}

void CServerData::ResOreArea(UI16 value)
{
    if (value < 10)
        value = 10;
    orerespawnarea = value;
}

UI16 CServerData::ResOreArea(void) const
{
    return orerespawnarea;
}

void CServerData::AccountFlushTimer(R64 value)
{
    flushTime = value;
}

R64 CServerData::AccountFlushTimer(void) const
{
    return flushTime;
}

void CServerData::SetClientFeature(ClientFeatures bitNum, bool nVal)
{
    clientFeatures.set(bitNum, nVal);
}

bool CServerData::GetClientFeature(ClientFeatures bitNum) const
{
    return clientFeatures.test(bitNum);
}

UI32 CServerData::GetClientFeatures(void) const
{
    return static_cast<UI32>(clientFeatures.to_ulong());
}

void CServerData::SetClientFeatures(UI32 nVal)
{
    char strnVal[256];
    _snprintf(strnVal, sizeof(strnVal), "%d", nVal);
    std::stringstream strValue;
    strValue << strnVal;
    int intValue;
    strValue >> intValue;
    clientFeatures = std::bitset<CF_BIT_COUNT>(intValue);
}


void CServerData::SetServerFeature(ServerFeatures bitNum, bool nVal)
{
    serverFeatures.set(bitNum, nVal);
}

bool CServerData::GetServerFeature(ServerFeatures bitNum) const
{
    return serverFeatures.test(bitNum);
}

size_t CServerData::GetServerFeatures(void) const
{
    return serverFeatures.to_ulong();
}

void CServerData::SetServerFeatures(size_t nVal)
{
    char strnVal[256];
    _snprintf(strnVal, sizeof(strnVal), "%d", nVal);
    std::stringstream strValue;
    strValue << strnVal;
    int intValue;
    strValue >> intValue;
    serverFeatures = std::bitset<SF_BIT_COUNT>(intValue);
}

//o------------------------------------------------------------------------
//| Function   - Load()
//| Programmer - EviLDeD
//| Date       - January 13, 2001
//o------------------------------------------------------------------------
//| Purpose    - Load up the uox.ini file and parse it into the internals
//| Returns    - pointer to the valid inmemory serverdata storage(this)
//|              NULL is there is an error, or invalid file type
//o------------------------------------------------------------------------
void CServerData::Load(void)
{
    const UString fileName = Directory(CSDDP_ROOT) + "uox.ini";
    ParseINI(fileName);
}

void CServerData::TrackingBaseRange(UI16 value)
{
    trackingbaserange = value;
}

UI16 CServerData::TrackingBaseRange(void) const
{
    return trackingbaserange;
}

void CServerData::TrackingMaxTargets(UI08 value)
{
    if (value >= MAX_TRACKINGTARGETS)
        trackingmaxtargets = MAX_TRACKINGTARGETS;
    else
        trackingmaxtargets = value;
}

UI08 CServerData::TrackingMaxTargets(void) const
{
    return trackingmaxtargets;
}

void CServerData::TrackingBaseTimer(UI16 value)
{
    trackingbasetimer = value;
}

UI16 CServerData::TrackingBaseTimer(void) const
{
    return trackingbasetimer;
}

void CServerData::TrackingRedisplayTime(UI16 value)
{
    trackingmsgredisplaytimer = value;
}

UI16 CServerData::TrackingRedisplayTime(void) const
{
    return trackingmsgredisplaytimer;
}

//o--------------------------------------------------------------------------o
//| Function/Class - bool CServerData::ParseINI(const std::string filename)
//| Date           - 02/26/2001
//| Developer(s)   - EviLDeD
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Description    - Parse the uox.ini file into its required information.
//|
//| Modification   - 02/26/2002 - Make sure that we parse out the logs, access
//|                               and other directories that we were not parsing
//o--------------------------------------------------------------------------o
bool CServerData::ParseINI(const std::string& filename)
{
    bool rvalue = false;
    if (!filename.empty())
    {
        Console << "Processing INI Settings  ";

        FILE *file;
        errno_t err;

        err = fopen_s(&file, filename.c_str(), "r");
        if (err == 2)
            Console.Warning("%s File not found, Using default settings.", filename.c_str());

        std::ifstream config (filename.c_str());
        if (config.is_open())
        {
            while (config.good())
            {
                UString line;
                getline(config, line);
                line = line.simplifyWhiteSpace();
                if (line.empty() || line.at(0) == '#' || line.sectionCount("=") == 0) // # is comment line
                    continue;

                UString set = line.section("=", 0, 0).simplifyWhiteSpace();
                if (set.empty())
                    continue;

                settings.push_back(std::make_pair(set.upper(), line.section("=", 1, 1).simplifyWhiteSpace()));
            }
            config.close();
        }

        // CONNECTION AND DIRECTORIES
        SQLManager::getSingleton().SetDatabaseInfo(GetDefaultValue("DatabaseInfo", "127.0.0.1;3306;uox3;uox3;uo")); // No support for multiple connections from different ips yet

        char curWorkingDir[MAX_PATH];
        GetModuleFileName(NULL, curWorkingDir, MAX_PATH);
        std::string::size_type pos = std::string(curWorkingDir).find_last_of("\\");
        UString dir(std::string(curWorkingDir).substr(0, pos));
        dir = dir.fixDirectory();

        Directory(CSDDP_ROOT, GetDefaultValue("Directory", dir));
        Directory(CSDDP_DATA, GetDefaultValue("DataDirectory", dir+"muldata/"));
        Directory(CSDDP_DEFS, GetDefaultValue("DefsDirectory", dir+"dfndata/"));
        Directory(CSDDP_BOOKS, GetDefaultValue("BooksDirectory", dir+"books/"));
        Directory(CSDDP_SCRIPTS, GetDefaultValue("ScriptsDirectory", dir+"js/"));
        Directory(CSDDP_BACKUP, GetDefaultValue("BackupDirectory", dir+"archives/"));
        Directory(CSDDP_MSGBOARD, GetDefaultValue("MsgBoardDirectory", dir+"msgboards/"));
        Directory(CSDDP_LOGS, GetDefaultValue("LogsDirectory", dir+"logs/"));
        Directory(CSDDP_DICTIONARIES, GetDefaultValue("DictionaryDirectory", dir+"dictionaries/"));

        // SYSTEM SETTINGS
        ServerNetRcvTimeout(GetDefaultValue("NetRcvTimeout", (UI32)0).toULong());
        ServerNetRetryCount(GetDefaultValue("NetRetryCount", (UI32)0).toULong());
        ServerConsoleLog(GetDefaultValue("ConsoleLog", (UI08)1).toUByte());
        ServerCrashProtection(GetDefaultValue("CrashProtection", (UI08)1).toUByte());
        ServerCommandPrefix(GetDefaultValue("CommandPrefix", '\'').at(0));
        ServerAnnounceSaves(GetDefaultValue("AnnounceWorldSaves", true).toBoolean());
        ServerJoinPartAnnouncements(GetDefaultValue("JoinPartMsgs", true).toBoolean());
        ServerBackups(GetDefaultValue("BackupsEnabled", true).toBoolean());
        BackupRatio(GetDefaultValue("BackupSaveRatio", (SI16)5).toShort());
        ServerSavesTimer(GetDefaultValue("SavesTimer", (UI32)300).toULong());
        ServerUOGEnabled(GetDefaultValue("UogEnabled", false).toBoolean());

        // CLIENT SUPPORT
        ClientSupport4000(GetDefaultValue("ClientSupport4000", true).toBoolean());
        ClientSupport5000(GetDefaultValue("ClientSupport5000", true).toBoolean());
        ClientSupport6000(GetDefaultValue("ClientSupport6000", true).toBoolean());
        ClientSupport6050(GetDefaultValue("ClientSupport6050", true).toBoolean());
        ClientSupport7000(GetDefaultValue("ClientSupport7000", true).toBoolean());
        ClientSupport7090(GetDefaultValue("ClientSupport7090", true).toBoolean());
        ClientSupport70160(GetDefaultValue("ClientSupport70160", true).toBoolean());
        ClientSupport70240(GetDefaultValue("ClientSupport70240", true).toBoolean());

        // SERVER LIST
        AddServer(GetDefaultValue("ServerList", "Default UOX3 Server;127.0.0.1;2593"));

        // SKILL AND STATS
        SkillLevel(GetDefaultValue("SkillLevel", (UI08)5).toUByte());
        ServerSkillTotalCap(GetDefaultValue("SkillCap", (UI16)7000).toUShort());
        ServerSkillDelay(GetDefaultValue("SkillDelay", (UI08)5).toUByte());
        ServerStatCap(GetDefaultValue("StatCap", (UI16)325).toUShort());
        ExtendedStartingStats(GetDefaultValue("ExtendedStartingStats", true).toBoolean());
        ExtendedStartingSkills(GetDefaultValue("ExtendedStartingSkills", true).toBoolean());
        MaxStealthMovement(GetDefaultValue("MaxStealthMovements", (SI16)10).toShort());
        MaxStaminaMovement(GetDefaultValue("MaxStaminaMovements", (SI16)15).toShort());
        SnoopIsCrime(GetDefaultValue("SnoopIsCrime", false).toBoolean());
        ArmorAffectManaRegen(GetDefaultValue("ArmorAffectManaRegen", true).toBoolean());

        // TIMERS
        SystemTimer(tSERVER_CORPSEDECAY, GetDefaultValue("CorpseDecayTimer", (UI16)900).toUShort());
        SystemTimer(tSERVER_WEATHER, GetDefaultValue("WeatherTimer", (UI16)60).toUShort());
        SystemTimer(tSERVER_SHOPSPAWN, GetDefaultValue("ShopSpawnTimer", (UI16)300).toUShort());
        SystemTimer(tSERVER_DECAY, GetDefaultValue("DecayTimer", (UI16)300).toUShort());
        SystemTimer(tSERVER_INVISIBILITY, GetDefaultValue("InvisibilityTimer", (UI16)60).toUShort());
        SystemTimer(tSERVER_OBJECTUSAGE, GetDefaultValue("ObjectUseTimer", (UI16)1).toUShort());
        SystemTimer(tSERVER_GATE, GetDefaultValue("GateTimer", (UI16)30).toUShort());
        SystemTimer(tSERVER_POISON, GetDefaultValue("PoisonTimer", (UI16)180).toUShort());
        SystemTimer(tSERVER_LOGINTIMEOUT, GetDefaultValue("LoginTimeout", (UI16)300).toUShort());
        SystemTimer(tSERVER_HITPOINTREGEN, GetDefaultValue("HitpointRegenTimer", (UI16)8).toUShort());
        SystemTimer(tSERVER_STAMINAREGEN, GetDefaultValue("StaminaRegenTimer", (UI16)3).toUShort());
        SystemTimer(tSERVER_MANAREGEN, GetDefaultValue("ManaRegenTimer", (UI16)5).toUShort());
        SystemTimer(tSERVER_FISHINGBASE, GetDefaultValue("BaseFishingTimer", (UI16)10).toUShort());
        SystemTimer(tSERVER_FISHINGRANDOM, GetDefaultValue("RandomFishingTimer", (UI16)5).toUShort());
        SystemTimer(tSERVER_SPIRITSPEAK, GetDefaultValue("SpiritSpeakTimer", (UI16)30).toUShort());
        SystemTimer(tSERVER_PETOFFLINECHECK, GetDefaultValue("PetOfflineCheckTimer", (UI16)600).toUShort());

        // SETTINGS
        CorpseLootDecay(GetDefaultValue("LootDecaysWithCorpse", true).toBoolean());
        GuardStatus(GetDefaultValue("GuardsActive", true).toBoolean());
        DeathAnimationStatus(GetDefaultValue("DeathAnimation", true).toBoolean());
        WorldAmbientSounds(GetDefaultValue("AmbientSounds", (SI16)5).toShort());
        AmbientFootsteps(GetDefaultValue("AmbientFootSteps", false).toBoolean());
        InternalAccountStatus(GetDefaultValue("InternalAccountCreation", false).toBoolean());
        ShowOfflinePCs(GetDefaultValue("ShowOfflinePcs", true).toBoolean());
        RogueStatus(GetDefaultValue("RoguesEnabled", true).toBoolean());
        PlayerPersecutionStatus(GetDefaultValue("PlayerPersecution", false).toBoolean());
        AccountFlushTimer(GetDefaultValue("AccountFlush", (R64)0.0).toDouble());
        SellByNameStatus(GetDefaultValue("SellByName", false).toBoolean());
        SellMaxItemsStatus(GetDefaultValue("SellMaxItems", (SI16)5).toShort());
        TradeSystemStatus(GetDefaultValue("TradeSystem", false).toBoolean());
        RankSystemStatus(GetDefaultValue("RankSystem", true).toBoolean());
        CutScrollRequirementStatus(GetDefaultValue("CutScrollRequirements", true).toBoolean());
        NPCTrainingStatus(GetDefaultValue("NpcTrainingEnabled", true).toBoolean());
        CharHideWhileMounted(GetDefaultValue("HideWhileMounted", true).toBoolean());
        WeightPerStr(GetDefaultValue("WeightPerStr", (R32)3.5).toFloat());
        SystemTimer(tSERVER_POLYMORPH, GetDefaultValue("PolyDuration", (UI16)90).toUShort());

        UI32 clientFeature = pow(2.0f, CF_BIT_CHAT)+pow(2.0f, CF_BIT_UOR)+pow(2.0f, CF_BIT_TD)+pow(2.0f, CF_BIT_LBR)+
            pow(2.0f, CF_BIT_AOS)+pow(2.0f, CF_BIT_SIXCHARS)+pow(2.0f, CF_BIT_SE)+pow(2.0f, CF_BIT_ML)+pow(2.0f, CF_BIT_EXPANSION);
        SetClientFeatures(GetDefaultValue("ClientFeatures", clientFeature).toULong());

        UI32 serverFeature = pow(2.0f, SF_BIT_CONTEXTMENUS)+pow(2.0f, SF_BIT_AOS)+pow(2.0f, SF_BIT_SIXCHARS)+
            pow(2.0f, SF_BIT_SE)+pow(2.0f, SF_BIT_ML);
        SetServerFeatures(GetDefaultValue("ServerFeatures", serverFeature).toULong());

        ServerOverloadPackets(GetDefaultValue("OverloadPackets", true).toBoolean());
        AdvancedPathfinding(GetDefaultValue("AdvancedPathfinding", true).toBoolean());
        LootingIsCrime(GetDefaultValue("LootingIsCrime", true).toBoolean());
        BasicTooltipsOnly(GetDefaultValue("BasicTooltipsOnly", false).toBoolean());
        GlobalItemDecay(GetDefaultValue("GlobalItemDecay", true).toBoolean());
        ScriptItemsDecayable(GetDefaultValue("ScriptItemsDecayable", true).toBoolean());
        BaseItemsDecayable(GetDefaultValue("BaseItemsDecayable", false).toBoolean());
        ItemDecayInHouses(GetDefaultValue("ItemDecayInHouses", false).toBoolean());
        PaperdollGuildButton(GetDefaultValue("PaperdollGuildButton", true).toBoolean());

        // SPEEDUP
        CheckItemsSpeed(GetDefaultValue("CheckItems", (R64)1.5).toDouble());
        CheckBoatSpeed(GetDefaultValue("CheckBoats", (R64)0.65).toDouble());
        CheckNpcAISpeed(GetDefaultValue("CheckNpcAi", (R64)1).toDouble());
        CheckSpawnRegionSpeed(GetDefaultValue("CheckSpawnRegions", (R64)30).toDouble());
        NPCWalkingSpeed(GetDefaultValue("NpcMovementSpeed", (R32)0.5).toFloat());
        NPCRunningSpeed(GetDefaultValue("NpcRunningSpeed", (R32)0.2).toFloat());
        NPCFleeingSpeed(GetDefaultValue("NpcFleeingSpeed", (R32)0.4).toFloat());

        // MESSAGE BOARDS
        MsgBoardPostingLevel(GetDefaultValue("PostingLevel", (UI08)0).toUByte());
        MsgBoardPostRemovalLevel(GetDefaultValue("RemovalLevel", (UI08)0).toUByte());

        // ESCORT
        EscortsEnabled(GetDefaultValue("EscortEnabled", true).toBoolean());
        SystemTimer(tSERVER_ESCORTWAIT, GetDefaultValue("EscortInitExpire", (UI16)900).toUShort());
        SystemTimer(tSERVER_ESCORTACTIVE, GetDefaultValue("EscortActiveExpire", (UI16)600).toUShort());
        SystemTimer(tSERVER_ESCORTDONE, GetDefaultValue("EscortDoneExpire", (UI16)600).toUShort());

        // WORLDLIGHT
        DungeonLightLevel((LIGHTLEVEL)GetDefaultValue("DungeonLevel", (UI16)3).toUShort());
        WorldLightBrightLevel((LIGHTLEVEL)GetDefaultValue("BrightLevel", (UI16)0).toUShort());
        WorldLightDarkLevel((LIGHTLEVEL)GetDefaultValue("DarkLevel", (UI16)5).toUShort());
        ServerSecondsPerUOMinute(GetDefaultValue("SecondsPerUoMinute", (UI16)5).toUShort());

        // TRACKING
        TrackingBaseRange(GetDefaultValue("BaseRange", (UI16)10).toUShort());
        TrackingBaseTimer(GetDefaultValue("BaseTimer", (UI16)30).toUShort());
        TrackingMaxTargets(GetDefaultValue("MaxTargets", (UI08)20).toUByte());
        TrackingRedisplayTime(GetDefaultValue("MsgRedisplayTime", (UI16)30).toUShort());

        // REPUTATION
        SystemTimer(tSERVER_MURDERDECAY, GetDefaultValue("MurderDecayTimer", (UI16)60).toUShort());
        RepMaxKills(GetDefaultValue("MaxKills", (UI16)4).toUShort());
        SystemTimer(tSERVER_CRIMINAL, GetDefaultValue("CriminalTimer", (UI16)120).toUShort());

        // RESOURCES
        MineCheck(GetDefaultValue("MineCheck", (UI08)1).toUByte());
        ResOre(GetDefaultValue("OrePerArea", (SI16)10).toShort());
        ResOreTime(GetDefaultValue("OreRespawnTimer", (UI16)600).toUShort());
        ResOreArea(GetDefaultValue("OreRespawnArea", (UI16)10).toUShort());
        ResLogs(GetDefaultValue("LogsPerArea", (SI16)3).toShort());
        ResLogTime(GetDefaultValue("LogsRespawnTimer", (UI16)600).toUShort());
        ResLogArea(GetDefaultValue("LogsRespawnArea", (UI16)10).toUShort());

        // HUNGER
        SystemTimer(tSERVER_HUNGERRATE, GetDefaultValue("HungerRate", (UI16)6000).toUShort());
        HungerDamage(GetDefaultValue("HungerDmgVal", (SI16)2).toShort());
        PetHungerOffline(GetDefaultValue("PetHungerOffline", true).toBoolean());
        PetOfflineTimeout(GetDefaultValue("PetOfflineTimeout", (UI16)5).toUShort());

        // COMBAT
        CombatMaxRange(GetDefaultValue("MaxRange", (SI16)10).toShort());
        CombatArcherRange(GetDefaultValue("ArcherRange", (SI16)7).toShort());
        CombatMaxSpellRange(GetDefaultValue("SpellMaxRange", (SI16)10).toShort());
        CombatDisplayHitMessage(GetDefaultValue("DisplayHitMsg", true).toBoolean());
        CombatDisplayDamageNumbers(GetDefaultValue("DisplayDamageNumbers", true).toBoolean());
        CombatMonstersVsAnimals(GetDefaultValue("MonstersVsAnimals", true).toBoolean());
        CombatAnimalsAttackChance(GetDefaultValue("AnimalAttackChance", (UI08)15).toUByte());
        CombatAnimalsGuarded(GetDefaultValue("AnimalsGuarded", false).toBoolean());
        CombatNPCDamageRate(GetDefaultValue("NpcDamageRate", (SI16)2).toShort());
        CombatNPCBaseFleeAt(GetDefaultValue("NpcBaseFleeAt", (SI16)20).toShort());
        CombatNPCBaseReattackAt(GetDefaultValue("NpcBaseReattackAt", (SI16)40).toShort());
        CombatAttackStamina(GetDefaultValue("AttackStamina", (SI16)-2).toShort());
        CombatAttackSpeedFromStamina(GetDefaultValue("AttackSpeedFromStamina", true).toBoolean());
        ShootOnAnimalBack(GetDefaultValue("ShootOnAnimalBack", false).toBoolean());
        CombatExplodeDelay(GetDefaultValue("CombatExplodeDelay", (UI32)2).toULong());

        // START LOCATIONS
        ServerLocation(GetDefaultValue("Location", "Yew;Center;545;982;0;0;1075072"));
        ServerLocation(GetDefaultValue("Location", "Minoc;Tavern;2477;411;15;0;1075073"));
        ServerLocation(GetDefaultValue("Location", "Britain;Sweet Dreams Inn;1495;1629;10;0;1075074"));
        ServerLocation(GetDefaultValue("Location", "Moonglow;Docks;4406;1045;0;0;1075075"));
        ServerLocation(GetDefaultValue("Location", "Trinsic;West Gate;1832;2779;0;0;1075076"));
        ServerLocation(GetDefaultValue("Location", "Magincia;Docks;3675;2259;20;0;1075077"));
        ServerLocation(GetDefaultValue("Location", "Jhelom;Docks;1492;3696;0;0;1075078"));
        ServerLocation(GetDefaultValue("Location", "Skara Brae;Docks;639;2236;0;0;1075079"));
        ServerLocation(GetDefaultValue("Location", "Vesper;Ironwood Inn;2771;977;0;0;1075080"));

        // STARTUP
        ServerStartGold(GetDefaultValue("StartGold", (SI16)1000).toShort());
        ServerStartPrivs(GetDefaultValue("StartPrivs", (UI16)0).toUShort());

        // GUMPS
        TitleColour(GetDefaultValue("TitleColour", (UI16)0).toUShort());
        LeftTextColour(GetDefaultValue("LeftTextColour", (UI16)0).toUShort());
        RightTextColour(GetDefaultValue("RightTextColour", (UI16)0).toUShort());
        ButtonCancel(GetDefaultValue("ButtonCancel", (UI16)4017).toUShort());
        ButtonLeft(GetDefaultValue("ButtonLeft", (UI16)4014).toUShort());        
        ButtonRight(GetDefaultValue("ButtonRight", (UI16)4005).toUShort());
        BackgroundPic(GetDefaultValue("BackgroundPic", (UI16)2600).toUShort());

        // TOWNS
        TownNumSecsPollOpen(GetDefaultValue("PollTime", (UI32)3600).toULong());
        TownNumSecsAsMayor(GetDefaultValue("MayorTime", (UI32)36000).toULong());
        TownTaxPeriod(GetDefaultValue("TaxPeriod", (UI32)1800).toULong());
        TownGuardPayment(GetDefaultValue("GuardsPaid", (UI32)3600).toULong());

        Console << "Connecting to database ... ";
        if (!SQLManager::getSingleton().Connect())
            Console.PrintFailed();
        Console.PrintDone();
    }
    return rvalue;
}

template<typename T>
UString CServerData::GetDefaultValue(UString name, T def)
{
    UString uStr;
    auto itr = std::find_if (settings.begin(), settings.end(), isEqual(name.upper()));
    if (itr == settings.end())
        Console.Warning("%s is missing, using default.", name.c_str());
    else if (itr->second.empty())
        Console.Warning("%s hasn't a specified value, using default.", name.c_str());

    if (itr == settings.end() || itr->second.empty())
    {
        std::stringstream ss;
        ss << def;
        uStr = ss.str();
    }
    else
        uStr = itr->second;

    settings.erase(itr);
    return uStr;
}

void CServerData::ServerStartGold(SI16 value)
{
    if (value >= 0)
        startgold = value;
}

SI16 CServerData::ServerStartGold(void) const
{
    return startgold;
}

void CServerData::ServerStartPrivs(UI16 value)
{
    startprivs = value;
}

UI16 CServerData::ServerStartPrivs(void) const
{
    return startprivs;
}

void CServerData::ServerMoon(SI16 slot, SI16 value)
{
    if (slot >= 0 && slot <= 1 && value >= 0)
        moon[slot] = value;
}

SI16 CServerData::ServerMoon(SI16 slot) const
{
    SI16 rvalue = -1;
    if (slot >= 0 && slot <= 1)
        rvalue = moon[slot];
    return rvalue;
}

void CServerData::DungeonLightLevel(LIGHTLEVEL value)
{
    dungeonlightlevel = value;
}

LIGHTLEVEL CServerData::DungeonLightLevel(void) const
{
    return dungeonlightlevel;
}

void CServerData::WorldLightCurrentLevel(LIGHTLEVEL value)
{
    currentlightlevel = value;
}

LIGHTLEVEL CServerData::WorldLightCurrentLevel(void) const
{
    return currentlightlevel;
}

void CServerData::WorldLightBrightLevel(LIGHTLEVEL value)
{
    brightnesslightlevel = value;
}

LIGHTLEVEL CServerData::WorldLightBrightLevel(void) const
{
    return brightnesslightlevel;
}

void CServerData::WorldLightDarkLevel(LIGHTLEVEL value)
{
    darknesslightlevel=value;
}

LIGHTLEVEL CServerData::WorldLightDarkLevel(void) const
{
    return darknesslightlevel;
}

void CServerData::ServerLocation(std::string toSet)
{
    UString temp(toSet);
    if (temp.sectionCount(";") == 6) // Wellformed server location
    {
        STARTLOCATION toAdd;
        toAdd.x = temp.section(";", 2, 2).toShort();
        toAdd.y = temp.section(";", 3, 3).toShort();
        toAdd.z = temp.section(";", 4, 4).toShort();
        toAdd.worldNum = temp.section(";", 5, 5).toShort();
        toAdd.clilocDesc = temp.section(";", 6, 6).toLong();
        strcpy(toAdd.oldTown, temp.section(";", 0, 0).c_str());
        strcpy(toAdd.oldDescription, temp.section(";", 1, 1).c_str());
        strcpy(toAdd.newTown, temp.section(";", 0, 0).c_str());
        strcpy(toAdd.newDescription, temp.section(";", 1, 1).c_str());
        startlocations.push_back(toAdd);
    }
    else
        Console.Error("Malformed location entry in ini file");
}

LPSTARTLOCATION CServerData::ServerLocation(size_t locNum)
{
    LPSTARTLOCATION rvalue = NULL;
    if (locNum < startlocations.size())
        rvalue = &startlocations[locNum];
    return rvalue;
}

size_t CServerData::NumServerLocations(void) const
{
    return startlocations.size();
}

UI16 CServerData::ServerSecondsPerUOMinute(void) const
{
    return secondsperuominute;
}

void CServerData::ServerSecondsPerUOMinute(UI16 newVal)
{
    secondsperuominute = newVal;
}

//o--------------------------------------------------------------------------o
//| Function/Class - void CServerData::SaveTime(void)
//| Date           - January 28th, 2007
//| Developer(s)   - giwo
//| Company/Team   - UOX3 DevTeam
//o--------------------------------------------------------------------------o
//| Description    - Outputs server time information to time.wsc in the /shared/ directory
//o--------------------------------------------------------------------------o    
void CServerData::SaveTime(void)
{
    std::stringstream Str;
    Str << "REPLACE INTO time (currentlight,day,hour,minute,ampm,moon) VALUES ('";
    Str << int((UI08)static_cast<UI16>(WorldLightCurrentLevel())) << "', ";
    Str << "'" << ServerTimeDay() << "', ";
    Str << "'" << static_cast<UI16>(ServerTimeHours()) << "', ";
    Str << "'" << static_cast<UI16>(ServerTimeMinutes()) << "', ";
    Str << "'" << (ServerTimeAMPM() ? 1 : 0) << "', ";
    Str << "'" << ServerMoon(0) << "," << ServerMoon(1) << "')";
    SQLManager::getSingleton().ExecuteQuery(Str.str());
}

void ReadWorldTagData(std::ifstream &inStream, UString &tag, UString &data);

void CServerData::LoadTime(void)
{
    int index;
    if (SQLManager::getSingleton().ExecuteQuery("SELECT currentlight,day,hour,minute,ampm,moon FROM time", &index, false))
    {
        int ColumnCount = mysql_num_fields(SQLManager::getSingleton().GetMYSQLResult());
        while (SQLManager::getSingleton().FetchRow(&index))
        {
            for (int i = 0; i < ColumnCount; ++i)
            {
                UString value;
                if (SQLManager::getSingleton().GetColumn(i, value, &index))
                {
                    switch(i)
                    {
                    case 0: // time.currentlight
                        WorldLightCurrentLevel(value.toUByte());
                        break;
                    case 1: // time.day
                        ServerTimeDay(value.toShort());
                        break;
                    case 2: // time.hour
                        ServerTimeHours(value.toUShort());
                        break;
                    case 3: // time.minute
                        ServerTimeMinutes(value.toUShort());
                        break;
                    case 4: // time.ampm
                        ServerTimeAMPM(value.toUByte() == 1);
                        break;
                    case 5: // time.moon
                        if (!value.empty())
                            for (int i = 0; i < value.sectionCount(",")+1; ++i)
                                ServerMoon(i, value.section(",", i, i).stripWhiteSpace().toUShort());
                        break;
                    }
                }
            }
        }
        SQLManager::getSingleton().QueryRelease(false);
    }
}

void CServerData::LoadTimeTags(std::ifstream &input)
{
    UString UTag, tag, data;
    while (tag != "o---o")
    {
        ReadWorldTagData(input, tag, data);
        if (tag != "o---o")
        {
            UTag = tag.upper();
            if (UTag == "AMPM")
                ServerTimeAMPM((data.toByte() == 1));
            else if (UTag == "CURRENTLIGHT")
                WorldLightCurrentLevel(data.toUShort());
            else if (UTag == "DAY")
                ServerTimeDay(data.toShort());
            else if (UTag == "HOUR")
                ServerTimeHours(data.toUShort());
            else if (UTag == "MINUTE")
                ServerTimeMinutes(data.toUShort());
            else if (UTag == "MOON")
            {
                if (data.sectionCount(",") != 0)
                {
                    ServerMoon(0, data.section(",", 0, 0).stripWhiteSpace().toShort());
                    ServerMoon(1, data.section(",", 1, 1).stripWhiteSpace().toShort());
                }
            }
        }
    }
    tag = "";
}

SI16 CServerData::ServerTimeDay(void) const
{
    return days;
}
void CServerData::ServerTimeDay(SI16 nValue)
{
    days = nValue;
}

UI08 CServerData::ServerTimeHours(void) const
{
    return hours;
}

void CServerData::ServerTimeHours(UI08 nValue)
{
    hours = nValue;
}

UI08 CServerData::ServerTimeMinutes(void) const
{
    return minutes;
}

void CServerData::ServerTimeMinutes(UI08 nValue)
{
    minutes = nValue;
}

UI08 CServerData::ServerTimeSeconds(void) const
{
    return seconds;
}

void CServerData::ServerTimeSeconds(UI08 nValue)
{
    seconds = nValue;
}

bool CServerData::ServerTimeAMPM(void) const
{
    return ampm;
}

void CServerData::ServerTimeAMPM(bool nValue)
{
    ampm = nValue;
}

bool CServerData::incSecond(void)
{
    bool rvalue = false;
    ++seconds;
    if (seconds == 60)
    {
        seconds = 0;
        rvalue = incMinute();
    }
    return rvalue;
}

void CServerData::incMoon(int mNumber)
{
    moon[mNumber] = (SI16)((moon[mNumber] + 1)%8);
}

bool CServerData::incMinute(void)
{
    bool rvalue = false;
    ++minutes;
    if (minutes%8 == 0)
        incMoon(0);
    if (minutes%3 == 0)
        incMoon(1);

    if (minutes == 60)
    {
        minutes = 0;
        rvalue    = incHour();
    }
    return rvalue;
}

bool CServerData::incHour(void)
{
    ++hours;
    bool retVal = false;
    if (hours == 12)
    {
        if (ampm)
            retVal = incDay();
        hours    = 0;
        ampm    = !ampm;
    }
    return retVal;
}

bool CServerData::incDay(void)
{
    ++days;
    return true;
}

physicalServer *CServerData::ServerEntry(UI16 entryNum)
{
    physicalServer *rvalue = NULL;
    if (entryNum < serverList.size())
        rvalue = &serverList[entryNum];
    return rvalue;
}

UI16 CServerData::ServerCount(void) const
{
    return static_cast<UI16>(serverList.size());
}

void physicalServer::setName(const std::string& newName)
{
  name = newName;
}

void physicalServer::setDomain(const std::string& newDomain)
{
  domain = newDomain;
}

void physicalServer::setIP(const std::string& newIP)
{
  ip = newIP;
}

void physicalServer::setPort(UI16 newPort)
{
  port = newPort;
}

std::string physicalServer::getName(void) const
{
  return name;
}

std::string physicalServer::getDomain(void) const
{
  return domain;
}

std::string physicalServer::getIP(void) const
{
  return ip; 
}

UI16 physicalServer::getPort(void) const
{
  return port;
}

}
