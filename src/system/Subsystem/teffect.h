#ifndef __TEFFECT_H__
#define __TEFFECT_H__

namespace UOX
{

class CTEffect
{
private:
    SERIAL source;
    SERIAL dest;
    UI32 expiretime;
    UI08 num;
    UI16 more[3];
    bool dispellable;
    CBaseObject* objptr;
    UI16 assocScript;

public:
    CTEffect() : source(INVALIDSERIAL), dest(INVALIDSERIAL), expiretime(0), num(0), dispellable(false), objptr(NULL), assocScript(0xFFFF)
    {
        for (int i = 0; i < 3; ++i)
            more[i] = 0;
    }

    UI16 AssocScript(void) const { return assocScript; }
    CBaseObject *ObjPtr(void) const { return objptr; }
    bool Dispellable(void) const { return dispellable; }
    UI32 ExpireTime(void) const { return expiretime; }
    SERIAL Source(void) const { return source; }
    SERIAL Destination(void) const { return dest; }
    UI08 Number(void) const { return num; }
    UI16 More(UI16 num) const { return more[num]; }

    void Source(SERIAL value) { source = value; }
    void Destination(SERIAL value) { dest = value; }
    void ExpireTime(UI32 value) { expiretime = value; }
    void Number(UI08 value) { num = value; }
    void More(UI16 value, UI16 num) { more[num] = value; }
    void Dispellable(bool value) { dispellable = value; }
    void ObjPtr(CBaseObject *value) { objptr = value; }
    void AssocScript(UI16 value) { assocScript = value; }

    UString Save(void) const;
};

}

#endif

