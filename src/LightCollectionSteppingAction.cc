//
// ********************************************************************
// * License and Disclaimer                                           *
// *                                                                  *
// * The  Geant4 software  is  copyright of the Copyright Holders  of *
// * the Geant4 Collaboration.  It is provided  under  the terms  and *
// * conditions of the Geant4 Software License,  included in the file *
// * LICENSE and available at  http://cern.ch/geant4/license .  These *
// * include a list of copyright holders.                             *
// *                                                                  *
// * Neither the authors of this software system, nor their employing *
// * institutes,nor the agencies providing financial support for this *
// * work  make  any representation or  warranty, express or implied, *
// * regarding  this  software system or assume any liability for its *
// * use.  Please see the license in the file  LICENSE  and URL above *
// * for the full disclaimer and the limitation of liability.         *
// *                                                                  *
// * This  code  implementation is the result of  the  scientific and *
// * technical work of the GEANT4 collaboration.                      *
// * By using,  copying,  modifying or  distributing the software (or *
// * any work based  on the software)  you  agree  to acknowledge its *
// * use  in  resulting  scientific  publications,  and indicate your *
// * acceptance of all terms of the Geant4 Software license.          *
// ********************************************************************
//
//
/// \file LightCollectionSteppingAction.cc
/// \brief Implementation of the LightCollectionSteppingAction class

#include "LightCollectionSteppingAction.hh"
#include "LightCollectionRun.hh"

#include "G4Event.hh"
#include "G4OpBoundaryProcess.hh"
#include "G4OpticalPhoton.hh"
#include "G4RunManager.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4AnalysisManager.hh"
#include <vector>

#include "config.hh"
#include "MyTrackInfo.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......

LightCollectionSteppingAction::LightCollectionSteppingAction(LightCollectionEventAction* event)
  : G4UserSteppingAction()
  , fEventAction(event)
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
LightCollectionSteppingAction::~LightCollectionSteppingAction() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void LightCollectionSteppingAction::UserSteppingAction(const G4Step* step)
{

    // ----------------------------------------------------
    // 对次级粒子进行标记，判断是否已经穿越某层SD
    // ----------------------------------------------------
    // 母粒子 track
    G4Track* theTrack = step->GetTrack();
    MyTrackInfo* parentInfo = dynamic_cast<MyTrackInfo*>(theTrack->GetUserInformation());

    // 获取次级粒子列表
    const std::vector<const G4Track*>* secondaries = step->GetSecondaryInCurrentStep();
    if(!secondaries || secondaries->empty()) return;

    // 遍历所有次级粒子
    for (size_t i = 0; i < secondaries->size(); i++)
    {
        G4Track* childTrack = const_cast<G4Track*>((*secondaries)[i]);
        if(!childTrack) continue;

        // 为次级粒子分配新的 MyTrackInfo
        MyTrackInfo* childInfo = new MyTrackInfo();
        // 如果母粒子有标记，则继承
        if(parentInfo) {
            childInfo->InheritPassedLayers(parentInfo);
        }

        // 将 trackInfo 附加到二次粒子
        childTrack->SetUserInformation(childInfo);
    }

    // ----------------------------------------------------
    // 上面是对次级粒子进行标记，判断是否已经穿越某层SD的代码
    // ----------------------------------------------------

    static const G4ParticleDefinition* opticalphoton = G4OpticalPhoton::OpticalPhotonDefinition();
    const G4ParticleDefinition* particleDef = step->GetTrack()->GetParticleDefinition();

    // 检查是否为光子
    if (particleDef != opticalphoton) return;

    // 获取步骤的前后点
    G4StepPoint* preStepPoint = step->GetPreStepPoint();
    G4StepPoint* postStepPoint = step->GetPostStepPoint();
    if (!preStepPoint || !postStepPoint) return;

    G4TouchableHandle touchableHandle = postStepPoint->GetTouchableHandle();
    if (!touchableHandle) return;

    // 获取前后点的物理体积
    G4VPhysicalVolume* preVolume = preStepPoint->GetPhysicalVolume();
    G4VPhysicalVolume* postVolume = postStepPoint->GetPhysicalVolume();
    if (!preVolume || !postVolume) return;

    // 获取前后点的逻辑体积
    G4LogicalVolume* preLogicalVolume = preVolume->GetLogicalVolume();
    G4LogicalVolume* postLogicalVolume = postVolume->GetLogicalVolume();
    if (!preLogicalVolume || !postLogicalVolume) return;

    G4String preVolumeName = preLogicalVolume->GetName();
    G4String postVolumeName = postLogicalVolume->GetName();


    // 检查光子是否穿过crystal端窗和光阴极到达SensitiveVolume
    if(postVolumeName == "SD2" )
    {
      // 杀掉本次事例
      step->GetTrack()->SetTrackStatus(fStopAndKill);
    }


    // 考虑数值孔径
    // 
    if (preVolumeName == "World" && postVolumeName == "lg_fiber" )
    {
        
        G4Track* aTrack = step->GetTrack();
        G4int trackID = aTrack->GetTrackID();

    if (fEventAction->processedTrackIDs.find(trackID) == fEventAction->processedTrackIDs.end())
    {

        G4double energy = aTrack->GetTotalEnergy();
        G4double wavelength = (1239.841939 * CLHEP::nm) / energy;  // 将能量转换为波长

        // 获取折射率
        G4MaterialPropertyVector* rv_Outside = preStepPoint->GetMaterial()->GetMaterialPropertiesTable()->GetProperty("RINDEX");
        G4MaterialPropertyVector* rv_fiber_core = g_lg_fiber_material->GetMaterialPropertiesTable()->GetProperty("RINDEX");
        G4MaterialPropertyVector* rv_fiber_wrapper = g_lg_wrapper_material->GetMaterialPropertiesTable()->GetProperty("RINDEX");

        if (!rv_Outside) return;

        G4double n_Outside = rv_Outside->Value(energy);
        G4double n_fiber_core = rv_fiber_core->Value(energy);
        G4double n_fiber_wrapper = rv_fiber_wrapper->Value(energy);
        
        G4double NA;
        if(g_lg_na==-1){
            NA = std::sqrt(n_fiber_core*n_fiber_core - n_fiber_wrapper*n_fiber_wrapper);
        }
        else{
            NA = g_lg_na;
        }

        G4ThreeVector photonDirection = aTrack->GetMomentumDirection();
        G4ThreeVector normal = preStepPoint->GetTouchableHandle()->GetSolid()->SurfaceNormal(preStepPoint->GetPosition());

        // 计算入射角的余弦
        G4double cosTheta = -photonDirection.dot(normal);
        G4double theta = std::acos(cosTheta);

        // 根据折射率计算极限角
        G4double criticalAngle = std::asin(NA / n_Outside);

        // G4cout << "Photon ID: " << aTrack->GetTrackID() << G4endl;
        // G4cout << "Photon Energy: " << energy << " eV" << G4endl;
        // G4cout << "Photon Wavelength: " << (1239.841939 * nm) / energy << " nm" << G4endl;
        // G4cout << "n_Outside: " << n_Outside << G4endl;
        // G4cout << "n_fiber_core: " << n_fiber_core << G4endl;
        // G4cout << "n_fiber_wrapper: " << n_fiber_wrapper << G4endl;
        // G4cout << "Numerical Aperture (NA): " << NA << G4endl;
        // G4cout << "Photon Direction: " << photonDirection << G4endl;
        // G4cout << "Surface Normal: " << normal << G4endl;
        // G4cout << "cosTheta: " << cosTheta << G4endl;
        // G4cout << "Incident Angle: " << theta / CLHEP::deg << " degrees" << G4endl;
        // G4cout << "Critical Angle: " << criticalAngle / CLHEP::deg << " degrees" << G4endl;

        if (theta < criticalAngle)
        {
            // G4cout << "Photon ID: " << trackID << " is accepted" << G4endl;

            auto analysisManager = G4AnalysisManager::Instance();
            analysisManager->FillH1(2, wavelength);

            // 只有当该光子被记录后，才将其ID加入已处理列表。因为该光子可能在外层多次反射最终进入
            fEventAction->processedTrackIDs.insert(trackID);
        }

        
    }
    }

   
    
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
