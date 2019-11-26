/*
 * Copyright (c) 2014-2015, TAKAHASHI Tomohiro (TTRFTECH) edy555@gmail.com
 * All rights reserved.
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * The software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */
#include "cmsis_os.h"

#include "Font.h"
extern MWCFONT font_06x13;
extern MWCFONT font_08x15;
extern MWCFONT font_10x20;
extern MWCFONT font_12x24;

extern int8_t welcom_8bit[];

#define APP_VERSION  "v0.0.4"

#define REAL_PART   (0)
#define IMAG_PART   (1)
#define REAL_PART_AND_IMAG_PART_LEN (2)

#define  REFLECTION (0)
#define  TRANSMISSION (1)
#define REFLECTION_AND_TRANSMISSION_LEN    (2)

#define SWEEP_POINTS 101

#define USE_ILI_LCD  0
#if USE_ILI_LCD
#define LCD_WIDTH    320
#define LCD_HEIGHT   240
#else
#define LCD_WIDTH    400
#define LCD_HEIGHT   240
#endif

#define LANG_EN      0
#define LANG_CN      1

#define START_MIN    50000
#define BASE_MAX     270000000
#define STOP_MAX     1000000000

#define M_PI        3.14159265358979323846

/*
 * main.c
 */
extern float measured[2][SWEEP_POINTS][2];

/*
常用的校准技术有三种：
SOLT（短路－开路－负载－直通）
TRL（直通－反射－线路）
ECal（电子校准）模块
SOLT校准方法使用12项误差修正模型，其中被测件的正向有6项，反向有6项。
正向误差项：ED（方向）、ES（源匹配）、EL（负载匹配）、ERF（反射跟踪）、ETF（发射跟踪）和EX（串扰）。
 */

#define CAL_LOAD  0    // 负载 接50欧姆负载
#define CAL_OPEN  1    // 开路 接开路负载
#define CAL_SHORT 2    // 短路 接短路负载
#define CAL_THRU  3    // 直通 PORT1 PORT2 短接
#define CAL_ISOLN 4    // 隔离 PORT1 PORT2 都接负载

#define CALSTAT_LOAD   (1<<0)
#define CALSTAT_OPEN   (1<<1)
#define CALSTAT_SHORT  (1<<2)
#define CALSTAT_THRU   (1<<3)
#define CALSTAT_ISOLN  (1<<4)

#define CALSTAT_ED (1<<0)          /* CALSTAT_LOAD error term directivity */
#define CALSTAT_EX (1<<4)          /* CALSTAT_ISOLN error term isolation */
#define CALSTAT_ES (1<<5)          /* error term source match */
#define CALSTAT_ER (1<<6)          /* error term refrection tracking */
#define CALSTAT_ET (1<<7)          /* error term transmission tracking */

#define CALSTAT_APPLY (1<<8)
#define CALSTAT_INTERPOLATED (1<<9)

#define ETERM_ED 0 /* error term directivity */
#define ETERM_EX 4 /* error term isolation */
#define ETERM_ES 1 /* error term source match */
#define ETERM_ER 2 /* error term refrection tracking */
#define ETERM_ET 3 /* error term transmission tracking */

void cal_collect(int type);
void cal_done(void);

enum {
  ST_START, ST_STOP, ST_CENTER, ST_SPAN, ST_CW
};

void set_sweep_frequency(int type, int frequency);
uint32_t get_sweep_frequency(int type);

double my_atof(const char *p);

void toggle_sweep(void);

extern int8_t sweep_enabled;

/*
 * ui.c
 */
extern void ui_init(void);
extern void ui_process(void);

/*
 * dsp.c
 */
// 5ms @ 48kHz
// #define AUDIO_BUFFER_LEN 480  // 240点*2CH - 240/48000 测试采集 5ms
#define AUDIO_BUFFER_LEN 96   //  48点*2CH - 48/48000 实际采集 1ms（46875 为 1024us）

extern int16_t rx_buffer[];

#define STATE_LEN        32
#define SAMPLE_LEN       48

extern int16_t samp_buf[];
extern int16_t ref_buf[];

void dsp_process(int16_t *src, size_t len);
void calculate_gamma(float *gamma);

int si5351_set_frequency_with_offset(int freq, int offset, uint8_t drive_strength);
int si5351_set_frequency_with_offset_expand(int freq, int offset, uint8_t drive_strength);

/*
 * tlv320aic3204.c
 */
typedef struct {
  int target_level;
  int gain_hysteresis;
  int attack;
  int attack_scale;
  int decay;
  int decay_scale;
} tlv320aic3204_agc_config_t;

extern void tlv320aic3204_init(void);
extern void tlv320aic3204_init_slave(void);
extern void tlv320aic3204_set_gain(int lgain, int rgain);
extern void tlv320aic3204_set_digital_gain(int gain);
extern void tlv320aic3204_set_volume(int gain);
extern void tlv320aic3204_agc_config(tlv320aic3204_agc_config_t *conf);
extern void tlv320aic3204_select_in1(void);
extern void tlv320aic3204_select_in3(void);
extern void tlv320aic3204_adc_filter_enable(int enable);


/*
 * plot.c
 */
#if USE_ILI_LCD

#define OFFSETX            15     // 10+5
#define OFFSETY            0
#define WIDTH              291    // 320-15*2+1
#define HEIGHT             233    // 233+7

#define CELLOFFSETX        5      // Mark点标记
#define AREA_WIDTH_NORMAL (WIDTH + CELLOFFSETX*2) // 301

extern int area_width;
extern int area_height;

#define GRIDY              29     // 232/8

#else

#define OFFSETX           (24/2+24/2+5)  // 29

#define OFFSETY            0
#define WIDTH              341    // 400-29*2+1，X方向膨胀
#define HEIGHT             233

#define CELLOFFSETX        5      // Mark点标记
#define AREA_WIDTH_NORMAL (WIDTH + CELLOFFSETX*2) // 351

extern int area_width;
extern int area_height;

#define GRIDY              29     // 232/8

#endif

// font

extern const uint16_t x5x7_bits [];
extern const uint32_t numfont20x24[][24];

extern const MWIMAGEBITS _06x13_bits [];
extern const MWIMAGEBITS _08x15_bits [];
extern const MWIMAGEBITS _10x20_bits [];
extern const MWIMAGEBITS _12x24_bits [];

#define S_PI              "\034"  // 0x1C π
#define S_MICRO           "\035"  // 0x1D μ
#define S_OHM             "\036"  // 0x1E Ω
#define S_DEGREE          "\037"  // 0x1F
#define S_LARROW          "\032"  // 0x1A
#define S_RARROW          "\033"  // 0x1B

// trace 

#define TRACES_MAX         4

enum {
  TRC_LOGMAG, TRC_PHASE, TRC_DELAY, TRC_SMITH, TRC_POLAR, TRC_LINEAR, TRC_SWR, TRC_OFF
};

extern const char *trc_type_name[];

// LOGMAG: SCALE, REFPOS, REFVAL
// PHASE: SCALE, REFPOS, REFVAL
// DELAY: SCALE, REFPOS, REFVAL
// SMITH: SCALE, <REFPOS>, <REFVAL>
// LINMAG: SCALE, REFPOS, REFVAL
// SWR: SCALE, REFPOS, REFVAL

// Electrical Delay
// Phase

typedef struct {
  uint8_t enabled;  // 使能
  uint8_t type;     // 类型
  uint8_t channel;  // 通道
  uint8_t polar;    // 极性
  float scale;      // 比例、尺度
  float refpos;     // 参考位置
} trace_t;

typedef struct {
  int32_t magic;  // 魔术字
  uint16_t dac_value;  // DAC值
  uint16_t grid_color; // grid 颜色
  uint16_t menu_normal_color;  // 菜单正常颜色
  uint16_t menu_active_color;  // 菜单按下颜色
  uint16_t trace_color[TRACES_MAX];  // 曲线颜色
  int16_t touch_cal[4];  // 触摸校准
  int8_t default_loadcal;  // 默认载入校准
  int8_t lang;  // 语言
  int32_t checksum;  // 校验值
} config_t;

extern config_t config;

//extern trace_t trace[TRACES_MAX];

void set_trace_type(int t, int type);
void set_trace_channel(int t, int channel);
void set_trace_scale(int t, float scale);
void set_trace_refpos(int t, float refpos);
float get_trace_scale(int t);
float get_trace_refpos(int t);
void draw_battery_status(void);

void set_electrical_delay(float picoseconds);
float get_electrical_delay(void);

// marker

typedef struct {
  int8_t enabled;
  int16_t index;
  uint32_t frequency;
} marker_t;

//extern marker_t markers[4];
//extern int active_marker;

void plot_init(void);
void update_grid(void);
void request_to_redraw_grid(void);
void redraw_frame(void);
//void redraw_all(void);
void request_to_draw_cells_behind_menu(void);
void request_to_draw_cells_behind_numeric_input(void);
void redraw_marker(int marker, int update_info);
void trace_get_info(int t, char *buf, int len);
void plot_into_index(float measured[2][SWEEP_POINTS][2]);
void force_set_markmap(void);
void draw_all_cells(void);

void draw_cal_status(void);

void markmap_all_markers(void);

void marker_position(int m, int t, int *x, int *y);
int search_nearest_index(int x, int y, int t);

extern int8_t redraw_requested;

extern int16_t vbat;

/*
 * nt35510.c
 */
// #define BRG556(b,r,g)     ( (((b)<<8)&0xfc00) | (((r)<<2)&0x03e0) | (((g)>>3)&0x001f) )
#define BRG556(b,r,g)     ( (((r)<<8)&0xf800) | (((g)<<3)&0x07e0) | (((b)>>3)&0x001f) )

typedef struct {
	uint16_t width;
	uint16_t height;
	uint16_t scaley;
	uint16_t slide;
	const uint32_t *bitmap;
} font_t;

extern const font_t NF20x24;

extern uint16_t lcd_buffer[4096];

void nt35510_init(void);
void nt35510_test(int mode);
void nt35510_bulk_x2(int x, int y, int w, int h);
void nt35510_fill_x2(int x, int y, int w, int h, int color);
void nt35510_drawchar_5x7_x2(uint8_t ch, int x, int y, uint16_t fg, uint16_t bg);
void nt35510_drawstring_5x7_x2(const char *str, int x, int y, uint16_t fg, uint16_t bg);
void nt35510_drawfont_x2(uint8_t ch, const font_t *font, int x, int y, uint16_t fg, uint16_t bg);

void nt35510_fill(int x, int y, int w, int h, int color);
void nt35510_drawchar(MWCFONT *font, uint8_t ch, int x, int y, uint16_t fg, uint16_t bg);
void nt35510_drawstring(MWCFONT *font, const char *str, int x, int y, uint16_t fg, uint16_t bg);
void nt35510_drawhz24x24(const char *str, int x, int y, uint16_t fg, uint16_t bg);

void nt35510_drawchar_x2(MWCFONT *font, uint8_t ch, int x, int y, uint16_t fg, uint16_t bg);
void nt35510_drawstring_x2(MWCFONT *font, const char *str, int x, int y, uint16_t fg, uint16_t bg);

void nt35510_drawchar_5x7(uint8_t ch, int x, int y, uint16_t fg, uint16_t bg);
void nt35510_drawstring_5x7(const char *str, int x, int y, uint16_t fg, uint16_t bg);

/*
 * flash.c
 */
#define SAVEAREA_MAX 5

typedef struct {
  int32_t magic;
  int32_t _frequency0; // start or center
  int32_t _frequency1; // stop or span
  int16_t _sweep_points; // 扫描点数
  uint16_t _cal_status;  // 校准状态

  uint32_t _frequencies[SWEEP_POINTS];
  float _cal_data[5][SWEEP_POINTS][REAL_PART_AND_IMAG_PART_LEN];
  float _electrical_delay; // picoseconds
  
  trace_t _trace[TRACES_MAX];
  marker_t _markers[4];
  int _active_marker;

  int32_t checksum;
} properties_t;

#define CONFIG_MAGIC 0x434f4e45 /* 'CONF' */

extern int16_t lastsaveid;
extern properties_t *active_props;
extern properties_t current_props;

extern uint8_t previous_marker;

#define frequency0 current_props._frequency0
#define frequency1 current_props._frequency1
#define sweep_points current_props._sweep_points
#define cal_status current_props._cal_status
#define frequencies current_props._frequencies
#define cal_data active_props->_cal_data
#define electrical_delay current_props._electrical_delay

#define trace current_props._trace
#define markers current_props._markers
#define active_marker current_props._active_marker

int caldata_save(int id);
int caldata_recall(int id);
const properties_t *caldata_ref(int id);

int config_save(void);
int config_recall(void);

void clear_all_config_prop_data(void);

/*
 * ui.c
 */

typedef struct {
  int8_t digit; /* 0~5 */
  int8_t digit_mode;
  int8_t current_trace; /* 0..3 */
  uint32_t value;
} uistat_t;

extern uistat_t uistat;
  
void ui_init(void);
void ui_show(void);
void ui_hide(void);

extern uint8_t operation_requested;

void handle_touch_interrupt(void);

#define TOUCH_THRESHOLD 2000

void touch_cal_exec(void);
void touch_draw_test(void);

/*
 * misclinous
 */
#define PULSE // do { palClearPad(GPIOC, GPIOC_LED); palSetPad(GPIOC, GPIOC_LED);} while(0)

// convert vbat [mV] to battery indicator
static inline uint8_t vbat2bati(int16_t vbat)
{
  if (vbat < 3200) return 0;
  if (vbat < 3450) return 25;
  if (vbat < 3700) return 50;
  if (vbat < 4100) return 75;
  return 100;
}

/*EOF*/
