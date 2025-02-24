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
/// \file LightCollection/src/LightCollectionRunAction.cc
/// \brief Implementation of the LightCollectionRunAction class
//
//
//
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
#include "LightCollectionRunAction.hh"
#include "LightCollectionPrimaryGeneratorAction.hh"
#include "LightCollectionRun.hh"
#include "G4ParticleDefinition.hh"
#include "G4Run.hh"
#include <fstream>
#include <filesystem>
#include <sstream>
#include <mutex>
#include "G4UnitsTable.hh"
#include "G4SystemOfUnits.hh"

#include "G4AnalysisManager.hh"
#include "G4AccumulableManager.hh"

#include "LightCollectionRunActionMessenger.hh"

#include "config.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
LightCollectionRunAction::LightCollectionRunAction(LightCollectionPrimaryGeneratorAction *prim)
    : G4UserRunAction(), fRun(nullptr), fPrimary(prim)
{

  // 创建Messenger
  fMessenger = new LightCollectionRunActionMessenger(this);
  fSaveFileName = "LightCollection"; // 默认文件名
  
  auto analysisManager = G4AnalysisManager::Instance();
  analysisManager->SetVerboseLevel(1);
  analysisManager->SetNtupleMerging(true);

  // Ntuple-ID = 0
  analysisManager->CreateNtuple("Spectrum", "Spectrum");
  analysisManager->CreateNtupleDColumn("Source Spectrum");
  analysisManager->CreateNtupleDColumn("After-shield Spectrum");
  analysisManager->CreateNtupleDColumn("Edep in Crystal");
  analysisManager->FinishNtuple();

  // 创建Ntuple
  for (int id = 0; id < g_shield_layers; ++id)
  {
    G4String ntupleName = "Shield_layer_" + std::to_string(id);
    G4String ntupleTitle = "Energy and particles in shield layer " + std::to_string(id);
    analysisManager->CreateNtuple(ntupleName, ntupleTitle);
    analysisManager->CreateNtupleDColumn("energyDeposit"); // 在当前层沉积的能量
    analysisManager->CreateNtupleDColumn("PassingEnergy"); // 穿过当前层的总能谱
    analysisManager->CreateNtupleDColumn("TruelyPassingEnergy"); // 穿过当前层的总能谱(Truely, 不包括多次穿越的重复统计)
    analysisManager->CreateNtupleDColumn("PassingEnergy_Secondary"); // 穿过当前层的次级射线能谱
    analysisManager->CreateNtupleDColumn("TruelyPassingEnergy_Secondary"); // 穿过当前层的次级射线能谱(Truely, 不包括多次穿越的重复统计)
    analysisManager->CreateNtupleDColumn("HEphotonEnergy"); // 当前层产生并出射的次级gamma能谱
    analysisManager->CreateNtupleDColumn("NeutronEnergy"); // 当前层产生并出射的次级中子能谱
    analysisManager->FinishNtuple();
  }

  // Creating histograms for the spectra
  analysisManager->CreateH1("ScintillationWavelength", "Scintillation Wavelength in Crystal", 600, 200.0, 800.0);
  analysisManager->CreateH1("CherenkovLightWavelength", "CherenkovLight Wavelength in Crystal", 600, 200.0, 800.0);
  analysisManager->CreateH1("FiberNumericalApertureWavelength", "Wavelength of Light Entering Fiber Numerical Aperture", 600, 200.0, 800.0);
  analysisManager->CreateH1("FiberEntryWavelength", "Wavelength of Light Entering Fiber", 600, 200.0, 800.0);
  analysisManager->CreateH2("SourcePosition", "Source Position", 200, -0.5 * g_worldX, 0.5 * g_worldX, 200, -0.5 * g_worldY, 0.5 * g_worldY);
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
LightCollectionRunAction::~LightCollectionRunAction() {
  delete fMessenger;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
G4Run *LightCollectionRunAction::GenerateRun()
{
  fRun = new LightCollectionRun();
  return fRun;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void LightCollectionRunAction::BeginOfRunAction(const G4Run *)
{
  if (fPrimary)
  {
    G4double energy;
    G4ParticleDefinition *particle;

    fPrimary->isInitialized = false;
    if (fPrimary->GetUseParticleGun())
    {
      particle = fPrimary->GetParticleGun()->GetParticleDefinition();
      energy = fPrimary->GetParticleGun()->GetParticleEnergy();
    }
    else
    {
      particle = fPrimary->GetGPS()->GetParticleDefinition();
      energy = fPrimary->GetGPS()->GetCurrentSource()->GetEneDist()->GetMonoEnergy();
    }

    fRun->SetPrimary(particle, energy);
  }

  // Open an output file
  //
  auto analysisManager = G4AnalysisManager::Instance();

  G4String fileName = getNewfileName(fSaveFileName);
  if (!analysisManager->OpenFile(fileName))
  {
    G4cerr << "Error: could not open file " << fileName << G4endl;
  }
  else
  {
    G4cout << "Successfully opened file " << fileName << G4endl;
  }
  G4cout << "Using " << analysisManager->GetType() << G4endl;
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void LightCollectionRunAction::EndOfRunAction(const G4Run * run)
{

  G4int runID = run->GetRunID();

  auto analysisManager = G4AnalysisManager::Instance();

  if (isMaster)
  {

    static std::mutex fileMutex;
    std::lock_guard<std::mutex> lock(fileMutex); // 使用互斥锁确保线程安全

    std::ofstream outFile("run_data.csv", std::ios::app); // 以追加模式打开文件

    // 在文件为空时写入标题行
    if (outFile.tellp() == 0)
    {
      outFile << "runID,"
              << "Scintillation Photon Count,"
              << "Cherenkov Photon Count,"
              << "Fiber Numerical Aperture Photon Count,"
              << "Fiber Entry Photon Count\n";
    }

    // 统计光子产生与收集
    G4int scintillationPhotonCount = analysisManager->GetH1(0)->entries();
    G4int cherenkovPhotonCount = analysisManager->GetH1(1)->entries();
    G4int fiberNumericalAperturePhotonCount = analysisManager->GetH1(2)->entries();
    G4int fiberEntryPhotonCount = analysisManager->GetH1(3)->entries();

    // 写入数据
    outFile << runID << ","
            << scintillationPhotonCount << ","
            << cherenkovPhotonCount << ","
            << fiberNumericalAperturePhotonCount << ","
            << fiberEntryPhotonCount << "\n";

    // 关闭文件
    outFile.close();
  }
  analysisManager->Write();
  analysisManager->CloseFile();

  G4cout << "Data written and file closed." << G4endl;
}

bool LightCollectionRunAction::fileExists(const G4String &fileName)
{
  std::ifstream file(fileName.c_str());
  return file.good();
}

G4String LightCollectionRunAction::getNewfileName(G4String baseFileName)
{
  G4String fileExtension = ".root";
  G4String fileName;
  int fileIndex = 0;

  do
  {
    std::stringstream ss;
    ss << baseFileName;
    if (fileIndex > 0)
    {
      ss << "(" << fileIndex << ")";
    }
    ss << fileExtension;
    fileName = ss.str();
    fileIndex++;
  } while (fileExists(fileName));

  return fileName;
}
//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
