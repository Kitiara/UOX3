#include "uox3.h"
#include "skills.h"
#include "cMagic.h"
#include "CJSMapping.h"
#include "mapstuff.h"
#include "cScript.h"
#include "cEffects.h"
#include "teffect.h"
#include "CPacketSend.h"
#include "classes.h"
#include "regions.h"
#include "combat.h"
#include "CJSMapping.h"
#include "SQLManager.h"

namespace UOX
{

//o---------------------------------------------------------------------------o
//| Function   - void deathAction(CChar *s, CItem *x, UI08 fallDirection)
//| Programmer - Unknown
//o---------------------------------------------------------------------------o
//| Purpose    - Plays a characters death animation
//o---------------------------------------------------------------------------o
void cEffects::deathAction(CChar *s, CItem *x, UI08 fallDirection)
{
    CPDeathAction toSend((*s), (*x));
    toSend.FallDirection(fallDirection);
    SOCKLIST nearbyChars = FindNearbyPlayers(s);
    for (SOCKLIST_CITERATOR cIter = nearbyChars.begin(); cIter != nearbyChars.end(); ++cIter)
        if ((*cIter)->CurrcharObj() != s) // Death action screws up corpse display for the person who died.
            (*cIter)->Send(&toSend);
}

void cEffects::PlayMovingAnimation(CBaseObject *source, CBaseObject *dest, UI16 effect, UI08 speed, UI08 loop, bool explode, UI32 hue, UI32 renderMode)
{    //0x0f 0x42 = arrow 0x1b 0xfe=bolt
    if (!ValidateObject(source) || !ValidateObject(dest)) 
        return;

    CPGraphicalEffect2 toSend(0, (*source), (*dest));
    toSend.Model(effect);
    toSend.SourceLocation((*source));
    toSend.TargetLocation((*dest));
    toSend.Speed(speed);
    toSend.Duration(loop);
    toSend.ExplodeOnImpact(explode);
    toSend.Hue(hue);
    toSend.RenderMode(renderMode);
    Network->PushConn();
    for (CSocket *tSock = Network->FirstSocket(); !Network->FinishedSockets(); tSock = Network->NextSocket())
        if (objInRange(tSock, source, DIST_SAMESCREEN) && objInRange(tSock, dest, DIST_SAMESCREEN))
            tSock->Send(&toSend);

    Network->PopConn();
}

void cEffects::PlayMovingAnimation(CBaseObject *source, SI16 x, SI16 y, SI08 z, UI16 effect, UI08 speed, UI08 loop, bool explode, UI32 hue, UI32 renderMode)
{
    //0x0f 0x42 = arrow 0x1b 0xfe=bolt
    if (!ValidateObject(source)) 
        return;

    CPGraphicalEffect2 toSend(0, (*source));
    toSend.TargetSerial(INVALIDSERIAL);
    toSend.Model(effect);
    toSend.SourceLocation((*source));
    toSend.TargetLocation(x, y, z);
    toSend.Speed(speed);
    toSend.Duration(loop);
    toSend.ExplodeOnImpact(explode);
    toSend.Hue(hue);
    toSend.RenderMode(renderMode);

    SOCKLIST nearbyChars = FindNearbyPlayers(source, DIST_SAMESCREEN);
    for (SOCKLIST_CITERATOR cIter = nearbyChars.begin(); cIter != nearbyChars.end(); ++cIter)
        (*cIter)->Send(&toSend);
}

//o---------------------------------------------------------------------------o
//| Function   - void PlayCharacterAnimation(CChar *mChar, UI16 actionID)
//| Programmer - UOX3 DevTeam
//o---------------------------------------------------------------------------o
//| Purpose    - Character does a certain action
//o---------------------------------------------------------------------------o
void cEffects::PlayCharacterAnimation(CChar *mChar, UI16 actionID, UI08 frameDelay)
{
    CPCharacterAnimation toSend = (*mChar);
    toSend.Action(actionID);
    toSend.FrameDelay(frameDelay);
    SOCKLIST nearbyChars = FindNearbyPlayers(mChar);
    for (SOCKLIST_CITERATOR cIter = nearbyChars.begin(); cIter != nearbyChars.end(); ++cIter)
        (*cIter)->Send(&toSend);
}

//o---------------------------------------------------------------------------o
//| Function   - void PlaySpellCastingAnimation(CChar *mChar, UI16 actionID)
//| Programmer - UOX3 DevTeam
//o---------------------------------------------------------------------------o
//| Purpose    - Handles spellcasting action
//o---------------------------------------------------------------------------o
void cEffects::PlaySpellCastingAnimation(CChar *mChar, UI16 actionID)
{
    if (mChar->IsOnHorse() && (actionID == 0x10 || actionID == 0x11))
    {
        PlayCharacterAnimation(mChar, 0x1B);
        return;
    }
    if ((mChar->IsOnHorse() || !cwmWorldState->creatures[mChar->GetID()].IsHuman()) && actionID == 0x22)
        return;
    PlayCharacterAnimation(mChar, actionID);
}


void cEffects::PlayStaticAnimation(CBaseObject *target, UI16 effect, UI08 speed, UI08 loop, bool explode)
{
    if (!ValidateObject(target)) 
        return;

    CPGraphicalEffect toSend(3, (*target));
    if (target->GetObjType() != OT_CHAR)
        toSend.TargetSerial((*target));
    else
        toSend.TargetSerial(INVALIDSERIAL);
    toSend.Model(effect);
    toSend.SourceLocation((*target));
    if (target->GetObjType() != OT_CHAR)
        toSend.TargetLocation((*target));
    toSend.Speed(speed);
    toSend.Duration(loop);
    toSend.AdjustDir(false);
    toSend.ExplodeOnImpact(explode);

    SOCKLIST nearbyChars = FindNearbyPlayers(target, DIST_SAMESCREEN);
    for (SOCKLIST_CITERATOR cIter = nearbyChars.begin(); cIter != nearbyChars.end(); ++cIter)
        (*cIter)->Send(&toSend);
}

// for effects on items
void cEffects::PlayStaticAnimation(SI16 x, SI16 y, SI08 z, UI16 effect, UI08 speed, UI08 loop, bool explode)
{
    CPGraphicalEffect toSend(2);
    toSend.Model(effect);
    toSend.SourceLocation(x, y, z);
    toSend.TargetLocation(x, y, z);
    toSend.Speed(speed);
    toSend.Duration(loop);
    toSend.AdjustDir(false);
    toSend.ExplodeOnImpact(explode);
    Network->PushConn();
    for (CSocket *tSock = Network->FirstSocket(); !Network->FinishedSockets(); tSock = Network->NextSocket()) // if inrange of effect and online send effect
        tSock->Send(&toSend);
    Network->PopConn();
}

void cEffects::bolteffect(CChar *player)
{
    if (!ValidateObject(player)) 
        return;

    CPGraphicalEffect toSend(1, (*player));
    toSend.SourceLocation((*player));
    toSend.ExplodeOnImpact(false);
    toSend.AdjustDir(false);
    SOCKLIST nearbyChars = FindNearbyPlayers(player);
    for (SOCKLIST_CITERATOR cIter = nearbyChars.begin(); cIter != nearbyChars.end(); ++cIter)
        (*cIter)->Send(&toSend);
}

//o---------------------------------------------------------------------------o
//| Function   - void explodeItem(CSocket *mSock, CItem *nItem)
//| Programmer - Unknown
//o---------------------------------------------------------------------------o
//| Purpose    - Explode an item
//o---------------------------------------------------------------------------o
void explodeItem(CSocket *mSock, CItem *nItem)
{
    CChar *c = mSock->CurrcharObj();
    UI32 dmg = 0;
    UI32 dx, dy, dz;
    // - send the effect (visual and sound)
    if (nItem->GetCont() != NULL)
    {
        Effects->PlayStaticAnimation(c, 0x36B0, 0x00, 0x09);
        nItem->SetCont(NULL);
        nItem->SetLocation(c);
        Effects->PlaySound(c, 0x0207);
    }
    else
    {
        Effects->PlayStaticAnimation(nItem, 0x36B0, 0x00, 0x09, 0x00);
        Effects->PlaySound(nItem, 0x0207);
    }
    UI32 len = nItem->GetTempVar(CITV_MOREX) / 250; //4 square max damage at 100 alchemy
    dmg = RandomNum(nItem->GetTempVar(CITV_MOREZ) * 5, nItem->GetTempVar(CITV_MOREZ) * 10);

    if (dmg < 5)
        dmg = RandomNum(5, 10);  // 5 points minimum damage
    if (len < 2)
        len = 2;  // 2 square min damage range

    REGIONLIST nearbyRegions = MapRegion->PopulateList(nItem);
    for (REGIONLIST_CITERATOR rIter = nearbyRegions.begin(); rIter != nearbyRegions.end(); ++rIter)
    {
        CMapRegion *Cell = (*rIter);
        bool chain = false;
    
        CDataList< CChar * > *regChars = Cell->GetCharList();
        regChars->Push();
        for (CChar *tempChar = regChars->First(); !regChars->Finished(); tempChar = regChars->Next())
        {
            if (!ValidateObject(tempChar))
                continue;

            dx = abs(tempChar->GetX() - nItem->GetX());
            dy = abs(tempChar->GetY() - nItem->GetY());
            dz = abs(tempChar->GetZ() - nItem->GetZ());
            if (dx <= len && dy <= len && dz <= len)
                if (!tempChar->IsGM() && !tempChar->IsInvulnerable() && (tempChar->IsNpc() || isOnline((*tempChar))))
                {
                    //UI08 hitLoc = Combat->CalculateHitLoc();
                    SI16 damage = Combat->ApplyDefenseModifiers(HEAT, c, tempChar, ALCHEMY, 0, ((SI32)dmg + (2 - UOX_MIN(dx, dy))), true);
                    tempChar->Damage(damage, c, true);
                }
        }
        regChars->Pop();
        CDataList< CItem * > *regItems = Cell->GetItemList();
        regItems->Push();
        for (CItem *tempItem = regItems->First(); !regItems->Finished(); tempItem = regItems->Next())
            if (tempItem->GetID() == 0x0F0D && tempItem->GetType() == IT_POTION)
            {
                dx = abs(nItem->GetX() - tempItem->GetX());
                dy = abs(nItem->GetY() - tempItem->GetY());
                dz = abs(nItem->GetZ() - tempItem->GetZ());

                if (dx <= 2 && dy <= 2 && dz <= 2 && !chain) // only trigger if in a 2*2*2 cube
                    if (!(dx == 0 && dy == 0 && dz == 0))
                    {
                        if (RandomNum(0, 1) == 1)
                            chain = true;
                        Effects->tempeffect(c, tempItem, 22, 0, 1, 0);
                    }
            }
        regItems->Pop();
    }
    nItem->Delete();
}

void cEffects::HandleMakeItemEffect(CTEffect *tMake)
{
    if (tMake == NULL)
        return;

    CChar *src = calcCharObjFromSer(tMake->Source());
    UI16 iMaking = tMake->More(1);
    createEntry *toMake = Skills->FindItem(iMaking);
    if (toMake == NULL)
        return;

    CSocket *sock = src->GetSocket();
    UString addItem = toMake->addItem;
    UI16 amount = 1;
    if (addItem.sectionCount(",") != 0)
    {
        amount = addItem.section(",", 1, 1).toUShort();
        addItem = addItem.section(",", 0, 0);
    }

    CItem *targItem = Items->CreateScriptItem(sock, src, addItem, amount, OT_ITEM, true);
    for (size_t skCounter = 0; skCounter < toMake->skillReqs.size(); ++skCounter)
        src->SkillUsed(false, toMake->skillReqs[skCounter].skillNumber);

    if (targItem == NULL)
    {
        Console.Error("cSkills::MakeItem() bad script item # %s, made by player 0x%X", addItem.c_str(), src->GetSerial());
        return;
    }
    else
    {
        targItem->SetName2(targItem->GetName().c_str());
        SI32 rank = Skills->CalcRankAvg(src, (*toMake));
        SI32 maxrank = toMake->maxRank;
        Skills->ApplyRank(sock, targItem, static_cast<UI08>(rank), static_cast<UI08>(maxrank));
        
        // if we're not a GM, see if we should store our creator
        if (!src->IsGM() && !toMake->skillReqs.empty())
        {
            targItem->SetCreator(src->GetSerial());
            int avgSkill, sumSkill = 0;
            // Find the average of our player's skills
            for (size_t resCounter = 0; resCounter < toMake->skillReqs.size(); ++resCounter)
                sumSkill += src->GetSkill(toMake->skillReqs[resCounter].skillNumber);
            avgSkill = sumSkill / toMake->skillReqs.size();
            if (avgSkill > 950) 
                targItem->SetMadeWith(toMake->skillReqs[0].skillNumber + 1);
            else 
                targItem->SetMadeWith(-(toMake->skillReqs[0].skillNumber + 1));
        }
        else
        {
            targItem->SetCreator(INVALIDSERIAL);
            targItem->SetMadeWith(0);
        }
        targItem->EntryMadeFrom(iMaking);
    }

    // Make sure it's movable
    targItem->SetMovable(1);
    if (toMake->soundPlayed)
        PlaySound(sock, toMake->soundPlayed, true);

    sock->sysmessage(985);
}

void cEffects::checktempeffects(void)
{
    CItem *i = NULL;
    CChar *s = NULL, *src = NULL;
    CSocket *tSock = NULL;
    CBaseObject *myObj = NULL;

    const UI32 j = cwmWorldState->GetUICurrentTime();
    cwmWorldState->tempEffects.Push();
    for (CTEffect *Effect = cwmWorldState->tempEffects.First(); !cwmWorldState->tempEffects.Finished(); Effect = cwmWorldState->tempEffects.Next())
    {
        if (Effect == NULL)
        {
            cwmWorldState->tempEffects.Remove(Effect);
            continue;
        }
        else if (Effect->Destination() == INVALIDSERIAL)
        {
            cwmWorldState->tempEffects.Remove(Effect, true);
            continue;
        }

        if (Effect->ExpireTime() > j)
            continue;

        if (Effect->Destination() < BASEITEMSERIAL)
        {
            s = calcCharObjFromSer(Effect->Destination());
            if (!ValidateObject(s))
            {
                cwmWorldState->tempEffects.Remove(Effect, true);
                continue;
            }
            tSock = s->GetSocket();
        }
        bool equipCheckNeeded = false;

        switch(Effect->Number())
        {
            case 1:
                if (s->IsFrozen())
                {
                    s->SetFrozen(false);
                    if (tSock != NULL)
                        tSock->sysmessage(700);
                }
                break;
            case 2:
                s->SetFixedLight(255);
                doLight(tSock, cwmWorldState->ServerData()->WorldLightCurrentLevel());
                break;
            case 3:
                s->IncDexterity2(Effect->More(0));
                equipCheckNeeded = true;
                break;
            case 4:
                s->IncIntelligence2(Effect->More(0));
                equipCheckNeeded = true;
                break;
            case 5:
                s->IncStrength2(Effect->More(0));
                equipCheckNeeded = true;
                break;
            case 6:
                s->IncDexterity2(-Effect->More(0));
                s->SetStamina(UOX_MIN(s->GetStamina(), s->GetMaxStam()));
                equipCheckNeeded = true;
                break;
            case 7:
                s->IncIntelligence2(-Effect->More(0));
                s->SetMana(UOX_MIN(s->GetMana(), s->GetMaxMana()));
                equipCheckNeeded = true;
                break;
            case 8:
                s->IncStrength2(-Effect->More(0));
                s->SetHP(UOX_MIN(s->GetHP(), static_cast<SI16>(s->GetMaxHP())));
                equipCheckNeeded = true;
                break;
            case 9: // Grind potion (also used for necro stuff)
                switch(Effect->More(0))
                {
                    case 0:
                        if (Effect->More(1) != 0)
                            s->TextMessage(NULL, 1270, EMOTE, true, s->GetName().c_str());
                        PlaySound(s, 0x0242);
                        break;
                }
                break;
            case 11:
                s->IncStrength2(-Effect->More(0));
                s->SetHP( UOX_MIN(s->GetHP(), static_cast<SI16>(s->GetMaxHP())));
                s->IncDexterity2(-Effect->More(1));
                s->SetStamina(UOX_MIN(s->GetStamina(), s->GetMaxStam()));
                s->IncIntelligence2(-Effect->More(2));
                s->SetMana(UOX_MIN(s->GetMana(), s->GetMaxMana()));
                equipCheckNeeded = true;
                break;
            case 12: // Curse
                if (s != NULL)
                {
                    s->IncStrength2(Effect->More(0));
                    s->IncDexterity2(Effect->More(1));
                    s->IncIntelligence2(Effect->More(2));
                    equipCheckNeeded = true;
                }
                else
                    equipCheckNeeded = false;
                break;
            case 15: // Reactive Armor
                s->SetReactiveArmour(false);
                break;
            case 16: // Explosion potion messages
                src = calcCharObjFromSer(Effect->Source());
                if (src->GetTimer(tCHAR_ANTISPAM) < cwmWorldState->GetUICurrentTime())
                {
                    src->SetTimer(tCHAR_ANTISPAM, BuildTimeValue(1));
                    UString mTemp = UString::number(Effect->More(2));
                    if (tSock != NULL)
                        tSock->sysmessage(mTemp.c_str());
                } 
                break;
            case 17: // Explosion potion
                src = calcCharObjFromSer(Effect->Source());
                explodeItem(src->GetSocket(), static_cast<CItem *>(Effect->ObjPtr())); //explode this item
                break;
            case 18: // Polymorph spell
                s->SetID(s->GetOrgID());
                s->IsPolymorphed(false);
                break;
            case 19: //Incognito spell
                s->SetID(s->GetOrgID());

                // ------ NAME -----
                s->SetName(s->GetOrgName());
                
                i = s->GetItemAtLayer(IL_HAIR);
                if (ValidateObject(i)) 
                {
                    i->SetColour(s->GetHairColour());
                    i->SetID(s->GetHairStyle());
                }

                i = s->GetItemAtLayer(IL_FACIALHAIR);
                if (ValidateObject(i) && s->GetID(2) == 0x90)
                {
                    i->SetColour(s->GetBeardColour());
                    i->SetID(s->GetBeardStyle());
                }
                if (tSock != NULL)
                    s->SendWornItems(tSock);
                s->IsIncognito(false);
                break;
            case 21:
                UI16 toDrop;
                toDrop = Effect->More(0);
                if ((s->GetBaseSkill(PARRYING) - toDrop) < 0)
                    s->SetBaseSkill(0, PARRYING);
                else
                    s->SetBaseSkill(s->GetBaseSkill(PARRYING) - toDrop, PARRYING);
                equipCheckNeeded = true;
                break;
            case 25:
                // Same result no matter if the if-check is true or false? What is this effect even for?
                if (Effect->More(1) == 0)
                    Effect->ObjPtr()->SetDisabled(false);
                else
                    Effect->ObjPtr()->SetDisabled(false);
                break;
            case 26:
                s->SetUsingPotion(false);
                break;
            case 27:
                src = calcCharObjFromSer(Effect->Source());
                if (!ValidateObject(src) || !ValidateObject(s))
                    break;
                Magic->playSound(src, 43);
                Magic->doMoveEffect(43, s, src);
                Magic->doStaticEffect(s, 43);
                Magic->MagicDamage(s, Effect->More(0), src, HEAT);
                equipCheckNeeded = true;
                break;
            case 40:
            {
                UI16 scpNum = 0xFFFF;
                cScript *tScript = JSMapping->GetScript(Effect->AssocScript());

                if (Effect->Source() >= BASEITEMSERIAL) // item's have serials of 0x40000000 and above, and we already know it's not INVALIDSERIAL
                {
                    myObj = calcItemObjFromSer(Effect->Source());
                    equipCheckNeeded = false;
                }
                else
                {
                    myObj = calcCharObjFromSer(Effect->Source());
                    equipCheckNeeded = true;
                }

                if (!ValidateObject(myObj)) // item no longer exists!
                    break;

                if (tScript == NULL) // No associated script, so it must be another callback variety
                {
                    if (Effect->More(1) != 0xFFFF)
                        scpNum = Effect->More(1);
                    else
                        scpNum = myObj->GetScriptTrigger();
                    tScript = JSMapping->GetScript(scpNum);
                }

                //Make sure to check for a specific script when the previous checks ended in the global script.
                if ((tScript == NULL || scpNum == 0) && Effect->Source() >= BASEITEMSERIAL)
                    if (JSMapping->GetEnvokeByType()->Check(static_cast<UI16>((static_cast<CItem *>(myObj))->GetType())))
                    {
                        scpNum = JSMapping->GetEnvokeByType()->GetScript(static_cast<UI16>((static_cast<CItem *>(myObj))->GetType()));
                        tScript = JSMapping->GetScript(scpNum);
                    }
                    else if (JSMapping->GetEnvokeByID()->Check(myObj->GetID()))
                    {
                        scpNum = JSMapping->GetEnvokeByID()->GetScript(myObj->GetID());
                        tScript = JSMapping->GetScript(scpNum);
                    }

                if (tScript != NULL) // do OnTimer stuff here
                    tScript->OnTimer(myObj, static_cast<UI08>(Effect->More(0)));
                break;
            }
            case 41: // Creating an item
                HandleMakeItemEffect(Effect);
                break;
            case 42:
                src = calcCharObjFromSer(Effect->Source());
                PlaySound(src, Effect->More(1));
                break;
            case 43:
                src = calcCharObjFromSer(Effect->Source());
                if (!ValidateObject(src)) // char doesn't exist!
                    break;
                else if (src->GetID() == 0xCF)
                    break;

                src->SetID(0xCF); // Thats all we need to do
                break;
            default:
                Console.Error(" Fallout of switch statement without default (%i). checktempeffects()", Effect->Number());            
                break;
        }
        if (ValidateObject(s) && equipCheckNeeded)
            Items->CheckEquipment(s); // checks equipments for stat requirements
        cwmWorldState->tempEffects.Remove(Effect, true);
    }
    cwmWorldState->tempEffects.Pop();
}

//o---------------------------------------------------------------------------o
//| Function   - void reverseEffect(UI16 i)
//| Programmer - Unknown
//o---------------------------------------------------------------------------o
//| Purpose    - Reverse a temp effect
//o---------------------------------------------------------------------------o
void reverseEffect(CTEffect *Effect)
{
    CChar *s = calcCharObjFromSer(Effect->Destination());
    if (ValidateObject(s))
    {
        switch(Effect->Number())
        {
            case 1:
                s->SetFrozen(false);
                break;
            case 2:
                s->SetFixedLight(255);
                break;
            case 3: s->IncDexterity2(Effect->More(0));     break;
            case 4: s->IncIntelligence2(Effect->More(0));  break;
            case 5: s->IncStrength2(Effect->More(0));      break;
            case 6: s->IncDexterity2(-Effect->More(0));    break;
            case 7: s->IncIntelligence2(-Effect->More(0)); break;
            case 8: s->IncStrength2(-Effect->More(0));     break;
            case 11:
                s->IncStrength2(-Effect->More(0));
                s->IncDexterity2(-Effect->More(1));
                s->IncIntelligence2(-Effect->More(2));
                break;
            case 12:
                s->IncStrength2(Effect->More(0));
                s->IncDexterity2(Effect->More(1));
                s->IncIntelligence2(Effect->More(2));
                break;
            case 18: // Polymorph spell
                s->SetID(s->GetOrgID());
                s->IsPolymorphed(false);
                break;
            case 19: // Incognito
                CItem *j;
                // ------ SEX ------
                s->SetID(s->GetOrgID());
                s->SetName(s->GetOrgName());
                j = s->GetItemAtLayer(IL_HAIR);
                if (ValidateObject(j))
                {
                    j->SetColour(s->GetHairColour());
                    j->SetID(s->GetHairStyle());
                }
                j = s->GetItemAtLayer(IL_FACIALHAIR);
                if (ValidateObject(j) && s->GetID(2) == 0x90)
                {
                    j->SetColour(s->GetBeardColour());
                    j->SetID(s->GetBeardStyle());
                }
                // only refresh once
                CSocket *tSock;
                tSock = s->GetSocket();
                s->SendWornItems(tSock);
                s->IsIncognito(false);
                break;
            case 21:
                int toDrop;
                toDrop = Effect->More(0);
                if ((s->GetBaseSkill(PARRYING) - toDrop) < 0)
                    s->SetBaseSkill(0, PARRYING);
                else
                    s->SetBaseSkill(s->GetBaseSkill(PARRYING) - toDrop, PARRYING);
                break;
            case 26:
                s->SetUsingPotion(false);
                break;
            default:
                Console.Error(" Fallout of switch statement without default. uox3.cpp, reverseEffect()");
                break;
        }
    }
    Items->CheckEquipment(s);
}

void cEffects::tempeffect(CChar *source, CChar *dest, UI08 num, UI16 more1, UI16 more2, UI16 more3)
{
    if (!ValidateObject(source) || !ValidateObject(dest))
        return;

    bool spellResisted = false;
    CTEffect *toAdd    = new CTEffect;
    SERIAL sourSer     = source->GetSerial();
    SERIAL targSer     = dest->GetSerial();
    UI16 more[3] = { more1, more2, more3 };
    bool allowed[3] = { true, true, true };
    toAdd->Source(sourSer);
    toAdd->Destination(targSer);

    cwmWorldState->tempEffects.Push();
    for (CTEffect *Effect = cwmWorldState->tempEffects.First(); !cwmWorldState->tempEffects.Finished(); Effect = cwmWorldState->tempEffects.Next())
        if (Effect->Destination() == targSer)
            if (Effect->Number() == num)
            {
                switch(Effect->Number())
                {
                    case 3:
                    case 4:
                    case 5:
                    case 6:
                    case 7:
                    case 8:
                    case 10:
                    case 11:
                    case 14:
                    case 15:
                    case 16:
                        reverseEffect(Effect);
                        cwmWorldState->tempEffects.Remove(Effect, true);
                        break;
                    default:
                        break;
                }
            }
    cwmWorldState->tempEffects.Pop();

    CSocket *tSock = dest->GetSocket();
    toAdd->Number(num);
    switch(num)
    {
        case 1:
            dest->SetFrozen(true);
            toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 100.0f));
            toAdd->Dispellable(true);
            break;
        case 2:
            SI16 worldbrightlevel;
            worldbrightlevel = cwmWorldState->ServerData()->WorldLightBrightLevel();
            dest->SetFixedLight(static_cast<UI08>(worldbrightlevel));
            doLight(tSock, static_cast<char>(worldbrightlevel));
            toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 2.0f));
            toAdd->Dispellable(true);
            break;
        case 3:
        {
            if (dest->GetDexterity() < more[0])
                more[0] = dest->GetDexterity();
            dest->IncDexterity2(-more[0]);
            dest->SetStamina(UOX_MIN(dest->GetStamina(), dest->GetMaxStam()));
            R32 duration = (R32)((source->GetSkill(EVALUATINGINTEL) / 5) + 1) * 6;
            if (Magic->CheckResist(source, dest, 1)) // Halve effect-timer on resist
                duration /= 2;
            toAdd->ExpireTime(BuildTimeValue(duration));
            toAdd->Dispellable(true);
            break;
        }
        case 4:
            if (dest->GetIntelligence() < more[0])
                more[0] = dest->GetIntelligence();
            dest->IncIntelligence2(-more[0]);
            dest->SetMana(UOX_MIN(dest->GetMana(), dest->GetMaxMana()));
            if (Magic->CheckResist(source, dest, 1)) // Halve effect-timer on resist
                toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 20.0f));
            else
                toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 10.0f));
            toAdd->Dispellable(true);
            break;
        case 5:
            if (dest->GetStrength() < more[0])
                more[0] = dest->GetStrength();
            dest->IncStrength2(-more[0]);
            dest->SetHP(UOX_MIN(dest->GetHP(), static_cast<SI16>(dest->GetMaxHP())));
            if (Magic->CheckResist(source, dest, 4)) // Halve effect-timer on resist
                toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 20.0f));
            else
                toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 10.0f));
            toAdd->Dispellable(true);
            break;
        case 6:
            dest->IncDexterity(more[0]);
            toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 10.0f));
            toAdd->Dispellable(true);
            break;
        case 7:
            dest->IncIntelligence2(more[0]);
            toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 10.0f));
            toAdd->Dispellable(true);
            break;
        case 8:
            dest->IncStrength2(more[0]);
            toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 10.0f));
            toAdd->Dispellable(true);
            break;
        case 9:
            toAdd->ExpireTime(BuildTimeValue((R32)more2));
            toAdd->Dispellable(false);
            break;
        case 10: // Bless
            dest->IncStrength2(more[0]);
            dest->IncDexterity2(more[1]);
            dest->IncIntelligence2(more[2]);
            toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 10.0f));
            toAdd->Dispellable(true);
            break;
        case 11: // Curse
            if (dest->GetStrength() < more[0])
                more[0] = dest->GetStrength();
            if (dest->GetDexterity() < more[1])
                more[1] = dest->GetDexterity();
            if (dest->GetIntelligence() < more[2])
                more[2] = dest->GetIntelligence();
            dest->IncStrength2(-more[0]);
            dest->IncDexterity2(-more[1]);
            dest->IncIntelligence2(-more[2]);
            if (Magic->CheckResist(source, dest, 4)) // Halve effect-timer on resist
                toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 20.0f));
            else
                toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 10.0f));
            toAdd->Dispellable(true);
            break;
        case 12: // Reactive Armor
            toAdd->ExpireTime(BuildTimeValue((R32)source->GetSkill(MAGERY) / 10.0f));
            toAdd->Dispellable(true);
            break;
        case 13: //Explosion potions
            toAdd->ExpireTime(BuildTimeValue((R32)more[1]));
            toAdd->Dispellable(false);
            break;
        case 14: // Polymorph
        {
            toAdd->ExpireTime(cwmWorldState->ServerData()->BuildSystemTimeValue(tSERVER_POLYMORPH));
            toAdd->Dispellable(false);

            // Grey flag when polymorphed
            dest->SetTimer(tCHAR_CRIMFLAG, cwmWorldState->ServerData()->BuildSystemTimeValue(tSERVER_POLYMORPH));
            if (dest->IsOnHorse()) 
                DismountCreature(dest);
            UI16 k = (more[0] << 8) + more[1];
            
            if (k <= 0x03e2) // lord binary, body-values >0x3e1 crash the client
                dest->SetID(k);
            allowed[0] = false;
            allowed[1] = false;
        }
            break;
        case 15: // incognito spell - AntiChrist (10/99)
            toAdd->ExpireTime(BuildTimeValue(90.0f)); // 90 seconds
            toAdd->Dispellable(false);
            break;
        case 16: // protection
            toAdd->ExpireTime(BuildTimeValue(120.0f));
            toAdd->Dispellable(true);
            dest->SetBaseSkill(dest->GetBaseSkill(PARRYING) + more[0], PARRYING);
            break;
        case 17:
            toAdd->ExpireTime(cwmWorldState->ServerData()->BuildSystemTimeValue(tSERVER_POTION));
            dest->SetUsingPotion(true);
            break;
        case 18:
            toAdd->ExpireTime(BuildTimeValue(static_cast<R32>(cwmWorldState->ServerData()->CombatExplodeDelay())));
            break;
        case 19: // creating item
            toAdd->ExpireTime(BuildTimeValue((R32)(more[0]) / 100.0f));
            allowed[0] = false;
            break;
        case 20: // delayed sound effect
            toAdd->ExpireTime(BuildTimeValue((R32)(more[0]) / 100.0f));
            allowed[0] = false;
            break;
        case 21: // regain wool for sheeps
            toAdd->ExpireTime(BuildTimeValue((R32)(more[0])));
            allowed[0] = false;
            break;
        default:
            Console.Error(" Fallout of switch statement (%d) without default. uox3.cpp, tempeffect()", num);
            return;
    }

    for (int i = 0; i < 3; ++i)
        if (more[i] != NULL && allowed[i])
            toAdd->More(more[i], i);

    cwmWorldState->tempEffects.Add(toAdd);
}

void cEffects::tempeffect(CChar *source, CItem *dest, UI08 num, UI16 more1, UI16 more2, UI16 more3)
{
    if (!ValidateObject(dest))
        return;

    CTEffect *toAdd = new CTEffect;
    UI16 more[3] = { more1, more2, more3 };
    bool allowed[3] = { true, true, true };

    if (ValidateObject(source))
        toAdd->Source(source->GetSerial());
    else
        toAdd->Source(INVALIDSERIAL);

    toAdd->Destination(dest->GetSerial());
    toAdd->Number(num);
    switch(num)
    {
        case 22: //Explosion potion (explosion)  Tauriel (explode in 4 seconds)
            toAdd->ExpireTime(BuildTimeValue(4));
            toAdd->ObjPtr(dest);
            toAdd->Dispellable(false);
            break;
        case 23:
            toAdd->ExpireTime(BuildTimeValue(more[0]));
            toAdd->ObjPtr(dest);
            dest->SetDisabled(true);
            allowed[0] = false;
            break;
        default:
            Console.Error(" Fallout of switch statement without default. uox3.cpp, tempeffect2()");
            return;
    }

    for (int i = 0; i < 3; ++i)
        if (more[i] != NULL && allowed[i])
            toAdd->More(more[i], i);

    cwmWorldState->tempEffects.Add(toAdd);
}

//o--------------------------------------------------------------------------
//| Function   - SaveEffects()
//| Date       - 16 March, 2002
//| Programmer - sereg
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Saves out the TempEffects
//o--------------------------------------------------------------------------
void cEffects::SaveEffects(void)
{
    int StartTime = getclock();

    Console << "Saving Effects...   ";
    Console.TurnYellow();

    std::string str = "DELETE FROM effects";
    bool Started = false;
    cwmWorldState->tempEffects.Push();
    for (CTEffect *currEffect = cwmWorldState->tempEffects.First(); !cwmWorldState->tempEffects.Finished(); currEffect = cwmWorldState->tempEffects.Next())
        if (currEffect != NULL)
        {
            if (Started)
                str += ",";
            else
            {
                str += "\nINSERT INTO effects VALUES ";
                Started = true;
            }

            str += currEffect->Save();
        }
    cwmWorldState->tempEffects.Pop();

    std::istringstream iss;
    iss.str(str);
    str.clear();
    while (std::getline(iss, str))
        SQLManager::getSingleton().ExecuteQuery(str);

    Console.PrintDone();
    Console.Print("Effects saved in %.02fsec\n", ((float)(getclock()-StartTime))/1000.0f);
}

//o--------------------------------------------------------------------------
//| Function   - LoadEffects(void)
//| Date       - 16 March, 2002
//| Programmer - sererg
//| Modified   -
//o--------------------------------------------------------------------------
//| Purpose    - Loads in Effects from disk
//o--------------------------------------------------------------------------
void ReadWorldTagData(std::ifstream &inStream, UString &tag, UString &data);
void cEffects::LoadEffects(void)
{
    int index;
    if (SQLManager::getSingleton().ExecuteQuery("SELECT * FROM effects", &index, false))
    {
        int ColumnCount = mysql_num_fields(SQLManager::getSingleton().GetMYSQLResult());
        while (SQLManager::getSingleton().FetchRow(&index))
        {
            CTEffect *toLoad = new CTEffect;
            for (int i = 0; i < ColumnCount; ++i)
            {
                UString value;
                SQLManager::getSingleton().GetColumn(i, value, &index);
                switch(i)
                {
                case 0: // effects.source
                    toLoad->Source(value.toULong());
                    break;
                case 1: // effects.dest
                    toLoad->Destination(value.toULong());
                    break;
                case 2: // effects.objptr
                    {
                        SERIAL objSer = value.empty() ? INVALIDSERIAL : value.toULong();
                        if (objSer != INVALIDSERIAL)
                        {
                            if (objSer < BASEITEMSERIAL)
                                toLoad->ObjPtr(calcCharObjFromSer(objSer));
                            else
                                toLoad->ObjPtr(calcItemObjFromSer(objSer));
                        }
                        else
                            toLoad->ObjPtr(NULL);
                    }
                    break;
                case 3: // effects.expire
                    toLoad->ExpireTime(static_cast<UI32>(value.toULong() + cwmWorldState->GetUICurrentTime()));
                    break;
                case 4: // effects.number
                    toLoad->Number(value.toUByte());
                    break;
                case 5: // effects.more[0]
                    toLoad->More(value.toUShort(), 0);
                    break;
                case 6: // effects.more[1]
                    toLoad->More(value.toUShort(), 1);
                    break;
                case 7: // effects.more[2]
                    toLoad->More(value.toUShort(), 2);
                    break;
                case 8: // effects.dispel
                    toLoad->Dispellable(value.toUByte() == 1 ? true : false);
                    break;
                case 9: // effects.assocscript
                    toLoad->AssocScript(value.empty() ? 65535 : value.toUShort());
                    break;
                }
            }
            cwmWorldState->tempEffects.Add(toLoad);
        }
        SQLManager::getSingleton().QueryRelease(false);
    }
}

//o-----------------------------------------------------------------------o
//| Function   - Save(ofstream &effectDestination)
//| Date       - March 16, 2002
//| Programmer - sereg
//o-----------------------------------------------------------------------o
//| Returns    - true/false indicating the success of the write operation
//o-----------------------------------------------------------------------o
UString CTEffect::Save(void) const
{
    std::stringstream Str;
    CBaseObject *getPtr = NULL;
    Str << "('" << Source() << "', ";
    Str << "'" << Destination() << "', ";

    if (!ValidateObject(getPtr))
        Str << "NULL, ";
    else
        Str << "'" << getPtr->GetSerial() << "', ";

    Str << "'" << (ExpireTime() - cwmWorldState->GetUICurrentTime()) << "', ";
    Str << "'" << int((UI08)static_cast<UI16>(Number())) << "', ";
    Str << "'" << More(0) << "', ";
    Str << "'" << More(1) << "', ";
    Str << "'" << More(2) << "', ";
    Str << "'" << Dispellable() << "', ";

    if (AssocScript() == 65535)
        Str << "NULL";
    else
        Str << "'" << AssocScript() << "'";
    Str << ")";
    return Str.str();
}

}
