/***************************************************************
** Copyright (C),  2020,  OPPO Mobile Comm Corp.,  Ltd
** VENDOR_EDIT
** File : oppo_aod.c
** Description : oppo aod feature
** Version : 1.0
** Date : 2020/04/23
**
** ------------------------------- Revision History: -----------
**  <author>        <data>        <version >        <desc>
**   Qianxu         2020/04/23        1.0           Build this moudle
******************************************************************/

#include "dsi_defs.h"
#include "sde_crtc.h"
#include "oppo_aod.h"
#include <drm/drm_mipi_dsi.h>

int aod_light_mode = 1;
DEFINE_MUTEX(oppo_aod_light_mode_lock);

int __oppo_display_set_aod_light_mode(int mode) {
	mutex_lock(&oppo_aod_light_mode_lock);
	if(mode != aod_light_mode) {
		aod_light_mode = mode;
	}
	mutex_unlock(&oppo_aod_light_mode_lock);
	return 0;
}

int oppo_update_aod_light_mode_unlock(struct dsi_panel *panel)
{
	int rc = 0;
	enum dsi_cmd_set_type type;
	int bl = 0;
	struct mipi_dsi_device *mipi_device;
	mipi_device = &panel->mipi_device;
	int threshold = panel->oppo_priv.aod_low_brightness_threshold;

	if (threshold != 0) {
		if (panel->bl_config.bl_level > threshold)
			aod_light_mode = 0;
		else
			aod_light_mode = 1;
	}


	if (aod_light_mode == 1) {
		type = DSI_CMD_AOD_LOW_LIGHT_MODE;
		bl = 27;
	} else {
		type = DSI_CMD_AOD_HIGH_LIGHT_MODE;
		bl = 137;
	}

	if (!strcmp(panel->oppo_priv.vendor_name, "S6E3HC2"))
		rc = mipi_dsi_dcs_set_display_brightness(mipi_device, bl);
	else
		rc = dsi_panel_tx_cmd_set(panel, type);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_AOD_LIGHT_MODE cmds, rc=%d\n",
		       panel->name, rc);
	}

	return rc;
}

#ifdef OPLUS_FEATURE_AOD_RAMLESS
extern bool is_oppo_display_aod_mode(void);
#endif /* OPLUS_FEATURE_AOD_RAMLESS */
int oppo_update_aod_light_mode(void)
{
	struct dsi_display *display = get_main_display();
	int ret = 0;

	if (!display || !display->panel) {
		printk(KERN_INFO "oppo_set_aod_light_mode and main display is null");
		return -EINVAL;
	}

	if (display->panel->is_hbm_enabled) {
		pr_err("%s error panel->is_hbm_enabled\n", __func__);
		return -EINVAL;
	}

	if (get_oppo_display_scene() != OPPO_DISPLAY_AOD_SCENE) {
		pr_err("%s error get_oppo_display_scene = %d, \n", __func__, get_oppo_display_scene());
		return -EFAULT;
	}
	mutex_lock(&display->display_lock);
	/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	mutex_lock(&display->panel->panel_lock);

#ifdef OPLUS_FEATURE_AOD_RAMLESS
	if (display->panel->oppo_priv.is_aod_ramless &&
			!is_oppo_display_aod_mode()) {
		pr_err("not support update aod_light_mode at non-aod mode\n");
		ret = -EINVAL;
		goto error;
	}
#endif /* OPLUS_FEATURE_AOD_RAMLESS */

	if (!dsi_panel_initialized(display->panel)) {
		pr_err("dsi_panel_aod_low_light_mode is not init\n");
		ret = -EINVAL;
		goto error;
	}

	ret = oppo_update_aod_light_mode_unlock(display->panel);

	if (ret) {
		pr_err("failed to set aod light status ret=%d", ret);
		goto error;
	}

error:
	mutex_unlock(&display->panel->panel_lock);
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_OFF);
	}
	mutex_unlock(&display->display_lock);

	return ret;
}

int oppo_panel_set_aod_light_mode(void *buf)
{
	unsigned int *temp_save = buf;

	__oppo_display_set_aod_light_mode(*temp_save);
	oppo_update_aod_light_mode();

	return 0;
}

int oppo_panel_get_aod_light_mode(void *buf)
{
	unsigned int *aod_mode = buf;
	(*aod_mode) = aod_light_mode;

	printk(KERN_INFO "oppo_get_aod_light_mode = %d\n", aod_light_mode);

	return 0;
}

int dsi_panel_aod_on(struct dsi_panel *panel) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_AOD_ON);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_AOD_ON cmds, rc=%d\n",
		       panel->name, rc);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_panel_aod_off(struct dsi_panel *panel) {
	int rc = 0;

	if (!panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&panel->panel_lock);

	if (!dsi_panel_initialized(panel)) {
		rc = -EINVAL;
		goto error;
	}

	rc = dsi_panel_tx_cmd_set(panel, DSI_CMD_AOD_OFF);
	if (rc) {
		pr_err("[%s] failed to send DSI_CMD_AOD_OFF cmds, rc=%d\n",
		       panel->name, rc);
	}

error:
	mutex_unlock(&panel->panel_lock);
	return rc;
}

int dsi_display_aod_on(struct dsi_display *display) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_aod_on(display->panel);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_aod_on, rc=%d\n",
			       display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_display_aod_off(struct dsi_display *display) {
	int rc = 0;
	if (!display || !display->panel) {
		pr_err("Invalid params\n");
		return -EINVAL;
	}

	mutex_lock(&display->display_lock);

		/* enable the clk vote for CMD mode panels */
	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
		dsi_display_clk_ctrl(display->dsi_clk_handle,
			DSI_CORE_CLK, DSI_CLK_ON);
	}

	rc = dsi_panel_aod_off(display->panel);
		if (rc) {
			pr_err("[%s] failed to dsi_panel_aod_off, rc=%d\n",
			       display->name, rc);
	}

	if (display->config.panel_mode == DSI_OP_CMD_MODE) {
	rc = dsi_display_clk_ctrl(display->dsi_clk_handle,
				DSI_CORE_CLK, DSI_CLK_OFF);
	}
	mutex_unlock(&display->display_lock);
	return rc;
}

int dsi_panel_parse_oplus_aod_high_brightness_config(struct dsi_panel *panel)
{
	int rc = 0;
	int i;
	u32 length = 0;
	u32 count = 0;
	u32 size = 0;
	u32 *arr_32 = NULL;
	const u32 *arr;
	struct dsi_parser_utils *utils = &panel->utils;
	struct oppo_brightness_alpha *seq;

	if (panel->host_config.ext_bridge_num)
		return 0;

	arr = utils->get_property(utils->data, "oplus,dsi-aod-high-brightness", &length);
	if (!arr) {
		DSI_DEBUG("[%s] oplus,dsi-aod-high-brightness not found\n", panel->name);
		return -EINVAL;
	}

	if (length & 0x1) {
		DSI_ERR("[%s] oplus,dsi-aod-high-brightness length error\n", panel->name);
		return -EINVAL;
	}

	DSI_DEBUG("RESET SEQ LENGTH = %d\n", length);
	length = length / sizeof(u32);
	size = length * sizeof(u32);

	arr_32 = kzalloc(size, GFP_KERNEL);
	if (!arr_32) {
		rc = -ENOMEM;
		goto error;
	}

	rc = utils->read_u32_array(utils->data, "oplus,dsi-aod-high-brightness",
					arr_32, length);
	if (rc) {
		DSI_ERR("[%s] cannot read dsi-aod-high-brightness\n", panel->name);
		goto error_free_arr_32;
	}

	count = length / 2;
	size = count * sizeof(*seq);
	seq = kzalloc(size, GFP_KERNEL);
	if (!seq) {
		rc = -ENOMEM;
		goto error_free_arr_32;
	}

	panel->aod_high_ba_seq = seq;
	panel->aod_high_ba_count = count;

	for (i = 0; i < length; i += 2) {
		seq->brightness = arr_32[i];
		seq->alpha = arr_32[i + 1];
		seq++;
	}

error_free_arr_32:
	kfree(arr_32);
error:
	return rc;
}

int dsi_panel_parse_oplus_aod_low_brightness_config(struct dsi_panel *panel)
{
	int rc = 0;
	int i;
	u32 length = 0;
	u32 count = 0;
	u32 size = 0;
	u32 *arr_32 = NULL;
	const u32 *arr;
	struct dsi_parser_utils *utils = &panel->utils;
	struct oppo_brightness_alpha *seq;

	if (panel->host_config.ext_bridge_num)
		return 0;

	arr = utils->get_property(utils->data, "oplus,dsi-aod-low-brightness", &length);
	if (!arr) {
		DSI_DEBUG("[%s] oplus,dsi-aod-low-brightness not found\n", panel->name);
		return -EINVAL;
	}

	if (length & 0x1) {
		DSI_ERR("[%s] oplus,dsi-aod-low-brightness length error\n", panel->name);
		return -EINVAL;
	}

	DSI_DEBUG("RESET SEQ LENGTH = %d\n", length);
	length = length / sizeof(u32);
	size = length * sizeof(u32);

	arr_32 = kzalloc(size, GFP_KERNEL);
	if (!arr_32) {
		rc = -ENOMEM;
		goto error;
	}

	rc = utils->read_u32_array(utils->data, "oplus,dsi-aod-low-brightness",
					arr_32, length);
	if (rc) {
		DSI_ERR("[%s] cannot read dsi-aod-low-brightness\n", panel->name);
		goto error_free_arr_32;
	}

	count = length / 2;
	size = count * sizeof(*seq);
	seq = kzalloc(size, GFP_KERNEL);
	if (!seq) {
		rc = -ENOMEM;
		goto error_free_arr_32;
	}

	panel->aod_low_ba_seq = seq;
	panel->aod_low_ba_count = count;

	for (i = 0; i < length; i += 2) {
		seq->brightness = arr_32[i];
		seq->alpha = arr_32[i + 1];
		seq++;
	}

error_free_arr_32:
	kfree(arr_32);
error:
	return rc;
}

void dsi_panel_parse_oplus_aod_config(struct dsi_panel *panel)
{
	struct dsi_parser_utils *utils = &panel->utils;
	int ret = 0;

	ret = dsi_panel_parse_oplus_aod_high_brightness_config(panel);
	if (ret) {
		pr_err("[%s] could not parse aod high brightness config\n", __func__);
	}

	ret = dsi_panel_parse_oplus_aod_low_brightness_config(panel);
	if (ret) {
		pr_err("[%s] could not parse aod low brightness config\n", __func__);
	}

	ret = utils->read_u32(utils->data, "oplus,dsi-aod-low-brightness-threshold",
			&panel->oppo_priv.aod_low_brightness_threshold);
	if (ret) {
		pr_err("[%s]failed get panel parameter: oplus,dsi-aod-low-brightness-threshold\n", __func__);
		panel->oppo_priv.aod_low_brightness_threshold = 0;
	} else {
		DSI_INFO("oplus,dsi-aod-low-brightness-threshold: %d", panel->oppo_priv.aod_low_brightness_threshold);
	}
}
