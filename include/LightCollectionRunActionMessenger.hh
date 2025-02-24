#ifndef RunActionMessenger_h
#define RunActionMessenger_h 1

#include "G4UImessenger.hh"
#include "globals.hh"

class G4UIcmdWithAString;
class LightCollectionRunAction;

class LightCollectionRunActionMessenger : public G4UImessenger
{
public:
    LightCollectionRunActionMessenger(LightCollectionRunAction *runAction);
    virtual ~LightCollectionRunActionMessenger();

    // 核心：解析命令行时会调用此方法
    virtual void SetNewValue(G4UIcommand *cmd, G4String newValue);

private:
    LightCollectionRunAction *fRunAction;               // 指向你的RunAction实例
    G4UIcmdWithAString *fSetFileNameCmd; // 设置文件名的命令
};

#endif
