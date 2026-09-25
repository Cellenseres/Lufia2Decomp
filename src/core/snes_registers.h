#ifndef LUFIA2_CORE_SNES_REGISTERS_H
#define LUFIA2_CORE_SNES_REGISTERS_H

/* SNES MMIO addresses, official register names. */

#define SNES_INIDISP 0x2100u
#define SNES_MOSAIC 0x2106u
#define SNES_BG1HOFS 0x210du
#define SNES_BG1VOFS 0x210eu
#define SNES_BG2HOFS 0x210fu
#define SNES_BG2VOFS 0x2110u
#define SNES_BG3HOFS 0x2111u
#define SNES_BG3VOFS 0x2112u
#define SNES_VMAIN 0x2115u
#define SNES_VMADDL 0x2116u
#define SNES_VMADDH 0x2117u
#define SNES_M7A 0x211bu
#define SNES_M7B 0x211cu
#define SNES_M7C 0x211du
#define SNES_M7D 0x211eu
#define SNES_M7X 0x211fu
#define SNES_M7Y 0x2120u
#define SNES_CGADD 0x2121u
#define SNES_CGDATA 0x2122u
#define SNES_W12SEL 0x2123u
#define SNES_W34SEL 0x2124u
#define SNES_WOBJSEL 0x2125u
#define SNES_WH0 0x2126u
#define SNES_WH1 0x2127u
#define SNES_WH2 0x2128u
#define SNES_WH3 0x2129u
#define SNES_WBGLOG 0x212au
#define SNES_WOBJLOG 0x212bu
#define SNES_TM 0x212cu
#define SNES_TS 0x212du
#define SNES_TMW 0x212eu
#define SNES_TSW 0x212fu
#define SNES_CGWSEL 0x2130u
#define SNES_CGADSUB 0x2131u
#define SNES_COLDATA 0x2132u
#define SNES_MPYL 0x2134u
#define SNES_MPYM 0x2135u
#define SNES_MPYH 0x2136u
#define SNES_WMDATA 0x2180u
#define SNES_WMADDL 0x2181u
#define SNES_WMADDM 0x2182u
#define SNES_WMADDH 0x2183u
#define SNES_NMITIMEN 0x4200u
#define SNES_WRMPYA 0x4202u
#define SNES_WRMPYB 0x4203u
#define SNES_WRDIVL 0x4204u
#define SNES_WRDIVH 0x4205u
#define SNES_WRDIVB 0x4206u
#define SNES_MDMAEN 0x420bu
#define SNES_HDMAEN 0x420cu
#define SNES_RDDIVL 0x4214u
#define SNES_RDDIVH 0x4215u
#define SNES_RDMPYL 0x4216u
#define SNES_RDMPYH 0x4217u

/* DMA channel n registers. */
#define SNES_DMAP(n) (0x4300u + ((unsigned)(n) << 4))
#define SNES_BBAD(n) (0x4301u + ((unsigned)(n) << 4))
#define SNES_A1TL(n) (0x4302u + ((unsigned)(n) << 4))
#define SNES_A1TH(n) (0x4303u + ((unsigned)(n) << 4))
#define SNES_A1B(n) (0x4304u + ((unsigned)(n) << 4))
#define SNES_DASL(n) (0x4305u + ((unsigned)(n) << 4))
#define SNES_DASH(n) (0x4306u + ((unsigned)(n) << 4))
#define SNES_DASB(n) (0x4307u + ((unsigned)(n) << 4))

#endif
