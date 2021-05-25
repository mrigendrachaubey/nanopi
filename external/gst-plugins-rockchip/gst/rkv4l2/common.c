/*
 * Copyright 2017 Rockchip Electronics Co., Ltd
 *     Author: Jacob Chen <jacob2.chen@rock-chips.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "gstv4l2object.h"

#include <gst/gst-i18n-plugin.h>

#define SYS_PATH		"/sys/class/video4linux/"
#define DEV_PATH		"/dev/"

enum
{
  PROP_0,
  V4L2_STD_OBJECT_PROPS
};

gboolean
rk_common_v4l2device_find_by_name (const char *name, char *ret_name)
{
  DIR *dir;
  struct dirent *ent;
  gboolean ret = FALSE;

  if ((dir = opendir (SYS_PATH)) != NULL) {
    while ((ent = readdir (dir)) != NULL) {
      FILE *fp;
      char path[512];
      char dev_name[512];

      snprintf (path, 512, SYS_PATH "%s/name", ent->d_name);
      fp = fopen (path, "r");
      if (!fp)
        continue;
      if (!fgets (dev_name, 32, fp))
        dev_name[0] = '\0';
      fclose (fp);

      if (!strstr (dev_name, name))
        continue;

      if (ret_name)
        snprintf (ret_name, 512, DEV_PATH "%s", ent->d_name);

      ret = TRUE;
      break;
    }
    closedir (dir);
  }

  return ret;
}

/*
 * properties
 */

GType
gst_rk_3a_mode_get_type (void)
{
  static GType rk_3a_mode = 0;

  if (!rk_3a_mode) {
    static const GEnumValue modes_3a[] = {
      {RK_3A_DISABLE, "RK_3A_DISABLE", "0A"},
      {RK_3A_AEAWB, "RK_3A_AEAWB", "2A"},
      {RK_3A_AEAWBAF, "RK_3A_AEAWBAF", "3A"},
      {0, NULL, NULL}
    };
    rk_3a_mode = g_enum_register_static ("GstRk3AMode", modes_3a);
  }
  return rk_3a_mode;
}

void
rk_common_install_rockchip_properties_helper (GObjectClass * gobject_class)
{
  /* isp */
  g_object_class_install_property (gobject_class, PROP_3A_MODE,
      g_param_spec_enum ("isp-mode", "ISP 3A mode", " ",
          GST_TYPE_RK_3A_MODE, RK_3A_AEAWB,
          G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_XML_FILE,
      g_param_spec_string ("xml-path", "tuning xml file path",
          " ", " ", G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));
}

gboolean
rk_common_set_property_helper (GstV4l2Object * v4l2object,
    guint prop_id, const GValue * value, GParamSpec * pspec)
{
  /* isp */
  switch (prop_id) {
    case PROP_3A_MODE:
      v4l2object->isp_mode = g_value_get_enum (value);
      break;
    case PROP_XML_FILE:
      v4l2object->xml_path = g_value_dup_string (value);
      break;
    default:
      break;
  }

  return TRUE;
}

gboolean
rk_common_get_property_helper (GstV4l2Object * v4l2object,
    guint prop_id, GValue * value, GParamSpec * pspec)
{
  /* isp */
  switch (prop_id) {
    case PROP_3A_MODE:
      g_value_set_enum (value, v4l2object->isp_mode);
      break;
    case PROP_XML_FILE:
      g_value_set_string (value, v4l2object->xml_path);
      break;
    default:
      break;
  }

  return TRUE;
}

void
rk_common_new (GstV4l2Object * v4l2object)
{
  /* rkisp */
  v4l2object->xml_path = "/etc/cam_iq.xml";
}
