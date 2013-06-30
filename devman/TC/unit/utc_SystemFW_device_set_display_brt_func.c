#include <tet_api.h>
#include <devman.h>

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_SystemFW_device_set_display_brt_func_01(void);
static void utc_SystemFW_device_set_display_brt_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_SystemFW_device_set_display_brt_func_01, POSITIVE_TC_IDX },
	{ utc_SystemFW_device_set_display_brt_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 },
};

static void startup(void)
{
}

static void cleanup(void)
{
}

/**
 * @brief Positive test case of device_set_display_brt()
 */
static void utc_SystemFW_device_set_display_brt_func_01(void)
{
	int ret_val = 0;
	display_num_t disp = DEV_DISPLAY_0;

	ret_val = device_set_display_brt(disp,6);
	if(ret_val > 0) {
		tet_infoline("device_set_display_brt() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init device_set_display_brt()
 */
static void utc_SystemFW_device_set_display_brt_func_02(void)
{
	int ret_val = 0;

	display_num_t disp = -1;

	ret_val = device_set_display_brt(disp,6);
	if(ret_val >= 0) {
		tet_infoline("device_set_display_brt() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}