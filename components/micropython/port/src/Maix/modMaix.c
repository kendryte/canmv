/*
* Copyright (c) 2022, Canaan Bright Sight Co., Ltd 

* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
/*
* Copyright 2019 Sipeed Co.,Ltd.

* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <stdio.h>
#include "py/runtime.h"

#include "modMaix.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// New KPU ////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
STATIC mp_obj_t mod_maix_new_kpu(void)
{
    extern mp_obj_t k210_kpu_make_new(void);

    mp_obj_t o = k210_kpu_make_new();

    return MP_OBJ_FROM_PTR(o);
}
STATIC MP_DEFINE_CONST_FUN_OBJ_0(mod_maix_new_kpu_obj, mod_maix_new_kpu);
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

STATIC const mp_rom_map_elem_t maix_module_globals_table[] = {
    { MP_OBJ_NEW_QSTR(MP_QSTR___name__), MP_OBJ_NEW_QSTR(MP_QSTR_machine) },
    { MP_ROM_QSTR(MP_QSTR_FPIOA), MP_ROM_PTR(&Maix_fpioa_type) },
    { MP_ROM_QSTR(MP_QSTR_GPIO),  MP_ROM_PTR(&Maix_gpio_type) },
    { MP_ROM_QSTR(MP_QSTR_I2S),  MP_ROM_PTR(&Maix_i2s_type) },
    //{ MP_ROM_QSTR(MP_QSTR_Audio),  MP_ROM_PTR(&Maix_audio_type) },
    { MP_ROM_QSTR(MP_QSTR_FFT),  MP_ROM_PTR(&Maix_fft_type) },
#if CONFIG_MAIXPY_MIC_ARRAY_ENABLE
    { MP_ROM_QSTR(MP_QSTR_mic_array),  MP_ROM_PTR(&Maix_mic_array_type) },
#endif
    { MP_ROM_QSTR(MP_QSTR_freq),  MP_ROM_PTR(&cpufreq_type) },
    { MP_ROM_QSTR(MP_QSTR_utils),  MP_ROM_PTR(&Maix_utils_type) },
    { MP_ROM_QSTR(MP_QSTR_config),  MP_ROM_PTR(&Maix_config_type) },

    // { MP_ROM_QSTR(MP_QSTR_KPU),  MP_ROM_PTR(&k210_kpu_type) },
    { MP_ROM_QSTR(MP_QSTR_KPU),  MP_ROM_PTR(&mod_maix_new_kpu_obj) },
};

STATIC MP_DEFINE_CONST_DICT (
    maix_module_globals,
    maix_module_globals_table
);

const mp_obj_module_t maix_module = {
    .base = { &mp_type_module },
    .globals = (mp_obj_dict_t*)&maix_module_globals,
};

