/*******************************************************************************
 *	Perf chk interfaces
 ******************************************************************************/
void __maybe_unused vi_record_sof_perf(struct sop_vi_dev *vdev, u8 raw_num, u8 chn_num);
void __maybe_unused vi_record_fe_perf(struct sop_vi_dev *vdev, u8 raw_num, u8 chn_num);
void __maybe_unused vi_record_be_perf(struct sop_vi_dev *vdev, u8 raw_num, u8 chn_num);
void __maybe_unused vi_record_post_end(struct sop_vi_dev *vdev, u8 raw_num);
void __maybe_unused vi_record_post_trigger(struct sop_vi_dev *vdev, u8 raw_num);
void __maybe_unused vi_perf_record_dump(void);

