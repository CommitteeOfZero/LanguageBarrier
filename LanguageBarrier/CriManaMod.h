#ifndef __CRIMANAMOD_H__
#define __CRIMANAMOD_H__

#include <d3d11.h>

typedef int(__thiscall* SurfMapProc)(void* pThis, int dummy,
                                     int texId, int unk2);
typedef int(__thiscall* SurfUnMapProc)(void* pThis, int texId);

#pragma pack(push, 1)
struct CSurfaceVftable
{
  SurfMapProc mapTex;
  SurfUnMapProc unmapTex;
};

struct CSurface
{
  CSurfaceVftable* vftable;
  char field_4[8];
  int field_C;
  char field_10[4];
  int mgsFormat;
  int field_18;
  int field_1C;
  char field_20[4];
  int field_24;
  short field_28;
  short field_2A;
  int field_2C;
  int field_30;
  void* textureData;
  int field_38;
  int field_3C;
  int textureWidth;
  int textureHeight;
  float field_48;
  float field_4C;
  int field_50;
  int field_54;
  int field_58;
  short field_5C;
  short field_5E;
  short field_60;
  short field_62;
  short field_64;
  short field_66;
  int field_68;
  int field_6C;
  short field_70;
  short field_72;
  short field_74;
  short field_76;
  short field_78;
  short field_7A;
  int field_7C;
  int field_80;
  short field_84;
  short field_86;
  short field_88;
  short field_8A;
  short field_8C;
  short field_8E;
  int field_90;
  int field_94;
  short field_98;
  short field_9A;
  short field_9C;
  short field_9E;
  short field_A0;
  char field_A2[18];
  ID3D11Texture2D* dx11Texture[4];
  ID3D11Texture2D* dx11ReplaceTexture[4];
  char gap_C4[16];
  ID3D11ShaderResourceView* shaderRscView;
  int field_E8;
  int field_EC;
  int field_F0;
  char gap_F4[74];
  short field_13C;
};
#pragma pack(pop)

namespace lb {
bool criManaModInit();
}

#endif  // !__CRIMANAMOD_H__
