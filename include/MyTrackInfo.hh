#ifndef MYTRACKINFO_HH
#define MYTRACKINFO_HH

#include "G4VUserTrackInformation.hh"
#include <map>
#include <string>

class MyTrackInfo : public G4VUserTrackInformation
{
public:
    MyTrackInfo();
    virtual ~MyTrackInfo();

    // 设置 / 获取针对某个 layer 的 "HasPassed"
    void SetHasPassedLayer(const G4String& layerName, bool flag);
    bool HasPassedLayer(const G4String& layerName) const;

    // 将母粒子的标记复制到当前对象
    // 用于在二次粒子创建时继承母粒子的已穿过信息
    void InheritPassedLayers(const MyTrackInfo* parentInfo);

private:
    // 针对不同 layer 的标记表
    // key: layerName, value: 是否已经穿过
    std::map<G4String, bool> fPassedLayers;
};

#endif
