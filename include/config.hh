#ifndef CONFIG_HH
#define CONFIG_HH

#include "G4ThreeVector.hh"
#include "MyMaterials.hh"
#include "G4SystemOfUnits.hh"

// g_ means global_

// switch
inline G4bool g_has_opticalPhysics = true;  // 是否模拟光学过程
inline G4bool g_has_cherenkov = true;       // 是否考虑切伦科夫光

/*
        ↑ z
        |
        |               **** -- Source
        |———→ x   —————————————— -- shield layers_0
       ∕          —————————————— -- shield layers_n
    y↙              ■■———— -- Scintillator&LightGuide
*/
// 射线沿Z轴负半轴入射，遮挡材料在XOY平面，闪烁体/光导沿X轴读出

// world
inline G4double g_worldX = 20 * cm;
inline G4double g_worldY = 20 * cm;
inline G4double g_worldZ = 20 * cm;
inline G4Material *g_world_material = MyMaterials::Vacuum();

// shield = n*layers
inline G4double g_shieldX = 0.99 * g_worldX;
inline G4double g_shieldY = 0.99 * g_worldY;
inline G4int g_shield_layers = 5;                                                             // 遮挡层层数n
inline G4double g_shield_thickness = 0.5 * cm;                                                  // 遮挡总厚度 = n*d
inline G4double g_layer_thickness = g_shield_thickness / g_shield_layers;                     // 遮挡厚度 = d
inline G4ThreeVector g_shield_pos = G4ThreeVector(0, 0, 0.4 * g_worldZ - g_shield_thickness); // 遮挡层位置，相对世界体
// inline G4Material* g_shield_material = MyMaterials::GarthTypographicAlloy();                             // 遮挡层材料
inline G4Material *g_shield_material = MyMaterials::PVC(); // 遮挡层材料

// scintillator = crystal + wrapper
// 闪烁体实际的尺寸是 scintillator - 2*g_sc_wrapper_thickness
inline G4bool g_is_Tub_sc = false; // 闪烁体是否是圆柱形的。圆柱形时只考虑沿X轴读出
inline G4int g_lg_orientation = 0; // 0: 沿X轴读出，1: 沿Z轴读出
inline G4double g_scintillatorY = 1 * cm;
inline G4double g_scintillatorZ = 1 * cm;
inline G4double g_scintillatorR = 1 * cm;                                                                              // 径向
inline G4double g_scintillatorX = 1 * cm;                                                                              // 轴向
inline G4ThreeVector g_scintillator_pos = G4ThreeVector(-0.2 * g_worldX, 0, -0.25 * g_worldZ + 0.5 * g_scintillatorZ); // 闪烁体位置，相对世界体
inline G4Material *g_sc_crystal_material = MyMaterials::GAGG_Ce_Mg(1000, 1, -1);                                     // 闪烁体材料
inline G4double g_sc_wrapper_thickness = 1 * mm;                                                                       // 闪烁体封装层厚度
inline G4Material *g_sc_wrapper_material = MyMaterials::PVC();
inline G4ThreeVector g_crystal_pos = G4ThreeVector(0, 0, 0); // crystal位置，相对闪烁体。可以调节该项，调整晶体在封装中的位置
// inline G4ThreeVector g_crystal_pos = G4ThreeVector(0, 0, g_sc_wrapper_thickness); // crystal位置，相对闪烁体。可以调节该项，调整晶体在封装中的位置

// lightguide = fiber + wrapper
// lightguide实际的尺寸是 g_lightguide_r - 2*g_lg_wrapper_thickness
inline G4double g_lg_wrapper_thickness = 0.1 * mm;                // 光导封装层厚度
inline G4double g_lightguide_length = 1 * cm;                     // 光导长度
inline G4double g_lightguide_r = 8 * mm + g_lg_wrapper_thickness; // 光导半径
inline G4ThreeVector g_lightguide_pos = g_scintillator_pos;       // 光导位置，相对闪烁体
inline G4Material *g_lg_fiber_material = MyMaterials::Quartz();   // 光导材料
inline G4Material *g_lg_wrapper_material = MyMaterials::PVC();    // 光导封装层材料，PVC的折射率数值给的随意供参考
inline G4int g_lg_nums = 1;                                       // 光导数量
inline G4double g_lg_gap = 1.1 * g_lightguide_r;                  // 光导间隙，应大于光导直径
inline G4double g_lg_depth = 0.618 * cm;                          // 穿过depth的光子才被计数
inline G4double g_lg_na = -1;                                     // 光导数值孔径。-1表示根据前面定义的材料和实际光子能量进行计算

// surface
inline G4OpticalSurface *surf_Teflon = MyMaterials::surf_Teflon();

// source
inline G4double g_source_scale = 1; // 源的尺度是scintillator投影的若干倍

#endif // CONFIG_HH