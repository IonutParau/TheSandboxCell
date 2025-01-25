#include "libtsc.h"
#include "pushers.h"
#include "pullers.h"
#include "common.h"
#include <assert.h>

more_movers_ids_t more_movers_ids;

void more_movers_init() {
    more_movers_ids.puller = tsc_registerCell("puller", "Puller", "Pulls the cells behind it");

    tsc_celltable *pullertable = tsc_cell_newTable(more_movers_ids.puller);
    pullertable->update = moremovers_doPuller;
    
    more_movers_ids.fan = tsc_registerCell("fan", "Fan", "Pushes the cell in front");

    tsc_celltable *fantable = tsc_cell_newTable(more_movers_ids.fan);
    fantable->update = moremovers_doFan;

    tsc_category *movers = tsc_getCategory(tsc_rootCategory(), "Movers");
    assert(movers != NULL);

    tsc_subtick_t *moversSub = tsc_subtick_find("movers");
    tsc_subtick_addCell(moversSub, more_movers_ids.fan);
    
    tsc_subtick_t *pullerSubtick = tsc_subtick_addTracked("pullers", 3.1, 0, true);
    tsc_subtick_addCell(pullerSubtick, more_movers_ids.puller);

    tsc_addCell(movers, more_movers_ids.fan);
    tsc_addCell(movers, more_movers_ids.puller);
}
