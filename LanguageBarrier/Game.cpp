#include "LanguageBarrier.h"
#include "MinHook.h"
#include "Game.h"
#include "SigScan.h"
#include "BinkMod.h"

typedef int(__cdecl *EarlyInitProc)(int unk0, int unk1);
static EarlyInitProc gameExeEarlyInit = NULL;
static EarlyInitProc gameExeEarlyInitReal = NULL;

typedef int(__thiscall *MpkFopenByIdProc)(void *pThis, void *mpkObject,
                                          int fileId, int unk3);
static MpkFopenByIdProc gameExeMpkFopenById = NULL;
static MpkFopenByIdProc gameExeMpkFopenByIdReal = NULL;

static uintptr_t gameExeTextureLoadInit1 = NULL;
static uintptr_t gameExeTextureLoadInit2 = NULL;
static uintptr_t gameExeGslPngload = NULL;
static uintptr_t gameExeMpkMount = NULL;
static uintptr_t gameExePpLotsOfState = NULL;
static uintptr_t gameExePCurrentBgm = NULL;
// scroll height is +6A78

namespace lb {
int __cdecl earlyInitHook(int unk0, int unk1);
int __fastcall mpkFopenByIdHook(void *pThis, void *EDX, void *mpkObject,
                                int fileId, int unk3);

void gameInit() {
  // TODO: maybe just scan for all signatures inside SigScan?
  // auto-initialisation, similarly to Config.h
  gameExeTextureLoadInit1 = sigScan("game", "textureLoadInit1");
  gameExeTextureLoadInit2 = sigScan("game", "textureLoadInit2");
  gameExeGslPngload = sigScan("game", "gslPngload");
  gameExeMpkMount = sigScan("game", "mpkMount");
  gameExeEarlyInit = (EarlyInitProc)sigScan("game", "earlyInit");
  gameExePCurrentBgm = *((uint32_t *)sigScan("game", "useOfPCurrentBgm"));

  // TODO: fault tolerance - we don't need to call it quits entirely just
  // because one *feature* can't work
  if (!scanCreateEnableHook("game", "earlyInit", (uintptr_t *)&gameExeEarlyInit,
                            (LPVOID)&earlyInitHook,
                            (LPVOID *)&gameExeEarlyInitReal))
    return;

  gameExePpLotsOfState = *((uint32_t *)sigScan("game", "useOfPpLotsOfState"));

  binkModInit();
}

// earlyInit is called after all the subsystems have been initialised but before
// the game 'starts' proper
// so, probably a good place to do all of our initialisation that requires
// interacting with them
int __cdecl earlyInitHook(int unk0, int unk1) {
  int retval = gameExeEarlyInitReal(unk0, unk1);

  if (!scanCreateEnableHook(
          "game", "mpkFopenById", (uintptr_t *)&gameExeMpkFopenById,
          (LPVOID)&mpkFopenByIdHook, (LPVOID *)&gameExeMpkFopenByIdReal))
    return retval;

  return retval;
}

int __fastcall mpkFopenByIdHook(void *pThis, void *EDX, void *mpkObject,
                                int fileId, int unk3) {
#ifdef _DEBUG
  std::stringstream logstr;
  uint8_t *mpkFilename = (uint8_t *)mpkObject + 1;
  logstr << "mpkFopenById(" << mpkFilename << ".mpk, 0x" << std::hex << fileId
         << ")";
  LanguageBarrierLog(logstr.str());
#endif
  return gameExeMpkFopenByIdReal(pThis, mpkObject, fileId, unk3);
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
// TODO: figure out how to turn looping off
void gameSetBgm(uint32_t fileId) { *(uint32_t *)gameExePCurrentBgm = fileId; }
}