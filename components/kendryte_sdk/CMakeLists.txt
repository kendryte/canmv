
if(CONFIG_COMPONENT_KENDRYTE_SDK_ENABLE)
	# xtl options
	add_definitions(-DTCB_SPAN_NO_EXCEPTIONS -DTCB_SPAN_NO_CONTRACT_CHECKING)
	# nncase options
	add_definitions(-DNNCASE_TARGET=k210)
    ################# Add include #################
    list(APPEND ADD_INCLUDE "include"
                            "kendryte-standalone-sdk/lib/bsp/include"
                            "kendryte-standalone-sdk/lib/drivers/include"
                            "kendryte-standalone-sdk/lib/utils/include"
    )

    ############## Add source files ###############
    append_srcs_dir(ADD_SRCS "src")

    append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/bsp")
    append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/drivers")

    if(CONFIG_FREERTOS_ENABLE)
        list(APPEND ADD_INCLUDE "kendryte-standalone-sdk/lib/freertos/include")

        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/freertos")
        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/freertos/portable")

        list(REMOVE_ITEM ADD_SRCS "kendryte-standalone-sdk/lib/freertos/portable/heap_4.c")
        list(APPEND ADD_SRCS "src2/heap_4.c")
        list(REMOVE_ITEM ADD_SRCS "kendryte-standalone-sdk/lib/freertos/locks.c")
        list(APPEND ADD_SRCS "src2/locks.c")
        # asm with freerots support
        set(ADD_ASM_SRCS "kendryte-standalone-sdk/lib/bsp/crt.S"
                         "kendryte-standalone-sdk/lib/freertos/portable/portasm.S"
           )
    else()
        # asm with no freerots support
        set(ADD_ASM_SRCS "src2/crt.S")
        list(APPEND ADD_SRCS "src2/locks.c")
        list(APPEND ADD_SRCS "src2/bsp2.c")
    endif()

    ############ Add nncase #######################
    if(CONFIG_CANMV_ENABLE_NNCASE)
        list(APPEND ADD_INCLUDE "kendryte-standalone-sdk/lib/nncase/include")

        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/nncase")
    endif()

    if(CONFIG_CANMV_ENABLE_KMODEL_V4)
        list(APPEND ADD_INCLUDE "kendryte-standalone-sdk/lib/nncase/v0"
                                "kendryte-standalone-sdk/lib/nncase/v0/include"
                                "kendryte-standalone-sdk/third_party/xtl/include"
        )

        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/nncase/v0/runtime/cpu")
        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/nncase/v0/runtime/k210")
        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/nncase/v0/runtime/neutral")
        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/nncase/v0/runtime")
        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/nncase/v0")

        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/third_party/xtl")
    endif()

    if(CONFIG_CANMV_ENABLE_KMODEL_V5)
        list(APPEND ADD_INCLUDE "kendryte-standalone-sdk/lib/nncase/v1"
                                "kendryte-standalone-sdk/lib/nncase/v1/include"
                                "kendryte-standalone-sdk/third_party/gsl-lite/include"
                                "kendryte-standalone-sdk/third_party/mpark-variant/include"
        )

        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/lib/nncase/v1")
        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/third_party/mpark-variant")
        append_srcs_dir(ADD_SRCS "kendryte-standalone-sdk/third_party/gsl-lite")
    
        list(APPEND ADD_STATIC_LIB "kendryte-standalone-sdk/lib/nncase/v1/lib/libnncase.runtime.a")
        list(APPEND ADD_STATIC_LIB "kendryte-standalone-sdk/lib/nncase/v1/lib/libnncase.rt_modules.k210.a")
    endif()
    ###############################################

    list(APPEND ADD_SRCS ${ADD_ASM_SRCS})
    # list(REMOVE_ITEM COMPONENT_SRCS "src/test.c")
    SET_PROPERTY(SOURCE ${ADD_ASM_SRCS} PROPERTY LANGUAGE C)
    SET_SOURCE_FILES_PROPERTIES(${ADD_ASM_SRCS} PROPERTIES COMPILE_FLAGS "-x assembler-with-cpp -D __riscv64")
    ###############################################

    ###### Add required/dependent components ######
    list(APPEND ADD_REQUIREMENTS gcc m c atomic)
    ###############################################

    ############ Add static libs ##################
    if(CONFIG_MIC_ARRAY_ENABLE)
        list(APPEND ADD_STATIC_LIB "libs/lib_mic.a")
    endif()
    ###############################################

    list(APPEND ADD_DEFINITIONS -DCONFIG_LOG_LEVEL=${CONFIG_SDK_LOG_LEVEL}
                                -DCONFIG_LOG_ENABLE=1
                                -DCONFIG_LOG_COLORS=1
                                -DLOG_KERNEL=1
                                -D__riscv64
                                )

    register_component()
endif()
