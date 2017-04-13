//-----------------------------------------------------------------------------
// Torque
// Copyright GarageGames, LLC 2011
//-----------------------------------------------------------------------------

#include "math/mMath.h"

#if defined(TORQUE_SUPPORTS_VC_INLINE_X86_ASM)
static S32 m_mulDivS32_ASM(S32 a, S32 b, S32 c)
{  // a * b / c
   S32 r;
   _asm
   {
      mov   eax, a
      imul  b
      idiv  c
      mov   r, eax
   }
   return r;
}


static U32 m_mulDivU32_ASM(S32 a, S32 b, U32 c)
{  // a * b / c
   S32 r;
   _asm
   {
      mov   eax, a
      mov   edx, 0
      mul   b
      div   c
      mov   r, eax
   }
   return r;
}

static void m_sincos_ASM( F32 angle, F32 *s, F32 *c )
{
   _asm
   {
      fld     angle
      fsincos
      mov     eax, c
      fstp    dword ptr [eax]
      mov     eax, s
      fstp    dword ptr [eax]
   }
}

U32 Platform::getMathControlState()
{
   U16 cw;
   _asm
   {
      fstcw cw
   }
   return cw;
}

void Platform::setMathControlState(U32 state)
{
   U16 cw = state;
   _asm
   {
      fldcw cw
   }
}

void Platform::setMathControlStateKnown()
{
   U16 cw = 0x27F;
   _asm
   {
      fldcw cw
   }
}


#endif

//------------------------------------------------------------------------------
void mInstallLibrary_ASM()
{
#if defined(TORQUE_SUPPORTS_VC_INLINE_X86_ASM)
   m_mulDivS32              = m_mulDivS32_ASM;
   m_mulDivU32              = m_mulDivU32_ASM;
   
   m_sincos = m_sincos_ASM;
#endif
}


