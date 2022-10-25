

=============================

Files:

* canmv_*.bin: firmware
* elf_*.7z: elf files, just used for debug


=============================

canmv_*.bin: normal firmware, including
    * basic api
    * kmodel V4 support
    * no LVGL support
    * NES emulator support
    * AVI format video support
    * IDE support

canmv_*_minimum.bin: minimum function firmware, including
    * basic api
    * only few OpenMV's API, no some API like find_lines
    * only kmodel V3 support
    * no LVGL support
    * no NES emulator support
    * no AVI format video support
    * no IDE support

canmv_*_minimum_with_kmodel_v4_support: minimum function firmware, including
    * the same as minimum.bin
    * add kmodel v4 support

canmv_*_openmv_kmodel_v4_with_ide_support: minimum function firmware, including
    * the same as normal
    * add kmodel v4 support
    * IDE support

canmv_*_minimum_with_ide_support.bin: minimum function firmware, including
    * same as minimum
    * IDE support

canmv_*_with_lvgl.bin: add lvgl support, including
    * basic api
    * only kmodel V3 support
    * LVGL support
    * NES emulator support
    * AVI format video support
    * IDE support



