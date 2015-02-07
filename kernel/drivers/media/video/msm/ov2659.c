/*
 * ov2659.c
 * ov2659 Camera Driver for QC
 */

#include <linux/delay.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <media/msm_camera.h>
#include <mach/gpio.h>
#include <mach/camera.h>
#include <mach/vreg.h>
#include <linux/proc_fs.h>

#undef CDBG
#define CDBG(fmt, args...) //printk("ov2659: %s %d() | " fmt, __func__, __LINE__,  ##args)
#define CDB //printk(KERN_INFO "litao debug - %s %5d %s\n", __FILE__, __LINE__, __func__);

/* OV2659 Registers and their values */
#define  REG_OV2659_MODEL_ID 0x300A
#define  OV2659_MODEL_ID     0x5626
#define  REG_OV2659_SENSOR_RESET     0x3012

struct ov2659_work {
	struct work_struct work;
};

static struct ov2659_work *ov2659_sensorw;
static struct i2c_client *ov2659_client;

struct ov2659_ctrl_t {
	const struct msm_camera_sensor_info *sensordata;
};

static struct ov2659_ctrl_t *ov2659_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(ov2659_wait_queue);
DECLARE_MUTEX(ov2659_sem);

/*define for other i2c related func*/
#define write_i2c write_cmos_sensor
#define OV2659_W write_cmos_sensor
#define OV2659_R read_cmos_sensor_1

/*litao add for ov2659 proc file*/
struct ov2659_proc_t {
	unsigned int i2c_addr;
	unsigned int i2c_data;
};

#define OV2659_PROC_NAME "ov2659"
#define SINGLE_OP_NAME "sbdata"
#define DOUBLE_READ_NAME "double_byte_read"
#define I2C_ADDR_NAME "i2c_addr"
#define OV2659_PROC_LEN 16
static struct proc_dir_entry *ov2659_dir, *s_file, *w_file, *dr_file;
static char ov2659_proc_buffer[OV2659_PROC_LEN];
static struct ov2659_proc_t ov2659_proc_dt = {
	.i2c_addr = 0,
	.i2c_data = 0,
};

/*litao add end*/

/*=============================================================*/
static int ov2659_reset(const struct msm_camera_sensor_info *dev)
{
	int rc = 0;
	CDBG("ov2659 reset\n");

	rc = gpio_request(dev->sensor_reset, "ov2659");

	if (!rc) {
		rc = gpio_direction_output(dev->sensor_reset, 0);
		mdelay(20);
		rc = gpio_direction_output(dev->sensor_reset, 1);
	}

	gpio_free(dev->sensor_reset);
	return rc;
}

static int32_t ov2659_i2c_txdata(unsigned short saddr, unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = length,
		 .buf = txdata,
		 },
	};

	if (i2c_transfer(ov2659_client->adapter, msg, 1) < 0) {
		CDBG("ov2659_i2c_txdata faild\n");
		return -EIO;
	}

	return 0;
}

static int32_t write_cmos_sensor(unsigned short waddr, unsigned char wdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[4];
	//printk("w %4x %2x\n", waddr, wdata);

	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = wdata;
	rc = ov2659_i2c_txdata(ov2659_client->addr, buf, 3);

	if (rc < 0)
		CDBG("i2c_write failed, addr = 0x%x, val = 0x%x!\n", waddr, wdata);

	return rc;
}

static int ov2659_i2c_rxdata(unsigned short saddr, unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
		 .addr = saddr,
		 .flags = 0,
		 .len = 2,
		 .buf = rxdata,
		 },
		{
		 .addr = saddr,
		 .flags = I2C_M_RD,
		 .len = length,
		 .buf = rxdata,
		 },
	};

	if (i2c_transfer(ov2659_client->adapter, msgs, 2) < 0) {
		CDBG("ov2659_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t read_cmos_sensor_2(unsigned short raddr, void *rdata)
{
	int32_t rc = 0;
	unsigned char buf[4];
	int len = 2;

	//printk("raddr=%4x, len=%4x\n", raddr, len);
	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00) >> 8;
	buf[1] = (raddr & 0x00FF);

	rc = ov2659_i2c_rxdata(ov2659_client->addr, buf, len);
	memcpy(rdata, buf, len);

	if (rc < 0)
		CDBG("ov2659_i2c_read failed!\n");

	return rc;
}

/*
static void reg_dump()
{
	unsigned short a, d;
	int rc = 0;

	printk("reg dump===\n");
	a = 0x3000;
	while (a < 0x3500) {
		if (!(a & 0x0f))
			printk("\n%4x: ", a);
		rc = read_cmos_sensor_2(a, &d);
		if (rc < 0) {
			printk("error\n");
			return;
		}
		printk("%4x ", d);
		a += 2;
	}
	printk("\nreg dump over===\n");
}
*/

static int32_t read_cmos_sensor(unsigned short raddr, void *rdata, int len)
{
	int32_t rc = 0;
	unsigned char buf[4];
	//printk("raddr=%x, len=%x\n", raddr, len);

	if (!rdata || len > 4)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00) >> 8;
	buf[1] = (raddr & 0x00FF);

	rc = ov2659_i2c_rxdata(ov2659_client->addr, buf, len);

	memcpy(rdata, buf, len);

	if (rc < 0)
		CDBG("ov2659_i2c_read failed!\n");

	return rc;
}

static unsigned char read_cmos_sensor_1(unsigned short raddr)
{
	unsigned short tmp16;
	unsigned char tmp;

	read_cmos_sensor(raddr, &tmp16, 2);
	tmp = tmp16 & 0xff;
	return tmp;
}

static long ov2659_reg_init(void)
{
	write_i2c(0x0103, 0x01);
	write_i2c(0x3000, 0x0f);
	write_i2c(0x3001, 0xff);
	write_i2c(0x3002, 0xff);
	write_i2c(0x0100, 0x01);
	write_i2c(0x3633, 0x3d);
	write_i2c(0x3620, 0x02);
	write_i2c(0x3631, 0x11);
	write_i2c(0x3612, 0x04);
	write_i2c(0x3630, 0x20);
	write_i2c(0x4702, 0x02);
	write_i2c(0x370c, 0x34);
	write_i2c(0x3004, 0x10);
	write_i2c(0x3005, 0x18);
	write_i2c(0x3800, 0x00);
	write_i2c(0x3801, 0x00);
	write_i2c(0x3802, 0x00);
	write_i2c(0x3803, 0x00);
	write_i2c(0x3804, 0x06);
	write_i2c(0x3805, 0x5f);
	write_i2c(0x3806, 0x04);
	//write_i2c(0x3807, 0xb7);
	write_i2c(0x3808, 0x03);
	write_i2c(0x3809, 0x20);
	write_i2c(0x380a, 0x02);
	write_i2c(0x380b, 0x58);
	write_i2c(0x380c, 0x05);
	write_i2c(0x380d, 0x14);
	write_i2c(0x380e, 0x02);
	write_i2c(0x380f, 0x68);
	write_i2c(0x3811, 0x08);
	write_i2c(0x3813, 0x02);
	write_i2c(0x3814, 0x31);
	write_i2c(0x3815, 0x31);
	write_i2c(0x3a02, 0x02);
	write_i2c(0x3a03, 0x68);
	write_i2c(0x3a08, 0x00);
	write_i2c(0x3a09, 0x5c);
	write_i2c(0x3a0a, 0x00);
	write_i2c(0x3a0b, 0x4d);
	write_i2c(0x3a0d, 0x08);
	write_i2c(0x3a0e, 0x06);
	write_i2c(0x3a14, 0x02);
	write_i2c(0x3a15, 0x28);
	write_i2c(0x3623, 0x00);
	write_i2c(0x3634, 0x76);
	write_i2c(0x3701, 0x44);
	write_i2c(0x3702, 0x18);
	write_i2c(0x3703, 0x24);
	write_i2c(0x3704, 0x24);
	write_i2c(0x3705, 0x0c);
	write_i2c(0x3820, 0x81);
	write_i2c(0x3821, 0x00);
	write_i2c(0x370a, 0x52);
	write_i2c(0x4608, 0x00);
	write_i2c(0x4609, 0x80);
	write_i2c(0x4300, 0x30);
	write_i2c(0x5086, 0x02);
	write_i2c(0x5000, 0xfb);
	write_i2c(0x5001, 0x1f);
	write_i2c(0x5002, 0x00);
	write_i2c(0x5025, 0x0e);
	write_i2c(0x5026, 0x18);
	write_i2c(0x5027, 0x34);
	write_i2c(0x5028, 0x4c);
	write_i2c(0x5029, 0x62);
	write_i2c(0x502a, 0x74);
	write_i2c(0x502b, 0x85);
	write_i2c(0x502c, 0x92);
	write_i2c(0x502d, 0x9e);
	write_i2c(0x502e, 0xb2);
	write_i2c(0x502f, 0xc0);
	write_i2c(0x5030, 0xcc);
	write_i2c(0x5031, 0xe0);
	write_i2c(0x5032, 0xee);
	write_i2c(0x5033, 0xf6);
	write_i2c(0x5034, 0x11);
	write_i2c(0x5070, 0x1c);
	write_i2c(0x5071, 0x5b);
	write_i2c(0x5072, 0x05);
	write_i2c(0x5073, 0x20);
	write_i2c(0x5074, 0x94);
	write_i2c(0x5075, 0xb4);
	write_i2c(0x5076, 0xb4);
	write_i2c(0x5077, 0xaf);
	write_i2c(0x5078, 0x05);
	write_i2c(0x5079, 0x98);
	write_i2c(0x507a, 0x21);
	write_i2c(0x5035, 0x6a);
	write_i2c(0x5036, 0x11);
	write_i2c(0x5037, 0x92);
	write_i2c(0x5038, 0x21);
	write_i2c(0x5039, 0xe1);
	write_i2c(0x503a, 0x01);
	write_i2c(0x503c, 0x05);
	write_i2c(0x503d, 0x08);
	write_i2c(0x503e, 0x08);
	write_i2c(0x503f, 0x64);
	write_i2c(0x5040, 0x58);
	write_i2c(0x5041, 0x2a);
	write_i2c(0x5042, 0xc5);
	write_i2c(0x5043, 0x2e);
	write_i2c(0x5044, 0x3a);
	write_i2c(0x5045, 0x3c);
	write_i2c(0x5046, 0x44);
	write_i2c(0x5047, 0xf8);
	write_i2c(0x5048, 0x08);
	write_i2c(0x5049, 0x70);
	write_i2c(0x504a, 0xf0);
	write_i2c(0x504b, 0xf0);
	write_i2c(0x500c, 0x03);
	write_i2c(0x500d, 0x20);
	write_i2c(0x500e, 0x02);
	write_i2c(0x500f, 0x5c);
	write_i2c(0x5010, 0x48);
	write_i2c(0x5011, 0x00);
	write_i2c(0x5012, 0x66);
	write_i2c(0x5013, 0x03);
	write_i2c(0x5014, 0x30);
	write_i2c(0x5015, 0x02);
	write_i2c(0x5016, 0x7c);
	write_i2c(0x5017, 0x40);
	write_i2c(0x5018, 0x00);
	write_i2c(0x5019, 0x66);
	write_i2c(0x501a, 0x03);
	write_i2c(0x501b, 0x10);
	write_i2c(0x501c, 0x02);
	write_i2c(0x501d, 0x7c);
	write_i2c(0x501e, 0x3a);
	write_i2c(0x501f, 0x00);
	write_i2c(0x5020, 0x66);
	write_i2c(0x506e, 0x44);
	write_i2c(0x5064, 0x08);
	write_i2c(0x5065, 0x10);
	write_i2c(0x5066, 0x12);
	write_i2c(0x5067, 0x02);
	write_i2c(0x506c, 0x08);
	write_i2c(0x506d, 0x10);
	write_i2c(0x506f, 0xa6);
	write_i2c(0x5068, 0x08);
	write_i2c(0x5069, 0x10);
	write_i2c(0x506a, 0x04);
	write_i2c(0x506b, 0x12);
	write_i2c(0x507e, 0x40);
	write_i2c(0x507f, 0x20);
	write_i2c(0x507b, 0x02);
	write_i2c(0x507a, 0x01);
	write_i2c(0x5084, 0x0c);
	write_i2c(0x5085, 0x3e);
	write_i2c(0x5005, 0x80);
	write_i2c(0x3a0f, 0x30);
	write_i2c(0x3a10, 0x28);
	write_i2c(0x3a1b, 0x32);
	write_i2c(0x3a1e, 0x26);
	write_i2c(0x3a11, 0x60);
	write_i2c(0x3a1f, 0x14);
	write_i2c(0x5060, 0x69);
	write_i2c(0x5061, 0x7d);
	write_i2c(0x5062, 0x7d);
	write_i2c(0x5063, 0x69);
	write_i2c(0x3004, 0x20);
	write_i2c(0x3820, 0x86); //0x87
	write_i2c(0x3821, 0x02);
	write_i2c(0x4003, 0x88);
	//add by ben for IQ
	//awb
	write_i2c(0x5035, 0x6a);
	write_i2c(0x5036, 0x11);
	write_i2c(0x5037, 0x92);
	write_i2c(0x5038, 0x25);
	write_i2c(0x5039, 0xf1);
	write_i2c(0x503a, 0x01);
	write_i2c(0x503c, 0x0f);
	write_i2c(0x503d, 0x16);
	write_i2c(0x503e, 0x18);
	write_i2c(0x503f, 0x60);
	write_i2c(0x5040, 0x5a);
	write_i2c(0x5041, 0x9d);
	write_i2c(0x5042, 0xa8);
	write_i2c(0x5043, 0x39);
	write_i2c(0x5044, 0x2f);
	write_i2c(0x5045, 0x51);
	write_i2c(0x5046, 0x48);
	write_i2c(0x5047, 0xf8);
	write_i2c(0x5048, 0x0a);
	write_i2c(0x5049, 0x70);
	write_i2c(0x504a, 0xf0);
	write_i2c(0x504b, 0xf0);
	//lens shading
	write_i2c(0x500c, 0x03);
	write_i2c(0x500d, 0x3e);
	write_i2c(0x500e, 0x02);
	write_i2c(0x500f, 0x53);
	write_i2c(0x5010, 0x4c);
	write_i2c(0x5011, 0x00);
	write_i2c(0x5012, 0x66);

	write_i2c(0x5013, 0x03);
	write_i2c(0x5014, 0x40);
	write_i2c(0x5015, 0x02);
	write_i2c(0x5016, 0x4c);
	write_i2c(0x5017, 0x42);
	write_i2c(0x5018, 0x00);
	write_i2c(0x5019, 0x66);

	write_i2c(0x501a, 0x03);
	write_i2c(0x501b, 0x30);
	write_i2c(0x501c, 0x02);
	write_i2c(0x501d, 0x5a);
	write_i2c(0x501e, 0x3c);
	write_i2c(0x501f, 0x00);
	write_i2c(0x5020, 0x66);

       //color matrix
       write_i2c(0x5070, 0x31);
	write_i2c(0x5071, 0x61);
	write_i2c(0x5072, 0x12);
	write_i2c(0x5073, 0x23);
	write_i2c(0x5074, 0x93);
	write_i2c(0x5075, 0xb6);
	write_i2c(0x5076, 0x9b);
	write_i2c(0x5077, 0x91);
	write_i2c(0x5078, 0x0a);
	write_i2c(0x5079, 0x9c);
	write_i2c(0x507a, 0x1);
	//gamma
	write_i2c(0x5025, 0x0e);
	write_i2c(0x5026, 0x18);
	write_i2c(0x5027, 0x34);
	write_i2c(0x5028, 0x4c);
	write_i2c(0x5029, 0x62);
	write_i2c(0x502a, 0x74);
	write_i2c(0x502b, 0x85);
	write_i2c(0x502c, 0x92);
	write_i2c(0x502d, 0x9e);
	write_i2c(0x502e, 0xb2);
	write_i2c(0x502f, 0xc0);
	write_i2c(0x5030, 0xcc);
	write_i2c(0x5031, 0xe0);
	write_i2c(0x5032, 0xee);
	write_i2c(0x5033, 0xf6);
	write_i2c(0x5034, 0x11);
	//ae target
	write_i2c(0x3a0f,  0x30);
	write_i2c(0x3a10, 0x28);
	write_i2c(0x3a1b, 0x30);
	write_i2c(0x3a1e, 0x28);
	write_i2c(0x3a11, 0x61);
	write_i2c(0x3a1f,  0x10);
	//denoise
	write_i2c(0x506e, 0x44);
	write_i2c(0x5068, 0x08);
	write_i2c(0x5069, 0x10);
	write_i2c(0x506a, 0x0c);
	write_i2c(0x506b, 0x20);
	return 0;
}

static long ov2659_priview(void)
{
	//write_i2c(0x3800, 0x00);
	//write_i2c(0x3801, 0x00);
	//write_i2c(0x3802, 0x00);
	//write_i2c(0x3803, 0x00);
	//write_i2c(0x3804, 0x06);
	//write_i2c(0x3805, 0x5f);
	//write_i2c(0x3806, 0x04);
	//write_i2c(0x3807, 0xb7);  //neil
	write_i2c(0x3808, 0x03);
	write_i2c(0x3809, 0x20);
	write_i2c(0x380a, 0x02);
	write_i2c(0x380b, 0x58);
	write_i2c(0x380c, 0x05);
	write_i2c(0x380d, 0x14);
	write_i2c(0x380e, 0x02);
	write_i2c(0x380f, 0x68);
	write_i2c(0x3811, 0x08);
	write_i2c(0x3813, 0x02);
	write_i2c(0x3814, 0x31);
	write_i2c(0x3815, 0x31);
	write_i2c(0x3a02, 0x02);
	write_i2c(0x3a03, 0x68);
	write_i2c(0x3a08, 0x00);
	write_i2c(0x3a09, 0xb8); //0x5c
	write_i2c(0x3a0a, 0x00);
	write_i2c(0x3a0b, 0x9a); //0x4d
	write_i2c(0x3a0d, 0x04); //0x08
	write_i2c(0x3a0e, 0x03); //0x03
	write_i2c(0x3a14, 0x06); //0x02
	write_i2c(0x3a15, 0x6f); //0x28
	//write_i2c(0x3623, 0x00);
	write_i2c(0x3634, 0x76);
	//write_i2c(0x3701, 0x44);
	write_i2c(0x3702, 0x18);
	write_i2c(0x3703, 0x24);
	write_i2c(0x3704, 0x24);
	write_i2c(0x3705, 0x0c);
	//write_i2c(0x3820, 0x81);
	//write_i2c(0x3821, 0x00);
	write_i2c(0x370a, 0x52);
	//write_i2c(0x4608, 0x00);
	//write_i2c(0x4609, 0x80);
	//write_i2c(0x5002, 0x10);
	write_i2c(0x3005, 0x18);
	write_i2c(0x3004, 0x20);

	write_i2c(0x3503, 0x00);
	write_i2c(0x3820, 0x87);
	write_i2c(0x3821, 0x03); //0x02
	return 0;
}/* OV2650_Preview */

unsigned int m_gain_preview;
unsigned int m_gain_still;
unsigned long m_shutter_preview;
unsigned long m_shutter_still;
unsigned long long m_dbExposure;
unsigned long m_dwFreq_preview;
unsigned long m_dwFreq_still;
unsigned char gain;

void OV2659Core_Get_ExposureValue(void)
{
	unsigned long tmp;
	unsigned char regv;
	unsigned long shutter;
	//Shutter
	regv = OV2659_R(0x3500);
	//printk("===========3500 %x\n", regv);
	shutter = regv & 0x0F;
	shutter <<= 12;
	//printk("shutter ========= %x\n", shutter);
	regv = OV2659_R(0x3501);
	//printk("===========3501 %x\n", regv);
	tmp = regv;
	tmp <<= 4;
	//printk("===========tmp %x\n", tmp);
	shutter += tmp;
	//printk("shutter = %x\r\n", shutter);
	//shutter <<= 4;
	regv = OV2659_R(0x3502);
	//printk("===========3502 %x\n", regv);
	regv >>=4;
	//printk("===========regv %x\n", regv);
	shutter += regv;
	//printk("shutter = %x\r\n", shutter);
	m_shutter_preview = shutter;
	gain = OV2659_R(0x350B);
	//printk("w===========350B %x\n", gain);
}

void OV2659Core_Set_ExposureValue(void)
{
	unsigned char regv;
	unsigned long shutter ,capture_vsize;
	//neil add
	regv = OV2659_R(0x380e);
	capture_vsize = regv& 0x07;
	regv = OV2659_R(0x380f);
	capture_vsize = (capture_vsize << 8) | regv;

	//printk("=============================shutter is %ld\n", m_shutter_preview);
	//m_shutter_preview = m_shutter_preview * 3 * 0x514 / 0x79f / 2;
	m_shutter_preview=m_shutter_preview*2;
	m_shutter_preview = m_shutter_preview *24 /24; //36/24
	m_shutter_preview = m_shutter_preview *0x514/0x79f;
	shutter = m_shutter_preview;
/*********************************neil add *********************************/
    if (gain > 124)
    {
      gain = gain /2 ;
      shutter = shutter * 2;
    }
   if (gain >31)
	{
	if (shutter*2<(capture_vsize -4))
		{
		gain = gain /2;
		 shutter  = shutter *2;
		}
	}

/***************************************************************************/
	shutter = shutter *16;
	//printk("shutter is %lx\n", shutter);
	regv = (shutter & 0xFF);
	//regv = (shutter <<4);
	//printk("w===========3502 %x\n", regv);
	OV2659_W(0x3502, regv);

	regv = ((shutter >> 8) & 0xFF);
	//regv = ((shutter >> 4) & 0xFF);
	//printk("w===========3501 %x\n", regv);
	OV2659_W(0x3501, regv);
	regv = ((shutter >> 16) & 0x0F);
	//regv = (shutter >> 12) ;
	//printk("w===========3500 %x\n", regv);
	OV2659_W(0x3500, regv);
	//gain
	//regv = OV2659_R(0x350B);
	//OV2659_W(0x350B, gain  + 5);
	//mdelay(50);
	OV2659_W(0x350B, gain );
	regv = OV2659_R(0x350B);
	//printk("w===========350B %x\n", regv);
}

static long ov2659_snapshot(void)
{
	long rc = 0;

	//stop AE
	write_i2c(0x3503, 0x07);
	OV2659Core_Get_ExposureValue();
	write_i2c(0x3800, 0x00);
	write_i2c(0x3801, 0x00);
	write_i2c(0x3802, 0x00);
	write_i2c(0x3803, 0x00);
	write_i2c(0x3804, 0x06);
	write_i2c(0x3805, 0x5f);
	write_i2c(0x3806, 0x04);
	write_i2c(0x3807, 0xbb);
	write_i2c(0x3808, 0x06);
	write_i2c(0x3809, 0x40);
	write_i2c(0x380a, 0x04);
	write_i2c(0x380b, 0xb0);
	write_i2c(0x380c, 0x07);
	write_i2c(0x380d, 0x9f);
	write_i2c(0x380e, 0x04);
	write_i2c(0x380f, 0xd0);
	write_i2c(0x3811, 0x10);
	write_i2c(0x3813, 0x06);
	write_i2c(0x3814, 0x11);
	write_i2c(0x3815, 0x11);
	write_i2c(0x3a02, 0x04);
	write_i2c(0x3a03, 0xd0);
	write_i2c(0x3a08, 0x00);
	write_i2c(0x3a09, 0xb8);
	write_i2c(0x3a0a, 0x00);
	write_i2c(0x3a0b, 0x9a);
	write_i2c(0x3a0d, 0x08);
	write_i2c(0x3a0e, 0x06);
	write_i2c(0x3a14, 0x04);
	write_i2c(0x3a15, 0x50);
	write_i2c(0x3623, 0x00);
	write_i2c(0x3634, 0x44);
	write_i2c(0x3701, 0x44);
	write_i2c(0x3702, 0x30);
	write_i2c(0x3703, 0x48);
	write_i2c(0x3704, 0x48);
	write_i2c(0x3705, 0x18);
	write_i2c(0x3820, 0x86);
	write_i2c(0x3821, 0x06);
	write_i2c(0x370a, 0x12);
	write_i2c(0x4608, 0x00);
	write_i2c(0x4609, 0x80);
	write_i2c(0x5002, 0x00);
	write_i2c(0x3005, 0x24);
	write_i2c(0x3004, 0x30); //0x40

	write_i2c(0x4002, 0xc5);  // for BLC function

	CDBG("ov2659 snapshot:\n");
	// reg_dump();
	OV2659Core_Set_ExposureValue();
	mdelay(200);

	return rc;

}				/* OV2650_Capture */

/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
static long ov2659_set_sensor_mode(int mode)
{
	long rc = 0;

	CDBG("ov2659 set sensor mode %d:\n", mode);
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		rc = ov2659_priview();
		break;
	case SENSOR_SNAPSHOT_MODE:
		rc = ov2659_snapshot();
		break;
	default:
		return -EFAULT;
	}

	return rc;
}

static long ov2659_set_antibanding(int value)
{
	long rc = 0;

	CDBG("ov2659 set antibanding not available%d:\n", value);
	switch (value) {
	case CAMERA_ANTIBANDING_OFF:
		//printk("%s:%d:%s-CAMERA_ANTIBANDING_OFF\n", __FILE__, __LINE__, __func__);
		break;
	case CAMERA_ANTIBANDING_50HZ:
		//printk("%s:%d:%s-CAMERA_ANTIBANDING_50hz\n", __FILE__, __LINE__, __func__);
		break;
	case CAMERA_ANTIBANDING_60HZ:
		//printk("%s:%d:%s-CAMERA_ANTIBANDING_60hz\n", __FILE__, __LINE__, __func__);
		break;
	case CAMERA_ANTIBANDING_AUTO:
		//printk("%s:%d:%s-CAMERA_ANTIBANDING_auto\n", __FILE__, __LINE__, __func__);
		break;
	default:
		return -EFAULT;
	}
	return rc;
}

static long ov2659_set_wb(int value)
{
	long rc = 0;
	unsigned char tmp_reg;

	CDBG("ov2659 set wb %d:\n", value);
	tmp_reg = read_cmos_sensor_1(0x3406);
	switch (value) {
	case CAMERA_WB_AUTO:
		//printk("%s:%d:%s-whitebalance auto\n", __FILE__, __LINE__, __func__);
		write_cmos_sensor(0x3406, tmp_reg & ~0x1);	// select Auto WB
		break;
	case CAMERA_WB_INCANDESCENT:	//office
		//printk("%s:%d:%s-CAMERA_WB_INCANDESCENT\n", __FILE__, __LINE__, __func__);
		write_cmos_sensor(0x3406, tmp_reg | 0x1);	// Disable AWB
		write_cmos_sensor(0x3400, 0x04); //0x06
		write_cmos_sensor(0x3401, 0x58); //0x2a
		write_cmos_sensor(0x3402, 0x05);
		write_cmos_sensor(0x3403, 0x00);
		write_cmos_sensor(0x3404, 0x07); //0x07
		write_cmos_sensor(0x3405, 0x90); //0x24
		break;
	case CAMERA_WB_FLUORESCENT:	//home
		//printk("%s:%d:%s-CAMERA_WB_FLUORESCENT\n", __FILE__, __LINE__, __func__);
		write_cmos_sensor(0x3406, tmp_reg | 0x1);	// Disable AWB
		write_cmos_sensor(0x3400, 0x05);
		write_cmos_sensor(0x3401, 0x58);
		write_cmos_sensor(0x3402, 0x04);
		write_cmos_sensor(0x3403, 0x00);
		write_cmos_sensor(0x3404, 0x06); //0x08
		write_cmos_sensor(0x3405, 0x00); //0x40
		break;
	case CAMERA_WB_DAYLIGHT:
		//printk("%s:%d:%s-CAMERA_WB_DAYLIGHT\n", __FILE__, __LINE__, __func__);
		write_cmos_sensor(0x3406, tmp_reg | 0x1);	// Disable AWB
		write_cmos_sensor(0x3400, 0x06);
		write_cmos_sensor(0x3401, 0x80);
		write_cmos_sensor(0x3402, 0x04);
		write_cmos_sensor(0x3403, 0x00);
		write_cmos_sensor(0x3404, 0x04); //0x05
		write_cmos_sensor(0x3405, 0x15); //0x15
		break;
	case CAMERA_WB_CLOUDY_DAYLIGHT:
		//printk("%s:%d:%s-CAMERA_WB_CLOUDY_DAYLIGHT\n", __FILE__, __LINE__, __func__);
		write_cmos_sensor(0x3406, tmp_reg | 0x1);	// select manual WB
		write_cmos_sensor(0x3400, 0x07); //0x07
		write_cmos_sensor(0x3401, 0x20); //0x08
		write_cmos_sensor(0x3402, 0x04);
		write_cmos_sensor(0x3403, 0x00);
		write_cmos_sensor(0x3404, 0x04); //0x05
		write_cmos_sensor(0x3405, 0x50); //0x00
		break;
	default:
		return -EFAULT;
	}
	return rc;
}

static long ov2659_set_brightness(int value)
{
	long rc = 0;
	//printk("%s:%d:%s-brightness is %d\n", __FILE__, __LINE__, __func__, value);
	switch (value) {
	case 8:
	case 7:
		rc = write_cmos_sensor(0x5001, 0x1f);
		rc = write_cmos_sensor(0x5082, 0x30);
		rc = write_cmos_sensor(0x507b, 0x04);
		rc = write_cmos_sensor(0x5083, 0x01);
	case 6:
		rc = write_cmos_sensor(0x5001, 0x1f);
		rc = write_cmos_sensor(0x5082, 0x20);
		rc = write_cmos_sensor(0x507b, 0x04);
		rc = write_cmos_sensor(0x5083, 0x01);
		break;
	case 5:
		rc = write_cmos_sensor(0x5001, 0x1f);
		rc = write_cmos_sensor(0x5082, 0x10);
		rc = write_cmos_sensor(0x507b, 0x04);
		rc = write_cmos_sensor(0x5083, 0x01);
		break;
	case 4:
		rc = write_cmos_sensor(0x5001, 0x1f);
		rc = write_cmos_sensor(0x5082, 0x00);
		rc = write_cmos_sensor(0x507b, 0x04);
		rc = write_cmos_sensor(0x5083, 0x01);
		break;
	case 3:
		rc = write_cmos_sensor(0x5001, 0x1f);
		rc = write_cmos_sensor(0x5082, 0x10);
		rc = write_cmos_sensor(0x507b, 0x04);
		rc = write_cmos_sensor(0x5083, 0x09);
		break;
	case 2:
		rc = write_cmos_sensor(0x5001, 0x1f);
		rc = write_cmos_sensor(0x5082, 0x20);
		rc = write_cmos_sensor(0x507b, 0x04);
		rc = write_cmos_sensor(0x5083, 0x09);
		break;
	case 1:
	case 0:
		rc = write_cmos_sensor(0x5001, 0x1f);
		rc = write_cmos_sensor(0x5082, 0x30);
		rc = write_cmos_sensor(0x507b, 0x04);
		rc = write_cmos_sensor(0x5083, 0x09);
		break;
	default:
		return -EFAULT;
	}
	return rc;
}

/*
static long ov2659_set_exposure(int value)
{
	long rc = 0;
	printk("%s:%d:%s-exposure is %d\n", __FILE__, __LINE__, __func__, value);
	switch (value) {
	case 10:
		rc = write_cmos_sensor(0x3a0f, 0x60);
		rc = write_cmos_sensor(0x3a10, 0x58);
		rc = write_cmos_sensor(0x3a1b, 0xa0);
		rc = write_cmos_sensor(0x3a1e, 0x60);
		rc = write_cmos_sensor(0x3a11, 0x58);
		rc = write_cmos_sensor(0x3a1f, 0x20);
	case 9:		//+4
		rc = write_cmos_sensor(0x3a0f, 0x58);
		rc = write_cmos_sensor(0x3a10, 0x50);
		rc = write_cmos_sensor(0x3a1b, 0x91);
		rc = write_cmos_sensor(0x3a1e, 0x58);
		rc = write_cmos_sensor(0x3a11, 0x50);
		rc = write_cmos_sensor(0x3a1f, 0x20);
	case 8:		//+3
		rc = write_cmos_sensor(0x3a0f, 0x50);
		rc = write_cmos_sensor(0x3a10, 0x48);
		rc = write_cmos_sensor(0x3a1b, 0x90);
		rc = write_cmos_sensor(0x3a1e, 0x50);
		rc = write_cmos_sensor(0x3a11, 0x48);
		rc = write_cmos_sensor(0x3a1f, 0x20);
	case 7:		//+2
		rc = write_cmos_sensor(0x3a0f, 0x48);
		rc = write_cmos_sensor(0x3a10, 0x40);
		rc = write_cmos_sensor(0x3a1b, 0x80);
		rc = write_cmos_sensor(0x3a1e, 0x48);
		rc = write_cmos_sensor(0x3a11, 0x40);
		rc = write_cmos_sensor(0x3a1f, 0x20);
	case 6:		//+1
		rc = write_cmos_sensor(0x3a0f, 0x40);
		rc = write_cmos_sensor(0x3a10, 0x38);
		rc = write_cmos_sensor(0x3a1b, 0x71);
		rc = write_cmos_sensor(0x3a1e, 0x40);
		rc = write_cmos_sensor(0x3a11, 0x38);
		rc = write_cmos_sensor(0x3a1f, 0x10);
	case 5:		//0
		rc = write_cmos_sensor(0x3a0f, 0x38);
		rc = write_cmos_sensor(0x3a10, 0x30);
		rc = write_cmos_sensor(0x3a1b, 0x61);
		rc = write_cmos_sensor(0x3a1e, 0x38);
		rc = write_cmos_sensor(0x3a11, 0x30);
		rc = write_cmos_sensor(0x3a1f, 0x10);
	case 4:		//-1
		rc = write_cmos_sensor(0x3a0f, 0x30);
		rc = write_cmos_sensor(0x3a10, 0x28);
		rc = write_cmos_sensor(0x3a1b, 0x60);
		rc = write_cmos_sensor(0x3a1e, 0x30);
		rc = write_cmos_sensor(0x3a11, 0x28);
		rc = write_cmos_sensor(0x3a1f, 0x10);
	case 3:		//-2
		rc = write_cmos_sensor(0x3a0f, 0x28);
		rc = write_cmos_sensor(0x3a10, 0x20);
		rc = write_cmos_sensor(0x3a1b, 0x51);
		rc = write_cmos_sensor(0x3a1e, 0x28);
		rc = write_cmos_sensor(0x3a11, 0x20);
		rc = write_cmos_sensor(0x3a1f, 0x10);
	case 2:		//-3
		rc = write_cmos_sensor(0x3a0f, 0x20);
		rc = write_cmos_sensor(0x3a10, 0x18);
		rc = write_cmos_sensor(0x3a1b, 0x41);
		rc = write_cmos_sensor(0x3a1e, 0x20);
		rc = write_cmos_sensor(0x3a11, 0x18);
		rc = write_cmos_sensor(0x3a1f, 0x10);
	case 1:		//-4
		rc = write_cmos_sensor(0x3a0f, 0x18);
		rc = write_cmos_sensor(0x3a10, 0x10);
		rc = write_cmos_sensor(0x3a1b, 0x18);
		rc = write_cmos_sensor(0x3a1e, 0x10);
		rc = write_cmos_sensor(0x3a11, 0x30);
		rc = write_cmos_sensor(0x3a1f, 0x10);
	case 0:		//-5
		rc = write_cmos_sensor(0x3a0f, 0x10);
		rc = write_cmos_sensor(0x3a10, 0x08);
		rc = write_cmos_sensor(0x3a1b, 0x10);
		rc = write_cmos_sensor(0x3a1e, 0x08);
		rc = write_cmos_sensor(0x3a11, 0x20);
		rc = write_cmos_sensor(0x3a1f, 0x10);
		break;
	default:
		return -EFAULT;
	}
	return rc;
}
*/

static long ov2659_set_nightmode(int mode)
{
	long rc = 0;
	unsigned char night;

	//printk("%s:%d:%s-set nightmode %d\n", __FILE__, __LINE__, __func__, mode);
	night = read_cmos_sensor_1(0x3a00);	//bit[2], 0: disable, 1:enable
	//printk("night %x\n", night);
	if (mode == CAMERA_NIGHTSHOT_MODE_ON) {
		//printk("%s:%d:%s-CAMERA_NIGHTSHOT_MODE_ON\n", __FILE__, __LINE__, __func__);
		rc = write_cmos_sensor(0x3a00, night | 0x04);	//Enable night mode

	} else {
		//printk("%s:%d:%s-CAMERA_NIGHTSHOT_MODE_OFF\n", __FILE__, __LINE__, __func__);
		rc = write_cmos_sensor(0x3a00, night & 0xfb);	//Disable night mode

	}
	return rc;
}

static long ov2659_set_effect(int mode, int8_t effect)
{
	long rc = 0;

	//printk("%s:%d:%s-set effect %d\n", __FILE__, __LINE__, __func__, effect);
	switch (effect) {
	case CAMERA_EFFECT_OFF:
		rc = write_cmos_sensor(0x507b, 0x00);
		break;

	case CAMERA_EFFECT_SEPIA:
		rc = write_cmos_sensor(0x507b, 0x18);
		rc = write_cmos_sensor(0x507e, 0x40);
		rc = write_cmos_sensor(0x507f, 0xa0);
		break;

	case CAMERA_EFFECT_NEGATIVE:
		rc = write_cmos_sensor(0x507b, 0x40);
		break;

	case CAMERA_EFFECT_MONO:	//B&W
		rc = write_cmos_sensor(0x507b, 0x20);
		break;

	default:
		rc = -1;
	}
	return rc;
}

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/
static int ov2659_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	uint16_t model_id = 0;
	int rc = 0;

	CDBG("init entry \n");
	rc = gpio_request(data->sensor_pwd, "ov2659");

	if (!rc) {
		printk("gpio sensor pwd set to 0\n");
		rc = gpio_direction_output(data->sensor_pwd, 0);
	}

	gpio_free(data->sensor_pwd);

	rc = ov2659_reset(data);
	if (rc < 0) {
		printk("reset failed!\n");
		goto init_probe_fail;
	}

	//soft reset
	rc = write_cmos_sensor(REG_OV2659_SENSOR_RESET, 0x80);
	mdelay(10);

	/* Read the Model ID of the sensor */
	rc = read_cmos_sensor_2(REG_OV2659_MODEL_ID, &model_id);

	if (rc < 0)
		goto init_probe_fail;

	printk("ov2659 model_id = 0x%x\n", model_id);

	/* Check if it matches it with the value in Datasheet */
	if (model_id != OV2659_MODEL_ID) {
		rc = -EFAULT;
		goto init_probe_fail;
	}

	rc = ov2659_reg_init();
	if (rc < 0)
		goto init_probe_fail;

	return rc;

init_probe_fail:
	return rc;
}

//=============================//

static int proc_ov2659_dread(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len, rc;
	unsigned short tmp16;

	rc = read_cmos_sensor_2(ov2659_proc_dt.i2c_addr, &tmp16);
	if (rc < 0) {
		len = sprintf(page, "double 0x%x@ov2659_i2c read error\n", ov2659_proc_dt.i2c_addr);
	} else {
		len = sprintf(page, "double 0x%x@ov2659_i2c = %4x\n", ov2659_proc_dt.i2c_addr, tmp16);
	}
	return len;
}

static int proc_ov2659_sread(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len, rc;
	unsigned char tmp8;

	tmp8 = read_cmos_sensor_1(ov2659_proc_dt.i2c_addr);
	rc = 0;
	if (rc < 0) {
		len = sprintf(page, "single byte 0x%x@ov2659_i2c read error\n", ov2659_proc_dt.i2c_addr);
	} else {
		len = sprintf(page, "single byte 0x%x@ov2659_i2c = %4x\n", ov2659_proc_dt.i2c_addr, tmp8);
	}

	return len;
}

static int proc_ov2659_swrite(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int len;

	memset(ov2659_proc_buffer, 0, OV2659_PROC_LEN);
	if (count > OV2659_PROC_LEN)
		len = OV2659_PROC_LEN;
	else
		len = count;

	//printk("count = %d\n", len);
	if (copy_from_user(ov2659_proc_buffer, buffer, len))
		return -EFAULT;
	//printk("%s\n", ov2659_proc_buffer);
	sscanf(ov2659_proc_buffer, "%x", &ov2659_proc_dt.i2c_data);
	//printk("received %x\n", ov2659_proc_dt.i2c_addr);
	write_cmos_sensor(ov2659_proc_dt.i2c_addr, ov2659_proc_dt.i2c_data);

	return len;

}

static int proc_ov2659_addr_read(char *page, char **start, off_t off, int count, int *eof, void *data)
{
	int len;

	len = sprintf(page, "addr is 0x%x\n", ov2659_proc_dt.i2c_addr);
	return len;
}

static int proc_ov2659_addr_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int len;

	memset(ov2659_proc_buffer, 0, OV2659_PROC_LEN);
	if (count > OV2659_PROC_LEN)
		len = OV2659_PROC_LEN;
	else
		len = count;

	//printk("count = %d\n", len);
	if (copy_from_user(ov2659_proc_buffer, buffer, len))
		return -EFAULT;
	//printk("%s\n", ov2659_proc_buffer);
	sscanf(ov2659_proc_buffer, "%x", &ov2659_proc_dt.i2c_addr);
	//printk("received %x\n", ov2659_proc_dt.i2c_addr);
	return len;

}

int ov2659_add_proc(void)
{
	int rc;

	/* add for proc*/
	/* create directory */
	ov2659_dir = proc_mkdir(OV2659_PROC_NAME, NULL);
	if (ov2659_dir == NULL) {
		rc = -ENOMEM;
		goto init_fail;
	}
	//ov2659_dir->owner = THIS_MODULE;

	/* create readfile */
	s_file = create_proc_entry(SINGLE_OP_NAME, 0644, ov2659_dir);
	if (s_file == NULL) {
		rc = -ENOMEM;
		goto no_s;
	}
	s_file->read_proc = proc_ov2659_sread;
	s_file->write_proc = proc_ov2659_swrite;
	//s_file->owner = THIS_MODULE;

	dr_file = create_proc_read_entry(DOUBLE_READ_NAME, 0444, ov2659_dir, proc_ov2659_dread, NULL);
	if (dr_file == NULL) {
		rc = -ENOMEM;
		goto no_dr;
	}
	//dr_file->owner = THIS_MODULE;

	/* create write file */
	w_file = create_proc_entry(I2C_ADDR_NAME, 0644, ov2659_dir);
	if (w_file == NULL) {
		rc = -ENOMEM;
		goto no_wr;
	}

	w_file->read_proc = proc_ov2659_addr_read;
	w_file->write_proc = proc_ov2659_addr_write;
	//w_file->owner = THIS_MODULE;

	/* OK, out debug message */
	//printk(KERN_INFO "%s %s %s initialised\n", SINGLE_OP_NAME, DOUBLE_READ_NAME, I2C_ADDR_NAME);

	/*litao add end*/
	return 0;
	/*litao add for proc*/
no_wr:
	remove_proc_entry(DOUBLE_READ_NAME, ov2659_dir);
no_dr:
	remove_proc_entry(SINGLE_OP_NAME, ov2659_dir);
no_s:
	remove_proc_entry(OV2659_PROC_NAME, NULL);
	/*litao add end*/
init_fail:
	return 1;

}

int ov2659_del_proc(void)
{
	remove_proc_entry(I2C_ADDR_NAME, ov2659_dir);
	remove_proc_entry(DOUBLE_READ_NAME, ov2659_dir);
	remove_proc_entry(SINGLE_OP_NAME, ov2659_dir);
	remove_proc_entry(OV2659_PROC_NAME, NULL);
	//printk(KERN_INFO "%s %s %s removed\n", SINGLE_OP_NAME, DOUBLE_READ_NAME, I2C_ADDR_NAME);
	return 0;
}

//=============================================//

int ov2659_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	printk("ov2659_sensor_init\n");
	ov2659_ctrl = kzalloc(sizeof(struct ov2659_ctrl_t), GFP_KERNEL);
	if (!ov2659_ctrl) {
		CDBG("ov2659_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	if (data)
		ov2659_ctrl->sensordata = data;

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	mdelay(5);

	msm_camio_camif_pad_reg_reset();

	rc = ov2659_sensor_init_probe(data);
	if (rc < 0) {
		CDBG("ov2659_sensor_init failed!\n");
		goto init_fail;
	}
	ov2659_add_proc();

init_done:
	return rc;

init_fail:
	kfree(ov2659_ctrl);
	return rc;
}

static int ov2659_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	CDBG("ov2659_init_client\n");
	init_waitqueue_head(&ov2659_wait_queue);
	return 0;
}

int ov2659_sensor_config(void __user * argp)
{
	struct sensor_cfg_data cfg_data;
	long rc = 0;

	CDBG("ov2659_sensor_config\n");
	if (copy_from_user(&cfg_data, (void *)argp, sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	down(&ov2659_sem);

	printk("cfgtype=%d mode=%d\n", cfg_data.cfgtype, cfg_data.mode);

	switch (cfg_data.cfgtype) {
	case CFG_SET_MODE:
		rc = ov2659_set_sensor_mode(cfg_data.mode);
		break;

	case CFG_SET_EFFECT:
		rc = ov2659_set_effect(cfg_data.mode, cfg_data.cfg.effect);
		break;

	case CFG_SET_NIGHTMODE:
		rc = ov2659_set_nightmode(cfg_data.cfg.value);
		break;

	case CFG_SET_BRIGHTNESS:
		rc = ov2659_set_brightness(cfg_data.cfg.value);
		break;

	case CFG_SET_WB:
		rc = ov2659_set_wb(cfg_data.cfg.value);
		break;

	case CFG_SET_ANTIBANDING:
		rc = ov2659_set_antibanding(cfg_data.cfg.value);
		break;

	default:
		rc = -EFAULT;
		break;
	}

	up(&ov2659_sem);
	printk("config end\n");
	return rc;
}

int ov2659_sensor_release(void)
{
	int rc = 0;

	CDBG("ov2659_sensor_release\n");
	down(&ov2659_sem);

	kfree(ov2659_ctrl);
	up(&ov2659_sem);

	ov2659_del_proc();
	return rc;
}

static int __exit ov2659_i2c_remove(struct i2c_client *client)
{
	struct ov2659_work_t *sensorw = i2c_get_clientdata(client);

	CDBG("ov2659_i2c_remove\n");
	free_irq(client->irq, sensorw);
	ov2659_client = NULL;
	ov2659_sensorw = NULL;
	kfree(sensorw);
	return 0;
}

static int ov2659_i2c_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int rc = 0;

	CDBG("ov2659_i2c_probe\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		goto probe_failure;
	}

	ov2659_sensorw = kzalloc(sizeof(struct ov2659_work), GFP_KERNEL);

	if (!ov2659_sensorw) {
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, ov2659_sensorw);
	ov2659_init_client(client);
	ov2659_client = client;

	CDBG("ov2659_probe succeeded!\n");

	return 0;

probe_failure:
	//kfree(ov2659_sensorw);
	ov2659_sensorw = NULL;
	CDBG("ov2659_probe failed!\n");
	return rc;
}

static const struct i2c_device_id ov2659_i2c_id[] = {
	{"ov2659", 0},
	{},
};

static struct i2c_driver ov2659_i2c_driver = {
	.id_table = ov2659_i2c_id,
	.probe = ov2659_i2c_probe,
	.remove = __exit_p(ov2659_i2c_remove),
	.driver = {
		   .name = "ov2659",
	},
};

static int ov2659_sensor_probe(const struct msm_camera_sensor_info *info, struct msm_sensor_ctrl *s)
{
	int rc;

	printk("ov2659_sensor_probe\n");
	rc = i2c_add_driver(&ov2659_i2c_driver);

	if (rc < 0 || ov2659_client == NULL) {
		rc = -ENOTSUPP;
		goto i2c_probe_error;
	}

	/* Input MCLK = 24MHz */
	msm_camio_clk_rate_set(24000000);
	mdelay(5);

	rc = ov2659_sensor_init_probe(info);
	if (rc < 0)
		goto sensor_init_error;

	s->s_init = ov2659_sensor_init;
	s->s_release = ov2659_sensor_release;
	s->s_config = ov2659_sensor_config;
	s->s_mount_angle  = 90;
	s->s_camera_type = BACK_CAMERA_2D;

	CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;

sensor_init_error:
	i2c_del_driver(&ov2659_i2c_driver);
i2c_probe_error:
	printk("XXX OV2659 sensor probe error!!! XXX\n");

	return rc;
}

static int __ov2659_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, ov2659_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __ov2659_probe,
	.driver = {
		   .name = "msm_camera_ov2659",
		   .owner = THIS_MODULE,
	},
};

static int __init ov2659_init(void)
{
	int rc;

	CDBG("ov2659 init\n");
	rc = platform_driver_register(&msm_camera_driver);
	return rc;

}

module_init(ov2659_init);
