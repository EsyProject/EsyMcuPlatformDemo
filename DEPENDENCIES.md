<!--
SPDX-FileCopyrightText: 2026 EsyProject contributors
SPDX-License-Identifier: Apache-2.0
-->

# Dependency revisions

The Git submodule entries pin the following source revisions. Versions below
describe the checked integration, not the newest available upstream releases.

| Dependency | Checked revision | License |
| --- | --- | --- |
| [EsyMcuPlatform](https://github.com/EsyProject/EsyMcuPlatform) | `395bffe5f1a8b732a3564dc17648d2b622dfaed0` (`v0.1.0`) | Apache-2.0 |
| [FreeRTOS-Kernel](https://github.com/raspberrypi/FreeRTOS-Kernel) | `4f7299d6ea746b27a9dd19e87af568e34bd65b15` | MIT |
| [CMSIS_5](https://github.com/ARM-software/CMSIS_5) | `2b7495b8535bdcb306dac29b9ded4cfb679d7e5c` | Apache-2.0 |
| [cmsis-device-f4](https://github.com/STMicroelectronics/cmsis-device-f4) | `5f41fb29d22773896c780052bf61e47fc924d524` | Apache-2.0 |
| [stm32f4xx-hal-driver](https://github.com/STMicroelectronics/stm32f4xx-hal-driver) | `c2e1406d7ea4b73aa42b98ddeb75a8670b1a5a16` | BSD-3-Clause |
| [micro-ecc](https://github.com/kmackay/micro-ecc) | `541b3a78026420a3e369c4c9281c396b5e531113` | BSD-2-Clause |
| [SEGGER_RTT](https://github.com/SEGGERMicro/RTT) | `4d8feab3150f86f37a9d323ddc88d6cdf5673072` | BSD-1-Clause |
| [printf](https://github.com/mpaland/printf) | `d3b984684bb8a8bdc48cc7a1abecb93ce59bbe3e` | MIT |

CMSIS_5 uses tag **5.9.0**, STM32 device support **v2.6.10**, and STM32 HAL
**v1.8.3**. The Raspberry Pi FreeRTOS fork supplies the RP2350 native SMP port.

The Pico example additionally uses an installed **Pico SDK 2.1.1**
(`bddd20f928ce76142793bef434d4f75f4af6e433`), matching **picotool 2.1.1**,
and GNU Arm Embedded **14.2.1** for the recorded builds. Pico SDK and its bundled
components retain their upstream licenses. The native host check used GCC
15.2.0 and OpenSSL 3.5.5; OpenSSL is Apache-2.0.

The STM32 hello logger selects the pinned SEGGER RTT source and a small
allocation-free `printf` provider, keeping its formatting independent of a
newlib heap. Pico hello uses the Pico SDK formatter.

No SDK, driver or kernel source is modified by this demo repository. Check each
dependency's own license and notices for file-specific terms.

The pinned EsyMcuPlatform commit is available from its public repository. It
differs from the earlier checked integration revision only in `CHANGELOG.md`;
the firmware sources and build files are identical. See
[VALIDATION.md](VALIDATION.md) for the recorded checks.
