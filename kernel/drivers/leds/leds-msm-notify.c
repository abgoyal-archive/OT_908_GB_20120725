/*
 * leds-notify.c - MSM CHARGER LEDs driver.
 *
 * Copyright (c) 2009, Code Aurora Forum. All rights reserved.
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
 
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/delay.h>
#include <mach/pmic.h>
#include <mach/mpp.h>
#include <linux/workqueue.h>
#include <linux/ctype.h>

#define JRD_DEBUG_LED_BLINK	1

static struct delayed_work notify_led_work;
struct mutex	jrd_led_blink_lock;

#define SET_MPP21_I_SINK_COMMAND        0xFF
#define SET_MPP21_INTERVAL_COMMAND      0xEE
#define SET_MPP21_BOOTON_COMMAND        0xDD

#if JRD_DEBUG_LED_BLINK
static unsigned int notify_led_interval = 0;
static unsigned int notify_led_brightness = 0;
#endif

#if 0
typedef enum {
	LED_BLINK_ON_1_OFF_3 = 0,
	LED_BLINK_ON_1_OFF_5 = 1,
	LED_BLINK_ON_1_OFF_6 = 2,
	LED_BLINK_ON_1_OFF_8 = 3,
	LED_BLINK_ON_1_OFF_10 = 4
}led_on_off_setting_type;
#endif

#if JRD_DEBUG_LED_BLINK

static ssize_t notify_led_brightness_set(unsigned brightness)
{
        int ret = -EINVAL;
        if((brightness > 8) || (brightness < 0)){
                printk(KERN_ERR "%s: invalid led blink interval value!\n",__func__);
                return ret;
        }
#if 0	
	if(brightness > 0)	/*convert brightness value to pm_mpp_i_sink_level_type in amss*/
	        brightness = (brightness >> 5) + 1;
#endif	

        printk(KERN_INFO "%s: jrd_enter set brightness:%d!\n",__func__, brightness);

        mutex_lock(&jrd_led_blink_lock);
        ret = config_mpp21_blink_led(SET_MPP21_I_SINK_COMMAND, brightness);
        mutex_unlock(&jrd_led_blink_lock);
        notify_led_brightness = brightness; /*record setted interval used for blink_attr_show*/
        return ret;
}

static ssize_t notify_led_brightness_store(struct device *dev,
        struct device_attribute *attr, const char *buf, size_t size)
{
        ssize_t ret = -EINVAL;
        char *after;
        unsigned long brightness = simple_strtoul(buf, &after, 10);       /*compare to strict_strtoul ?*/
        size_t count = after - buf;

        printk(KERN_INFO "jrd_enter set blink led brightness now!\n");
        if(*after && isspace(*after))
                count++;

        if(count == size){
                ret = count;
                notify_led_brightness_set(brightness);
        }
        return ret;
}

static ssize_t notify_led_brightness_show(struct device *dev,
        struct device_attribute *attr, char *buf)
{
        printk(KERN_INFO "jrd_enter now:%u!\n",notify_led_brightness);
        return sprintf(buf, "%u\n", notify_led_brightness);
}

static ssize_t notify_led_interval_set(unsigned interval)
{
	int ret = -EINVAL;
	if((interval > 4) || (interval < 0)){
		printk(KERN_ERR "%s: invalid led blink interval value!\n",__func__);
		return ret; 	
	}
	printk(KERN_INFO "%s: jrd_enter set interval:%d!\n",__func__, interval);	
		 
	mutex_lock(&jrd_led_blink_lock);
	ret = config_mpp21_blink_led(SET_MPP21_INTERVAL_COMMAND, interval);
	mutex_unlock(&jrd_led_blink_lock);
        notify_led_interval = interval; /*record setted interval used for blink_attr_show*/
	return ret;
}

static ssize_t notify_led_blink_show(struct device *dev, 
	struct device_attribute *attr, char *buf)
{
	printk(KERN_INFO "jrd_enter now:%u!\n",notify_led_interval);
	return sprintf(buf, "%u\n", notify_led_interval);	
}

static ssize_t notify_led_blink_store(struct device *dev, 	
	struct device_attribute *attr, const char *buf, size_t size)
{
	ssize_t ret = -EINVAL;
	char *after;
	unsigned long interval = simple_strtoul(buf, &after, 10);	/*compare to strict_strtoul ?*/
	size_t count = after - buf;
	
	printk(KERN_INFO "jrd_enter set interval now!\n");	
	if(*after && isspace(*after))
		count++;
	
	if(count == size){
		ret = count;
		notify_led_interval_set(interval);
	}
	return ret;
}

static DEVICE_ATTR(blink_interval, 0644, notify_led_blink_show, notify_led_blink_store);
static DEVICE_ATTR(blink_brightness, 0644, notify_led_brightness_show, notify_led_brightness_store);
#endif

/*===========================================================================

 FUNCTION :msm_notify_led_set

DESCRIPTION
@para: value
		[0,0]    		OFF
		[1,31]               ON	PM_MPP__I_SINK__LEVEL_5mA     
		[32,63]		ON	PM_MPP__I_SINK__LEVEL_10mA
		[64,95]		ON	PM_MPP__I_SINK__LEVEL_15mA
		[96,127]		ON	PM_MPP__I_SINK__LEVEL_20mA
		[128,159]		ON	PM_MPP__I_SINK__LEVEL_25mA
		[160,191]		ON	PM_MPP__I_SINK__LEVEL_30mA
		[192,223]		ON	PM_MPP__I_SINK__LEVEL_35mA
		[224,255]		ON	PM_MPP__I_SINK__LEVEL_40mA

===========================================================================*/


static void msm_notify_led_set(struct led_classdev *led_cdev,
	enum led_brightness value)
{

	unsigned int i;
	if(value>255)
		value=255;
	i=(value>>5)+1;
      	if(value==0)
  		i=0;
	led_cdev->brightness=value;

	printk(KERN_INFO "jrd_enter %s: set ledbrightness:%d, i:%d!\n",__func__, value, i);

	mutex_lock(&jrd_led_blink_lock);
//	config_mpp21_blink_led(SET_MPP21_I_SINK_COMMAND, i);
	config_mpp21_blink_led(SET_MPP21_BOOTON_COMMAND, i);	/*used for control led when bootup or system switch from sleep/wakeup to wakeup/sleep*/
	mutex_unlock(&jrd_led_blink_lock);
#if 0
switch(i)
{
case 0:
	 mpp_config_i_sink(20, MPP_CFG_INPUT(PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_DIS));
	break;
case 1:
	 mpp_config_i_sink(20, MPP_CFG_INPUT(PM_MPP__I_SINK__LEVEL_5mA, PM_MPP__I_SINK__SWITCH_ENA));
	break;
case 2:
	 mpp_config_i_sink(20, MPP_CFG_INPUT(PM_MPP__I_SINK__LEVEL_10mA, PM_MPP__I_SINK__SWITCH_ENA));
	break;
case 3:
	 mpp_config_i_sink(20, MPP_CFG_INPUT(PM_MPP__I_SINK__LEVEL_15mA, PM_MPP__I_SINK__SWITCH_ENA));
	break;
case 4:
	 mpp_config_i_sink(20, MPP_CFG_INPUT(PM_MPP__I_SINK__LEVEL_20mA, PM_MPP__I_SINK__SWITCH_ENA));
	break;
case 5:
	 mpp_config_i_sink(20, MPP_CFG_INPUT(PM_MPP__I_SINK__LEVEL_25mA, PM_MPP__I_SINK__SWITCH_ENA));
	break;
case 6:
	 mpp_config_i_sink(20, MPP_CFG_INPUT(PM_MPP__I_SINK__LEVEL_30mA, PM_MPP__I_SINK__SWITCH_ENA));
	break;
case 7:
	 mpp_config_i_sink(20, MPP_CFG_INPUT(PM_MPP__I_SINK__LEVEL_35mA, PM_MPP__I_SINK__SWITCH_ENA));
	break;
case 8:
	 mpp_config_i_sink(20, MPP_CFG_INPUT(PM_MPP__I_SINK__LEVEL_40mA, PM_MPP__I_SINK__SWITCH_ENA));
	break;

}
#endif
	printk(KERN_ERR"notification state is %s\t Current case is %d\n",(value>0)?"ON":"OFF",i);
}

static int msm_notify_blink_set(struct led_classdev *led_cdev,unsigned long *delay_on, unsigned long *delay_off)
{

	return 0;
}

static struct led_classdev msm_notify_led = {
	.name			= "notification-leds",
	.brightness_set		= msm_notify_led_set,
	.blink_set		= msm_notify_blink_set,
	.brightness		= LED_OFF,
};

static void update_notify_led_state(struct work_struct *work)
{
	msm_notify_led_set(&msm_notify_led, LED_OFF);
}

static int msm_notify_led_probe(struct platform_device *pdev)
{
	int rc;

	mutex_init(&jrd_led_blink_lock);

	rc = led_classdev_register(&pdev->dev, &msm_notify_led);
	if (rc) {
		dev_err(&pdev->dev, "unable to register led class driver.\n");
		return rc;
	}

#if JRD_DEBUG_LED_BLINK	
	rc = device_create_file(&pdev->dev, &dev_attr_blink_interval);
	if(rc)
		{
	        dev_err(&pdev->dev, "unable to create blink_interval device node!\n");
		goto err_attr_blink_interval;
		}
	rc = device_create_file(&pdev->dev, &dev_attr_blink_brightness);
	if(rc)
		{
	        dev_err(&pdev->dev, "unable to create blink_brightness device node!\n");
		goto err_attr_blink_brightness;
		}
#endif
	
	msm_notify_led_set(&msm_notify_led, LED_FULL);
	schedule_delayed_work(&notify_led_work, 3 * HZ);
	return rc;

#if JRD_DEBUG_LED_BLINK
err_attr_blink_brightness:
	device_remove_file(&pdev->dev, &dev_attr_blink_interval);
err_attr_blink_interval:
	led_classdev_unregister(&msm_notify_led);
	return rc;
#endif
}

static int __devexit msm_notify_led_remove(struct platform_device *pdev)
{
#if JRD_DEBUG_LED_BLINK
	device_remove_file(&pdev->dev, &dev_attr_blink_interval);
	device_remove_file(&pdev->dev, &dev_attr_blink_brightness);
#endif
	led_classdev_unregister(&msm_notify_led);

	return 0;
}

#ifdef CONFIG_PM
static int msm_notify_led_suspend(struct platform_device *dev,
		pm_message_t state)
{
	led_classdev_suspend(&msm_notify_led);
	return 0;
}

static int msm_notify_led_resume(struct platform_device *dev)
{
	led_classdev_resume(&msm_notify_led);
	return 0;
}
#else
#define msm_notify_led_suspend NULL
#define msm_notify_led_resume NULL
#endif

static struct platform_driver msm_notify_led_driver = {
	.probe		= msm_notify_led_probe,
	.remove		= __devexit_p(msm_notify_led_remove),
	.suspend		= msm_notify_led_suspend,
	.resume		= msm_notify_led_resume,
	.driver		= {
		.name	= "notify-leds",
		.owner	= THIS_MODULE,
	},
};

static int __init msm_notify_led_init(void)
{
	printk(KERN_ERR"msm notify led init.\n");
	INIT_DELAYED_WORK(&notify_led_work, update_notify_led_state);
	return platform_driver_register(&msm_notify_led_driver);
}
module_init(msm_notify_led_init);

static void __exit msm_notify_led_exit(void)
{
	platform_driver_unregister(&msm_notify_led_driver);
}
module_exit(msm_notify_led_exit);

MODULE_DESCRIPTION("MSM NOTIFY LEDs driver");
MODULE_LICENSE("GPL v2");
MODULE_ALIAS("platform:notify-leds");


