#include <vi_defines.h>
#include "module/osal_math.h"
#define ISP_PERF_MEASURE
#ifdef ISP_PERF_MEASURE
#define ISP_MEASURE_FRM	100
#define STOUS		1000000

struct isp_perf_chk {
	osal_timeval sof_time[ISP_PRERAW_MAX][ISP_MEASURE_FRM];
	osal_timeval eof_time[ISP_PRERAW_MAX][ISP_MEASURE_FRM];
	osal_timeval post_trig[VI_MAX_PIPE_NUM][ISP_MEASURE_FRM];
	osal_timeval post_eof[VI_MAX_PIPE_NUM][ISP_MEASURE_FRM];

	u8 sof_end;
	u8 pre_fe_end;
	u8 post_end;
};

static struct isp_perf_chk time_chk;
#endif //ISP_PERF_MEASURE

void vi_record_sof_perf(struct sop_vi_dev *vdev, u8 raw_num, u8 chn_num)
{
#ifdef ISP_PERF_MEASURE
	if (vdev->pre_fe_sof_cnt[raw_num][chn_num] < ISP_MEASURE_FRM) {
		osal_timeval osal_tv;

		osal_gettimeofday(&osal_tv);

		time_chk.sof_time[raw_num][vdev->pre_fe_sof_cnt[raw_num][chn_num]].tv_sec = osal_tv.tv_sec;
		time_chk.sof_time[raw_num][vdev->pre_fe_sof_cnt[raw_num][chn_num]].tv_usec = osal_tv.tv_usec;

		if (vdev->pre_fe_sof_cnt[raw_num][chn_num] == ISP_MEASURE_FRM - 1)
			time_chk.sof_end = true;
	}
#endif
}

void vi_record_fe_perf(struct sop_vi_dev *vdev, u8 raw_num, u8 chn_num)
{
#ifdef ISP_PERF_MEASURE
	if (vdev->pre_fe_frm_num[raw_num][chn_num] < ISP_MEASURE_FRM) {
		osal_timeval osal_tv;

		osal_gettimeofday(&osal_tv);

		time_chk.eof_time[raw_num][vdev->pre_fe_frm_num[raw_num][chn_num]].tv_sec = osal_tv.tv_sec;
		time_chk.eof_time[raw_num][vdev->pre_fe_frm_num[raw_num][chn_num]].tv_usec = osal_tv.tv_usec;

		if (vdev->pre_fe_frm_num[raw_num][chn_num] == ISP_MEASURE_FRM - 1)
			time_chk.pre_fe_end = true;
	}
#endif
}

void vi_record_post_end(struct sop_vi_dev *vdev, u8 pipe)
{
#ifdef ISP_PERF_MEASURE
	if (vdev->postraw_frame_number[pipe] < ISP_MEASURE_FRM) {
		osal_timeval osal_tv;

		osal_gettimeofday(&osal_tv);

		time_chk.post_eof[pipe][vdev->postraw_frame_number[pipe]].tv_sec = osal_tv.tv_sec;
		time_chk.post_eof[pipe][vdev->postraw_frame_number[pipe]].tv_usec = osal_tv.tv_usec;

		if (vdev->postraw_frame_number[pipe] == ISP_MEASURE_FRM - 1)
			time_chk.post_end = true;
	}
#endif
}

void vi_record_post_trigger(struct sop_vi_dev *vdev, u8 pipe)
{
#ifdef ISP_PERF_MEASURE
	if (vdev->postraw_frame_number[pipe] < ISP_MEASURE_FRM) {
		osal_timeval osal_tv;

		osal_gettimeofday(&osal_tv);

		time_chk.post_trig[pipe][vdev->postraw_frame_number[pipe]].tv_sec = osal_tv.tv_sec;
		time_chk.post_trig[pipe][vdev->postraw_frame_number[pipe]].tv_usec = osal_tv.tv_usec;
	}
#endif
}

void vi_perf_record_dump(void)
{
#ifdef ISP_PERF_MEASURE
	u64 time_0 = 0, time_1 = 0;
	u32 i = 0;
	u64 sof_sum = 0, eof_sum = 0, fe_cost = 0, post_cost = 0;
	u8 raw_num = 0, pipe = 0;

	if (!(time_chk.sof_end && time_chk.pre_fe_end && time_chk.post_end))
		return;

	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {

		time_0 = (time_chk.sof_time[raw_num][i].tv_sec * STOUS) + time_chk.sof_time[raw_num][i].tv_usec;
		time_1 = (time_chk.sof_time[raw_num][i + 1].tv_sec * STOUS) + time_chk.sof_time[raw_num][i + 1].tv_usec;

		sof_sum += (time_1 - time_0);

		time_0 = (time_chk.eof_time[raw_num][i].tv_sec * STOUS) + time_chk.eof_time[raw_num][i].tv_usec;
		time_1 = (time_chk.eof_time[raw_num][i + 1].tv_sec * STOUS) + time_chk.eof_time[raw_num][i + 1].tv_usec;

		eof_sum += (time_1 - time_0);
	}

	vi_pr(VI_DBG, "SOF interval avg=%llu\n", osal_div_u64(sof_sum, i));
	vi_pr(VI_DBG, "EOF interval avg=%llu\n", osal_div_u64(eof_sum, i));

	for (i = 0; i < ISP_MEASURE_FRM - 1; i++) {

		time_0 = (time_chk.sof_time[raw_num][i].tv_sec * STOUS) + time_chk.sof_time[raw_num][i].tv_usec;
		time_1 = (time_chk.eof_time[raw_num][i].tv_sec * STOUS) + time_chk.eof_time[raw_num][i].tv_usec;
		fe_cost += (time_1 - time_0);

		time_0 = (time_chk.post_trig[pipe][i].tv_sec * STOUS) + time_chk.post_trig[pipe][i].tv_usec;
		time_1 = (time_chk.post_eof[pipe][i].tv_sec * STOUS) + time_chk.post_eof[pipe][i].tv_usec;

		post_cost += (time_1 - time_0);
	}

	vi_pr(VI_DBG, "SOF->EOF cost time avg=%llu\n", osal_div_u64(fe_cost, i));
	vi_pr(VI_DBG, "Post[trig->done]=%llu\n", osal_div_u64(post_cost, i));

	time_chk.post_end = false;
	time_chk.pre_fe_end = false;
	time_chk.sof_end = false;
#endif
}
