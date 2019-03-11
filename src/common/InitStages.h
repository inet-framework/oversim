//
// Copyright (C) 2006 Institut fuer Telematik, Universitaet Karlsruhe (TH)
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//

/**
 * @file InitStages.h
 * @author Bernhard Heep
 */

#ifndef __INITSTAGES_H_
#define __INITSTAGES_H_

#include "common/OverSimDefs.h"

#include <inet/common/InitStages.h>

/**
 * enumeration for global definiton of init stages
 */
enum INIT_STAGES
{
    MIN_STAGE_UNDERLAY = inet::INITSTAGE_NETWORK_LAYER, /**< first stage for underlay configurators */
    MAX_STAGE_UNDERLAY = inet::INITSTAGE_NETWORK_LAYER_3, /**< last stage for underlay configurators */
    NUM_STAGES_UNDERLAY =
        MAX_STAGE_UNDERLAY -
        MIN_STAGE_UNDERLAY + 1, /**< number of stages for underlay configurators */

    REGISTER_STAGE = inet::NUM_INIT_STAGES,

    MIN_STAGE_COMPONENTS = inet::NUM_INIT_STAGES+1,
    MAX_STAGE_COMPONENTS = inet::NUM_INIT_STAGES+2,
    NUM_STAGES_COMPONENTS = MAX_STAGE_COMPONENTS - MIN_STAGE_COMPONENTS,
    MIN_STAGE_OVERLAY = inet::NUM_INIT_STAGES+3, /**< first stage for overlay modules (Tier 0 / KBR) */
    MAX_STAGE_OVERLAY = inet::NUM_INIT_STAGES+4, /**< last stage for overlay modules (Tier 0 / KBR) */
    NUM_STAGES_OVERLAY =
        MAX_STAGE_OVERLAY -
        MIN_STAGE_OVERLAY + 1, /**< number of stages for overlay modules (Tier 0 / KBR) */

    MIN_STAGE_APP = inet::NUM_INIT_STAGES+5, /**< deprecated */
    MAX_STAGE_APP = inet::NUM_INIT_STAGES+6, /**< deprecated */
    NUM_STAGES_APP = MAX_STAGE_APP - MIN_STAGE_APP + 1, /**< deprecated */

    MIN_STAGE_TIER_1 = MIN_STAGE_APP, /**< first stage for overlay application modules (Tier 1) */
    MAX_STAGE_TIER_1 = MAX_STAGE_APP, /**< last stage for overlay application modules (Tier 1) */
    NUM_STAGES_TIER_1 =
        MAX_STAGE_TIER_1 -
        MIN_STAGE_TIER_1 + 1, /**< number of stages for overlay application modules (Tier 1) */

    MIN_STAGE_TIER_2 = MAX_STAGE_TIER_1+1, /**< first stage for overlay application modules (Tier 2) */
    MAX_STAGE_TIER_2 = MAX_STAGE_TIER_1+2, /**< last stage for overlay application modules (Tier 2) */
    NUM_STAGES_TIER_2 =
        MAX_STAGE_TIER_2 -
        MIN_STAGE_TIER_2 + 1, /**< number of stages for overlay application modules (Tier 2) */

    MIN_STAGE_TIER_3 = MAX_STAGE_TIER_2+1, /**< first stage for overlay application modules (Tier 3) */
    MAX_STAGE_TIER_3 = MAX_STAGE_TIER_2+2, /**< last stage for overlay application modules (Tier 3) */
    NUM_STAGES_TIER_3 =
        MAX_STAGE_TIER_3 -
        MIN_STAGE_TIER_3 + 1, /**< number of stages for overlay application modules (Tier 3) */

    NUM_STAGES_ALL = MAX_STAGE_TIER_3 + 1 /**< total number of stages */
};

#endif
