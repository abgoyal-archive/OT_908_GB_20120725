/* Copyright (c) 2010, JRD Communication Inc. All rights reserved.
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
 */

#include <linux/delay.h>
#include <mach/gpio.h>
#include "msm_fb.h"
#include <mach/mpp.h>
#include <linux/leds.h>
#include <mach/pmic.h>
#include <linux/workqueue.h>

static int spi_cs;
static int spi_sclk;
static int spi_sdi;


struct ili9338_state_type {
	boolean disp_initialized;
	boolean display_on;
	boolean disp_powered_up;
};
static struct ili9338_state_type ili9338_state = { 0 };

const int ili9338_bk_intensity[6] = {0,12,9,6,5,1};

static struct msm_panel_common_pdata *lcdc_ili9338_pdata;

#define GPIO_LCD_BK 90

static uint32_t set_gpio_bk_table[] = {
	GPIO_CFG(GPIO_LCD_BK, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_2MA)
};

static uint32_t lcdc_gpio_table_sleep[] = {
	GPIO_CFG(132, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	//sclk
	GPIO_CFG(131, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	//cs
	GPIO_CFG(103, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	//sdi
	GPIO_CFG(88, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),        //id
	GPIO_CFG(90, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),	//bk
};

static void config_lcdc_gpio_table(uint32_t *table, int len, unsigned enable)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n],
			enable ? GPIO_CFG_ENABLE : GPIO_CFG_DISABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

static int spi_write(unsigned char index, unsigned char val)
{
	unsigned char sda;
	int i;
	gpio_set_value(spi_cs, 0);

	gpio_set_value(spi_sdi, index);
	gpio_set_value(spi_sclk, 0);
	udelay(0);
	gpio_set_value(spi_sclk, 1);
	udelay(0);

	for (i = 7; i >= 0; i--) {
		sda = ((val >> i) & 0x1);
		gpio_set_value(spi_sdi, sda);
		gpio_set_value(spi_sclk, 0);
		udelay(0);
		gpio_set_value(spi_sclk, 1);
		udelay(0);
	}
	gpio_set_value(spi_cs, 1);

	return 0;
}

static int ili9338_reg_data(unsigned char data)
{
	spi_write(1, data);
	return 0;
}

static int ili9338_reg_cmd(unsigned char cmd)
{
	spi_write(0, cmd);
	return 0;
}

static int spi_init(void)
{
	spi_sclk = *(lcdc_ili9338_pdata->gpio_num);
	spi_cs = *(lcdc_ili9338_pdata->gpio_num + 1);
	spi_sdi = *(lcdc_ili9338_pdata->gpio_num + 2);

	gpio_set_value(spi_sclk, 1);
	gpio_set_value(spi_sdi, 1);

	gpio_set_value(spi_cs, 1);

	return 0;
}

static int ili9338_reset(void)
{
	msleep(10);
	mpp_config_digital_out(13, MPP_CFG(MPP_DLOGIC_LVL_MSME, MPP_DLOGIC_OUT_CTRL_LOW));
	msleep(10);
	mpp_config_digital_out(13, MPP_CFG(MPP_DLOGIC_LVL_MSME, MPP_DLOGIC_OUT_CTRL_HIGH));
	msleep(100);

	return 0;
}

static int ili9338_init(void)
{
	ili9338_reset();

	ili9338_reg_cmd(0x11); //Exit Sleep
	msleep(120);

	ili9338_reg_cmd(0x21);
	msleep(50);

	ili9338_reg_cmd(0x36);
	ili9338_reg_data(0xC8);

	ili9338_reg_cmd(0x3A); //Set Pixel format is 18bit/pixel.
	ili9338_reg_data(0x66);

	ili9338_reg_cmd(0xB0); //signal polarity
	ili9338_reg_data(0x0A);

	ili9338_reg_cmd(0xB1); //frame rate
	ili9338_reg_data(0x00);
	ili9338_reg_data(0x13);

	ili9338_reg_cmd(0xB6);
	ili9338_reg_data(0x0A);
	ili9338_reg_data(0x22);
	ili9338_reg_data(0x27);

	ili9338_reg_cmd(0xB8);
	ili9338_reg_data(0x89);
	ili9338_reg_data(0x11);
	ili9338_reg_data(0x7C);

	ili9338_reg_cmd(0xC0); //Power control
	ili9338_reg_data(0x22); //VRH[5:0]
	ili9338_reg_data(0x00); //VCI1[2:0]

	ili9338_reg_cmd(0xC1); //Power control
	ili9338_reg_data(0x11); //SAP[2:0];BT[3:0]

	ili9338_reg_cmd(0xC2);
	ili9338_reg_data(0x11);

	ili9338_reg_cmd(0xC5);
	ili9338_reg_data(0x3A);
	ili9338_reg_data(0x21);

	ili9338_reg_cmd(0xC7);
	ili9338_reg_data(0xCC);

	ili9338_reg_cmd(0xCB); //AP[2:0]
	ili9338_reg_data(0x01);

	ili9338_reg_cmd(0xE0);   //Set Gamma
	ili9338_reg_data(0x08);    //VP63[3:0]0
	ili9338_reg_data(0x0C);    //VP62[5:0]
	ili9338_reg_data(0x23);    //VP61[5:0]
	ili9338_reg_data(0x06);    //VP59[3:0]
	ili9338_reg_data(0x06);    //VP57[4:0]
	ili9338_reg_data(0x03);    //VP50[3:0]
	ili9338_reg_data(0x37);    //VP43[6:0]
	ili9338_reg_data(0x99);    //VP36[3:0] VP27[3:0]
	ili9338_reg_data(0x1F);    //VP20[6:0]
	ili9338_reg_data(0x0A);    //VP13[3:0]
	ili9338_reg_data(0x16);    //VP6[4:0]
	ili9338_reg_data(0x05);    //VP4[3:0]
	ili9338_reg_data(0x10);    //VP2[5:0]
	ili9338_reg_data(0x0A);    //VP1[5:0]
	ili9338_reg_data(0x00);    //VP0[3:0]

	ili9338_reg_cmd(0xE1);   //Set Gamma
	ili9338_reg_data(0x08);    //VN63[3:0]
	ili9338_reg_data(0x28);    //VN62[5:0]
	ili9338_reg_data(0x33);    //VN61[5:0]
	ili9338_reg_data(0x0A);    //VN59[3:0]
	ili9338_reg_data(0x08);    //VN57[4:0]
	ili9338_reg_data(0x0A);    //VN50[3:0]
	ili9338_reg_data(0x37);    //VN43[6:0]
	ili9338_reg_data(0x39);    //VN36[3:0] VN27[3:0]
	ili9338_reg_data(0x3F);    //VN20[6:0]
	ili9338_reg_data(0x05);    //VN13[3:0]
	ili9338_reg_data(0x09);    //VN6[4:0]
	ili9338_reg_data(0x0A);    //VN4[3:0]
	ili9338_reg_data(0x2F);    //VN2[5:0]
	ili9338_reg_data(0x35);    //VN1[5:0]
	ili9338_reg_data(0x0F);    //VN0[3:0]

	ili9338_reg_cmd(0x2A); //CASET (Column Address Set)
	ili9338_reg_data(0x00);
	ili9338_reg_data(0x00);
	ili9338_reg_data(0x00);
	ili9338_reg_data(0xEF);

	ili9338_reg_cmd(0x2B); //PASET (Page Address Set)
	ili9338_reg_data(0x00);
	ili9338_reg_data(0x00);
	ili9338_reg_data(0x01);
	ili9338_reg_data(0x3F);

	ili9338_reg_cmd(0x29); //display on
	msleep(120);

	return 0;

}

static int ili9338_sleep_enter(void)
{
	ili9338_reg_cmd(0x00);//28
	ili9338_reg_cmd(0x10);
	msleep(120);
	return 0;
}

static int lcdc_ili9338_bk_setting(const int step)
{
	int i;
	static unsigned int nr_pulse;

	printk(KERN_ERR"ILI9338 BL:%d\n",step);

	if(step==0){
		gpio_set_value(GPIO_LCD_BK,0);
		return 0;
		}

	gpio_set_value(GPIO_LCD_BK, 0);
	udelay(500);

	nr_pulse = ili9338_bk_intensity[step];
	if(nr_pulse>12)
		nr_pulse = 12;

	for (i = 0; i < nr_pulse; i++){
		gpio_set_value(GPIO_LCD_BK, 0);
		udelay(10);
		gpio_set_value(GPIO_LCD_BK, 1);
		udelay(10);
	}

	return 0;

}

static void lcdc_ili9338_set_backlight(struct msm_fb_data_type *mfd)
{
	msleep(80);//wait 80ms for lcd stable.
	if (ili9338_state.display_on&&ili9338_state.disp_initialized&&ili9338_state.disp_powered_up)
		lcdc_ili9338_bk_setting(mfd->bl_level);
}

static void ili9338_disp_on(void)
{
	if (ili9338_state.disp_powered_up && !ili9338_state.display_on) {
		ili9338_init();
		ili9338_state.display_on = TRUE;
	}
}

static void ili9338_disp_powerup(void)
{
	if (!ili9338_state.disp_powered_up && !ili9338_state.display_on)
		ili9338_state.disp_powered_up = TRUE;
}

static int lcdc_ili9338_panel_on(struct platform_device *pdev)
{
	if (!ili9338_state.disp_initialized) {
		lcdc_ili9338_pdata->panel_config_gpio(1);
		config_lcdc_gpio_table(set_gpio_bk_table, 1, 1);
		spi_init();
		ili9338_disp_powerup();
		ili9338_disp_on();
		ili9338_state.disp_initialized = TRUE;
	}
	printk(KERN_ERR"lcd ili9338 panel on.\n");
	return 0;
}

static int lcdc_ili9338_panel_off(struct platform_device *pdev)
{
	ili9338_sleep_enter();
	config_lcdc_gpio_table(lcdc_gpio_table_sleep, ARRAY_SIZE(lcdc_gpio_table_sleep), 1);
	mpp_config_digital_out(13, MPP_CFG(MPP_DLOGIC_LVL_MSME, MPP_DLOGIC_OUT_CTRL_LOW));

	if (ili9338_state.disp_powered_up && ili9338_state.display_on) {
		lcdc_ili9338_pdata->panel_config_gpio(0);
		ili9338_state.display_on = FALSE;
		ili9338_state.disp_initialized = FALSE;
	}

	printk(KERN_ERR"lcd ili9338 panel off.\n");
	return 0;
}

static int ili9338_probe(struct platform_device *pdev)
{
	if (pdev->id == 0) {
		lcdc_ili9338_pdata = pdev->dev.platform_data;
		return 0;
	}
	msm_fb_add_device(pdev);
	lcdc_ili9338_bk_setting(5);

	printk(KERN_ERR"lcd ili9338 probe.\n");
	return 0;
}

static struct platform_driver this_driver = {
	.probe = ili9338_probe,
	.driver = {
		   .name = "lcdc_ili9338_rgb",
		   },
};

static struct msm_fb_panel_data ili9338_panel_data = {
	.on = lcdc_ili9338_panel_on,
	.off = lcdc_ili9338_panel_off,
	.set_backlight = lcdc_ili9338_set_backlight,
};

static struct platform_device this_device = {
	.name = "lcdc_ili9338_rgb",
	.id = 1,
	.dev = {
		.platform_data = &ili9338_panel_data,
		}
};

static int __init lcdc_ili9338_panel_init(void)
{
	int ret;
	struct msm_panel_info *pinfo;

	printk(KERN_ERR"lcd ili9338 panel initialize.\n");

#ifdef CONFIG_FB_MSM_TRY_MDDI_CATCH_LCDC_PRISM
	if (msm_fb_detect_client("lcdc_ili9338_rgb"))
		return 0;
#endif
	ret = platform_driver_register(&this_driver);
	if (ret)
		return ret;

	pinfo = &ili9338_panel_data.panel_info;
	pinfo->xres = 240;
	pinfo->yres = 320;
	pinfo->type = LCDC_PANEL;
	pinfo->pdest = DISPLAY_1;
	pinfo->wait_cycle = 0;
	pinfo->bl_max = LCDC_BK_INTEN_MAX;
	pinfo->bl_min = 0;
	pinfo->bpp = 18;
	pinfo->fb_num = 2;
	pinfo->clk_rate = 6100000;

	pinfo->lcdc.h_back_porch = 5;
	pinfo->lcdc.h_front_porch = 5;
	pinfo->lcdc.h_pulse_width = 2;
	pinfo->lcdc.v_back_porch = 5;
	pinfo->lcdc.v_front_porch = 5;
	pinfo->lcdc.v_pulse_width = 5;
	pinfo->lcdc.border_clr = 0;
	pinfo->lcdc.underflow_clr = 0xff;
	pinfo->lcdc.hsync_skew = 4;

	ret = platform_device_register(&this_device);
	if (ret) {
		platform_driver_unregister(&this_driver);
	}

	return ret;
}
module_init(lcdc_ili9338_panel_init);

