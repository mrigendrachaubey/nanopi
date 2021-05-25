/*
 * Copyright 2016 Rockchip Electronics Co., Ltd
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "gstvpudec.h"

GST_DEBUG_CATEGORY (CAT_PERFORMANCE);

static gboolean
plugin_init (GstPlugin * plugin)
{
  if (!gst_element_register (plugin, "vpudec", GST_RANK_PRIMARY + 1,
          GST_TYPE_VPUDEC))
    return FALSE;

  GST_DEBUG_CATEGORY_GET (CAT_PERFORMANCE, "GST_PERFORMANCE");

  return TRUE;
}


GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    vpu,
    "VPU decoder",
    plugin_init, VERSION, GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN);
