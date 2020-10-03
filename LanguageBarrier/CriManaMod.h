#ifndef __CRIMANAMOD_H__
#define __CRIMANAMOD_H__

#include <cstdint>
#include <d3d11.h>

typedef int(__thiscall* SurfMapProc)(void* pThis, int dummy,
                                     int texId, int unk2);
typedef int(__thiscall* SurfUnMapProc)(void* pThis, int texId);

extern uint32_t RENDER_TARGET_SURF_ID;
extern uint32_t SUBS_LAYER_SURF_ID;

namespace lb {
bool criManaModInit();
}

#endif  // !__CRIMANAMOD_H__
