#include "LightCollectionRunActionMessenger.hh"
#include "LightCollectionRunAction.hh"
#include "G4UIcmdWithAString.hh"
#include "G4UIdirectory.hh"
#include "G4UIcommand.hh"

LightCollectionRunActionMessenger::LightCollectionRunActionMessenger(LightCollectionRunAction* runAction)
 : G4UImessenger(),
   fRunAction(runAction)
{
    // 定义一个命令目录，比如 /MySim/，只是为了分层管理命令
    // 也可用 /YourProject/ 或任意你喜欢的名字
    G4UIdirectory* simDir = new G4UIdirectory("/MySim/");
    simDir->SetGuidance("Commands for my simulation");

    // 定义一个字符串类型的命令
    fSetFileNameCmd = new G4UIcmdWithAString("/MySim/setSaveName", this);
    fSetFileNameCmd->SetGuidance("Set output file name");
    fSetFileNameCmd->SetParameterName("filename", false); // false表示必须提供参数
    fSetFileNameCmd->AvailableForStates(G4State_PreInit, G4State_Idle);
}

LightCollectionRunActionMessenger::~LightCollectionRunActionMessenger()
{
    delete fSetFileNameCmd;
    // 如果有 simDir, 需要根据实际写法决定是否要 delete
}

void LightCollectionRunActionMessenger::SetNewValue(G4UIcommand* cmd, G4String newValue)
{
    if(cmd == fSetFileNameCmd) {
        fRunAction->SetFileName(newValue);  // 将命令传入的值传给RunAction
    }
}
