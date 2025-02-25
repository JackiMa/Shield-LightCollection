TODO：
python自动化，光学过程下，自动提取 两个光子数的结果 文件

# 2025年2月25日 update
- 新增了python自动化的脚本，具体见文件scripts/auto_python/Geant4_BatchDataProc.ipynb
    - 使用方法：
        1. 在“加载环境和基础设置”中，设置好对应的路径名。默认就可以
        2. 在“实际使用”中，设置待仿真的各种调节。程序会自动遍历生成所有需要仿真的任务

# 2025年2月24日 update
- 更新了在mac文件对设置保存文件名的支持
    - 改动: 新增了RunActionMessenger
    - 用法: /MySim/setSaveName 新设置的名字

# 2025年2月18日 update
- 新增了若干闪烁体可供选择
    但是请注意 **没有仔细设置光学性质**，使用时请务必注意。
    另外，这些材料的光产额可能也需要在使用时手动指定更符合需求的值
- 新增了TruelyPassingEnergyScorer_Secondary，逻辑和TruelyPassingEnergyScorer类似

# 2024年12月22日 update
更新了TruelyPassingEnergyScorer，从而实现只统计从当前层（从上向下）离开的总能量，避免了重复统计。  
该功能代码涉及到：`CustomScorer.hh/cc, MyTrackInfo.hh/cc, MyTrackingAction.hh/cc, LightCollectionSteppingAction/cc`  
说明：
- 目标：统计从上到下穿越某层的总能量，包含母粒子和子粒子，但要避免重复统计。
- “重复”指什么？：如果同一条(或同一家族的)粒子多次从上到下穿过该层，会被加多次能量，这是不想要的。
- 引入标记：为每条粒子（Track）维护一个布尔或 map 结构，表示“是否已经从上到下穿越过该层”。一旦标记为 true，后面再穿越就不再统计。
- 母-子关系：
    - 母粒子如果在产生子粒子的那一刻已经穿透过该层，则“子粒子也继承这个已穿透的标记”，从而在子粒子后续的演化中，假如它自己又回到层上方再下穿时，就不再重复统计(因为它的家族早已被视为“穿透过了”)。
    - 但如果母粒子“还没有(或不曾)穿透过该层”，则子粒子诞生时就***不继承***这个标记，后续如果它真的从上方穿透该层，就会被统计一次。
- 关键时机：“在产生次级粒子的瞬间”，是因为：
    - 这是母粒子当下的物理状态；
    - 后续母粒子再怎样反弹/再次穿越，都不影响这条子粒子之前继承到的标记；
    - 这样就能符合“只考虑母粒子在产生子粒子时所具有的历史”的需求。

## 统计信息
### 获取的统计信息
#### Energy Spectrum
- Source Spectrum: 放射源抽样的能谱
- PassingEnergy: 从上到下穿透所有屏蔽层的总能谱，包含初级粒子和次级粒子
- Edep in Crystal: 在闪烁体中沉积能量的总能谱
#### Shield_layer_#
- energyDeposit: 在当前层中沉积的总能量
- PassingEnergy: 从上到下穿透该层的总能量
- PassingEnergy_Secondary: 穿过当层并向下传播的所有二次粒子的总能量
- HEphotonEnergy: 该层产生，并且离开该层的总次级 xray/gamma 能谱
- NeutronEnergy: 该层产生，并且离开该层的总次级 中子 能谱
说明：PassingEnergy可能小于HEphotonEnergy，因为并不一定所有的次级射线都从下表面离开
#### Spectrum
- ScintillationWavelength: 闪烁体中产生的闪烁光光谱
- CherenkovLightWavelength: 闪烁体中产生的切伦科夫光光谱
- FiberNumericalAperturelength: 从闪烁体进入到光纤数值孔径的光谱
- ScintillationWavelength: 从闪烁体进入到光纤的光谱
#### SourcePosition
- 所抽样的放射源位置分布

## 使用方法
- 在 `include/config.hh` 中配置需要设置的几何体和相应要求
- `make -j8` 编译
- `./LightCollection` GUI界面运行
- `./LightCollection -m **path/to/mac**` 按照宏文件mac运行