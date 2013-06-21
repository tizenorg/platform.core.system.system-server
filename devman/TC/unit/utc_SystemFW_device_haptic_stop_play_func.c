#include <tet_api.h>
#include <devman.h>
#include <devman_haptic.h>

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_SystemFW_device_haptic_stop_play_func_01(void);
static void utc_SystemFW_device_haptic_stop_play_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_SystemFW_device_haptic_stop_play_func_01, POSITIVE_TC_IDX },
	{ utc_SystemFW_device_haptic_stop_play_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0 },
};

static void startup(void)
{
}

static void cleanup(void)
{
}

/**
 * @brief Positive test case of device_haptic_stop_play()
 */
static void utc_SystemFW_device_haptic_stop_play_func_01(void)
{
	int ret_val = 0;
	haptic_dev_idx dev_idx = DEV_IDX_0;
	unsigned int mode =0;
	int dev_handle = 0;

	dev_handle = device_haptic_open(dev_idx,mode);
	if(dev_handle < 0)
	{
		tet_infoline("device_haptic_stop_play() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}

	ret_val = device_haptic_play_monotone(dev_handle ,100);                    
	if(ret_val < 0)
	{
		tet_infoline("device_haptic_stop_play() failed in positive test case");
		tet_result(TET_FAIL);
		device_haptic_close(dev_handle);
		return;
	}

	 ret_val = device_haptic_stop_play(dev_handle);
	 if(ret_val < 0)
	 {
		 tet_infoline("device_haptic_stop_play() failed in positive test case");
		 tet_result(TET_FAIL);
		 device_haptic_close(dev_handle);
		 return;
	 }
					 

	device_haptic_close(dev_handle);
	tet_result(TET_PASS);
	int r = 0;

/*
   	r = device_haptic_stop_play(...);
*/
	if (r) {
		tet_infoline("device_haptic_stop_play() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init device_haptic_stop_play()
 */
static void utc_SystemFW_device_haptic_stop_play_func_02(void)
{
	int r = 0;


   	r = device_haptic_stop_play(-1);

	if (r>=0) {
		tet_infoline("device_haptic_stop_play() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}