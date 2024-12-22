#include "MyTrackingAction.hh"
#include "MyTrackInfo.hh"
#include "G4Track.hh"
#include "G4VUserTrackInformation.hh"

MyTrackingAction::MyTrackingAction()
 : G4UserTrackingAction()
{}

MyTrackingAction::~MyTrackingAction()
{}

void MyTrackingAction::PreUserTrackingAction(const G4Track* track)
{
    // 如果是主粒子 (ParentID=0)，则给它分配 MyTrackInfo
    if(track->GetParentID() == 0)
    {
        MyTrackInfo* trackInfo = new MyTrackInfo();
        // 给当前Track附加上
        const_cast<G4Track*>(track)->SetUserInformation(trackInfo);
    }
}

void MyTrackingAction::PostUserTrackingAction(const G4Track* /*track*/)
{}
