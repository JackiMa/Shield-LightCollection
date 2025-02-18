#include <vector>

#include "LightCollectionDetectorConstruction.hh"
#include "LightCollectionDetectorMessenger.hh"
#include "LightCollectionLayerSensitiveDetector.hh"

#include "G4Element.hh"
#include "G4GDMLParser.hh"
#include "G4Box.hh"
#include "G4LogicalBorderSurface.hh"
#include "G4LogicalSkinSurface.hh"
#include "G4LogicalVolume.hh"
#include "G4OpticalSurface.hh"
#include "G4VPhysicalVolume.hh"
#include "G4SystemOfUnits.hh"
#include "G4ThreeVector.hh"
#include "G4VisAttributes.hh"
#include "G4SubtractionSolid.hh"
#include "G4UnionSolid.hh"
#include "G4AssemblyVolume.hh"

#include "G4GlobalMagFieldMessenger.hh"
#include "G4SDManager.hh"
#include "G4SDChargedFilter.hh"
#include "G4MultiFunctionalDetector.hh"
#include "G4VPrimitiveScorer.hh"
#include "G4PSEnergyDeposit.hh"
#include "G4Exception.hh"

#include "MyMaterials.hh"
#include "MyPhysicalVolume.hh"
#include "CustomScorer.hh"
#include "utilities.hh"
#include "config.hh"

std::vector<G4ThreeVector> generateCoordinates(int nums, double gaps);

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
// 在这里对类相关参数进行初始化
LightCollectionDetectorConstruction::LightCollectionDetectorConstruction()
    : G4VUserDetectorConstruction()
{
  fDumpGdmlFileName = "LightCollecion.gdml";
  fVerbose = false;  // 是否输出详细信息
  fDumpGdml = false; // 是否保存GDML的几何文件
  // create a messenger for this class
  fDetectorMessenger = new LightCollectionDetectorMessenger(this);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
LightCollectionDetectorConstruction::~LightCollectionDetectorConstruction()
{
  delete fDetectorMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4VPhysicalVolume *LightCollectionDetectorConstruction::Construct()
{
  G4bool checkOverlaps = true;

  // ------------- Volumes --------------
  // s_ for soild_
  // l_ for logical_
  // p_ for physical_
  //
  // The world
  G4Box *s_world = new G4Box("World", 0.5 * g_worldX, 0.5 * g_worldY, 0.5 * g_worldZ);
  G4LogicalVolume *l_world = new G4LogicalVolume(s_world, g_world_material, "World");
  MyPhysicalVolume *p_world = new MyPhysicalVolume(0, G4ThreeVector(), "World", l_world, nullptr, false, 0, checkOverlaps);

  // ====================================
  // ============== Shield ==============
  // ====================================
  //

  // Shield = n*layers + SD1
  G4Box *s_shield = new G4Box("shield", 0.5 * g_shieldX, 0.5 * g_shieldY, 0.5 * g_shield_thickness);
  G4LogicalVolume *l_shield = new G4LogicalVolume(s_shield, g_world_material, "shield");
  MyPhysicalVolume *p_shield = new MyPhysicalVolume(0, g_shield_pos, "shield", l_shield, p_world, false, checkOverlaps);
  fVolumeMap["shield"] = p_shield;
  for (long unsigned int i = 0; i < g_custom_shield.size(); ++i)
  {
    const auto &layer = g_custom_shield[i];
    G4cout << "layer.thickness: " << layer.thickness << " layer.material: " << layer.material->GetName() << G4endl;
    G4String layer_name = "shield_layer_" + std::to_string(i);
    G4Box *s_layer = new G4Box(layer_name, 0.5 * g_shieldX, 0.5 * g_shieldY, 0.5 * layer.thickness);
    G4LogicalVolume *l_layer = new G4LogicalVolume(s_layer, layer.material, layer_name);
    G4ThreeVector layer_pos = G4ThreeVector(0, 0, (g_shield_thickness / 2.0 - 0.5 * layer.thickness - std::accumulate(g_custom_shield.begin(), g_custom_shield.begin() + i, 0.0, [](double sum, const ShieldLayer &l)
                                                                                                                      { return sum + l.thickness; })));
    MyPhysicalVolume *p_layer = new MyPhysicalVolume(0, layer_pos, layer_name, l_layer, p_shield, false, checkOverlaps);
    fVolumeMap[layer_name] = p_layer;
    // set shield layers colour
    double ratio = static_cast<double>(i) / (g_custom_shield.size() - 1);
    double red = ratio; // 从0到1
    double green = 0.0;
    double blue = 1.0 - ratio;
    G4VisAttributes *layer_VisAtt = new G4VisAttributes(G4Colour(red, green, blue, 0.8));
    layer_VisAtt->SetForceSolid(true); // 设置为实心，以便可以看到透明度
    layer_VisAtt->SetVisibility(true);
    l_layer->SetVisAttributes(layer_VisAtt);
  }

  // SD1
  G4double SD1_thickness = 1 * mm;
  G4ThreeVector SD1_pos = g_shield_pos - G4ThreeVector(0, 0, 2 * mm + 0.5 * g_shield_thickness + 0.5 * SD1_thickness);
  G4Box *s_SD1 = new G4Box("SD1", 0.5 * g_shieldX, 0.5 * g_shieldY, SD1_thickness);
  G4LogicalVolume *l_SD1 = new G4LogicalVolume(s_SD1, g_world_material, "SD1");
  MyPhysicalVolume *p_SD1 = new MyPhysicalVolume(0, SD1_pos, "SD1", l_SD1, p_world, false, checkOverlaps);
  fVolumeMap["SD1"] = p_SD1;
  G4VisAttributes *SD1_VisAtt = new G4VisAttributes(G4Colour(1, 1, 1, 0.3));
  SD1_VisAtt->SetForceSolid(true); // 设置为实心，以便可以看到透明度
  SD1_VisAtt->SetVisibility(true);
  l_SD1->SetVisAttributes(SD1_VisAtt);

  // ====================================
  // =========== Scintillator ===========
  // ====================================
  //

  // ▣
  // scintillator = crystal + wrapper
  G4VSolid *s_sc_wrapper = nullptr;
  G4VSolid *s_sc_crystal = nullptr;
  G4RotationMatrix *rotationMatrix = new G4RotationMatrix();
  G4RotationMatrix *detector_RM = new G4RotationMatrix();
  G4RotationMatrix *detector_RM_inverse = new G4RotationMatrix();
  G4ThreeVector holes_pos;
  if (g_is_Tub_sc == true)
  {
    g_lg_orientation = 0; // 闪烁体是圆柱时，只考虑沿X读出
    s_sc_wrapper = new G4Tubs("sc_wrapper", 0, 0.5 * g_scintillatorR, 0.5 * g_scintillatorZ, 0, 360 * deg);
    s_sc_crystal = new G4Tubs("sc_crystal", 0, 0.5 * g_scintillatorR - g_sc_wrapper_thickness, 0.5 * g_scintillatorZ - g_sc_wrapper_thickness, 0, 360 * deg);
    holes_pos = G4ThreeVector(0, 0, 0.5 * g_scintillatorZ - 0.5 * (g_sc_wrapper_thickness));
    detector_RM->rotateY(270 * deg);
  }
  else
  {
    s_sc_wrapper = new G4Box("sc_wrapper", 0.5 * g_scintillatorX, 0.5 * g_scintillatorY, 0.5 * g_scintillatorZ);
    s_sc_crystal = new G4Box("sc_crystal", 0.5 * g_scintillatorX - g_sc_wrapper_thickness, 0.5 * g_scintillatorY - g_sc_wrapper_thickness, 0.5 * g_scintillatorZ - g_sc_wrapper_thickness);
    rotationMatrix->rotateY(90 * deg);
    holes_pos = G4ThreeVector(0.5 * g_scintillatorX - 0.5 * (g_sc_wrapper_thickness), 0, 0);
  }

  // ==0，沿X读出。==1，沿Z读出
  G4double diff = 0;
  if (g_lg_orientation == 0)
  {
    g_lightguide_pos = g_lightguide_pos + G4ThreeVector(0.5 * g_scintillatorX - 0.99 * g_sc_wrapper_thickness + 0.5 * g_lightguide_length, 0, 0); // 光纤紧贴晶体
    // g_lightguide_pos = g_lightguide_pos + G4ThreeVector(0.5 * g_scintillatorX + 0.5 * g_lightguide_length, 0, 0); // 光纤在晶体封装外面
  }
  else if (g_lg_orientation == 1)
  {
    detector_RM->rotateY(270 * deg);
    detector_RM_inverse->rotateY(270 * deg);
    diff = g_crystal_pos.mag();                                                                                                                            // 晶体在封装中的位置偏移的绝对值。晶体偏移后，封装开洞的位置和厚度也要变
    g_lightguide_pos = g_lightguide_pos + G4ThreeVector(0, 0, 0.99 * g_sc_wrapper_thickness + diff - (0.5 * g_scintillatorX + 0.5 * g_lightguide_length)); // 光纤紧贴晶体
    // g_lightguide_pos = g_lightguide_pos + G4ThreeVector(0, 0, -(0.5 * g_scintillatorX + 0.5 * g_lightguide_length)); // 光纤在晶体封装外面
    holes_pos = G4ThreeVector(0.5 * g_scintillatorX - 0.5 * (g_sc_wrapper_thickness + diff), 0, 0);
  }

  // wrapper_hole for lightguide
  std::vector<G4ThreeVector> coordinates = generateCoordinates(g_lg_nums, g_lg_gap); // 根据lightguide的数量生成坐标
  // 创建初始几何体
  G4Tubs *s_wrapper_hole = new G4Tubs("wrapper_hole", 0, 1.1 * 0.5 * g_lightguide_d, 0.5 * (g_sc_wrapper_thickness + diff), 0, 360 * deg);

  G4VSolid *s_sc_wrapper_wrapper_with_hole = s_sc_wrapper;
  // 遍历 coordinates，依次减去所有的 hole
  for (const auto &coord : coordinates)
  {
    G4ThreeVector hole_position = coord + holes_pos;
    s_sc_wrapper_wrapper_with_hole = new G4SubtractionSolid("sc_wrapper_wrapper_with_hole", s_sc_wrapper_wrapper_with_hole, s_wrapper_hole, rotationMatrix, hole_position);
  }
  G4LogicalVolume *l_sc_wrapper = new G4LogicalVolume(s_sc_wrapper_wrapper_with_hole, g_sc_wrapper_material, "sc_wrapper");
  // 如果是沿Z读出，那么把沿X读出设置的scintillator的位置绕Y轴旋转90° or 270°
  // 这里要注意，scintillator的位置绕Y轴后，需要提前把crystal的坐标反向旋转，以保证crystal的位置不变
  MyPhysicalVolume *p_sc_wrapper = new MyPhysicalVolume(detector_RM, g_scintillator_pos, "sc_wrapper", l_sc_wrapper, p_world, false, 0, checkOverlaps);
  fVolumeMap["sc_wrapper"] = p_sc_wrapper;
  G4VisAttributes *sc_wrapper_VisAtt = new G4VisAttributes(G4Colour(1.0, 1, 1, 0.3));
  sc_wrapper_VisAtt->SetVisibility(true);
  sc_wrapper_VisAtt->SetForceSolid(true); // 设置为实心，以便可以看到透明度
  l_sc_wrapper->SetVisAttributes(sc_wrapper_VisAtt);

  // crystal
  G4LogicalVolume *l_sc_crystal = new G4LogicalVolume(s_sc_crystal, g_sc_crystal_material, "sc_crystal");
  MyPhysicalVolume *p_sc_crystal = new MyPhysicalVolume(0, *detector_RM_inverse * g_crystal_pos, "sc_crystal", l_sc_crystal, p_sc_wrapper, false, checkOverlaps);
  fVolumeMap["sc_crystal"] = p_sc_crystal;
  G4VisAttributes *sc_crystal_VisAtt = new G4VisAttributes(G4Colour(1.0, 0.65, 0, 0.3));
  sc_crystal_VisAtt->SetForceSolid(true); // 设置为实心，以便可以看到透明度
  sc_crystal_VisAtt->SetVisibility(true);
  l_sc_crystal->SetVisAttributes(sc_crystal_VisAtt);

  // ====================================
  // ============ Lightguide ============
  // ====================================
  //

  // lightguide = fiber + wrapper
  G4Tubs *s_lightguide = new G4Tubs("lightguide", 0, 0.5 * g_lightguide_d, 0.5 * g_lightguide_length, 0, 360 * deg);
  G4LogicalVolume *l_lightguide = new G4LogicalVolume(s_lightguide, g_lg_wrapper_material, "lightguide");

  // lg_fiber
  G4double fiber_r = g_lightguide_d - 2 * g_lg_wrapper_thickness;
  G4Tubs *s_lg_fiber = new G4Tubs("lg_fiber", 0, 0.5 * fiber_r, 0.5 * g_lightguide_length, 0, 360 * deg);
  G4LogicalVolume *l_lg_fiber = new G4LogicalVolume(s_lg_fiber, g_lg_fiber_material, "lg_fiber");
  // 将 lg_fiber 放置在 lightguide 中
  new G4PVPlacement(0, G4ThreeVector(0, 0, 0), l_lg_fiber, "lg_fiber", l_lightguide, false, 0, checkOverlaps);

  // SD2
  G4double SD2_length = 1 * mm;
  G4double SD2_pos_z = 0.5 * g_lightguide_length - g_lg_depth; // 希望设置在紧靠crystal的一端（X负半轴），由于放置圆柱体时要旋转，所以这里指定为Z负半轴。
  G4Tubs *s_SD2 = new G4Tubs("SD2", 0, 0.5 * fiber_r, 0.5 * SD2_length, 0, 360 * deg);
  G4LogicalVolume *l_SD2 = new G4LogicalVolume(s_SD2, g_lg_fiber_material, "SD2");
  // 将 SD2 放置在 lg_fiber 中
  new G4PVPlacement(0, G4ThreeVector(0, 0, SD2_pos_z), l_SD2, "SD2", l_lg_fiber, false, 0, checkOverlaps);

  G4AssemblyVolume *assembly = new G4AssemblyVolume();
  // 根据坐标生成nums个lightguide
  int copyNumber = 0;
  // for (const auto &coord : coordinates)
  // {
  //   G4ThreeVector lg_pos = coord + g_lightguide_pos;
  //   MyPhysicalVolume *p_lightguide = new MyPhysicalVolume(rotationMatrix, lg_pos, "lightguide", l_lightguide, p_world, false, copyNumber, checkOverlaps);
  //   fVolumeMap["lightguide" + std::to_string(copyNumber)] = p_lightguide;
  //   copyNumber++;
  // }
  for (const auto &coord : coordinates)
  {
    G4ThreeVector lg_pos = coord;
    G4Transform3D transform(*rotationMatrix, lg_pos);
    assembly->AddPlacedVolume(l_lightguide, transform);
    copyNumber++;
  }
  G4Transform3D assembly_transform(*detector_RM, g_lightguide_pos);
  assembly->MakeImprint(l_world, assembly_transform, 0, checkOverlaps);

  // 设置 lightguide 的颜色
  G4VisAttributes *lightguide_VisAtt = new G4VisAttributes(G4Colour(0.2, 0.2, 0.2, 0.8));
  lightguide_VisAtt->SetForceSolid(true);
  lightguide_VisAtt->SetVisibility(true);
  l_lightguide->SetVisAttributes(lightguide_VisAtt);
  G4VisAttributes *SD2_VisAtt = new G4VisAttributes(G4Colour(1, 1, 1, 0.3));
  SD2_VisAtt->SetForceSolid(true);
  SD2_VisAtt->SetVisibility(true);
  l_SD2->SetVisAttributes(SD2_VisAtt);
  G4VisAttributes *lg_fiber_VisAtt = new G4VisAttributes(G4Colour(0.8, 0.65, 0.2, 0.7));
  lg_fiber_VisAtt->SetForceSolid(true);
  lg_fiber_VisAtt->SetVisibility(true);
  l_lg_fiber->SetVisAttributes(lg_fiber_VisAtt);

  // 设置两个界面的光学特性
  // new G4LogicalBorderSurface("PhotocathodeInterface", physcrystal, physPhotocathode, surf_GlassToPhotocathode);
  // 设置材料表面的光学特性
  new G4LogicalSkinSurface("TeflonSurface", l_sc_wrapper, surf_Teflon);
  new G4LogicalSkinSurface("TeflonSurface", l_lightguide, surf_Teflon);

  myPrint(lv, f("fVolumeMap length is {}\n", fVolumeMap.size()));
  for (const auto &pair : fVolumeMap)
  {
    std::cout << "Volume name: " << pair.first << ", Volume address: " << pair.second << std::endl;
  }

  if (fDumpGdml)
  {
    std::ifstream ifile(fDumpGdmlFileName);
    if (ifile)
    {
      G4cout << fDumpGdmlFileName << " 已存在，不再写入。" << G4endl;
    }
    else
    {
      G4GDMLParser *parser = new G4GDMLParser();
      parser->Write(fDumpGdmlFileName, p_world);
    }
  }

  return p_world;
}

MyPhysicalVolume *LightCollectionDetectorConstruction::GetMyVolume(G4String volumeName) const
{
  auto it = fVolumeMap.find(volumeName);
  if (it != fVolumeMap.end())
  {
    myPrint(lv, "Volume is found!\n");
    return it->second;
  }
  else
  {
    myPrint(lv, "Volume not found!\n");
    G4Exception("LightCollectionDetectorConstruction::GetMyVolume",
                "VolumeNotFound", FatalException,
                ("Volume " + volumeName + " not found in the volume map.").c_str());
    return nullptr; // 这行代码实际上不会被执行，因为G4Exception会终止程序
  }
}

void LightCollectionDetectorConstruction::ConstructSDandField()
{
  G4SDManager::GetSDMpointer()->SetVerboseLevel(1);
  //
  // Scorers
  //
  G4VPrimitiveScorer *primitive;
  G4int fillH1ID;
  // declare crystal as a MultiFunctionalDetector scorer
  // include Edep and scintillation light
  G4String crystal_name = "sc_crystal";
  auto crystal = new G4MultiFunctionalDetector(crystal_name);
  G4SDManager::GetSDMpointer()->AddNewDetector(crystal);
  primitive = new G4PSEnergyDeposit("Edep");
  crystal->RegisterPrimitive(primitive);

  fillH1ID = 0;
  primitive = new SCLightScorer("crystal_scintillation_H1", fillH1ID);
  crystal->RegisterPrimitive(primitive);

  fillH1ID = 1;
  primitive = new CherenkovLightScorer("crystal_cherenkovLight_H1", fillH1ID);
  crystal->RegisterPrimitive(primitive);

  SetSensitiveDetector(crystal_name, crystal);

  // declare lg_fiber as a MultiFunctionalDetector scorer
  // include Edep and scintillation light
  G4String fiber_name = "lg_fiber";
  auto fiber = new G4MultiFunctionalDetector(fiber_name);
  G4SDManager::GetSDMpointer()->AddNewDetector(fiber);
  // fillH1ID = 1;
  // 这样考虑数值孔径结果不对（获取的PreStepPoint永远是lg_fiber），只好在steppingAction中进行考虑
  // primitive = new FiberAcceptanceScorer("fiber_LightCollect_H1",fillH1ID);
  // fiber->RegisterPrimitive(primitive);
  fillH1ID = 3;
  primitive = new FiberEntryPhotonScorer("fiber_Entry_H1", fillH1ID);
  fiber->RegisterPrimitive(primitive);
  SetSensitiveDetector(fiber_name, fiber);

  // declare SD1 as a MultiFunctionalDetector scorer
  G4String SD1_name = "SD1";
  auto SD1 = new G4MultiFunctionalDetector(SD1_name);
  G4SDManager::GetSDMpointer()->AddNewDetector(SD1);
  primitive = new PassingEnergyScorer("PassingEng", SD1_name);
  SD1->RegisterPrimitive(primitive);
  primitive = new TruelyPassingEnergyScorer("TruelyPassingEng", SD1_name);
  SD1->RegisterPrimitive(primitive);
  SetSensitiveDetector(SD1_name, SD1);

  // declare shield as a MultiFunctionalDetector scorer
  //
  for (int i = 0; i < g_shield_layers; ++i)
  {
    G4String layer_name = "shield_layer_" + std::to_string(i);

    auto shield_layer = new G4MultiFunctionalDetector(layer_name);
    G4SDManager::GetSDMpointer()->AddNewDetector(shield_layer);

    // B4d示例的基于HitCollection的统计
    primitive = new G4PSEnergyDeposit("Edep");
    shield_layer->RegisterPrimitive(primitive);

    // 全能量探测器
    primitive = new TotalEnergyScorer("TotalEnergy");
    shield_layer->RegisterPrimitive(primitive);

    // 自上而下穿过该层的总能量的探测器
    primitive = new PassingEnergyScorer("PassingEnergy", layer_name);
    shield_layer->RegisterPrimitive(primitive);

    // 自上而下穿过该层的总能量的探测器(Truely，避免多次散射多次穿越时的重复统计)
    primitive = new TruelyPassingEnergyScorer("TruelyPassingEnergy", layer_name);
    shield_layer->RegisterPrimitive(primitive);

    // 自上而下穿过该层的次级粒子总能量的探测器
    primitive = new PassingEnergyScorer_Secondary("PassingEnergy_Secondary", layer_name);
    shield_layer->RegisterPrimitive(primitive);

    // 自上而下穿过该层的次级粒子总能量的探测器(Truely，避免多次散射多次穿越时的重复统计)
    primitive = new TruelyPassingEnergyScorer_Secondary("TruelyPassingEnergy_Secondary", layer_name);
    shield_layer->RegisterPrimitive(primitive);

    // 中子探测器
    primitive = new NeutronScorer("NeutronEnergy");
    shield_layer->RegisterPrimitive(primitive);

    // 光子探测器
    primitive = new HEPhotonScorer("HEPhotonEnergy");
    shield_layer->RegisterPrimitive(primitive);

    SetSensitiveDetector(layer_name, shield_layer);
  }
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void LightCollectionDetectorConstruction::SetDumpGdml(G4bool val) { fDumpGdml = val; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4bool LightCollectionDetectorConstruction::IsDumpGdml() const { return fDumpGdml; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void LightCollectionDetectorConstruction::SetVerbose(G4bool val) { fVerbose = val; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4bool LightCollectionDetectorConstruction::IsVerbose() const { return fVerbose; }

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void LightCollectionDetectorConstruction::SetDumpGdmlFile(G4String filename)
{
  fDumpGdmlFileName = filename;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4String LightCollectionDetectorConstruction::GetDumpGdmlFile() const
{
  return fDumpGdmlFileName;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void LightCollectionDetectorConstruction::PrintError(G4String ed)
{
  G4Exception("LightCollectionDetectorConstruction:MaterialProperty test", "op001",
              FatalException, ed);
}

// 根据输入数量和间隔生成坐标
std::vector<G4ThreeVector> generateCoordinates(int nums, double gaps)
{
  std::vector<G4ThreeVector> coordinates;

  if (nums <= 0)
  {
    return coordinates;
  }

  if (nums == 1)
  {
    coordinates.push_back(G4ThreeVector(0, 0, 0));
  }
  else if (nums == 2)
  {
    coordinates.push_back(G4ThreeVector(0, 0, -gaps / 2));
    coordinates.push_back(G4ThreeVector(0, 0, gaps / 2));
  }
  else if (nums == 3)
  {
    coordinates.push_back(G4ThreeVector(0, -gaps / 2, -sqrt(3) * gaps / 6));
    coordinates.push_back(G4ThreeVector(0, gaps / 2, -sqrt(3) * gaps / 6));
    coordinates.push_back(G4ThreeVector(0, sqrt(3) * gaps / 3, 0));
  }
  else if (nums == 4)
  {
    coordinates.push_back(G4ThreeVector(0, -gaps / 2, -gaps / 2));
    coordinates.push_back(G4ThreeVector(0, -gaps / 2, gaps / 2));
    coordinates.push_back(G4ThreeVector(0, gaps / 2, -gaps / 2));
    coordinates.push_back(G4ThreeVector(0, gaps / 2, gaps / 2));
  }
  else if (nums == 5 or nums == 6 or nums == 7)
  {
    coordinates.push_back(G4ThreeVector(0, 0, 0));
    double radius = gaps;
    double angle = 2 * M_PI / (nums - 1);
    for (int i = 0; i < (nums - 1); ++i)
    {
      double y = radius * std::cos(i * angle);
      double z = radius * std::sin(i * angle);
      coordinates.push_back(G4ThreeVector(0, y, z));
    }
  }
  else
  {
    G4Exception("LightCollectionDetectorConstruction", "op001", FatalException,
                "The number of lightguides is not supported. Supported numbers are integers from 1 to 7.");
  }

  return coordinates;
}