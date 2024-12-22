#include "MyTrackInfo.hh"
#include "G4String.hh"

MyTrackInfo::MyTrackInfo()
 : G4VUserTrackInformation()
{
    // 构造时，没有任何记录，全部默认 false
}

MyTrackInfo::~MyTrackInfo()
{}

void MyTrackInfo::SetHasPassedLayer(const G4String& layerName, bool flag)
{
    // 直接在 map 中写入
    fPassedLayers[layerName] = flag;
}

bool MyTrackInfo::HasPassedLayer(const G4String& layerName) const
{
    auto it = fPassedLayers.find(layerName);
    if(it == fPassedLayers.end()) {
        // 如果没在 map 中找到，说明没有被标记过，默认是 false
        return false;
    }
    return it->second;
}

void MyTrackInfo::InheritPassedLayers(const MyTrackInfo* parentInfo)
{
    if(!parentInfo) return;
    // 将母粒子所有的 layer 标记复制到当前粒子
    for(const auto& kv : parentInfo->fPassedLayers)
    {
        // 如果母粒子某 layer 是 true，子粒子也应该是 true
        // 这里给你一个选择，如果你希望保留“子粒子一开始都为 false，再自己统计”
        // 也可以跳过复制。但是通常是需要继承。
        if(kv.second == true) {
            this->fPassedLayers[kv.first] = true;
        }
    }
}
