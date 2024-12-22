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
/// \file LightCollectionActionInitialization.cc
/// \brief Implementation of the LightCollectionActionInitialization class

#include "LightCollectionActionInitialization.hh"
#include "LightCollectionEventAction.hh"
#include "LightCollectionPrimaryGeneratorAction.hh"
#include "LightCollectionRunAction.hh"
#include "LightCollectionStackingAction.hh"
#include "LightCollectionSteppingAction.hh"
#include "LightCollectionDetectorConstruction.hh"
#include "MyTrackingAction.hh"

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
LightCollectionActionInitialization::LightCollectionActionInitialization()
  : G4VUserActionInitialization()
{}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
LightCollectionActionInitialization::~LightCollectionActionInitialization() {}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void LightCollectionActionInitialization::BuildForMaster() const
{
  SetUserAction(new LightCollectionRunAction());
}

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
void LightCollectionActionInitialization::Build() const
{
  LightCollectionDetectorConstruction* detectorConstruction = new LightCollectionDetectorConstruction();
  LightCollectionPrimaryGeneratorAction* primary = new LightCollectionPrimaryGeneratorAction(detectorConstruction);
  SetUserAction(primary);
  SetUserAction(new LightCollectionRunAction(primary));
  LightCollectionEventAction* event = new LightCollectionEventAction();
  SetUserAction(event);
  SetUserAction(new LightCollectionSteppingAction(event));
  SetUserAction(new MyTrackingAction());
  SetUserAction(new LightCollectionStackingAction());
}
