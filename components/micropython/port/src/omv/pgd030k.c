/* Copyright 2018 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS},
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <stdio.h>

#include "pgd030k.h"

#include "dvp.h"
#include "plic.h"
#include "printf.h"
#include "sleep.h"

#include "cambus.h"
#include "sensor.h"

#include "mphalport.h"
#include "py/runtime.h"

/**
 * \brief Struct to store an register address and its value.
 */
struct Register {
    uint16_t address; /**< struct Register address. */
    uint16_t value; /**< struct Register value. */
};

static const struct Register pgd030k_regs_init_seq[] = {
    // ###############################################################################
    // # TG Timing Setting
    // ###############################################################################

    // # Frame structure setting
    { 0x03, 0x00 }, // bank A
    { 0x06, 0x03 }, // framewidth_h
    { 0x07, 0x59 }, // framewidth_l
    { 0x08, 0x02 }, // fheight_a_h
    { 0x09, 0x0C }, // fheight_a_l
    { 0x03, 0x01 }, // bank B
    { 0x04, 0x80 }, // bayer_control_01
    { 0x1B, 0x00 }, // dmyfwa_h
    { 0x1C, 0x00 }, // dmyfwa_l
    { 0x1D, 0x00 }, // dmyfwb_h
    { 0x1E, 0x00 }, // dmyfwb_l
    { 0x1F, 0x00 }, // retrace_h
    { 0x20, 0x00 }, // retrace_l
    { 0x24, 0x00 }, // tgvstart_h
    { 0x25, 0x08 }, // tgvstart_l
    { 0x26, 0x01 }, // tgvstop_h
    { 0x27, 0xEC }, // tgvstop_l
    { 0x2C, 0x00 }, // blvstart_0_h
    { 0x2D, 0x02 }, // blvstart_0_l
    { 0x2E, 0x00 }, // blvstop_0_h
    { 0x2F, 0x08 }, // blvstop_0_l
    { 0x03, 0x02 }, // bank C
    { 0x48, 0x0F }, // illuminance_h
    { 0x49, 0xFF }, // illuminance_l

    // # mirror setting
    { 0x03, 0x00 }, // bank A
    { 0x05, 0x00 }, // mirror
    { 0x03, 0x01 }, // bank
    { 0x0E, 0x01 }, // bayer_control_11
    { 0x0F, 0x00 }, // bayer_control_12

    // ###############################################################################
    // # Format Timing Setting
    // ###############################################################################

    // # Window setting
    { 0x03, 0x00 }, // bank
    { 0x0C, 0x00 }, // windowx1_h
    { 0x0D, 0x01 }, // windowx1_l
    { 0x10, 0x01 }, // windowx2_h
    { 0x11, 0x41 }, // windowx2_l
    { 0x0E, 0x00 }, // windowy1_h
    { 0x0F, 0x02 }, // windowy1_l
    { 0x12, 0x01 }, // windowy2_h
    { 0x13, 0xE2 }, // windowy2_l

    // # Format fifo setting
    { 0x03, 0x00 }, // bank
    { 0xAC, 0x01 }, // rd_bit_sel
    { 0x97, 0x02 }, // rd_cnt_init_h
    { 0x98, 0x50 }, // rd_cnt_init_l
    { 0x99, 0x00 }, // rd_x_start0_h
    { 0x9A, 0x30 }, // rd_x_start0_l
    { 0x9B, 0x02 }, // rd_x_stop0_h
    { 0x9C, 0xB0 }, // rd_x_stop0_l
    { 0x9D, 0x00 }, // rd_y_start_h
    { 0x9E, 0x0B }, // rd_y_start_l
    { 0x9F, 0x01 }, // rd_y_stop_h
    { 0xA0, 0xEB }, // rd_y_stop_l

    // # vsync setting
    { 0x03, 0x00 }, // bank
    { 0x74, 0x00 }, // vsyncstartrow0_h
    { 0x75, 0x0B }, // vsyncstartrow0_l
    { 0x76, 0x01 }, // vsyncstoprow0_h
    { 0x77, 0xEB }, // vsyncstoprow0_l

    // ###############################################################################
    // # TG Data Setting
    // ###############################################################################

    // # ADC setting
    { 0x03, 0x01 }, // bank
    { 0x8B, 0x06 }, // dat_max_h
    { 0x8C, 0xCC }, // dat_max_l
    { 0x8D, 0x01 }, // dat_ofst_h
    { 0x8E, 0x00 }, // dat_ofst_l
    { 0x8F, 0x01 }, // max_robp_h
    { 0x90, 0xCD }, // max_robp_l

    // # DBLC setting
    { 0x03, 0x01 }, // bank
    { 0x05, 0xBE }, // bayer_control_02

    // # ABLC setting
    { 0x03, 0x01 }, // bank
    { 0x06, 0x80 }, // bayer_control_03
    { 0x07, 0xE0 }, // bayer_control_04
    { 0xDB, 0x01 }, // blc_top_th_h
    { 0xDC, 0xB0 }, // blc_top_th_l
    { 0xDD, 0x01 }, // blc_bot_th_h
    { 0xDE, 0x40 }, // blc_bot_th_l

    // # Column inttime setting
    { 0x03, 0x01 }, // bank
    { 0x0D, 0x86 }, // bayer_control_10
    { 0x03, 0x03 }, // bank
    { 0x4A, 0x0E }, // lineth
    { 0x4B, 0xFF }, // expcorr
    { 0x4D, 0x80 }, // prestop

    // # Test Pattern setting
    // W0301 # bank
    // W0D86 # bayer_control_10 [4:3]tp_seq 2'b00
    // W931A # tp_control_0
    // W9403 # tp_control_1_h
    // W95FF # tp_control_1_l
    // W9603 # tp_control_2_h
    // W97FF # tp_control_2_l
    // W9803 # tp_control_3_h
    // W99FF # tp_control_3_l
    // W9A03 # tp_control_4_h
    // W9BFF # tp_control_4_l
    // W9C02 # hact_width_h
    // W9D84 # hact_width_l
    // W9E03 # Yos_init_h
    // W9F0E # Yos_init_l
    // WA002 # Xos_init_h
    // WA1BA # Xos_init_l

    // ###############################################################################
    // # Format Data Setting
    // ###############################################################################

    // # Data max/min setting
    { 0x03, 0x00 }, // bank
    { 0xB2, 0x02 }, // sync_control_1 //0x22 HSync_all_line
    { 0xB4, 0x00 }, // data_min_h
    { 0xB5, 0x04 }, // data_min_l
    { 0xB6, 0x03 }, // data_max_h
    { 0xB7, 0xF8 }, // data_max_l

    // ###############################################################################
    // # MIPI Setting
    // ###############################################################################

    // //# MIPI setting
    // {0x03,0x05}, // bank
    // {0x04,0x30}, // mipi_control_0
    // {0x05,0x9B}, // mipi_control_1
    // {0x06,0x99}, // mipi_control_2
    // {0x07,0x99}, // mipi_control_3
    // {0x08,0x01}, // mipi_control_4

    // //# MIPI packet setting
    // //# 640/4*5=800
    // {0x36,0x03}, // mipi_pkt_size0_h
    // {0x37,0x20}, // mipi_pkt_size0_l

    // //# MIPI timing setting
    // {0x1C,0x00}, // mipi_T_lpx
    // {0x1D,0x00}, // mipi_T_clk_prepare
    // {0x1E,0x00}, // mipi_T_hs_prepare
    // {0x1F,0x00}, // mipi_T_hs_zero
    // {0x20,0x00}, // mipi_T_hs_trail
    // {0x21,0x00}, // mipi_T_clk_zero
    // {0x22,0x00}, // mipi_T_clk_trail
    // {0x23,0x00}, // mipi_T_clk_pre
    // {0x24,0x00}, // mipi_T_clk_post
    // {0x25,0x00}, // mipi_T_wakeup
    // {0x26,0x00}, // mipi_T_hsexit
    // {0x27,0x00}, // mipi_T_clk_hsexit

    // //# MIPI datatype setting
    // {0x42,0x00}, // mipi_data_id0

    // ###############################################################################
    // # DVP Setting
    // ###############################################################################

    // # PAD drivability setting
    { 0x03, 0x00 }, // bank A
    { 0x24, 0xF0 }, // pad_control2

    // # PAD setting
    { 0x03, 0x00 }, // bank
    { 0x25, 0x98 }, // pad_control3
    { 0x26, 0x0F }, // pad_control4
    { 0x27, 0xF0 }, // pad_control5
    { 0x28, 0x00 }, // pad_control6

    // ###############################################################################
    // # Clock Setting
    // ###############################################################################

    // # pll bandwidth setting
    { 0x03, 0x00 }, // bank
    { 0x4E, 0x7A }, // pll_control2
    { 0x4F, 0x7E }, // pll_control3

    // # pll & clock divider setting
    { 0x03, 0x00 }, // bank
    { 0x1A, 0xF0 }, // i2c_control_1
    { 0x51, 0x20 }, // pll_tg_n_cnt
    { 0x52, 0x02 }, // pll_tg_r_cnt
    { 0x53, 0x20 }, // pll_mp_n_cnt
    { 0x54, 0x02 }, // pll_mp_r_cnt
    { 0x56, 0x12 }, // clkdiv1
    { 0x57, 0x10 }, // clkdiv2
    { 0x58, 0x80 }, // clkdiv3
    { 0x1A, 0x50 }, // i2c_control_1

    // # PLL power & bypass setting
    { 0x4E, 0x52 }, // pll_control2
    //$0200 //	wait for 200 us;
    // {0xffff,0x01},	//for dothinkey delay
    { 0xFFFF, 10 }, // delay 1ms

    { 0x4E, 0x42 }, // pll_control2

    /////! PSD030K_ANALOG_230303 (PSD030K_Analog_global_640x480_DVP_Bayer_240fps_230303.ccf)

    // 0x03,0x00, W0220 W0223 W0203 W0203

    // ## Analog Setting

    //------------------------------------ for pd to xcp sequency
    { 0x03, 0x00 }, // A bank
    { 0x3E, 0x7F }, // xcp & cp pd disable

    { 0x03, 0x01 }, // B bank
    { 0x0B, 0x54 }, // xcp_clk_en[4]=1

    { 0x03, 0x00 }, // A bank
    { 0x3E, 0x03 }, // xcp & cp pd disable
    //------------------------------------

    { 0x03, 0x00 }, // A bank
    { 0x38, 0x27 }, // Pixelbias1 0111
    { 0x3B, 0x67 }, // Ramprange 1.35V --> 1.40625V
    { 0x3D, 0x65 }, // S2(VDD/NCP_L), S1(VDD/NCP_L) ==>S2 NCP
    { 0x3F, 0x8A }, // NCP_T -0.9 --> -1.1V
    { 0x40, 0x81 }, // NCP_LS -0.7-->-0.2V
    { 0x41, 0x88 }, // PCP_RT 3.3V-->3.5V
    { 0x42, 0x82 }, // PCP_L 4.3V-->3.0V
    { 0x43, 0x07 }, // VPC 0.7V

    { 0x03, 0x01 }, // B bank
    { 0x7A, 0x10 }, // globalgain x2(global shutter x1)

    { 0x03, 0x02 }, // C bank
    { 0xC7, 0x10 }, // blacksun[6:0] 60h-->20h-->10h
    { 0xC8, 0x20 }, // blacksun x4 20h

    { 0x03, 0x03 }, // D bank
    { 0x4D, 0x00 }, // prestop 80h-->31h(top h-line remove@x8)

    // ## Timing Setting

    { 0x03, 0x02 }, // C bank

    { 0x13, 0x00 }, // row_ls_en_start_h (1)
    { 0x14, 0x01 }, // row_ls_en_start_l
    { 0x15, 0x03 }, // row_ls_en_stop_h (856)
    { 0x16, 0x58 }, // row_ls_en_stop_l

    { 0x1F, 0x00 }, // row_r_ls_rst_start_h (20)
    { 0x20, 0x14 }, // row_r_ls_rst_start_l
    { 0x21, 0x00 }, // row_r_ls_rst_stop_h (20)
    { 0x22, 0x14 }, // row_r_ls_rst_stop_l

    { 0x34, 0x00 }, // row_r_rs_tx_start_h (0)
    { 0x35, 0x00 }, // row_r_rs_tx_start_l
    { 0x36, 0x00 }, // row_r_rs_tx_stop_h (0)
    { 0x37, 0x00 }, // row_r_rs_tx_stop_l

    { 0x38, 0x01 }, // row_r_ls_tx_start_h (350)
    { 0x39, 0x5E }, // row_r_ls_tx_start_l
    { 0x3A, 0x01 }, // row_r_ls_tx_stop_h (350)
    { 0x3B, 0x5E }, // row_r_ls_tx_stop_l

    { 0x1B, 0x00 }, // row_g_pr_rst_stop_rcnt_h (0)
    { 0x1C, 0x00 }, // row_g_pr_rst_stop_rcnt_l
    { 0x17, 0x00 }, // row_g_pr_rst_start_rcnt_h (0)
    { 0x18, 0x00 }, // row_g_pr_rst_start_rcnt_l

    { 0x2F, 0x02 }, // row_g_pr_tx_stop1_rcnt_h (512)
    { 0x30, 0x00 }, // row_g_pr_tx_stop1_rcnt_l
    { 0x2B, 0x00 }, // row_g_pr_tx_start1_rcnt_h (0)
    { 0x2C, 0x00 }, // row_g_pr_tx_start1_rcnt_l

    { 0x27, 0x00 }, // row_g_pr_tx_stop0_rcnt_h (0)
    { 0x28, 0x00 }, // row_g_pr_tx_stop0_rcnt_l
    { 0x23, 0x00 }, // row_g_pr_tx_start0_rcnt_h (0)
    { 0x24, 0x00 }, // row_g_pr_tx_start0_rcnt_l

    { 0x03, 0x01 }, // B bank

    { 0x6C, 0x01 }, // inttime_h (511)
    { 0x6D, 0xFF }, // inttime_m

    { 0x03, 0x02 }, // C bank

    { 0x40, 0x00 }, // row_g_pr_s2_stop_rcnt_h (0)
    { 0x41, 0x00 }, // row_g_pr_s2_stop_rcnt_l
    { 0x3C, 0x00 }, // row_g_pr_s2_start_rcnt_h (0)
    { 0x3D, 0x00 }, // row_g_pr_s2_start_rcnt_l

    { 0x5C, 0x00 }, // row_g_pr_s1_stop_rcnt_h (0)
    { 0x5D, 0x00 }, // row_g_pr_s1_stop_rcnt_l
    { 0x58, 0x00 }, // row_g_pr_s1_start_rcnt_h (0)
    { 0x59, 0x00 }, // row_g_pr_s1_start_rcnt_l

    { 0x6C, 0x00 }, // row_g_pr_vpc_stop1_rcnt_h (0)
    { 0x6D, 0x00 }, // row_g_pr_vpc_stop1_rcnt_l
    { 0x68, 0x00 }, // row_g_pr_vpc_start1_rcnt_h (0)
    { 0x69, 0x00 }, // row_g_pr_vpc_start1_rcnt_l

    { 0x64, 0x00 }, // row_g_pr_vpc_stop0_rcnt_h (0)
    { 0x65, 0x00 }, // row_g_pr_vpc_stop0_rcnt_l
    { 0x60, 0x00 }, // row_g_pr_vpc_start0_rcnt_h (0)
    { 0x61, 0x00 }, // row_g_pr_vpc_start0_rcnt_l

    { 0x19, 0x00 }, // row_g_pr_rst_start_ccnt_h (250)
    { 0x1A, 0xFA }, // row_g_pr_rst_start_ccnt_l
    { 0x1D, 0x03 }, // row_g_pr_rst_stop_ccnt_h (800)
    { 0x1E, 0x20 }, // row_g_pr_rst_stop_ccnt_l

    { 0x29, 0x01 }, // row_g_pr_tx_stop0_ccnt_h (380)
    { 0x2A, 0x7C }, // row_g_pr_tx_stop0_ccnt_l
    { 0x2D, 0x01 }, // row_g_pr_tx_start1_ccnt_h (430)
    { 0x2E, 0xAE }, // row_g_pr_tx_start1_ccnt_l
    { 0x31, 0x03 }, // row_g_pr_tx_stop1_ccnt_h (831)
    { 0x32, 0x3F }, // row_g_pr_tx_stop1_ccnt_l

    { 0x3E, 0x01 }, // row_g_pr_s2_start_ccnt_h (270)
    { 0x3F, 0x0E }, // row_g_pr_s2_start_ccnt_l
    { 0x42, 0x01 }, // row_g_pr_s2_stop_ccnt_h (368)
    { 0x43, 0x70 }, // row_g_pr_s2_stop_ccnt_l

    { 0x5A, 0x01 }, // row_g_pr_s1_start_ccnt_h (270)
    { 0x5B, 0x0E }, // row_g_pr_s1_start_ccnt_l
    { 0x5E, 0x03 }, // row_g_pr_s1_stop_ccnt_h (785)
    { 0x5F, 0x11 }, // row_g_pr_s1_stop_ccnt_l

    { 0x6A, 0x01 }, // row_g_pr_vpc_start1_ccnt_h (280)
    { 0x6B, 0x18 }, // row_g_pr_vpc_start1_ccnt_l
    { 0x6E, 0x01 }, // row_g_pr_vpc_stop1_ccnt_h (348)
    { 0x6F, 0x5C }, // row_g_pr_vpc_stop1_ccnt_l

    { 0x62, 0x01 }, // row_g_pr_vpc_start0_ccnt_h (388)
    { 0x63, 0x84 }, // row_g_pr_vpc_start0_ccnt_l
    { 0x66, 0x01 }, // row_g_pr_vpc_stop0_ccnt_h (445)
    { 0x67, 0xBD }, // row_g_pr_vpc_stop0_ccnt_l

    { 0x48, 0x02 }, // row_s2_pcp_sel_stop_rcnt_h (600)
    { 0x49, 0x58 }, // row_s2_pcp_sel_stop_rcnt_l
    { 0x44, 0x00 }, // row_s2_pcp_sel_start_rcnt_h (0)
    { 0x45, 0x00 }, // row_s2_pcp_sel_start_rcnt_l

    { 0x46, 0x00 }, // row_s2_pcp_sel_start_ccnt_h (1)
    { 0x47, 0x01 }, // row_s2_pcp_sel_start_ccnt_l
    { 0x4A, 0x03 }, // row_s2_pcp_sel_stop_ccnt_h (802)
    { 0x4B, 0x22 }, // row_s2_pcp_sel_stop_ccnt_l

    { 0x50, 0x02 }, // row_s2_ncp_sel_stop_rcnt_h (600)
    { 0x51, 0x58 }, // row_s2_ncp_sel_stop_rcnt_l
    { 0x4C, 0x00 }, // row_s2_ncp_sel_start_rcnt_h (0)
    { 0x4D, 0x00 }, // row_s2_ncp_sel_start_rcnt_l

    { 0x4E, 0x00 }, // row_s2_ncp_sel_start_ccnt_h (1)
    { 0x4F, 0x01 }, // row_s2_ncp_sel_start_ccnt_l
    { 0x52, 0x03 }, // row_s2_ncp_sel_stop_ccnt_h (802)
    { 0x53, 0x22 }, // row_s2_ncp_sel_stop_ccnt_l

    { 0x54, 0x01 }, // row_r_ls_s2_start_h (280)
    { 0x55, 0x18 }, // row_r_ls_s2_start_l
    { 0x56, 0x03 }, // row_r_ls_s2_stop_h (780)
    { 0x57, 0x0C }, // row_r_ls_s2_stop_l

    { 0x05, 0x03 }, // row_flush_start_start_h (855)
    { 0x06, 0x57 }, // row_flush_start_start_l
    { 0x07, 0x03 }, // row_flush_start_stop_h (855)
    { 0x08, 0x57 }, // row_flush_start_stop_l

    { 0x09, 0x00 }, // row_flush_stop_start_h (21)
    { 0x0A, 0x15 }, // row_flush_stop_start_l
    { 0x0B, 0x00 }, // row_flush_stop_stop_h (21)
    { 0x0C, 0x15 }, // row_flush_stop_stop_l

    { 0x0D, 0x03 }, // row_latch_rst_start_h (855)
    { 0x0E, 0x57 }, // row_latch_rst_start_l
    { 0x0F, 0x03 }, // row_latch_rst_stop_h (855)
    { 0x10, 0x57 }, // row_latch_rst_stop_l

    { 0x88, 0x01 }, // cds_bls_en_start_h (300)
    { 0x89, 0x2C }, // cds_bls_en_start_l

    { 0x80, 0x00 }, // cds_store1_start_h (10)
    { 0x81, 0x0A }, // cds_store1_start_l
    { 0x82, 0x00 }, // cds_store1_stop_h (80)
    { 0x83, 0x50 }, // cds_store1_stop_l

    { 0x84, 0x00 }, // cds_store2_start_h (10)
    { 0x85, 0x0A }, // cds_store2_start_l
    { 0x86, 0x00 }, // cds_store2_stop_h (110)
    { 0x87, 0x6E }, // cds_store2_stop_l

    { 0x8E, 0x00 }, // cds_pxl_track_start1_h (0)
    { 0x8F, 0x00 }, // cds_pxl_track_start1_l
    { 0x90, 0x03 }, // cds_pxl_track_stop1_h (857)
    { 0x91, 0x59 }, // cds_pxl_track_stop1_l
    { 0x92, 0x03 }, // cds_pxl_track_start2_h (857)
    { 0x93, 0x59 }, // cds_pxl_track_start2_l
    { 0x94, 0x03 }, // cds_pxl_track_stop2_h (857)
    { 0x95, 0x59 }, // cds_pxl_track_stop2_l

    { 0x9A, 0x00 }, // cds_amp1_rst_start1_h (155)
    { 0x9B, 0x9B }, // cds_amp1_rst_start1_l
    { 0x9C, 0x00 }, // cds_amp1_rst_stop1_h (185)
    { 0x9D, 0xB9 }, // cds_amp1_rst_stop1_l
    { 0x9E, 0x01 }, // cds_amp1_rst_start2_h (330)
    { 0x9F, 0x4A }, // cds_amp1_rst_start2_l
    { 0xA0, 0x01 }, // cds_amp1_rst_stop2_h (360)
    { 0xA1, 0x68 }, // cds_amp1_rst_stop2_l

    { 0xA2, 0x00 }, // cds_amp2_clamp_en_start1_h (155)
    { 0xA3, 0x9B }, // cds_amp2_clamp_en_start1_l
    { 0xA4, 0x03 }, // cds_amp2_clamp_en_stop1_h (857)
    { 0xA5, 0x59 }, // cds_amp2_clamp_en_stop1_l
    { 0xA6, 0x03 }, // cds_amp2_clamp_en_start2_h (857)
    { 0xA7, 0x59 }, // cds_amp2_clamp_en_start2_l
    { 0xA8, 0x03 }, // cds_amp2_clamp_en_stop2_h (857)
    { 0xA9, 0x59 }, // cds_amp2_clamp_en_stop2_l

    { 0xAA, 0x03 }, // cds_amp2_rst_start1_h (857)
    { 0xAB, 0x59 }, // cds_amp2_rst_start1_l
    { 0xAC, 0x03 }, // cds_amp2_rst_stop1_h (857)
    { 0xAD, 0x59 }, // cds_amp2_rst_stop1_l
    { 0xAE, 0x03 }, // cds_amp2_rst_start2_h (857)
    { 0xAF, 0x59 }, // cds_amp2_rst_start2_l
    { 0xB0, 0x03 }, // cds_amp2_rst_stop2_h (857)
    { 0xB1, 0x59 }, // cds_amp2_rst_stop2_l

    { 0xB2, 0x00 }, // cds_sf_rst_start1_h (157)
    { 0xB3, 0x9D }, // cds_sf_rst_start1_l
    { 0xB4, 0x00 }, // cds_sf_rst_stop1_h (187)
    { 0xB5, 0xBB }, // cds_sf_rst_stop1_l
    { 0xB6, 0x01 }, // cds_sf_rst_start2_h (332)
    { 0xB7, 0x4C }, // cds_sf_rst_start2_l
    { 0xB8, 0x01 }, // cds_sf_rst_stop2_h (362)
    { 0xB9, 0x6A }, // cds_sf_rst_stop2_l

    { 0x03, 0x03 }, // D bank

    { 0x06, 0x01 }, // ramp_ablc_sw_en_start1_h (280)
    { 0x07, 0x18 }, // ramp_ablc_sw_en_start1_l
    { 0x08, 0x01 }, // ramp_ablc_sw_en_stop1_h (282)
    { 0x09, 0x1A }, // ramp_ablc_sw_en_stop1_l

    { 0x0A, 0x03 }, // ramp_ablc_sw_en_start2_h (805)
    { 0x0B, 0x25 }, // ramp_ablc_sw_en_start2_l
    { 0x0C, 0x03 }, // ramp_ablc_sw_en_stop2_h (807)
    { 0x0D, 0x27 }, // ramp_ablc_sw_en_stop2_l

    { 0x16, 0x00 }, // ramp_state_lsb_start1_h (115)
    { 0x17, 0x73 }, // ramp_state_lsb_start1_l
    { 0x18, 0x00 }, // ramp_state_lsb_stop1_h (117)
    { 0x19, 0x75 }, // ramp_state_lsb_stop1_l

    { 0x1A, 0x01 }, // ramp_state_lsb_start2_h (280)
    { 0x1B, 0x18 }, // ramp_state_lsb_start2_l
    { 0x1C, 0x01 }, // ramp_state_lsb_stop2_h (282)
    { 0x1D, 0x1A }, // ramp_state_lsb_stop2_l

    { 0x1E, 0x03 }, // ramp_state_lsb_start3_h (805)
    { 0x1F, 0x25 }, // ramp_state_lsb_start3_l
    { 0x20, 0x03 }, // ramp_state_lsb_stop3_h (807)
    { 0x21, 0x27 }, // ramp_state_lsb_stop3_l

    { 0x12, 0x00 }, // ramp_state_msb_start_h (118)
    { 0x13, 0x76 }, // ramp_state_msb_start_l
    { 0x14, 0x00 }, // ramp_state_msb_stop_h (118)
    { 0x15, 0x76 }, // ramp_state_msb_stop_l

    { 0x32, 0x00 }, // ramp_clk_start1_h (118)
    { 0x33, 0x76 }, // ramp_clk_start1_l
    { 0x24, 0x00 }, // ramp_clk_stop1_y0_h (122)
    { 0x25, 0x7A }, // ramp_clk_stop1_y0_l

    { 0x34, 0x00 }, // ramp_clk_start2_h (195)
    { 0x35, 0xC3 }, // ramp_clk_start2_l
    { 0x36, 0x01 }, // ramp_clk_stop2_h (300)
    { 0x37, 0x2C }, // ramp_clk_stop2_l

    { 0x38, 0x01 }, // ramp_clk_start3_h (405)
    { 0x39, 0x95 }, // ramp_clk_start3_l
    { 0x3A, 0x03 }, // ramp_clk_stop3_h (777)
    { 0x3B, 0x09 }, // ramp_clk_stop3_l

    { 0x3C, 0x03 }, // ramp_clk_start4_h (807)
    { 0x3D, 0x27 }, // ramp_clk_start4_l
    { 0x2C, 0x03 }, // ramp_clk_stop4_y0_h (837)
    { 0x2D, 0x45 }, // ramp_clk_stop4_y0_l

    { 0x03, 0x03 }, // D bank

    { 0x0E, 0x03 }, // ramp_ref_sample_start_h (810)
    { 0x0F, 0x2A }, // ramp_ref_sample_start_l
    { 0x10, 0x03 }, // ramp_ref_sample_stop_h (857)
    { 0x11, 0x59 }, // ramp_ref_sample_stop_l

    { 0x3E, 0x03 }, // ramp_dvdd_sample_start_h (810)
    { 0x3F, 0x2A }, // ramp_dvdd_sample_start_l
    { 0x40, 0x03 }, // ramp_dvdd_sample_stop_h (857)
    { 0x41, 0x59 }, // ramp_dvdd_sample_stop_l

    { 0x42, 0x03 }, // ramp_ablc_ref_sample_start_h (810)
    { 0x43, 0x2A }, // ramp_ablc_ref_sample_start_l
    { 0x44, 0x03 }, // ramp_ablc_ref_sample_stop_h (857)
    { 0x45, 0x59 }, // ramp_ablc_ref_sample_stop_l

    { 0x03, 0x02 }, // C bank

    { 0xCA, 0x00 }, // adc_clk_en_start1_h (195)
    { 0xCB, 0xC3 }, // adc_clk_en_start1_l
    { 0xCC, 0x01 }, // adc_clk_en_stop1_h (275)
    { 0xCD, 0x13 }, // adc_clk_en_stop1_l
    { 0xCE, 0x01 }, // adc_clk_en_start2_h (405)
    { 0xCF, 0x95 }, // adc_clk_en_start2_l
    { 0xD0, 0x03 }, // adc_clk_en_stop2_h (777)
    { 0xD1, 0x09 }, // adc_clk_en_stop2_l

    { 0xD2, 0x01 }, // cntr_pol_start_h (280)
    { 0xD3, 0x18 }, // cntr_pol_start_l
    { 0xD4, 0x03 }, // cntr_pol_stop_h (805)
    { 0xD5, 0x25 }, // cntr_pol_stop_l

    { 0xDA, 0x00 }, // cntr_load_en_start1_h (191)
    { 0xDB, 0xBF }, // cntr_load_en_start1_l
    { 0xDC, 0x01 }, // cntr_load_en_stop1_h (279)
    { 0xDD, 0x17 }, // cntr_load_en_stop1_l
    { 0xDE, 0x01 }, // cntr_load_en_start2_h (401)
    { 0xDF, 0x91 }, // cntr_load_en_start2_l
    { 0xE0, 0x03 }, // cntr_load_en_stop2_h (781)
    { 0xE1, 0x0D }, // cntr_load_en_stop2_l

    { 0xE2, 0x00 }, // cntr_latch_en1_start1_h (193)
    { 0xE3, 0xC1 }, // cntr_latch_en1_start1_l
    { 0xE4, 0x01 }, // cntr_latch_en1_stop1_h (277)
    { 0xE5, 0x15 }, // cntr_latch_en1_stop1_l
    { 0xE6, 0x01 }, // cntr_latch_en1_start2_h (403)
    { 0xE7, 0x93 }, // cntr_latch_en1_start2_l
    { 0xE8, 0x03 }, // cntr_latch_en1_stop2_h (779)
    { 0xE9, 0x0B }, // cntr_latch_en1_stop2_l

    { 0xEA, 0x00 }, // cntr_latch_en2_start1_h (193)
    { 0xEB, 0xC1 }, // cntr_latch_en2_start1_l
    { 0xEC, 0x01 }, // cntr_latch_en2_stop1_h (277)
    { 0xED, 0x15 }, // cntr_latch_en2_stop1_l
    { 0xEE, 0x01 }, // cntr_latch_en2_start2_h (403)
    { 0xEF, 0x93 }, // cntr_latch_en2_start2_l
    { 0xF0, 0x03 }, // cntr_latch_en2_stop2_h (779)
    { 0xF1, 0x0B }, // cntr_latch_en2_stop2_l

    { 0xD6, 0x03 }, // cntr_rstb_start_h (855)
    { 0xD7, 0x57 }, // cntr_rstb_start_l
    { 0xD8, 0x03 }, // cntr_rstb_stop_h (857)
    { 0xD9, 0x59 }, // cntr_rstb_stop_l

    { 0xF2, 0x03 }, // cntr_transfer_start_h (852)
    { 0xF3, 0x54 }, // cntr_transfer_start_l
    { 0xF4, 0x03 }, // cntr_transfer_stop_h (854)
    { 0xF5, 0x56 }, // cntr_transfer_stop_l

    { 0x8A, 0x03 }, // cds_pxl_bias1_track_start_h (810)
    { 0x8B, 0x2A }, // cds_pxl_bias1_track_start_l
    { 0x8C, 0x03 }, // cds_pxl_bias1_track_stop_h (857)
    { 0x8D, 0x59 }, // cds_pxl_bias1_track_stop_l

    { 0x96, 0x03 }, // cds_amp1_bias1_track_start_h (810)
    { 0x97, 0x2A }, // cds_amp1_bias1_track_start_l
    { 0x98, 0x03 }, // cds_amp1_bias1_track_stop_h (857)
    { 0x99, 0x59 }, // cds_amp1_bias1_track_stop_l

    { 0xBA, 0x00 }, // cds_amp2_clamp_sample_start_h (0)
    { 0xBB, 0x00 }, // cds_amp2_clamp_sample_start_l
    { 0xBC, 0x03 }, // cds_amp2_clamp_sample_stop_h (857)
    { 0xBD, 0x59 }, // cds_amp2_clamp_sample_stop_l

    { 0x03, 0x01 }, // B bank

    { 0x28, 0x02 }, // tghstart_h (528)
    { 0x29, 0x10 }, // tghstart_l
    { 0x2A, 0x03 }, // tghstop_h (850)
    { 0x2B, 0x52 }, // tghstop_l

    { 0x3C, 0x07 }, // coffset_h (1874)
    { 0x3D, 0x52 }, // coffset_l

    { 0x38, 0x02 }, // blhstart_h (528)
    { 0x39, 0x10 }, // blhstart_l
    { 0x3A, 0x03 }, // blhstop_h (850)
    { 0x3B, 0x52 }, // blhstop_l

    // ############################################################
    // ########### Don't change setting value stop ################
    // ############################################################

    // MIPI disable
    { 0x03, 0x05 }, // bank F
    { 0x04, 0x30 }, // mipi_control_0

    // 320x240_DVP_8bit
    // Window setting
    { 0x03, 0x00 }, // bank_A

    // 320
    { 0x0C, 0x00 }, // windowx1_h
    { 0x0D, 0x01 }, // windowx1_l
    { 0x10, 0x01 }, // windowx2_h
    { 0x11, 0x41 }, // windowx2_l
    // 240
    { 0x0E, 0x00 }, // windowy1_h
    { 0x0F, 0x7A }, // windowy1_l
    { 0x12, 0x01 }, // windowy2_h
    { 0x13, 0x6A }, // windowy2_l
    // vsync setting
    // 240
    { 0x74, 0x00 }, // vsyncstartrow0_h
    { 0x75, 0x83 }, // vsyncstartrow0_l
    { 0x76, 0x01 }, // vsyncstoprow0_h
    { 0x77, 0x73 }, // vsyncstoprow0_l
    // Format fifo setting
    { 0x97, 0x02 }, // rd_cnt_init_h
    { 0x98, 0x50 }, // rd_cnt_init_l
    // 320
    { 0x99, 0x00 }, // rd_x_start0_h
    { 0x9A, 0x30 }, // rd_x_start0_l
    { 0x9B, 0x02 }, // rd_x_stop0_h
    { 0x9C, 0xB0 }, // rd_x_stop0_l
    // 240
    { 0x9D, 0x00 }, // rd_y_start0_h
    { 0x9E, 0x83 }, // rd_y_start0_l
    { 0x9F, 0x01 }, // rd_y_stop0_h
    { 0xA0, 0x73 }, // rd_y_stop0_l

    { 0xAC, 0x01 }, // rd_bit_sel
};

static const struct Register pgd030k_regs_qvga_seq[] = {
    // 320x240_DVP_8bit
    // Window setting
    { 0x03, 0x00 }, // bank_A

    // 320
    { 0x0C, 0x00 }, // windowx1_h
    { 0x0D, 0x01 }, // windowx1_l
    { 0x10, 0x01 }, // windowx2_h
    { 0x11, 0x41 }, // windowx2_l
    // 240
    { 0x0E, 0x00 }, // windowy1_h
    { 0x0F, 0x7A }, // windowy1_l
    { 0x12, 0x01 }, // windowy2_h
    { 0x13, 0x6A }, // windowy2_l
    // vsync setting
    // 240
    { 0x74, 0x00 }, // vsyncstartrow0_h
    { 0x75, 0x83 }, // vsyncstartrow0_l
    { 0x76, 0x01 }, // vsyncstoprow0_h
    { 0x77, 0x73 }, // vsyncstoprow0_l
    // Format fifo setting
    { 0x97, 0x02 }, // rd_cnt_init_h
    { 0x98, 0x50 }, // rd_cnt_init_l
    // 640
    { 0x99, 0x00 }, // rd_x_start0_h
    { 0x9A, 0x30 }, // rd_x_start0_l
    { 0x9B, 0x02 }, // rd_x_stop0_h
    { 0x9C, 0xB0 }, // rd_x_stop0_l
    // 240
    { 0x9D, 0x00 }, // rd_y_start0_h
    { 0x9E, 0x83 }, // rd_y_start0_l
    { 0x9F, 0x01 }, // rd_y_stop0_h
    { 0xA0, 0x73 }, // rd_y_stop0_l

    { 0xAC, 0x01 }, // rd_bit_sel
};

static int pgd030k_read_reg(sensor_t* sensor, uint8_t reg_addr)
{
    printk("%s sensor %p\r\n", __func__, sensor);
    return 0;
}

static int pgd030k_write_reg(sensor_t* sensor, uint8_t reg_addr, uint16_t reg_data)
{
    printk("%s sensor %p\r\n", __func__, sensor);
    return 0;
}

static int pgd030k_set_pixformat(sensor_t* sensor, pixformat_t pixformat)
{
    if (PIXFORMAT_GRAYSCALE != pixformat) {
        mp_raise_msg(&mp_type_ValueError, "PGD030 only support GRAYSCALE");
    }

    return 0;
}

static int pgd030k_set_framesize(sensor_t* sensor, framesize_t framesize)
{
    int regs_cnt = 0;
    struct Register* regs = (void*)0;

    uint16_t width = resolution[framesize][0];
    uint16_t height = resolution[framesize][1];

    // if ((640 < width) || (480 < height)) {
    //     mp_raise_msg(&mp_type_ValueError, "PGD030 max support 640x480");
    // }

    if (FRAMESIZE_QVGA != framesize) {
        mp_raise_msg(&mp_type_ValueError, "PGD030 only support QVGA");
        // mp_printf(&mp_plat_print, "PGD030 not support framesize(%dx%d), use QVGA\n", width, height);
    }

    regs = &pgd030k_regs_qvga_seq;
    regs_cnt = sizeof(pgd030k_regs_qvga_seq) / sizeof(struct Register);

    for (int i = 0; i < regs_cnt; i++) {
        if (0xFFFF == regs[i].address) {
            mp_hal_delay_ms(regs[i].value);
            continue;
        }
        cambus_writeb(sensor->slv_addr, regs[i].address & 0xFF, regs[i].value);
    }

    /* delay n ms */
    mp_hal_delay_ms(100);
    dvp_set_image_size(width, height);

    return 0;
}

static int pgd030k_set_framerate(sensor_t* sensor, framerate_t framerate)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_set_contrast(sensor_t* sensor, int level)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_set_brightness(sensor_t* sensor, int level)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_set_saturation(sensor_t* sensor, int level)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_set_gainceiling(sensor_t* sensor, gainceiling_t gainceiling)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_set_quality(sensor_t* sensor, int qs)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_set_colorbar(sensor_t* sensor, int enable)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_set_auto_gain(sensor_t* sensor, int enable, float gain_db, float gain_db_ceiling)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_get_gain_db(sensor_t* sensor, float* gain_db)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_set_auto_exposure(sensor_t* sensor, int enable, int exposure_us)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_get_exposure_us(sensor_t* sensor, int* exposure_us)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_set_auto_whitebal(sensor_t* sensor, int enable, float r_gain_db, float g_gain_db, float b_gain_db)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_get_rgb_gain_db(sensor_t* sensor, float* r_gain_db, float* g_gain_db, float* b_gain_db)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_set_hmirror(sensor_t* sensor, int enable)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_set_vflip(sensor_t* sensor, int enable)
{
    mp_printf(&mp_plat_print, "%s %d\n", __func__, __LINE__);
    return 0;
}

static int pgd030k_reset(sensor_t* sensor)
{
    for (int i = 0; i < (sizeof(pgd030k_regs_init_seq) / sizeof(struct Register)); i++) {
        if (0xFFFF == pgd030k_regs_init_seq[i].address) {
            mp_hal_delay_ms(pgd030k_regs_init_seq[i].value);
            continue;
        }
        cambus_writeb(sensor->slv_addr, pgd030k_regs_init_seq[i].address & 0xFF, pgd030k_regs_init_seq[i].value);
    }

    return 0;
}

int pgd030k_init(sensor_t* sensor)
{
    // Initialize sensor structure.
    sensor->gs_bpp = 2;
    sensor->reset = pgd030k_reset;
    sensor->read_reg = pgd030k_read_reg;
    sensor->write_reg = pgd030k_write_reg;
    sensor->set_pixformat = pgd030k_set_pixformat;
    sensor->set_framesize = pgd030k_set_framesize;
    sensor->set_framerate = pgd030k_set_framerate;
    sensor->set_contrast = pgd030k_set_contrast;
    sensor->set_brightness = pgd030k_set_brightness;
    sensor->set_saturation = pgd030k_set_saturation;
    sensor->set_gainceiling = pgd030k_set_gainceiling;
    sensor->set_quality = pgd030k_set_quality;
    sensor->set_colorbar = pgd030k_set_colorbar;
    sensor->set_auto_gain = pgd030k_set_auto_gain;
    sensor->get_gain_db = pgd030k_get_gain_db;
    sensor->set_auto_exposure = pgd030k_set_auto_exposure;
    sensor->get_exposure_us = pgd030k_get_exposure_us;
    sensor->set_auto_whitebal = pgd030k_set_auto_whitebal;
    sensor->get_rgb_gain_db = pgd030k_get_rgb_gain_db;
    sensor->set_hmirror = pgd030k_set_hmirror;
    sensor->set_vflip = pgd030k_set_vflip;

    return 0;
}
