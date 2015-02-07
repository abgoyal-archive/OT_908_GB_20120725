/*
 * Copyright (c) 2010, Hisense mobile technology Ltd. All Rights Reserved
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */
#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/camera.h>
#include <mach/board.h>
#include <linux/proc_fs.h>


#define CONFIG_HI253_DEBUG
#ifdef CONFIG_HI253_DEBUG
#define HI253_DBG(fmt, args...) printk(KERN_INFO fmt, ##args)
#else
#define HI253_DBG(fmt, args...) do { } while (0)
#endif

static int current_special_effect = -1;

struct hi253_work {
	struct work_struct work;
};
static struct  hi253_work *hi253_sensorw;

static struct  i2c_client *hi253_client;

struct hi253_ctrl {
	const struct msm_camera_sensor_info *sensordata;
};
static struct hi253_ctrl *hi253_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(hi253_wait_queue);
DECLARE_MUTEX(hi253_sem);

struct hi253_i2c_reg_conf {
	unsigned char  baddr;
	unsigned char  bdata;

};

/* Revision constants */
/* Revision constants */
#define PID_HI253		0x92	/* Product ID */
#define HI253_PID_REG	0x04	

/*****************************************************************************

 *  Register Settings Function

 *****************************************************************************/
 struct hi253_i2c_reg_conf hi253_reg_init_tab[] = 
{
	/////// Start Sleep ///////
	{0x01,0x71},
	{0x08,0x0f},
	{0x01,0x70},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x0e,0x03},
	{0x0e,0x73},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x0e,0x00},
	{0x01,0x71},
	{0x08,0x00},
	{0x01,0x73},
	{0x01,0x71},
	{0x03,0x20},
	{0x10,0x1c},
	{0x03,0x22},
	{0x10,0x69},

	{0x03,0x00},
	{0x10,0x1b}, //x13},//pre 2////0x11->0x13
	{0x11,0x93}, //0x90},
	{0x12,0x04},

	{0x0b,0xaa},
	{0x0c,0xaa},
	{0x0d,0xaa},
	{0x20,0x00},
	{0x21,0x04},
	{0x22,0x00},
	{0x23,0x02}, //0x07},
	{0x24,0x04},
	{0x25,0xb0},
	{0x26,0x06},
	{0x27,0x40},

	{0x40,0x01},
	{0x41,0x78}, 
	{0x42,0x00},
	{0x43,0x14},

	{0x45,0x04},
	{0x46,0x18},
	{0x47,0xd8},

	{0x80,0x2e},
	{0x81,0x7e},
	{0x82,0x90},
	{0x83,0x00},
	{0x84,0x0c},
	{0x85,0x00},
	{0x90,0x05}, //0a},
	{0x91,0x05}, //0a},
	{0x92,0xd8},
	{0x93,0xd0},
	{0x94,0x75},
	{0x95,0x70},
	{0x96,0xdc},
	{0x97,0xfe},
	{0x98,0x38},
	{0x99,0x43},
	{0x9a,0x43},
	{0x9b,0x43},
	{0x9c,0x43},
	{0xa0,0x00},
	{0xa2,0x00},
	{0xa4,0x00},
	{0xa6,0x00},
	{0xa8,0x43},
	{0xaa,0x43},
	{0xac,0x43},
	{0xae,0x43},

	{0x03,0x02},
	{0x12,0x03},
	{0x13,0x03},
	{0x16,0x00},
	{0x17,0x8C},
	{0x18,0x4c},
	{0x19,0x00},
	{0x1a,0x39},
	{0x1c,0x09},
	{0x1d,0x40},
	{0x1e,0x30},
	{0x1f,0x10},
	{0x20,0x77},
	{0x21,0xde},
	{0x22,0xa7},
	{0x23,0x30},
	{0x27,0x3c},
	{0x2b,0x80},
	{0x2e,0x11},
	{0x2f,0xa1},
	{0x30,0x05},
	{0x50,0x20},
	{0x52,0x01},
	{0x53,0xc1},
	{0x55,0x1c},
	{0x56,0x11},
	{0x5d,0xa2},
	{0x5e,0x5a},
	{0x60,0x87},
	{0x61,0x99},
	{0x62,0x88},
	{0x63,0x97},
	{0x64,0x88},
	{0x65,0x97},
	{0x67,0x0c},
	{0x68,0x0c},
	{0x69,0x0c},
	{0x72,0x89},
	{0x73,0x96},
	{0x74,0x89},
	{0x75,0x96},
	{0x76,0x89},
	{0x77,0x96},
	{0x7c,0x85},
	{0x7d,0xaf},
	{0x80,0x01},
	{0x81,0x7f},
	{0x82,0x13},
	{0x83,0x24},
	{0x84,0x7d},
	{0x85,0x81},
	{0x86,0x7d},
	{0x87,0x81},
	{0x92,0x48},
	{0x93,0x54},
	{0x94,0x7d},
	{0x95,0x81},
	{0x96,0x7d},
	{0x97,0x81},
	{0xa0,0x02},
	{0xa1,0x7b},
	{0xa2,0x02},
	{0xa3,0x7b},
	{0xa4,0x7b},
	{0xa5,0x02},
	{0xa6,0x7b},
	{0xa7,0x02},
	{0xa8,0x85},
	{0xa9,0x8c},
	{0xaa,0x85},
	{0xab,0x8c},
	{0xac,0x10},
	{0xad,0x16},
	{0xae,0x10},
	{0xaf,0x16},
	{0xb0,0x99},
	{0xb1,0xa3},
	{0xb2,0xa4},
	{0xb3,0xae},
	{0xb4,0x9b},
	{0xb5,0xa2},
	{0xb6,0xa6},
	{0xb7,0xac},
	{0xb8,0x9b},
	{0xb9,0x9f},
	{0xba,0xa6},
	{0xbb,0xaa},
	{0xbc,0x9b},
	{0xbd,0x9f},
	{0xbe,0xa6},
	{0xbf,0xaa},
	{0xc4,0x2c},
	{0xc5,0x43},
	{0xc6,0x63},
	{0xc7,0x79},
	{0xc8,0x2d},
	{0xc9,0x42},
	{0xca,0x2d},
	{0xcb,0x42},
	{0xcc,0x64},
	{0xcd,0x78},
	{0xce,0x64},
	{0xcf,0x78},
	{0xd0,0x0a},
	{0xd1,0x09},
	{0xd4,0x05}, //0a},
	{0xd5,0x05}, //0a},
	{0xd6,0xd8}, //d8},
	{0xd7,0xd0},
	{0xe0,0xc4},
	{0xe1,0xc4},
	{0xe2,0xc4},
	{0xe3,0xc4},
	{0xe4,0x00},
	{0xe8,0x80},
	{0xe9,0x40},
	{0xea,0x7f},
	{0xf0,0x01},
	{0xf1,0x01},
	{0xf2,0x01},
	{0xf3,0x01},
	{0xf4,0x01},

	{0x03,0x03},
	{0x10,0x10},

	{0x03,0x10},
	{0x10,0x01},
	{0x12,0x30},
	{0x13,0x02},
	{0x20,0x00},
	{0x30,0x00},
	{0x31,0x00},
	{0x32,0x00},
	{0x33,0x00},
	{0x34,0x30},
	{0x35,0x00},
	{0x36,0x00},
	{0x38,0x00},
	{0x3e,0x58},
	{0x3f,0x00},
	{0x40,0x03},
	{0x41,0x00},
	{0x48,0x83}, //jinkay_0318//contrast//0x80},
	{0x60,0x67},

	{0x61,0x80}, //0x7b},//0x78}, //0x7c}, //0x65}, //0x70}, 
	{0x62,0x80}, //0x7b},//0x78}, //0x7c}, //0x70},

	{0x63,0x50},
	{0x64,0x41},
	{0x66,0x42},
	{0x67,0x20},
	{0x6a,0x80},
	{0x6b,0x80},
	{0x6c,0x88},
	{0x6d,0x80},

	{0x03,0x11},
	{0x10,0x3f},
	{0x11,0x40},
	{0x12,0xba},
	{0x13,0xcb},
	{0x26,0x31},
	{0x27,0x34},
	{0x28,0x0f},
	{0x29,0x10},
	{0x2b,0x30},
	{0x2c,0x32},
	{0x30,0x70},
	{0x31,0x10},
	{0x32,0x63},
	{0x33,0x0a},
	{0x34,0x07},
	{0x35,0x04},
	{0x36,0x70},
	{0x37,0x18},
	{0x38,0x63},
	{0x39,0x0a},
	{0x3a,0x07},
	{0x3b,0x04},
	{0x3c,0x80},
	{0x3d,0x18},
	{0x3e,0x80},
	{0x3f,0x0c},
	{0x40,0x09},
	{0x41,0x06},
	{0x42,0x80},
	{0x43,0x18},
	{0x44,0x80},
	{0x45,0x13},
	{0x46,0x10},
	{0x47,0x11},
	{0x48,0x90},
	{0x49,0x40},
	{0x4a,0x80},
	{0x4b,0x13},
	{0x4c,0x10},
	{0x4d,0x11},
	{0x4e,0x80},
	{0x4f,0x30},
	{0x50,0x80},
	{0x51,0x13},
	{0x52,0x10},
	{0x53,0x13},
	{0x54,0x11},
	{0x55,0x17},
	{0x56,0x20},
	{0x57,0x20},
	{0x58,0x20},
	{0x59,0x30},
	{0x5a,0x18},
	{0x5b,0x00},
	{0x5c,0x00},
	{0x60,0x3f},
	{0x62,0x50},
	{0x70,0x06},
	{0x03,0x12},

	{0x20,0x0f},
	{0x21,0x0f},

	{0x25,0x30},
	{0x28,0x00},
	{0x29,0x00},
	{0x2a,0x00},
	{0x30,0x50},
	{0x31,0x18},
	{0x32,0x32},
	{0x33,0x40},
	{0x34,0x50},
	{0x35,0x70},
	{0x36,0xa0},
	{0x40,0xa0},
	{0x41,0x40},
	{0x42,0xa0},
	{0x43,0x90}, //0x70},//0x90},
	{0x44,0x85}, //0x70},//0x85},
	{0x45,0x80},
	{0x46,0xb0},
	{0x47,0x55},
	{0x48,0xa0},
	{0x49,0x90}, //0x70},//0x90},
	{0x4a,0x85}, //0x70},//0x85},
	{0x4b,0x80},
	{0x4c,0xb0},
	{0x4d,0x40},
	{0x4e,0x90},
	{0x4f,0x90},
	{0x50,0x90},
	{0x51,0x80},
	{0x52,0xb0},
	{0x53,0x60},
	{0x54,0xc0},
	{0x55,0xc0},
	{0x56,0xc0},
	{0x57,0x80},
	{0x58,0x90},
	{0x59,0x40},
	{0x5a,0xd0},
	{0x5b,0xd0},
	{0x5c,0xe0},
	{0x5d,0x80},
	{0x5e,0x88},
	{0x5f,0x40},
	{0x60,0xe0},
	{0x61,0xe0},
	{0x62,0xe0},
	{0x63,0x80},
	{0x70,0x13},
	{0x71,0x01},
	{0x72,0x15},
	{0x73,0x01},
	{0x74,0x25},
	{0x75,0x15},
	{0x80,0x15},
	{0x81,0x25},
	{0x82,0x50},
	{0x85,0x1a},
	{0x88,0x00},
	{0x89,0x00},

	{0x90,0x5c},

	{0xD0,0x0c},
	{0xD1,0x80},
	{0xD2,0x67},
	{0xD3,0x00},
	{0xD4,0x00},
	{0xD5,0x02},
	{0xD6,0xff},
	{0xD7,0x18},
	{0x3b,0x06},
	{0x3c,0x06},
	{0xc5,0x30},
	{0xc6,0x2a},

	{0x03,0x13},
	{0x10,0xab},
	{0x11,0x7b},
	{0x12,0x06},
	{0x14,0x00},
	{0x20,0x15},
	{0x21,0x13},
	{0x22,0x33},
	{0x23,0x04},
	{0x24,0x09},
	{0x25,0x08},
	{0x26,0x20},
	{0x27,0x30},
	{0x29,0x12},
	{0x2a,0x50},
	{0x2b,0x06},
	{0x2c,0x07},
	{0x25,0x08},
	{0x2d,0x0c},
	{0x2e,0x12},
	{0x2f,0x12},
	{0x50,0x10},
	{0x51,0x14},
	{0x52,0x12},
	{0x53,0x0c},
	{0x54,0x0f},
	{0x55,0x0f},
	{0x56,0x10},
	{0x57,0x13},
	{0x58,0x12},
	{0x59,0x0c},
	{0x5a,0x0f},
	{0x5b,0x0f},
	{0x5c,0x0a},
	{0x5d,0x0b},
	{0x5e,0x0a},
	{0x5f,0x08},
	{0x60,0x09},
	{0x61,0x08},
	{0x62,0x08},
	{0x63,0x09},
	{0x64,0x08},
	{0x65,0x06},
	{0x66,0x07},
	{0x67,0x06},
	{0x68,0x07},
	{0x69,0x08},
	{0x6a,0x07},
	{0x6b,0x06},
	{0x6c,0x07},
	{0x6d,0x06},
	{0x6e,0x07},
	{0x6f,0x08},
	{0x70,0x07},
	{0x71,0x06},
	{0x72,0x07},
	{0x73,0x06},
	{0x80,0x01}, //0x00},
	{0x81,0x1f},
	{0x82,0x01}, //0x03}, //0x01}, //2+center
	{0x83,0x31},
	{0x90,0x3f},
	{0x91,0x3f},
	{0x92,0x33},
	{0x93,0x30},
	{0x94,0x05},
	{0x95,0x18},
	{0x97,0x30},
	{0x99,0x35},
	{0xa0,0x01},
	{0xa1,0x02},
	{0xa2,0x01},
	{0xa3,0x02},
	{0xa4,0x02},
	{0xa5,0x04},
	{0xa6,0x04},
	{0xa7,0x06},
	{0xa8,0x05},
	{0xa9,0x08},
	{0xaa,0x05},
	{0xab,0x08},
	{0xb0,0x22},
	{0xb1,0x2a},
	{0xb2,0x28},
	{0xb3,0x22},
	{0xb4,0x2a},
	{0xb5,0x28},
	{0xb6,0x22},
	{0xb7,0x2a},
	{0xb8,0x28},
	{0xb9,0x22},
	{0xba,0x2a},
	{0xbb,0x28},
	{0xbc,0x27},
	{0xbd,0x30},
	{0xbe,0x2a},
	{0xbf,0x27},
	{0xc0,0x30},
	{0xc1,0x2a},
	{0xc2,0x1e},
	{0xc3,0x24},
	{0xc4,0x20},
	{0xc5,0x1e},
	{0xc6,0x24},
	{0xc7,0x20},
	{0xc8,0x18},
	{0xc9,0x20},
	{0xca,0x1e},
	{0xcb,0x18},
	{0xcc,0x20},
	{0xcd,0x1e},
	{0xce,0x18},
	{0xcf,0x20},
	{0xd0,0x1e},
	{0xd1,0x18},
	{0xd2,0x20},
	{0xd3,0x1e},

	{0x03,0x14},
	{0x10,0x11},
	{0x14,0x80},
	{0x15,0x80},
	{0x16,0x80},
	{0x17,0x80},
	{0x18,0x80},
	{0x19,0x80},
	{0x20,0x85},//0x60->0x85
	{0x21,0x80},
	{0x22,0x80},
	{0x23,0x80},
	{0x24,0x80},
	{0x30,0xc8},
	{0x31,0x2b},
	{0x32,0x00},
	{0x33,0x00},
	{0x34,0x90},
	{0x40,0x59},
	{0x50,0x40},
	{0x60,0x37},
	{0x70,0x40},



	{0x03,0x15},
	{0x10,0x0f},
	{0x14,0x42},
	{0x15,0x3a}, //0x32},
	{0x16,0x24},
	{0x17,0x2f},
	{0x30,0x8f},
	{0x31,0x59},
	{0x32,0x0a},
	{0x33,0x15},
	{0x34,0x5b},
	{0x35,0x06},
	{0x36,0x07},
	{0x37,0x40},
	{0x38,0x86},
	{0x40,0x95},
	{0x41,0x1f},
	{0x42,0x8a},
	{0x43,0x86},
	{0x44,0x0a},
	{0x45,0x84},
	{0x46,0x87},
	{0x47,0x9b},
	{0x48,0x23},
	{0x50,0x8c},
	{0x51,0x0c},
	{0x52,0x00},
	{0x53,0x07},
	{0x54,0x17},
	{0x55,0x9d},
	{0x56,0x00},
	{0x57,0x0b},
	{0x58,0x89},
	{0x80,0x03},
	{0x85,0x40},
	{0x87,0x02},
	{0x88,0x00},
	{0x89,0x00},
	{0x8a,0x00},

	{0x03,0x16},
	{0x10,0x31},
	{0x18,0x5e},
	{0x19,0x5d},
	{0x1a,0x0e},
	{0x1b,0x01},
	{0x1c,0xdc},
	{0x1d,0xfe},

	{0x30,0x00},
	{0x31,0x00},
	{0x32,0x08},
	{0x33,0x22},
	{0x34,0x45},
	{0x35,0x5f},
	{0x36,0x74},
	{0x37,0x86},
	{0x38,0x96},
	{0x39,0xa5},
	{0x3a,0xb3},
	{0x3b,0xbf},
	{0x3c,0xca},
	{0x3d,0xd4},
	{0x3e,0xde},
	{0x3f,0xe6},
	{0x40,0xee},
	{0x41,0xf4},
	{0x42,0xff},

	{0x50,0x00},  //0x00}, //0x00},	//
	{0x51,0x00},  //0x00}, //0x03},	//
	{0x52,0x0a},  //0x08}, //0x1b},	//
	{0x53,0x24},  //0x22}, //0x34},	//
	{0x54,0x47},  //0x45}, //0x58},	//
	{0x55,0x61},  //0x5f}, //0x74},	//
	{0x56,0x7b},  //0x74}, //0x8b},	//
	{0x57,0x8f},  //0x86}, //0x9e},	//
	{0x58,0xa0},  //0x96}, //0xae},	//
	{0x59,0xb0},  //0xa5}, //0xba},	//
	{0x5a,0xbd},  //0xb3}, //0xc9},	//
	{0x5b,0xc7},  //0xbf}, //0xd3},	//
	{0x5c,0xd1},  //0xca}, //0xdc},	//
	{0x5d,0xda},  //0xd4}, //0xe4},	//
	{0x5e,0xe2},  //0xde}, //0xeb},	//
	{0x5f,0xea},  //0xe6}, //0xf0},	//
	{0x60,0xf0},  //0xee}, //0xf5},	//
	{0x61,0xf6},  //0xf4}, //0xfa},	//
	{0x62,0xff},  //0xff}, //0xff},	//

	{0x70,0x00},
	{0x71,0x00},
	{0x72,0x08},
	{0x73,0x22},
	{0x74,0x45},
	{0x75,0x5f},
	{0x76,0x74},
	{0x77,0x86},
	{0x78,0x96},
	{0x79,0xa5},
	{0x7a,0xb3},
	{0x7b,0xbf},
	{0x7c,0xca},
	{0x7d,0xd4},
	{0x7e,0xde},
	{0x7f,0xe6},
	{0x80,0xee},
	{0x81,0xf4},
	{0x82,0xff},

	{0x03,0x17},
	{0x10,0xf7},
	{0xC4,0x42},
	{0xC5,0x37},

	{0x03, 0x18},
	{0x10, 0x00}, //scale off   

	{0x03,0x20},
	{0x11,0x1c},
	{0x18,0x30},
	{0x1a,0x08},
	{0x20,0x00},
	{0x21,0x30},
	{0x22,0x10},
	{0x23,0x00},
	{0x24,0x00},
	{0x28,0xe7},
	{0x29,0x0d},
	{0x2a,0xff},
	{0x2b,0x34},
	{0x2c,0xc2},
	{0x2d,0xcf},
	{0x2e,0x33},
	{0x30,0x78},
	{0x32,0x03},
	{0x33,0x2e},
	{0x34,0x30},
	{0x35,0xd4},
	{0x36,0xfe},
	{0x37,0x32},
	{0x38,0x04},
	{0x39,0x22},
	{0x3a,0xde},
	{0x3b,0x22},
	{0x3c,0xde},
	{0x50,0x45},
	{0x51,0x88},
	{0x56,0x03},
	{0x57,0xf7},
	{0x58,0x14},
	{0x59,0x88},
	{0x5a,0x04},
	{0x60,0x55},
	{0x61,0x55},
	{0x62,0x6a},
	{0x63,0xa9},
	{0x64,0x6a},
	{0x65,0xa9},
	{0x66,0x6a},
	{0x67,0xa9},
	{0x68,0x6b},
	{0x69,0xe9},
	{0x6a,0x6a},
	{0x6b,0xa9},
	{0x6c,0x6a},
	{0x6d,0xa9},
	{0x6e,0x55},
	{0x6f,0x55},


	{0x70, 0x7a}, //0x76}, //6e
	{0x71, 0x89}, //0x89}, //00 //-4

	// haunting control
	{0x76, 0x43},
	{0x77, 0xe2}, //04 //f2
	{0x78, 0x23}, //Yth1
	{0x79, 0x46}, //Yth2 //46
	{0x7a, 0x23}, //23
	{0x7b, 0x22}, //22
	{0x7d, 0x23},

	{0x83, 0x01}, 
	{0x84, 0x5f}, 
	{0x85, 0x90}, 

	{0x86, 0x01}, 
	{0x87, 0x2c}, 

	{0x88, 0x02},//frame 20
	{0x89, 0x49}, //
	{0x8a, 0xf0}, 

	{0x8B, 0x75}, //EXP100 
	{0x8C, 0x30}, 
	{0x8D, 0x61}, //EXP120 
	{0x8E, 0x44}, 

	{0x9c, 0x0f}, //EXP Limit 488.28 fps 
	{0x9d, 0x3c}, 
	{0x9e, 0x01}, //EXP Unit 
	{0x9f, 0x2c}, 

	{0xb0, 0x18},
	{0xb1, 0x14}, //ADC 400->560
	{0xb2, 0xe0},
	{0xb3, 0x18},
	{0xb4, 0x1a},
	{0xb5, 0x44},
	{0xb6, 0x2f},
	{0xb7, 0x28},
	{0xb8, 0x25},
	{0xb9, 0x22},
	{0xba, 0x21},
	{0xbb, 0x20},
	{0xbc, 0x1f},
	{0xbd, 0x1f},

	//AE_Adaptive Time option
	{0xc0, 0x14},
	{0xc1, 0x1f},
	{0xc2, 0x1f},
	{0xc3, 0x18}, //2b
	{0xc4, 0x10}, //08

	{0xc8, 0x80},
	{0xc9, 0x40},

	{0x03,0x22}, //awb
	{0x10,0xfd},
	{0x11,0x2e},
	{0x19,0x01},
	{0x20,0x30},
	{0x21,0x80},
	{0x24,0x01},
	{0x30,0x80},
	{0x31,0x80},
	{0x38,0x11},
	{0x39,0x34},
	{0x40,0xf4},
	{0x41,0x55},
	{0x42,0x33},
	{0x43,0xf6},
	{0x44,0x55},
	{0x45,0x44},
	{0x46,0x00},
	{0x50,0xb2},
	{0x51,0x81},
	{0x52,0x98},

	{0x80,0x2d},
	{0x81,0x20},
	{0x82,0x42},

	{0x83,0x50},
	{0x84,0x10},
	{0x85,0x60},
	{0x86,0x23},

	{0x87,0x49},
	{0x88,0x30},
	{0x89,0x37},
	{0x8a,0x28},

	{0x8b,0x41},
	{0x8c,0x30},
	{0x8d,0x34},
	{0x8e,0x28},

	{0x8f,0x5a}, //0x60}, //BGAINPARA 1
	{0x90,0x55}, //0x5C}, //BGAINPARA 2
	{0x91,0x4e}, //0x53}, //BGAINPARA 3
	{0x92,0x43}, //0x43}, //BGAINPARA 4
	{0x93,0x3A}, //BGAINPARA 5
	{0x94,0x35}, //0x37}, //BGAINPARA 6
	{0x95,0x2E}, //BGAINPARA 7
	{0x96,0x26}, //BGAINPARA 8
	{0x97,0x23}, //BGAINPARA 9
	{0x98,0x21}, //BGAINPARA 10
	{0x99,0x20}, //BGAINPARA 11
	{0x9a,0x20}, //BGAINPARA 12
	{0x9b,0x66},
	{0x9c,0x66},
	{0x9d,0x48},
	{0x9e,0x38},
	{0x9f,0x30},
	{0xa0,0x60},
	{0xa1,0x34},
	{0xa2,0x6f},
	{0xa3,0xff},
	{0xa4,0x14},
	{0xa5,0x2c},
	{0xa6,0xcf},
	{0xad,0x40},
	{0xae,0x4a},
	{0xaf,0x28},
	{0xb0,0x26},
	{0xb1,0x00},
	{0xb4,0xea},
	{0xb8,0xa0},
	{0xb9,0x00},
	{0x03,0x20},
	{0x10,0x8c},
	{0x03,0x20},
	{0x10,0x9c}, //cc},
	{0x03,0x22},
	{0x10,0xe9},
	{0x03,0x00},
	{0x0e,0x03},
	{0x0e,0x73},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},
	{0x03,0x00},

	{0x03,0x00},
	{0x01,0x50},


};

struct hi253_i2c_reg_conf hi253_reg_video_base_tab[] = 
{
   //preview
   {0x03,0x00},
   {0x01,0x51}, //0x71},//sleep on	 
   {0x0e,0x03}, //pll off
		 
   {0x03,0x20},//Page 20																		
   {0x10,0x0c},//AE OFF												   
   {0x03,0x22},//Page 22											   
   {0x10,0x69},//AWB OFF 
	 
		 
   {0x03,0x00}, //100412
   {0x10,0x1b}, //0412//0x11}, //0411//0x13}, //pre 2
   {0x11,0x93}, //
   {0x12,0x04}, //
   
   {0x20,0x00},
   {0x21,0x04},
   {0x22,0x00},
   {0x23,0x02}, //0x07},    
   
   //{0x40,0x01},
   //{0x41,0x98},
   //{0x40,0x00},
   //{0x40,0x14},
   
   
		 
   {0x03,0x10},
   {0x41,0x00},
	   
   {0x03,0x11},
   {0x3e,0xa0}, //for preview // 0x80 for capture
   {0x44,0xa0}, //for preview // 0x80 for capture
   {0x55,0x17}, //for preview // 0x0d for capture
   {0x5b,0x00}, //0x00}, //for preview // 0x3f for capture //impulse
   {0x62,0x60}, //for preview // 0x10 for capture
	   
   {0x03,0x12},
   {0x25,0x00}, //for preview // 0x30 for capture
   {0x30,0x50}, //for preview // 0x30 for capture
   {0x31,0x18}, //for preview // 0x38 for capture
   {0x32,0x32}, //for preview // 0x42 for capture
   {0x33,0x40}, //for preview // 0x60 for capture
   {0x34,0x50}, //for preview // 0x70 for capture
   {0x35,0x70}, //for preview // 0x80 for capture
   {0x4e,0x90}, //for preview // 0xb0 for capture
   {0x4f,0x90}, //for preview // 0xb0 for capture
   {0x50,0xa0}, //for preview // 0xc0 for capture
			 
   {0x03,0x13},
   {0x12,0x07}, //for preview // 0x06 for capture
   {0x5c,0x0b}, //for preview // 0x0d for capture
   {0x5d,0x0c}, //for preview // 0x0a for capture
   {0x5f,0x08}, //for preview // 0x0d for capture
   {0x60,0x09}, //for preview // 0x0c for capture
   {0x61,0x08}, //for preview // 0x0b for capture
   {0x80,0x01}, //0x00}, //for preview // 0x0b for capture
   {0x82,0x05}, //for preview // 0x0a for capture
   {0x94,0x03}, //for preview // 0x13 for capture
   {0x95,0x14}, //for preview // 0x54 for capture
   {0xa4,0x05}, //for preview // 0x01 for capture
   {0xa5,0x05}, //for preview // 0x01 for capture
   {0xbc,0x25}, //for preview // 0x55 for capture
   {0xbd,0x2a}, //for preview // 0x5a for capture
   {0xbe,0x27}, //for preview // 0x57 for capture
   {0xbf,0x25}, //for preview // 0x55 for capture
   {0xc0,0x2a}, //for preview // 0x5a for capture
   {0xc1,0x27}, //for preview // 0x57 for capture
		   
   {0x03, 0x18},
   {0x10, 0x00}, //scale off		 
		   
   {0x03,0x20},
   {0x2a,0xff}, 														  
   {0x2b,0x34}, 														  
   {0x2c,0xc2}, 														  
   {0x30,0x78},

  // {0x47,0x00}, 
   
   //{0x86,0x02},
   //{0x87,0x00},
																			  
   {0x88, 0x02}, //EXP Max 20.00 fps 
   {0x89, 0x49}, 
   {0x8a, 0xf0}, 
   
   //{0x8b,0x75},
   //{0x8c,0x30},
   //{0x8d,0x61},
   //{0x8e,0x00},
   
   //{0x9c,0x18},
   //{0x9d,0x00},
   
   //{0x9e,0x02},
   //{0x9f,0x00},
						   
   {0x03,0x20}, 		 
   {0x10,0x9c}, //AE ON 
   {0x03,0x22}, 		 
   {0x10,0xeb}, //AWB ON
				   
   {0x03,0x00}, //100412
   {0x0e,0x03}, //20091015 3)PLL off
   {0x0e,0x73}, //20091015 4)PLL x2, on
					  
   {0x03,0x00}, //dummy1	 
   {0x03,0x00}, //dummy2		 
   {0x03,0x00}, //dummy3
   {0x03,0x00}, //dummy4	 
   {0x03,0x00}, //dummy5		 
   {0x03,0x00}, //dummy6	
   {0x03,0x00}, //dummy7	 
   {0x03,0x00}, //dummy8		 
   {0x03,0x00}, //dummy9
   {0x03,0x00}, //dummy10
					  
   {0x01,0x70}, //0x70},//Sleep Off

};

struct hi253_i2c_reg_conf hi253_reg_video_15fps_tab[] =
{
};

struct hi253_i2c_reg_conf hi253_reg_video_30fps_tab[] =
{};

struct hi253_i2c_reg_conf hi253_reg_uxga_tab[] = 	
{	
	//UXGA capture

	{0x03,0x00},										   
	{0x01,0x51}, //0x71},//sleep On								
	{0x0e,0x03},//20091015 pll off  
																					
	{0x03,0x20},//Page 20								   
	{0x10,0x0c},//AE OFF								   
	{0x03,0x22},//Page 22								   
	{0x10,0x69},//AWB OFF		
			
	{0x03,0x00},										   
	{0x10,0x08},
	{0x12,0x05}, //pclk divider//0x04},//	  
		
	{0x20,0x00},
	{0x21,0x0c}, //0x08},
	{0x22,0x00},
	{0x23,0x08}, //0x15},						
		 
		
	{0x03,0x11},
	{0x3e,0x80}, //for capture
	{0x44,0x80}, //for capture //dark1 ratio
	{0x55,0x17}, //0418 0x0d}, //for capture //inddark1 skip th that over
	{0x5b,0x3f}, //for capture //impulse_LPF
	{0x62,0x10}, //for capture
		
	{0x03,0x12},
	{0x25,0x30}, //for capture
	{0x30,0x50}, //110126 noise improve//0x30}, //for capture
	{0x31,0x38}, //for capture
	{0x32,0x42}, //for capture
	{0x33,0x60}, //for capture
	{0x34,0x70}, //for capture
	{0x35,0x80}, //for capture
	{0x4e,0x90}, //0418 0xb0}, //for capture
	{0x4f,0x90}, //0418 0xb0}, //for capture
	{0x50,0x90}, //0418 0xc0}, //for capture

				
	{0x03,0x13},
	{0x12,0x06}, //for capture
	{0x5c,0x0d}, //for capture
	{0x5d,0x0a}, //for capture
	{0x5f,0x0d}, //for capture
	{0x60,0x0c}, //for capture
	{0x61,0x0b}, //for capture
	{0x80,0xfd}, //0418//for capture
	{0x82,0x0a}, //for capture
	{0x94,0x05}, //110126 noise improve//0x13}, //for capture
	{0x95,0x18}, //110126 noise improve//0x54}, //for capture
	{0xa4,0x01}, //for capture
	{0xa5,0x01}, //for capture
	{0xbc,0x27}, //0418 0x55}, //for capture
	{0xbd,0x30}, //0418 0x5a}, //for capture
	{0xbe,0x2a}, //0418 0x57}, //for capture
	{0xbf,0x27}, //0418 0x55}, //for capture
	{0xc0,0x30}, //0418 0x5a}, //for capture
	{0xc1,0x2a}, //0418 0x57}, //for capture

		
		//{0x03, 0x18},
		//{0x10, 0x00}, //0x07},

		
	{0x03,0x00},//20091015 							   
	{0x0e,0x03},//20091015 3)PLL off					   
	{0x0e,0x73},//20091015 4)PLL x2, on  
				
	{0x03,0x00},//dummy1	 
	{0x03,0x00},//dummy2		 
	{0x03,0x00},//dummy3
	{0x03,0x00},//dummy4	 
	{0x03,0x00},//dummy5		 
	{0x03,0x00},//dummy6	
	{0x03,0x00},//dummy7	 
	{0x03,0x00},//dummy8		 
	{0x03,0x00},//dummy9
	{0x03,0x00},//dummy10				   
				
	{0x03,0x00},//Page 0
	{0x01,0x50},//0x70},//Sensor sleep off, Output Drivability Max	
};


struct hi253_i2c_reg_conf hi253_reg_flip_mirror_off[] = 
{
	//mirror b[0]->x, flip  b[1]->y 
	{0x03, 0x00},
	{0x11, 0x90},
	{0x22, 0x00},
	{0x23, 0x06},
};

struct hi253_i2c_reg_conf hi253_reg_mirror[] = 
{
	//mirror b[0]->x, flip  b[1]->y 
	{0x03, 0x00},
	{0x11, 0x91},
	{0x22, 0x00},
	{0x23, 0x02},
};

struct hi253_i2c_reg_conf hi253_reg_flip[] = 
{
	//mirror b[0]->x, flip  b[1]->y 
	{0x03, 0x00},
	{0x11, 0x92},
	{0x22, 0x00},
	{0x23, 0x06},
};

struct hi253_i2c_reg_conf hi253_reg_flip_mirror[] = 
{
	//mirror b[0]->x, flip  b[1]->y 
	{0x03, 0x00},
	{0x11, 0x93},
	{0x22, 0x00},
	{0x23, 0x02},
};

struct hi253_i2c_reg_conf hi253_reg_evoff_n2ev[] =
{
	/* -2.0EV */
	{0x03, 0x10},
	{0x13, 0x0a},
	{0x4a, 0x40},
};

struct hi253_i2c_reg_conf hi253_reg_evoff_n1ev[] =
{	
	/* -1.0EV */
	{0x03, 0x10},
	{0x13, 0x0a},
	{0x4a, 0x60},
};

struct hi253_i2c_reg_conf hi253_reg_evoff_0ev[] =
{
	/* default */
	{0x03, 0x10},
	{0x13, 0x0a},
	{0x4a, 0x80},
};

struct hi253_i2c_reg_conf hi253_reg_evoff_1ev[] =
{
	/* 1.0EV */
	{0x03, 0x10},
	{0x13, 0x0a},
	{0x4a, 0xa0},
};

struct hi253_i2c_reg_conf hi253_reg_evoff_2ev[] =
{
	/* 2.0EV */
	{0x03, 0x10},
	{0x13, 0x0a},
	{0x4a, 0xc0},
};

struct hi253_i2c_reg_conf hi253_reg_wb_auto[] =
{
	/* Auto: */
	{0x03, 0x22},			
	{0x11, 0x2e},
	{0x80, 0x2d},
	{0x82, 0x42},
	{0x83, 0x50},
	{0x84, 0x10},
	{0x85, 0x60},
	{0x86, 0x23},			
};

struct hi253_i2c_reg_conf hi253_reg_wb_sunny[] =
{
	/* Sunny: *///riguang
	{0x03, 0x22},
	{0x11, 0x28},		  
	{0x80, 0x59},
	{0x82, 0x29},
	{0x83, 0x60},
	{0x84, 0x50},
	{0x85, 0x2f},
	{0x86, 0x23},
};

struct hi253_i2c_reg_conf hi253_reg_wb_cloudy[] =
{
	/* Cloudy: *///yintian
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x71},
	{0x82, 0x2b},
	{0x83, 0x72},
	{0x84, 0x70},
	{0x85, 0x2b},
	{0x86, 0x28},
};

struct hi253_i2c_reg_conf hi253_reg_wb_office[] =
{
	/* Office: *///yingguang
	{0x03, 0x22},
	{0x11, 0x28},
	{0x80, 0x41},
	{0x82, 0x42},
	{0x83, 0x44},
	{0x84, 0x34},
	{0x85, 0x46},
	{0x86, 0x3a},
};

struct hi253_i2c_reg_conf hi253_reg_wb_home[] =
{
	/* Home: *///baizhideng
	{0x03, 0x22},
	{0x11, 0x28},		  
	{0x80, 0x29},
	{0x82, 0x54},
	{0x83, 0x2e},
	{0x84, 0x23},
	{0x85, 0x58},
	{0x86, 0x4f},
};

struct hi253_i2c_reg_conf hi253_reg_contrast_4[] =
{
	/* contrast +2: */
	{0x03, 0x10},
	{0x48, 0xa4},

};

struct hi253_i2c_reg_conf hi253_reg_contrast_3[] =
{
	/* contrast +1: */
	{0x03, 0x10},
	{0x48, 0x94},

};

struct hi253_i2c_reg_conf hi253_reg_contrast_2[] =
{
	/* contrast 0: */
	{0x03, 0x10},
	{0x48, 0x84},

};

struct hi253_i2c_reg_conf hi253_reg_contrast_1[] =
{
	/* contrast -1: */
	{0x03, 0x10},
	{0x48, 0x74},

};

struct hi253_i2c_reg_conf hi253_reg_contrast_0[] =
{
	/* contrast -2: */
	{0x03, 0x10},
	{0x48, 0x64},

};

struct hi253_i2c_reg_conf hi253_reg_brightness_4[] =
{
	/* Brightness +2: */
	{0x03, 0x10},
	{0x40, 0x40},
};

struct hi253_i2c_reg_conf hi253_reg_brightness_3[] =
{
	/* Brightness +1: */
	{0x03, 0x10},
	{0x40, 0x20},
};

struct hi253_i2c_reg_conf hi253_reg_brightness_2[] =
{
	/* Brightness 0: */
	{0x03, 0x10},
	{0x40, 0x03},
};

struct hi253_i2c_reg_conf hi253_reg_brightness_1[] =
{
	/* Brightness -1: */
	{0x03, 0x10},
	{0x40, 0xa0},
};

struct hi253_i2c_reg_conf hi253_reg_brightness_0[] =
{
	/* Brightness -2: */
	{0x03, 0x10},
	{0x40, 0xd0},
};


struct hi253_i2c_reg_conf hi253_reg_effect_normal[] =
{
	/* normal: */
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x30},
	{0x13, 0x0a},
	{0x44, 0x80},
	{0x45, 0x80},
};

struct hi253_i2c_reg_conf hi253_reg_effect_black_white[] =
{
	/* B&W: */
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x03},
	{0x13, 0x0a},
	{0x40, 0x00},
	{0x44, 0x80},
	{0x45, 0x80},
};

struct hi253_i2c_reg_conf hi253_reg_effect_negative[] =
{
	/* Negative: */
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x08},
	{0x13, 0x0a},
	{0x14, 0x00},
};

struct hi253_i2c_reg_conf hi253_reg_effect_old_movie[] =
{
	/* Sepia(antique): */
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x33},
	{0x13, 0x0a},
	{0x44, 0x25},
	{0x45, 0xa6},
};

struct hi253_i2c_reg_conf hi253_reg_effect_bluish[] =
{
	/* Bluish: */
	{0x03, 0x10},
	{0x11, 0x03},
	{0x12, 0x03},
	{0x40, 0x00},
	{0x13, 0x0a},
	{0x44, 0xb0},
	{0x45, 0x40},
};


/************************************************************************

			general function

************************************************************************/
static int hi253_reset(const struct msm_camera_sensor_info *dev)
{
	int rc = 0;

	rc = gpio_request(dev->sensor_reset, "hi253");

	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 0);
		mdelay(5);
		rc = gpio_direction_output(dev->sensor_reset, 1);
	}

	gpio_free(dev->sensor_reset);
	return rc;
}

/************************************************************************

			general function

************************************************************************/
static int hi253_i2c_rxdata(unsigned short saddr, unsigned char *rxdata,
	int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr   = saddr,
			.flags = 0,
			.len   = 1,
			.buf   = rxdata,
		},
		{
			.addr   = saddr,
			.flags = I2C_M_RD,
			.len   = length,
			.buf   = rxdata,
		},
	};

	if (i2c_transfer(hi253_client->adapter, msgs, 2) < 0) {
		CDBG("hi253_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int hi253_i2c_txdata(unsigned short saddr,
	unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
		.addr  = saddr,
		.flags = 0,
		.len = length,
		.buf = txdata,
		},
	};

	if (i2c_transfer(hi253_client->adapter, msg, 1) < 0) {
		CDBG("hi253_i2c_txdata failed\n");
		return -EIO;
	}
	return 0;
}

static int hi253_i2c_write_b(unsigned short saddr, unsigned char baddr,
	unsigned char bdata)
{
	int rc = 0;
	unsigned char buf[3];

	memset(buf, 0, sizeof(buf));
	buf[0] = baddr;
	buf[1] = bdata;

	rc = hi253_i2c_txdata(saddr, buf, 2);

	if (rc < 0)
		printk("i2c_write_w failed, addr = 0x%x, val = 0x%x!\n", baddr, bdata);

	return rc;
}

static int hi253_i2c_write_table(struct hi253_i2c_reg_conf *reg_cfg_tbl, int num)
{
	int i;
	int rc = 0;
	for (i = 0; i < num; i++) 
	{
		rc = hi253_i2c_write_b(hi253_client->addr, 
				reg_cfg_tbl->baddr, reg_cfg_tbl->bdata);		
		if (rc < 0) 
		{
			printk("##################hi253_i2c_write_table %d!###############\n", i);
			//break;
			rc = hi253_i2c_write_b(hi253_client->addr,
					reg_cfg_tbl->baddr, reg_cfg_tbl->bdata);
		}
		reg_cfg_tbl++;
	}

//	return rc;
	return 0;
}

static int hi253_i2c_read(unsigned short saddr, unsigned char raddr,
	unsigned char *rdata)
{
	int rc = 0;

	if (!rdata)
		return -EIO;

	rc = hi253_i2c_rxdata(saddr, &raddr, 1);
	if (rc < 0)
		return rc;

	*rdata = raddr;

	if (rc < 0)
		CDBG("hi253_i2c_read failed!\n");

	return rc;
}

struct hi253_proc_t {
	unsigned int i2c_addr;
	unsigned int i2c_data;
};

#define HI253_PROC_NAME "hi253"
#define HI253_PROC_LEN 16
static struct proc_dir_entry *hi253_dir, *s_file, *w_file, *dr_file;
static char hi253_proc_buffer[HI253_PROC_LEN];
static struct hi253_proc_t hi253_proc_dt = {
	.i2c_addr = 0,
	.i2c_data = 0,
};

static int proc_hi253_dread(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len, rc;
	unsigned char tmp;

	rc = hi253_i2c_read(hi253_client->addr, hi253_proc_dt.i2c_addr, &tmp);
	if (rc < 0) {
		len = sprintf(page, " 0x%x@hi253_i2c read error\n", hi253_proc_dt.i2c_addr);
	} else {
		len = sprintf(page, " 0x%x@hi253_i2c = %4x\n", hi253_proc_dt.i2c_addr, tmp);
	}
	return len;
}

static int proc_hi253_sread(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len, rc = 0;
	unsigned char tmp8;

	rc = hi253_i2c_read(hi253_client->addr, hi253_proc_dt.i2c_addr, &tmp8);
	if (rc < 0) {
		len = sprintf(page, " byte 0x%x@hi253_i2c read error\n", hi253_proc_dt.i2c_addr);
	} else {
		len = sprintf(page, " byte 0x%x@hi253_i2c = %4x\n", hi253_proc_dt.i2c_addr, tmp8);
	}

	return len;
}

static int proc_hi253_swrite(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int len;

	memset(hi253_proc_buffer, 0, HI253_PROC_LEN);
	if (count > HI253_PROC_LEN)
		len = HI253_PROC_LEN;
	else
		len = count;

	//printk("count = %d\n", len);
	if (copy_from_user(hi253_proc_buffer, buffer, len))
		return -EFAULT;
	//printk("%s\n", hi253_proc_buffer);
	sscanf(hi253_proc_buffer, "%x", &hi253_proc_dt.i2c_data);
	//printk("received %x\n", hi253_proc_dt.i2c_addr);
	hi253_i2c_write_b(hi253_client->addr, hi253_proc_buffer[0], hi253_proc_buffer[1]);		

	return len;
}

static int proc_hi253_addr_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len;

	len = sprintf(page, "addr is 0x%x\n", hi253_proc_dt.i2c_addr);
	return len;
}

static int proc_hi253_addr_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int len;

	memset(hi253_proc_buffer, 0, HI253_PROC_LEN);
	if (count > HI253_PROC_LEN)
		len = HI253_PROC_LEN;
	else
		len = count;

	//printk("count = %d\n", len);
	if (copy_from_user(hi253_proc_buffer, buffer, len))
		return -EFAULT;
	//printk("%s\n", hi253_proc_buffer);
	sscanf(hi253_proc_buffer, "%x", &hi253_proc_dt.i2c_addr);
	//printk("received %x\n", hi253_proc_dt.i2c_addr);
	return len;

}

static int hi253_add_proc(void)
{
	int rc;

	/* add for proc*/
	/* create directory */
	hi253_dir = proc_mkdir(HI253_PROC_NAME, NULL);
	if (hi253_dir == NULL) {
		rc = -ENOMEM;
		goto init_fail;
	}
	//hi253_dir->owner = THIS_MODULE;

	/* create readfile */
	s_file = create_proc_entry("sbdata", 0644, hi253_dir);
	if (s_file == NULL) {
		rc = -ENOMEM;
		goto no_s;
	}
	s_file->read_proc = proc_hi253_sread;
	s_file->write_proc = proc_hi253_swrite;
	//s_file->owner = THIS_MODULE;

	dr_file = create_proc_read_entry("double_byte_read", 0444, hi253_dir, proc_hi253_dread, NULL);
	if (dr_file == NULL) {
		rc = -ENOMEM;
		goto no_dr;
	}
	//dr_file->owner = THIS_MODULE;

	/* create write file */
	w_file = create_proc_entry("i2c_addr", 0644, hi253_dir);
	if (w_file == NULL) {
		rc = -ENOMEM;
		goto no_wr;
	}

	w_file->read_proc = proc_hi253_addr_read;
	w_file->write_proc = proc_hi253_addr_write;
	//w_file->owner = THIS_MODULE;

	/* OK, out debug message */
	//printk(KERN_INFO "%s %s %s initialised\n", SINGLE_OP_NAME, DOUBLE_READ_NAME, I2C_ADDR_NAME);

	/*litao add end*/
	return 0;
	/*litao add for proc*/
no_wr:
	remove_proc_entry("double_byte_read", hi253_dir);
no_dr:
	remove_proc_entry("sbdata", hi253_dir);
no_s:
	remove_proc_entry(HI253_PROC_NAME, NULL);
	/*litao add end*/
init_fail:
	return 1;

}

static int hi253_del_proc(void)
{
	remove_proc_entry("i2c_addr", hi253_dir);
	remove_proc_entry("double_byte_read", hi253_dir);
	remove_proc_entry("sbdata", hi253_dir);
	remove_proc_entry(HI253_PROC_NAME, NULL);
	//printk(KERN_INFO "%s %s %s removed\n", SINGLE_OP_NAME, DOUBLE_READ_NAME, I2C_ADDR_NAME);
	return 0;
}
static int hi253_version_revision(unsigned char *pid)
{
	hi253_i2c_read(hi253_client->addr, HI253_PID_REG, pid);

	printk("****************************************************\n");
	printk("The value of hi253 ID is:pid=%x\n", *pid);
	printk("****************************************************\n");
	
	return 0;
}

static int hi253_reg_init(void)
{
	int rc = 0;
	HI253_DBG(KERN_INFO "hi253: sensor init\n");

	rc = hi253_i2c_write_table(&hi253_reg_init_tab[0],
			ARRAY_SIZE(hi253_reg_init_tab));
	if (rc < 0)
		return rc;
	
	HI253_DBG(KERN_INFO "hi253: sensor init end\n");
	
	return 0;
}


/*****************************************************************************

                 Settings
 
******************************************************************************/
static int hi253_set_effect(int value)
{
	int rc = 0;
	HI253_DBG("%s  value=%d\n", __func__, value);
	switch(value) {
	case CAMERA_EFFECT_OFF:
		rc = hi253_i2c_write_table(&hi253_reg_effect_normal[0],
		        ARRAY_SIZE(hi253_reg_effect_normal));
		break;
	case CAMERA_EFFECT_MONO:
		rc = hi253_i2c_write_table(&hi253_reg_effect_black_white[0],
		        ARRAY_SIZE(hi253_reg_effect_black_white));
		break;
	case CAMERA_EFFECT_NEGATIVE:
		rc = hi253_i2c_write_table(&hi253_reg_effect_negative[0],
		        ARRAY_SIZE(hi253_reg_effect_negative));
		break;
/*
	case CAMERA_EFFECT_BLUISH:
		rc = hi253_i2c_write_table(&hi253_reg_effect_bluish[0],
		        ARRAY_SIZE(hi253_reg_effect_bluish));
		break;
*/
	case CAMERA_EFFECT_SEPIA:
		rc = hi253_i2c_write_table(&hi253_reg_effect_old_movie[0],
		        ARRAY_SIZE(hi253_reg_effect_old_movie));
		break;
	default:
		rc = hi253_i2c_write_table(&hi253_reg_effect_normal[0],
		        ARRAY_SIZE(hi253_reg_effect_normal));
		break;
	}

	current_special_effect = value;

	return rc;
}

/*
static int hi253_set_contrast(int value)
{
	int rc = 0;
	HI253_DBG("%s  value=%d\n", __func__, value);
	switch (value) {
	case CAMERA_CONTRAST_LOWEST:
		rc = hi253_i2c_write_table(&hi253_reg_contrast_0[0],
		        ARRAY_SIZE(hi253_reg_contrast_0));
		break;
	case CAMERA_CONTRAST_LOW:
		rc = hi253_i2c_write_table(&hi253_reg_contrast_1[0],
		        ARRAY_SIZE(hi253_reg_contrast_1));
		break;
	case CAMERA_CONTRAST_MIDDLE:
		rc = hi253_i2c_write_table(&hi253_reg_contrast_2[0],
		        ARRAY_SIZE(hi253_reg_contrast_2));
		break;
	case CAMERA_CONTRAST_HIGH:
		rc = hi253_i2c_write_table(&hi253_reg_contrast_3[0],
		        ARRAY_SIZE(hi253_reg_contrast_3));
		break;
	case CAMERA_CONTRAST_HIGHEST:
		rc = hi253_i2c_write_table(&hi253_reg_contrast_4[0],
		        ARRAY_SIZE(hi253_reg_contrast_4));
		break;
	default:
		rc = hi253_i2c_write_table(&hi253_reg_contrast_2[0],
		        ARRAY_SIZE(hi253_reg_contrast_2));
		 break;
	}

	if (current_special_effect != -1) {
        hi253_set_effect(current_special_effect);
	}

	return rc;
}
*/

static int hi253_set_brightness(int value)
{
    int rc = 0;
	HI253_DBG("%s  value=%d\n", __func__, value);
	switch (value) {
	case 0:
	case 1:
		rc = hi253_i2c_write_table(&hi253_reg_brightness_0[0],
		        ARRAY_SIZE(hi253_reg_brightness_0));
		break;
	case 2:
	case 3: 
		rc = hi253_i2c_write_table(&hi253_reg_brightness_2[0],
		        ARRAY_SIZE(hi253_reg_brightness_2));
		break;
	case 4:
	case 5:
		rc = hi253_i2c_write_table(&hi253_reg_brightness_2[0],
		        ARRAY_SIZE(hi253_reg_brightness_2));
		break;
	case 6:
		rc = hi253_i2c_write_table(&hi253_reg_brightness_3[0],
		        ARRAY_SIZE(hi253_reg_brightness_3));
		break;
	case 7:
	case 8:
		rc = hi253_i2c_write_table(&hi253_reg_brightness_4[0],
		        ARRAY_SIZE(hi253_reg_brightness_4));
		break;
	default:
		rc = hi253_i2c_write_table(&hi253_reg_brightness_2[0],
		        ARRAY_SIZE(hi253_reg_brightness_2));
		 break;
	}

	if (current_special_effect != -1) {
		hi253_set_effect(current_special_effect);
	}

	return rc;
}

/*
static int hi253_set_exposure(int value)
{
    int rc = 0;
	HI253_DBG("%s  value=%d\n", __func__, value);
	switch(value) {
	case CAMERA_EXPOSURE_LOWEST:
		//-2.0EV
		rc = hi253_i2c_write_table(&hi253_reg_evoff_n2ev[0],
		        ARRAY_SIZE(hi253_reg_evoff_n2ev));
		break;
	case CAMERA_EXPOSURE_LOW:
		//-1.0EV 
		rc = hi253_i2c_write_table(&hi253_reg_evoff_n1ev[0],
		        ARRAY_SIZE(hi253_reg_evoff_n1ev));
		break;
	case CAMERA_EXPOSURE_MIDDLE:
		//0EV 
		rc = hi253_i2c_write_table(&hi253_reg_evoff_0ev[0],
		        ARRAY_SIZE(hi253_reg_evoff_0ev));
		break;
	case CAMERA_EXPOSURE_HIGH:
		//1.0EV 
		rc = hi253_i2c_write_table(&hi253_reg_evoff_1ev[0],
		        ARRAY_SIZE(hi253_reg_evoff_1ev));
		break;
	case CAMERA_EXPOSURE_HIGHEST:
		//2.0EV 
		rc = hi253_i2c_write_table(&hi253_reg_evoff_2ev[0],
		        ARRAY_SIZE(hi253_reg_evoff_2ev));
		break;
	default:
		rc = hi253_i2c_write_table(&hi253_reg_evoff_0ev[0],
		        ARRAY_SIZE(hi253_reg_evoff_0ev));
		break;
	}
	return rc;
}
*/

static int hi253_set_white_balance(int value)
{
	int rc = 0;
	HI253_DBG("%s  value=%d\n", __func__, value);
	switch((camera_wb_type)value) {
	case CAMERA_WB_AUTO:			/* Auto */
		rc = hi253_i2c_write_table(&hi253_reg_wb_auto[0],
		        ARRAY_SIZE(hi253_reg_wb_auto));
		break;

	case CAMERA_WB_INCANDESCENT:			/* Incandescent */
		rc = hi253_i2c_write_table(&hi253_reg_wb_home[0],
		        ARRAY_SIZE(hi253_reg_wb_home));
		break;

	case CAMERA_WB_DAYLIGHT:			/* Sunny */
		rc = hi253_i2c_write_table(&hi253_reg_wb_sunny[0],
		        ARRAY_SIZE(hi253_reg_wb_sunny));
		break;

	case CAMERA_WB_FLUORESCENT: 		/* Fluorescent */
		rc = hi253_i2c_write_table(&hi253_reg_wb_office[0],
		        ARRAY_SIZE(hi253_reg_wb_office));
		break;

	case CAMERA_WB_CLOUDY_DAYLIGHT:			/* ext: Cloudy */
		rc = hi253_i2c_write_table(&hi253_reg_wb_cloudy[0],
		        ARRAY_SIZE(hi253_reg_wb_cloudy));
		break;
	default:
		rc = hi253_i2c_write_table(&hi253_reg_wb_auto[0],
		        ARRAY_SIZE(hi253_reg_wb_auto));
		break;
	}	
	return rc;
}

/*
static int hi253_set_flip_mirror(int flip, int mirror)
{
    int rc = 0;
	HI253_DBG("%s  flip=%d, mirror=%d\n", __func__, flip, mirror);
	
	if ((1 == flip) && (1 == mirror)) {
		rc = hi253_i2c_write_table(&hi253_reg_flip_mirror[0],
			    ARRAY_SIZE(hi253_reg_flip_mirror));	
	}
	else if ((1 == flip) && (0 == mirror)) {
		rc = hi253_i2c_write_table(&hi253_reg_flip[0],
			    ARRAY_SIZE(hi253_reg_flip));
	}
	else if ((0 == flip) && (1 == mirror)) {
		rc = hi253_i2c_write_table(&hi253_reg_mirror[0],
			    ARRAY_SIZE(hi253_reg_mirror));
	}
	else {
		rc = hi253_i2c_write_table(&hi253_reg_flip_mirror_off[0],
			    ARRAY_SIZE(hi253_reg_flip_mirror_off));
	}
	
	return rc;
}
*/

unsigned int HI253_pv_HI253_exposure_lines=0x015f00;
int g_snapshot;

static int hi253_set_sensor_mode(int mode)
{
	int rc = 0;
	//unsigned char reg;

	unsigned int HI253_cp_HI253_exposure_lines;
	unsigned char reg1;
	unsigned char reg2;
	unsigned char reg3;
	//unsigned char reg11;
	//unsigned char reg22;
	//unsigned char reg33;

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		HI253_DBG("hi253_set_sensor_mode preview begin\n");
		HI253_DBG("ep=%x, g_snapshot=%d\n",HI253_pv_HI253_exposure_lines,g_snapshot);
		if(g_snapshot&&(HI253_pv_HI253_exposure_lines>0xffff)) {
			HI253_DBG("--------------------reset exposure value---------------------\n");
			hi253_i2c_write_b(hi253_client->addr, 0x03, 0x20);
			hi253_i2c_write_b(hi253_client->addr, 0x83, HI253_pv_HI253_exposure_lines >> 16);	
			hi253_i2c_write_b(hi253_client->addr, 0x84, (HI253_pv_HI253_exposure_lines >> 8) & 0x000000FF);			
			hi253_i2c_write_b(hi253_client->addr, 0x85, HI253_pv_HI253_exposure_lines & 0x000000FF);
		}
		rc = hi253_i2c_write_table(&hi253_reg_video_base_tab[0],
		ARRAY_SIZE(hi253_reg_video_base_tab));

		//rc = hi253_i2c_write_table(&hi253_reg_flip_mirror_on[0],  //flip and mirror
		//ARRAY_SIZE(hi253_reg_flip_mirror_on));

		HI253_DBG("hi253_set_sensor_mode preview end\n");

		break;
	case SENSOR_SNAPSHOT_MODE:
		HI253_DBG("hi253_set_sensor_mode snapshot begin\n");

		//msleep(150);

		hi253_i2c_write_b(hi253_client->addr, 0x03, 0x20);
		hi253_i2c_write_b(hi253_client->addr, 0x10, 0x1C);

		hi253_i2c_read(hi253_client->addr, 0x80, &reg1);
		hi253_i2c_read(hi253_client->addr, 0x81, &reg2);
		hi253_i2c_read(hi253_client->addr, 0x82, &reg3);
		HI253_DBG("reg1=0x%x\n", reg1);
		HI253_DBG("reg2=0x%x\n", reg2);
		HI253_DBG("reg3=0x%x\n", reg3);
		HI253_pv_HI253_exposure_lines =(reg1 << 16)|(reg2<< 8)|reg3;
		HI253_DBG("HI253_pv_HI253_exposure_lines=0x%x\n", HI253_pv_HI253_exposure_lines);

		HI253_cp_HI253_exposure_lines = HI253_pv_HI253_exposure_lines >> 1;
		HI253_DBG("HI253_pv_HI253_exposure_lines=0x%x\n", HI253_cp_HI253_exposure_lines);

		if(HI253_cp_HI253_exposure_lines<1)
		HI253_cp_HI253_exposure_lines=1;

		g_snapshot = 1;
		hi253_i2c_write_b(hi253_client->addr, 0x03, 0x20);
		hi253_i2c_write_b(hi253_client->addr, 0x83, HI253_cp_HI253_exposure_lines >> 16);	
		hi253_i2c_write_b(hi253_client->addr, 0x84, (HI253_cp_HI253_exposure_lines >> 8) & 0x000000FF);		
		hi253_i2c_write_b(hi253_client->addr, 0x85, HI253_cp_HI253_exposure_lines & 0x000000FF);	

		hi253_i2c_write_table(&hi253_reg_uxga_tab[0],ARRAY_SIZE(hi253_reg_uxga_tab));

		msleep(150);
		HI253_DBG("hi253_set_sensor_mode snapshot end\n");
		break;
	default:
		return -EINVAL;
	}

	return rc;
}

static int hi253_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	unsigned char cm_rev, cm_pid;
	int timeout;
	int status;

	CDBG("init entry \n");

	rc = gpio_request(data->sensor_pwd, "hi253");

	if (!rc) {
		rc = gpio_direction_output(data->sensor_pwd, 0);
	}

	gpio_free(data->sensor_pwd);

	rc = hi253_reset(data);
	if (rc < 0) {
		CDBG("reset failed!\n");
		goto init_probe_fail;
	}

	msleep(10);
	/* read out version */
	timeout = 3;
	do
	{
		cm_pid = cm_rev = 0;
		status = hi253_version_revision(&cm_pid);

		/* Check to make sure we are working with an hi253 */
		if (cm_pid != PID_HI253) {
			HI253_DBG("hi253: pid(%x) is not correct, try again!\n", cm_pid);
			msleep(10);
			rc = hi253_reset(data);
		}
		if (--timeout == 0) {
			return -EIO;
		}
	} while(cm_pid != PID_HI253);

	if (rc < 0)
		goto init_probe_fail;

	rc = hi253_reg_init();

	return rc;

init_probe_fail:
	return rc;
}

static int hi253_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	hi253_ctrl = kzalloc(sizeof(struct hi253_ctrl), GFP_KERNEL);
	if (!hi253_ctrl) {
		CDBG("hi253_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		hi253_ctrl->sensordata = data;

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	mdelay(5);

	msm_camio_camif_pad_reg_reset();

	rc = hi253_sensor_init_probe(data);
	if (rc < 0) {
		CDBG("hi253_sensor_init failed!\n");
		goto init_fail;
	}

	hi253_add_proc();

init_done:
	return rc;

init_fail:
	kfree(hi253_ctrl);
	return rc;
}

static int hi253_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&hi253_wait_queue);
	return 0;
}

static int hi253_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	int rc = 0;

	if (copy_from_user(&cfg_data,
			(void *)argp,
			sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	/* down(&hi253_sem); */

	CDBG("hi253_ioctl, cfgtype = %d, mode = %d\n",
		cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = hi253_set_sensor_mode(cfg_data.mode);
		break;

	case CFG_SET_EFFECT:
		rc = hi253_set_effect(cfg_data.cfg.effect);
		break;

	case CFG_SET_BRIGHTNESS:
		rc = hi253_set_brightness(cfg_data.cfg.value);
		break;
/*
	case CFG_SET_CONTRAST:
		rc = hi253_set_contrast(cfg_data.cfg.contrast);
		break;
*/
	case CFG_SET_WB:
		rc = hi253_set_white_balance(cfg_data.cfg.value);
		break;
/*
	case CFG_SET_EXP_GAIN:
		rc = hi253_set_exposure(cfg_data.cfg.exposure);
		break;
*/
/*
	case CFG_SET_FLIP_MIRROR:
		rc = hi253_set_flip_mirror(cfg_data.flip, cfg_data.mirror);
		break;
*/
	case CFG_GET_AF_MAX_STEPS:
	default:
		rc = -EINVAL;
		break;
	}

	/* up(&hi253_sem); */

	return rc;
}

static int hi253_sensor_release(void)
{
	int rc = 0;

	/* down(&hi253_sem); */

	kfree(hi253_ctrl);
	/* up(&hi253_sem); */

	hi253_del_proc();

	return rc;
}

static int __devexit hi253_i2c_remove(struct i2c_client *client)
{
	hi253_client = NULL;
	kfree(hi253_sensorw);
	hi253_sensorw = NULL;
	return 0;
}

static int __devinit hi253_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	hi253_sensorw =
		kzalloc(sizeof(struct hi253_work), GFP_KERNEL);

	if (!hi253_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, hi253_sensorw);
	hi253_init_client(client);
	hi253_client = client;

	CDBG("hi253_probe succeeded!\n");

	return 0;

probe_failure:
	kfree(hi253_sensorw);
	hi253_sensorw = NULL;
	CDBG("hi253_probe failed!\n");
	return rc;
}

static const struct i2c_device_id hi253_i2c_id[] = {
	{ "hi253", 0},
	{ },
};

static struct i2c_driver hi253_i2c_driver = {
	.id_table = hi253_i2c_id,
	.probe  = hi253_i2c_probe,
	.remove = hi253_i2c_remove,
	.driver = {
		.name = "hi253",
	},
};

static int hi253_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	int rc = i2c_add_driver(&hi253_i2c_driver);

	if (rc < 0 || hi253_client == NULL) {
		rc = -ENOTSUPP;
		goto i2c_probe_error;
	}

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	mdelay(5);

	rc = hi253_sensor_init_probe(info);
	if (rc < 0)
		goto sensor_init_error;

	s->s_init = hi253_sensor_init;
	s->s_release = hi253_sensor_release;
	s->s_config  = hi253_sensor_config;
	s->s_mount_angle  = 90;
	s->s_camera_type = BACK_CAMERA_2D;

	CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;

sensor_init_error:
	i2c_del_driver(&hi253_i2c_driver);
i2c_probe_error:
	printk("XXX HI253 sensor probe error!!! XXX\n");

	return rc;
}

static int __devinit __hi253_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, hi253_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __hi253_probe,
	.driver = {
		.name = "msm_camera_hi253",
		.owner = THIS_MODULE,
	},
};

static int __init hi253_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(hi253_init);
