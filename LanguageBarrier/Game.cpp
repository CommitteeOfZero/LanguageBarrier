#include "LanguageBarrier.h"
#include "MinHook.h"
#include "Game.h"
#include "SigScan.h"

typedef int(__cdecl *EarlyInitProc)(int unk0, int unk1);
static EarlyInitProc gameExeEarlyInit = NULL;
static EarlyInitProc gameExeEarlyInitReal = NULL;

static uintptr_t gameExeTextureLoadInit1 = NULL;
static uintptr_t gameExeTextureLoadInit2 = NULL;
static uintptr_t gameExeGslPngload = NULL;
static uintptr_t gameExeMpkMount = NULL;
static uintptr_t gameExeMpkFopenById = NULL;
static uintptr_t gameExePpLotsOfState = NULL;

namespace lb {
int __cdecl earlyInitHook(int unk0, int unk1);

void gameInit() {
  // TODO: maybe just scan for all signatures inside SigScan?
  // auto-initialisation, similarly to Config.h
  gameExeTextureLoadInit1 = sigScan("game", "textureLoadInit1");
  gameExeTextureLoadInit2 = sigScan("game", "textureLoadInit2");
  gameExeGslPngload = sigScan("game", "gslPngload");
  gameExeMpkMount = sigScan("game", "mpkMount");
  gameExeMpkFopenById = sigScan("game", "mpkFopenById");
  gameExeEarlyInit = (EarlyInitProc)sigScan("game", "earlyInit");

  if (!scanCreateEnableHook("game", "earlyInit", (uintptr_t *)&gameExeEarlyInit,
                            (LPVOID)&earlyInitHook,
                            (LPVOID *)&gameExeEarlyInitReal))
    return;

  gameExePpLotsOfState = *((uint32_t *)sigScan("game", "useOfPpLotsOfState"));
}

int __cdecl earlyInitHook(int unk0, int unk1)
{
    int retval = gameExeEarlyInitReal(unk0, unk1);
   
    return retval;
}

// TODO: I probably shouldn't be writing these in assembly given it looks like
// they're all cdecl or thiscall anyway
void gameLoadTexture(uint8_t textureId, void *buffer, size_t sz) {
  __asm {
    push 0
    call gameExeTextureLoadInit1
    add esp, 4
    push ebx
    push 0
    lea ebx, [sz]
    push ebx
    lea ebx, [buffer]
    push ebx
    call gameExeTextureLoadInit2
    push [sz]
    push [buffer]
    movzx ebx, [textureId]
    push ebx
    call gameExeGslPngload
    add esp, 0x18
    pop ebx
  }
}
// returns an archive object
void *gameMountMpk(char *mountpoint, char *directory, char *filename) {
  // usually this is pre-filled with some values
  // I should probably know why that is, but I don't
  // default constructor?
  void *retval = malloc(0x3a8);
  __asm {
    push ecx
    mov ecx, retval
    push 0
    push filename
    push directory
    push mountpoint
    call gameExeMpkMount
    pop ecx
  }
  return retval;
}
}