#pragma once

using drawMde_t = int(__cdecl*)(int a1, int a2, int a3, int a4, int a5,
                                float a6, float a7, float a8, float a9, int a10,
                                int a11, int a12);

using helpDisp_t = int(__cdecl*)(int a1);


namespace  {

lb::Hook<helpDisp_t>* helpDisp;
lb::Hook<drawMde_t>* drawMde;
}  // namespace Hooks