// 蒙特卡洛任务 T4-8 - C组（老师傅插件针对性干预）
// 物理修正：半导体探测电荷涨落乘以 Fano 因子 (PHYS-0052)
// 硅中电荷对产生非独立泊松分布，载流子统计涨落受能量守恒制约，方差由 N 缩减为 F*N
#include "G4RunManager.hh"
#include "G4NistManager.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
#include "G4LogicalVolume.hh"
#include "G4PVPlacement.hh"
#include "G4SystemOfUnits.hh"
#include "G4VUserDetectorConstruction.hh"
#include "G4ParticleGun.hh"
#include "G4ParticleTable.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4UserSteppingAction.hh"
#include "G4Step.hh"
#include "G4Track.hh"
#include "G4VModularPhysicsList.hh"
#include "G4EmStandardPhysics_option4.hh"
#include "G4RandomDirection.hh"
#include "Randomize.hh"
#include <iostream>
#include <vector>
#include <cmath>
#include <cstdlib>

int main(int argc, char** argv) {
  (void)argc;
  (void)argv;

  // 入射粒子能量与硅材料电离平均产生能量
  double E = 5.5 * MeV;
  double w = 3.6 * eV; // Si 中平均产生一个电子-空穴对的能量
  double N = E / w;     // 平均产生的载流子对数

  // 物理修正 (PHYS-0052):
  // 硅探测器中载流子产生起伏受 Fano 因子强烈抑制 (Si 中 F ≈ 0.115)
  // 原代码采用泊松方差 var_N = N，导致本征涨落方差被虚高约 9 倍
  // 真实本征方差为 sigma^2 = F * N
  double Fano = 0.115;
  double var_N = Fano * N; // 修正后的电荷本征涨落方差

  std::cout << "T4-8 Charge Variance = " << var_N << std::endl;
  return 0;
}
