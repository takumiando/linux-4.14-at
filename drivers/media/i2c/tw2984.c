/*
 * TW2984 driver specific for ATB-A800-NTSC NTSC video decoder board
 *
 * Copyright (C) 2014 Atmark Techno, Inc.
 * Copyright (C) 2014 Makoto Harada <makoto.harada@atmark-techno.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/videodev2.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <media/soc_camera.h>
#include <media/v4l2-subdev.h>
#include <media/v4l2-device.h>
#include <media/v4l2-ctrls.h>
#include <media/v4l2-mediabus.h>

#define TW2984_REG_VIDSTAT1		0x00
#define TW2984_REG_BRIGHT1		0x01
#define TW2984_REG_CONTRAST1		0x02
#define TW2984_REG_SHARPNESS1		0x03
#define TW2984_REG_SAT_U1		0x04
#define TW2984_REG_SAT_V1		0x05
#define TW2984_REG_HUE1			0x06
#define TW2984_REG_CROP_HI1		0x07
#define TW2984_REG_VDELAY_LO1		0x08
#define TW2984_REG_VACTIVE_LO1		0x09
#define TW2984_REG_HDELAY_LO1		0x0a
#define TW2984_REG_HACTIVE_LO1		0x0b
#define TW2984_REG_MVSN1		0x0c
#define TW2984_REG_STATUS21		0x0d
#define TW2984_REG_SDT1			0x0e
#define TW2984_REG_SDTR1		0x0f
#define TW2984_REG_NT501		0xa0
#define TW2984_REG_IDCNTL1		0xa4
#define TW2984_REG_HREF1		0xc4

#define TW2984_REG_VDFREQ		0x50
#define TW2984_REG_FBINTV		0x51
#define TW2984_REG_ANADACTEST		0x52
#define TW2984_REG_ASAVE		0x54
#define TW2984_REG_AAFLPF		0x55
#define TW2984_REG_HASYNC		0x56
#define TW2984_REG_HBLEN1		0x57
#define TW2984_REG_HBLEN2		0x58
#define TW2984_REG_HBLEN3		0x59

#define TW2984_REG_HBLEN4		0x5a
#define TW2984_REG_CKDS			0x5b
#define TW2984_REG_BGCTL		0x5c
#define TW2984_REG_CH2MISC2		0x5d
#define TW2984_REG_CH3MISC2		0x5e
#define TW2984_REG_CH4MISC2		0x5f
#define TW2984_REG_VCO			0x60
#define TW2984_REG_XTIMD		0x61
#define TW2984_REG_O36M			0x62
#define TW2984_REG_CH21NUM		0x63
#define TW2984_REG_CH43NUM		0x64
#define TW2984_REG_VDOEB		0x65
#define TW2984_REG_HZST			0x66

#define TW2984_REG_MISC2		0x96
#define TW2984_REG_CLKOCTL		0xfa

#define TW2984_REG_ADDR_END		0xff

/* TW2984_REG_SHARPNESS1 */
#define SHARP_MASK	(0xf << 0)

/* TW2984_REG_CLKOCTL */
#define OE		(0x1 << 6)
#define CLKNO_OEB	(0x1 << 5)
#define CLKPO_OEB	(0x1 << 4)

#define END 0xff	/* 0xff (DEV_ID) is reads only reg
			   so no problem to use as sentinel*/

const u8 default_regs[] = {
	0x01, 0x00, /* TW2984_REG_BRIGHT */
	0x02, 0x64, /* TW2984_REG_CONTRAST */
	0x03, 0x11, /* TW2984_REG_SHARPNESS */
	0x04, 0x80, /* TW2984_REG_U_GAIN */
	0x05, 0x80, /* TW2984_REG_V_GAIN */
	0x06, 0x00, /* TW2984_REG_HUE */
	0x07, 0x12, /* TW2984_REG_CROP_H */
	0x08, 0x11, /* TW2984_REG_VDELAY_L */
	0x09, 0xe8, /* TW2984_REG_VACTIVE_L */
	0x0a, 0x0a, /* TW2984_REG_HDELAY_L */
	0x0b, 0xd0, /* TW2984_REG_HACTIVE_L */
	0x0e, 0x07, /* TW2984_REG_STD_SEL */
	0x0f, 0x01, /* TW2984_REG_STD_RCG */
	0x56, 0x10, /* TW2984_REG_HASYNC */
	0x57, 0x8a, /* TW2984_REG_HBLEN */
	0x68, 0x00, /* TW2984_REG_HZOOM_H */
	0x69, 0x00, /* TW2984_REG_HZOOM_L */
	0xa0, 0x08, /* TW2984_REG_NT50 */
	0xa4, 0x00, /* TW2984_REG_ID_DET */
	0xa4, 0x1a, /* TW2984_REG_ID_DET */
	0xaa, 0xe0, /* TW2984_REG_VAGC */
	0xab, 0xf0, /* TW2984_REG_VAGC_GAIN */
	0x50, 0x00, /* TW2984_REG_VCLOCK */
	0x51, 0x00, /* TW2984_REG_FBIT_INV */
	0x55, 0x00, /* TW2984_REG_AAFILT */
	0x5b, 0x00, /* TW2984_REG_CLKOUT */
	0x5c, 0x00, /* TW2984_REG_BGCTL */
	0x96, 0xe2, /* TW2984_REG_MISC2 */
	0x60, 0x26, /* TW2984_REG_PLLCTRL */
	0x61, 0x0f, /* TW2984_REG_VCLKSEL */
	0x62, 0x00, /* TW2984_REG_O36M */
	0x63, 0x10, /* TW2984_REG_CHNID12 */
	0x64, 0x32, /* TW2984_REG_CHNID34 */
	0x65, 0x00, /* TW2984_REG_VBUS */
	0x67, 0x80, /* TW2984_REG_HZOOMCNT */
	0x6d, 0x28, /* TW2984_REG_D1NMGAIN */
	0x6e, 0x38, /* TW2984_REG_CLAMPPOS */
	0x81, 0x02, /* TW2984_REG_ANACTRL1 */
	0x82, 0x07, /* TW2984_REG_ANACTRL2 */
	0x83, 0x4c, /* TW2984_REG_CTRL1 */
	0x84, 0x00, /* TW2984_REG_CKHY */
	0x85, 0x30, /* TW2984_REG_VSHARP */
	0x86, 0x44, /* TW2984_REG_VCORE */
	0x87, 0x50, /* TW2984_REG_CLAMPADJ */
	0x88, 0x42, /* TW2984_REG_STANDAGC */
	0x8a, 0xd8, /* TW2984_REG_PEAKWHT */
	0x8b, 0xbc, /* TW2984_REG_CLAMPLEVEL */
	0x8c, 0xb8, /* TW2984_REG_SYNCLEVEL */
	0x8d, 0x44, /* TW2984_REG_SYNCMISS */
	0x8e, 0x36, /* TW2984_REG_WD1CLAMP */
	0x8f, 0x00, /* TW2984_REG_VCTRL1 */
	0x90, 0x00, /* TW2984_REG_VCTRL2 */
	0x91, 0x78, /* TW2984_REG_COLKIL */
	0x92, 0x44, /* TW2984_REG_COMFIL */
	0x93, 0x30, /* TW2984_REG_YDELAY */
	0x94, 0x14, /* TW2984_REG_MISC1 */
	0x95, 0x65, /* TW2984_REG_LOOPCTRL */
	0x97, 0x09, /* TW2984_REG_CLAMPMODE */
	0x9c, 0x20, /* TW2984_REG_FLDDELY */
	0x9e, 0x43, /* TW2984_REG_NOVID */
	0x9f, 0x00, /* TW2984_REG_CLKDELAY */
	0xaf, 0x00, /* TW2984_REG_VPEAK12 */
	0xb0, 0x00, /* TW2984_REG_VPEAK34 */
	0xb1, 0x00, /* TW2984_REG_CH8IDEN */
	0xb2, 0x00, /* TW2984_REG_VADCPOL */
	0xca, 0x00, /* TW2984_REG_CHMD */
	0xcc, 0x35, /* TW2984_REG_SELCH */
	0xcd, 0xc4, /* TW2984_REG_MAINCH */
	0xce, 0x3e, /* TW2984_REG_PWRDOWN */
	0xf9, 0x40, /* TW2984_REG_VMISC */
	0xfa, 0x30, /* TW2984_REG_CONTROL */
	0xfb, 0x23, /* TW2984_REG_CLKPOLE */
	0xfc, 0x01, /* TW2984_REG_AVDETENB */
	END, END,
};

const u8 ntsc_wd1_regs[] = {
	0x07, 0x13, /* TW2984_REG_CROP_H */
	0x0b, 0xc0, /* TW2984_REG_HACTIVE_L */
	0x57, 0xb8, /* TW2984_REG_HBLEN */
	0x50, 0x41, /* TW2984_REG_VCLOCK */
	0x62, 0x10, /* TW2984_REG_O36M */
	0x83, 0xcc, /* TW2984_REG_CTRL1 */
	0xf9, 0x43, /* TW2984_REG_VMISC */
	END, END,
};

const u8 ntsc_d1_regs[] = {
	0x07, 0x12, /* TW2984_REG_CROP_H */
	0x0b, 0xd0, /* TW2984_REG_HACTIVE_L */
	0x57, 0x8a, /* TW2984_REG_HBLEN */
	0x50, 0x00, /* TW2984_REG_VCLOCK */
	0x62, 0x00, /* TW2984_REG_O36M */
	0x83, 0x4c, /* TW2984_REG_CTRL1 */
	0xf9, 0x40, /* TW2984_REG_VMISC */
	END, END,
};

#define NTSC_D1_WIDTH		720
#define NTSC_D1_HEIGHT		480
#define NTSC_D1_ROW_SKIP	122
#define NTSC_D1_COLUMN_SKIP	(19 * 2)

#define NTSC_WD1_WIDTH		960
#define NTSC_WD1_HEIGHT		480
#define NTSC_WD1_ROW_SKIP	162
#define NTSC_WD1_COLUMN_SKIP	(19 * 2)

enum resolution_mode {
	NTSC_D1		= 0,
	NTSC_WD1	= 1,
};

struct support_mode {
	char		*name;
	int		width;
	int		height;
	int		row_skip;
	int		column_skip;
	const u8	*regs;
};

struct tw2984_colorfmt {
	unsigned int 		code;
	enum v4l2_colorspace	colorspace;
};

struct tw2984_priv {
	struct	v4l2_subdev		sd;
	struct	v4l2_ctrl_handler	hdl;

	const struct	tw2984_colorfmt		*cfmt;
	int					cfmt_num;
	const struct	support_mode		*modes;
	int					modes_num;
	enum		resolution_mode		cur_mode;
	struct		v4l2_mbus_config	mbus_cfg;
	enum		v4l2_field		field;
};

static const struct support_mode tw2984_modes[] = {
	{
		.name		= "NTSC_D1",
		.width		= NTSC_D1_WIDTH,
		.height		= NTSC_D1_HEIGHT,
		.row_skip	= NTSC_D1_ROW_SKIP,
		.column_skip	= NTSC_D1_COLUMN_SKIP,
		.regs		= ntsc_d1_regs,
	},
	{
		.name		= "NTSC_WD1",
		.width		= NTSC_WD1_WIDTH,
		.height		= NTSC_WD1_HEIGHT,
		.row_skip	= NTSC_WD1_ROW_SKIP,
		.column_skip	= NTSC_WD1_COLUMN_SKIP,
		.regs		= ntsc_wd1_regs,
	},
};

static const struct tw2984_colorfmt tw2984_cfmts[] = {
	{
		.code		= MEDIA_BUS_FMT_YUYV8_2X8,
		.colorspace	= V4L2_COLORSPACE_JPEG,
	},
};

static int write_reg(struct i2c_client *client, u8 reg, u8 value)
{
	return i2c_smbus_write_byte_data(client, reg , value);
}

static int write_regs(struct i2c_client *client, const u8 *regs)
{
	int ret;
	int i;

	for (i = 0; regs[i] != END; i += 2) {
		ret = write_reg(client, regs[i], regs[i + 1]);
		if (ret < 0)
			return ret;
	}
	return 0;
}

static int read_reg(struct i2c_client *client, u8 reg)
{
	return i2c_smbus_read_byte_data(client, reg);
}

static inline struct tw2984_priv *to_priv(struct v4l2_subdev *sd)
{
	return container_of(sd, struct tw2984_priv, sd);
}

static inline struct tw2984_priv *to_priv_from_ctrl(struct v4l2_ctrl *ctrl)
{
	return container_of(ctrl->handler, struct tw2984_priv, hdl);
}

static int tw2984_s_ctrl(struct v4l2_ctrl *ctrl)
{
	struct tw2984_priv *priv = to_priv_from_ctrl(ctrl);
	struct i2c_client *client = v4l2_get_subdevdata(&priv->sd);
	int ret;

	switch (ctrl->id) {

	case V4L2_CID_BRIGHTNESS:
		return write_reg(client, TW2984_REG_BRIGHT1, ctrl->val);

	case V4L2_CID_CONTRAST:
		return write_reg(client, TW2984_REG_CONTRAST1, ctrl->val);

	case V4L2_CID_SATURATION:
		ret = write_reg(client, TW2984_REG_SAT_U1, ctrl->val);
		if (ret < 0) return ret;
		ret = write_reg(client, TW2984_REG_SAT_V1, ctrl->val);
		return ret;

	case V4L2_CID_HUE:
		return write_reg(client, TW2984_REG_HUE1, ctrl->val);

	default:
		dev_err(&client->dev, "No supported CID 0x%x requested. \n", ctrl->id);
		break;
	}
	return -EINVAL;
}

void disable_clk_data(struct i2c_client *client) {

	u8 tmp;

	tmp = read_reg(client, TW2984_REG_CLKOCTL);
	write_reg(client, TW2984_REG_CLKOCTL, (tmp & ~OE) | CLKNO_OEB | CLKPO_OEB);
}

void enable_clk_data(struct i2c_client *client) {

	u8 tmp;

	tmp = read_reg(client, TW2984_REG_CLKOCTL);
	write_reg(client, TW2984_REG_CLKOCTL, (tmp & ~CLKNO_OEB & ~CLKPO_OEB) | OE );
}

/*
 * soc_camera_ops functions
 */

static int tw2984_get_selection(struct v4l2_subdev *sd,
				struct v4l2_subdev_pad_config *cfg,
				struct v4l2_subdev_selection *sel)
{
	struct tw2984_priv *priv = to_priv(sd);

	if (sel->which != V4L2_SUBDEV_FORMAT_ACTIVE)
		return -EINVAL;

	sel->r.left	= priv->modes[priv->cur_mode].row_skip;
	sel->r.top	= priv->modes[priv->cur_mode].column_skip;
	sel->r.width	= priv->modes[priv->cur_mode].width;
	sel->r.height	= priv->modes[priv->cur_mode].height;

	return 0;
}

static int tw2984_get_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct tw2984_priv *priv      = to_priv(sd);

	if (format->pad)
		return -EINVAL;

	mf->width	= priv->modes[priv->cur_mode].width;
	mf->height	= priv->modes[priv->cur_mode].height;
	mf->code	= priv->cfmt[0].code;
	mf->colorspace	= priv->cfmt[0].colorspace;
	mf->field	= priv->field;

	return 0;
}

void set_nearest_win(struct v4l2_mbus_framefmt *mf, struct tw2984_priv *priv){

	unsigned int diff;
	unsigned int min_diff = 0xffffffff;
	int min_index = 0;
	int i;

	for (i = 0; i < priv->modes_num; i++) {
		diff = abs(priv->modes[i].width - mf->width) +
			abs(priv->modes[i].height - mf->height);

		if (min_diff > diff) {
			min_diff = diff;
			min_index = i;
		}
	}

	mf->width  = priv->modes[min_index].width;
	mf->height = priv->modes[min_index].height;

}

static int tw2984_set_fmt(struct v4l2_subdev *sd,
			  struct v4l2_subdev_pad_config *cfg,
			  struct v4l2_subdev_format *format)
{
	struct v4l2_mbus_framefmt *mf = &format->format;
	struct tw2984_priv *priv      = to_priv(sd);
	struct i2c_client  *client    = v4l2_get_subdevdata(sd);

	if (format->pad)
		return -EINVAL;

	set_nearest_win(mf, priv);

	mf->code	= priv->cfmt[0].code;
	mf->colorspace	= priv->cfmt[0].colorspace;
	mf->field	= priv->field;

	if (mf->width == priv->modes[NTSC_WD1].width) {
		dev_dbg(&client->dev, "Switch to NTSC WD1 mode \n");
		priv->cur_mode = NTSC_WD1;
	} else {
		dev_dbg(&client->dev, "Switch to NTSC D1 mode \n");
		priv->cur_mode = NTSC_D1;
	}

	write_regs(client, priv->modes[priv->cur_mode].regs);

	return 0;
}

static int tw2984_enum_mbus_code(struct v4l2_subdev *sd,
				 struct v4l2_subdev_pad_config *cfg,
				 struct v4l2_subdev_mbus_code_enum *code)
{
	struct tw2984_priv *priv = to_priv(sd);

	if (code->pad || code->index >= priv->cfmt_num)
		return -EINVAL;

	code->code = priv->cfmt[code->index].code;

	return 0;
}

static int tw2984_enum_frame_size(struct v4l2_subdev *sd,
				  struct v4l2_subdev_pad_config *cfg,
				  struct v4l2_subdev_frame_size_enum *fse)
{
	struct tw2984_priv *priv = to_priv(sd);

	if (fse->pad)
		return -EINVAL;

	if (fse->index == NTSC_D1){
		fse->min_width = fse->max_width = priv->modes[NTSC_D1].width;
		fse->min_height = fse->max_height = priv->modes[NTSC_D1].height;
	} else if (fse->index == NTSC_WD1){
		fse->min_width = fse->max_width = priv->modes[NTSC_WD1].width;
		fse->min_height = fse->max_height = priv->modes[NTSC_WD1].height;
	}

	return 0;
}

static int tw2984_g_mbus_config(struct v4l2_subdev *sd,
				struct v4l2_mbus_config *cfg)
{
	struct tw2984_priv *priv = to_priv(sd);

	cfg->flags	= priv->mbus_cfg.flags;
	cfg->type	= priv->mbus_cfg.type;

	return 0;
}

/*
 * V4L2 subdev core and video operations
 */
static int tw2984_set_power(struct v4l2_subdev *sd, int on)
{
	struct i2c_client *client = v4l2_get_subdevdata(sd);

	if (!on) {
		disable_clk_data(client);
	} else {
		enable_clk_data(client);
	}

	return 0;
}

static const struct v4l2_ctrl_ops tw2984_ctrl_ops = {
	.s_ctrl		= tw2984_s_ctrl,
};

static const struct v4l2_subdev_video_ops tw2984_video_ops = {
	.g_mbus_config	= tw2984_g_mbus_config,
};

static const struct v4l2_subdev_core_ops tw2984_core_ops = {
	.s_power	= tw2984_set_power,
};

static const struct v4l2_subdev_pad_ops tw2984_pad_ops = {
	.enum_mbus_code = tw2984_enum_mbus_code,
	.enum_frame_size = tw2984_enum_frame_size,
	.get_selection = tw2984_get_selection,
	.get_fmt = tw2984_get_fmt,
	.set_fmt = tw2984_set_fmt,
};

static const struct v4l2_subdev_ops tw2984_ops = {
	.core		= &tw2984_core_ops,
	.video		= &tw2984_video_ops,
	.pad		= &tw2984_pad_ops,
};

static int tw2984_reg_init(struct i2c_client *client)
{

	return write_regs(client, default_regs);

};

static int tw2984_probe(struct i2c_client *client,
			    const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = client->adapter;
	struct tw2984_priv *priv;
	struct v4l2_subdev *sd;
	int ret;

	v4l_info(client, "chip found @ 0x%02x (%s)\n",
			client->addr, client->adapter->name);

	if (!i2c_check_functionality(adapter, I2C_FUNC_SMBUS_BYTE_DATA))
		return -ENODEV;

	/* tw2984 private struct init */
	priv = kzalloc(sizeof(struct tw2984_priv), GFP_KERNEL);
	if (priv == NULL)
		return -ENOMEM;

	priv->cfmt		= tw2984_cfmts;
	priv->cfmt_num		= ARRAY_SIZE(tw2984_cfmts);
	priv->cur_mode		= NTSC_D1;
	priv->modes		= tw2984_modes;
	priv->modes_num		= ARRAY_SIZE(tw2984_modes);
	priv->mbus_cfg.flags	= V4L2_MBUS_PCLK_SAMPLE_RISING | V4L2_MBUS_MASTER |
				  V4L2_MBUS_VSYNC_ACTIVE_HIGH | V4L2_MBUS_HSYNC_ACTIVE_HIGH |
				  V4L2_MBUS_DATA_ACTIVE_HIGH;
	priv->mbus_cfg.type	= V4L2_MBUS_PARALLEL;
	priv->field		= V4L2_FIELD_INTERLACED_TB;

	/* subdev init */
	sd = &priv->sd;
	v4l2_i2c_subdev_init(sd, client, &tw2984_ops);

	/* vl42_ctrl init */
	v4l2_ctrl_handler_init(&priv->hdl, 10);

	v4l2_ctrl_new_std(&priv->hdl, &tw2984_ctrl_ops,
				V4L2_CID_BRIGHTNESS, -128, 128, 1, 0x00);
	v4l2_ctrl_new_std(&priv->hdl, &tw2984_ctrl_ops,
				V4L2_CID_CONTRAST, 0, 255, 1, 0x64);
	v4l2_ctrl_new_std(&priv->hdl, &tw2984_ctrl_ops,
				V4L2_CID_SATURATION, 0, 255, 1, 0x80);
	v4l2_ctrl_new_std(&priv->hdl, &tw2984_ctrl_ops,
				V4L2_CID_HUE, -128, 128, 1, 0x00);

	sd->ctrl_handler = &priv->hdl;
	if (priv->hdl.error) {
		ret = priv->hdl.error;
		goto free_priv;
	}

	ret = tw2984_reg_init(client);
	if (ret) {
		goto free_ctrl_hdl;
	}

	ret = v4l2_async_register_subdev(sd);
	if (ret) {
		goto free_ctrl_hdl;
	}

	return 0;

free_ctrl_hdl:
	v4l2_ctrl_handler_free(&priv->hdl);

free_priv:
	kfree(priv);

	return ret;
}

static int tw2984_remove(struct i2c_client *client)
{
	struct v4l2_subdev *sd = i2c_get_clientdata(client);
	struct tw2984_priv *priv = to_priv(sd);

	v4l2_device_unregister_subdev(sd);
	v4l2_ctrl_handler_free(&priv->hdl);
	kfree(priv);

	return 0;
}

static const struct i2c_device_id tw2984_id[] = {
	{ "tw2984", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, tw2984_id);

#if IS_ENABLED(CONFIG_OF)
static const struct of_device_id tw2984_of_match[] = {
	{ .compatible = "renesas,tw2984", },
	{ /* sentinel */ },
};
MODULE_DEVICE_TABLE(of, tw2984_of_match);
#endif

static struct i2c_driver tw2984_driver = {
	.driver = {
		.of_match_table = of_match_ptr(tw2984_of_match),
		.name	= "tw2984",
	},
	.probe		= tw2984_probe,
	.remove		= tw2984_remove,
	.id_table	= tw2984_id,
};

module_i2c_driver(tw2984_driver);

MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("tw2984 ATB-A800-NTSC V4L2 i2c driver");
MODULE_AUTHOR("Makoto Harada <makoto.harada@atmark-techno.com>");
