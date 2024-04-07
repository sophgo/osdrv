// $Module: isp_rdma $
// $RegisterBank Version: V 1.0.00 $
// $Author: Siwi $
// $Date: Sun, 26 Sep 2021 04:20:44 PM $
//

#ifndef __REG_ISP_RDMA_STRUCT_H__
#define __REG_ISP_RDMA_STRUCT_H__

typedef unsigned int uint32_t;
typedef union {
	struct {
		/*Shadow reg read select
		0 : read active reg
		1 : read shadow reg;*/
		uint32_t reg_shadow_rd_sel:1;
		/*;*/
		uint32_t reg_abort_mode:1;
		uint32_t rsv_2_7:6;
		/*max ostanding per client;*/
		uint32_t reg_max_ostd:8;
		/*enable sw to overwrite max ostanding
		0 : hw mode , max ostanding is 2
		1 : sw mode , overwrite by reg_max_ostd;*/
		uint32_t reg_ostd_sw_en:1;
	};
	uint32_t val;
} ISP_RDMA_SHADOW_RD_SEL_C;
// typedef union {
// 	struct {
// 		/*IP Disable;*/
// 		uint32_t reg_ip_disable:32;
// 	};
// 	uint32_t val;
// } ISP_RDMA_IP_DISABLE_C;
typedef union {
	struct {
		uint32_t reg_up_ring_base:32;
	};
	uint32_t val;
} ISP_RDMA_UP_RING_BASE_C;
typedef union {
	struct {
		/*Abort done flag;*/
		uint32_t reg_abort_done:1;
		uint32_t rsv_1_3:3;
		/*Error flag : AXI response error;*/
		uint32_t reg_error_axi:1;
		uint32_t rsv_5_7:3;
		/*Error client ID;*/
		uint32_t reg_error_id:5;
		uint32_t rsv_13_15:3;
		/*Date of latest update;*/
		uint32_t reg_dma_version:16;
	};
	uint32_t val;
} ISP_RDMA_NORM_STATUS0_C;
typedef union {
	struct {
		/*reg_id_idle[id] : idle status of client id
		(RDMA is not active or it has transferred all data to the client);*/
		uint32_t reg_id_idle:32;
	};
	uint32_t val;
} ISP_RDMA_NORM_STATUS1_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_ring_enable:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_EN_C;
typedef union {
	struct {
		/*Bandwidth limiter window size;*/
		uint32_t reg_bwlwin:10;
		/*Bandwidth limiter transaction number;*/
		uint32_t reg_bwltxn:6;
	};
	uint32_t val;
} ISP_RDMA_NORM_PERF_C;
typedef union {
	struct {
		/*0 : selected by max total burst cnt
		1 : selected by max remain cnt;*/
		uint32_t reg_ar_priority_sel:1;
		/*0 : selected by remain slot number
		1 : selected by max remain cnt;*/
		uint32_t reg_qos_priority_sel:1;
		/*arbiter hist_disable;*/
		uint32_t reg_arb_hist_disable:1;
		uint32_t rsv_3_3:1;
		/*arbiter usage threshold;*/
		uint32_t reg_arb_usage_th:4;
	};
	uint32_t val;
} ISP_RDMA_AR_PRIORITY_SEL_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_ring_patch_enable:32;
	};
	uint32_t val;
} ISP_RDMA_RING_PATCH_ENABLE_C;
typedef union {
	struct {
		/*it's work if reg_ring_patch_enable = 1;*/
		uint32_t reg_set_ring_base:32;
	};
	uint32_t val;
} ISP_RDMA_SET_RING_BASE_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_ring_base_l:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BASE_ADDR_L_C;
typedef union {
	struct {
		/*;*/
		uint32_t reg_ring_base_h:8;
	};
	uint32_t val;
} ISP_RDMA_RING_BASE_ADDR_H_C;
typedef union {
	struct {
		/*ring buffer size for client0;*/
		uint32_t reg_rbuf_size0:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE0_C;
typedef union {
	struct {
		/*ring buffer size for client1;*/
		uint32_t reg_rbuf_size1:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE1_C;
typedef union {
	struct {
		/*ring buffer size for client2;*/
		uint32_t reg_rbuf_size2:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE2_C;
typedef union {
	struct {
		/*ring buffer size for client3;*/
		uint32_t reg_rbuf_size3:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE3_C;
typedef union {
	struct {
		/*ring buffer size for client4;*/
		uint32_t reg_rbuf_size4:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE4_C;
typedef union {
	struct {
		/*ring buffer size for client5;*/
		uint32_t reg_rbuf_size5:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE5_C;
typedef union {
	struct {
		/*ring buffer size for client6;*/
		uint32_t reg_rbuf_size6:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE6_C;
typedef union {
	struct {
		/*ring buffer size for client7;*/
		uint32_t reg_rbuf_size7:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE7_C;
typedef union {
	struct {
		/*ring buffer size for client8;*/
		uint32_t reg_rbuf_size8:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE8_C;
typedef union {
	struct {
		/*ring buffer size for client9;*/
		uint32_t reg_rbuf_size9:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE9_C;
typedef union {
	struct {
		/*ring buffer size for client10;*/
		uint32_t reg_rbuf_size10:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE10_C;
typedef union {
	struct {
		/*ring buffer size for client11;*/
		uint32_t reg_rbuf_size11:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE11_C;
typedef union {
	struct {
		/*ring buffer size for client12;*/
		uint32_t reg_rbuf_size12:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE12_C;
typedef union {
	struct {
		/*ring buffer size for client13;*/
		uint32_t reg_rbuf_size13:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE13_C;
typedef union {
	struct {
		/*ring buffer size for client14;*/
		uint32_t reg_rbuf_size14:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE14_C;
typedef union {
	struct {
		/*ring buffer size for client15;*/
		uint32_t reg_rbuf_size15:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE15_C;
typedef union {
	struct {
		/*ring buffer size for client16;*/
		uint32_t reg_rbuf_size16:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE16_C;
typedef union {
	struct {
		/*ring buffer size for client17;*/
		uint32_t reg_rbuf_size17:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE17_C;
typedef union {
	struct {
		/*ring buffer size for client18;*/
		uint32_t reg_rbuf_size18:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE18_C;
typedef union {
	struct {
		/*ring buffer size for client19;*/
		uint32_t reg_rbuf_size19:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE19_C;
typedef union {
	struct {
		/*ring buffer size for client20;*/
		uint32_t reg_rbuf_size20:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE20_C;
typedef union {
	struct {
		/*ring buffer size for client21;*/
		uint32_t reg_rbuf_size21:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE21_C;
typedef union {
	struct {
		/*ring buffer size for client22;*/
		uint32_t reg_rbuf_size22:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE22_C;
typedef union {
	struct {
		/*ring buffer size for client23;*/
		uint32_t reg_rbuf_size23:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE23_C;
typedef union {
	struct {
		/*ring buffer size for client24;*/
		uint32_t reg_rbuf_size24:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE24_C;
typedef union {
	struct {
		/*ring buffer size for client25;*/
		uint32_t reg_rbuf_size25:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE25_C;
typedef union {
	struct {
		/*ring buffer size for client26;*/
		uint32_t reg_rbuf_size26:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE26_C;
typedef union {
	struct {
		/*ring buffer size for client27;*/
		uint32_t reg_rbuf_size27:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE27_C;
typedef union {
	struct {
		/*ring buffer size for client28;*/
		uint32_t reg_rbuf_size28:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE28_C;
typedef union {
	struct {
		/*ring buffer size for client29;*/
		uint32_t reg_rbuf_size29:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE29_C;
typedef union {
	struct {
		/*ring buffer size for client30;*/
		uint32_t reg_rbuf_size30:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE30_C;
typedef union {
	struct {
		/*ring buffer size for client31;*/
		uint32_t reg_rbuf_size31:32;
	};
	uint32_t val;
} ISP_RDMA_RING_BUFFER_SIZE31_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr0:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS0_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr1:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS1_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr2:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS2_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr3:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS3_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr4:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS4_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr5:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS5_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr6:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS6_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr7:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS7_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr8:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS8_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr9:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS9_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr10:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS10_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr11:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS11_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr12:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS12_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr13:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS13_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr14:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS14_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr15:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS15_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr16:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS16_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr17:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS17_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr18:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS18_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr19:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS19_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr20:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS20_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr21:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS21_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr22:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS22_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr23:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS23_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr24:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS24_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr25:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS25_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr26:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS26_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr27:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS27_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr28:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS28_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr29:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS29_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr30:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS30_C;
typedef union {
	struct {
		/*next dma burst address status;*/
		uint32_t reg_next_dma_addr31:32;
	};
	uint32_t val;
} ISP_RDMA_NEXT_DMA_ADDR_STS31_C;
typedef struct {
	volatile ISP_RDMA_SHADOW_RD_SEL_C SHADOW_RD_SEL;
	// volatile ISP_RDMA_IP_DISABLE_C IP_DISABLE;
	volatile uint32_t _IP_DISABLE; // 0x04
	volatile uint32_t _UP_RING_BASE; // 0x08
	volatile ISP_RDMA_UP_RING_BASE_C UP_RING_BASE;
	volatile ISP_RDMA_NORM_STATUS0_C NORM_STATUS0;
	volatile ISP_RDMA_NORM_STATUS1_C NORM_STATUS1;
	volatile ISP_RDMA_RING_BUFFER_EN_C RING_BUFFER_EN;
	volatile uint32_t _NORM_PERF; // 0x1C
	volatile ISP_RDMA_NORM_PERF_C NORM_PERF;
	volatile ISP_RDMA_AR_PRIORITY_SEL_C AR_PRIORITY_SEL;
	volatile ISP_RDMA_RING_PATCH_ENABLE_C RING_PATCH_ENABLE;
	volatile ISP_RDMA_SET_RING_BASE_C SET_RING_BASE;
	volatile ISP_RDMA_RING_BASE_ADDR_L_C RING_BASE_ADDR_L;
	volatile ISP_RDMA_RING_BASE_ADDR_H_C RING_BASE_ADDR_H;
	volatile uint32_t _RING_BUFFER_SIZE0; // 0x38
	volatile uint32_t _RING_BUFFER_SIZE1; // 0x3c
	volatile uint32_t _RING_BUFFER_SIZE2; // 0x40
	volatile uint32_t _RING_BUFFER_SIZE3; // 0x44
	volatile uint32_t _RING_BUFFER_SIZE4; // 0x48
	volatile uint32_t _RING_BUFFER_SIZE5; // 0x4c
	volatile uint32_t _RING_BUFFER_SIZE6; // 0x50
	volatile uint32_t _RING_BUFFER_SIZE7; // 0x54
	volatile uint32_t _RING_BUFFER_SIZE8; // 0x58
	volatile uint32_t _RING_BUFFER_SIZE9; // 0x5c
	volatile uint32_t _RING_BUFFER_SIZE10; // 0x60
	volatile uint32_t _RING_BUFFER_SIZE11; // 0x64
	volatile uint32_t _RING_BUFFER_SIZE12; // 0x68
	volatile uint32_t _RING_BUFFER_SIZE13; // 0x6c
	volatile uint32_t _RING_BUFFER_SIZE14; // 0x70
	volatile uint32_t _RING_BUFFER_SIZE15; // 0x74
	volatile uint32_t _RING_BUFFER_SIZE16; // 0x78
	volatile uint32_t _RING_BUFFER_SIZE17; // 0x7c
	volatile ISP_RDMA_RING_BUFFER_SIZE0_C RING_BUFFER_SIZE0;
	volatile ISP_RDMA_RING_BUFFER_SIZE1_C RING_BUFFER_SIZE1;
	volatile ISP_RDMA_RING_BUFFER_SIZE2_C RING_BUFFER_SIZE2;
	volatile ISP_RDMA_RING_BUFFER_SIZE3_C RING_BUFFER_SIZE3;
	volatile ISP_RDMA_RING_BUFFER_SIZE4_C RING_BUFFER_SIZE4;
	volatile ISP_RDMA_RING_BUFFER_SIZE5_C RING_BUFFER_SIZE5;
	volatile ISP_RDMA_RING_BUFFER_SIZE6_C RING_BUFFER_SIZE6;
	volatile ISP_RDMA_RING_BUFFER_SIZE7_C RING_BUFFER_SIZE7;
	volatile ISP_RDMA_RING_BUFFER_SIZE8_C RING_BUFFER_SIZE8;
	volatile ISP_RDMA_RING_BUFFER_SIZE9_C RING_BUFFER_SIZE9;
	volatile ISP_RDMA_RING_BUFFER_SIZE10_C RING_BUFFER_SIZE10;
	volatile ISP_RDMA_RING_BUFFER_SIZE11_C RING_BUFFER_SIZE11;
	volatile ISP_RDMA_RING_BUFFER_SIZE12_C RING_BUFFER_SIZE12;
	volatile ISP_RDMA_RING_BUFFER_SIZE13_C RING_BUFFER_SIZE13;
	volatile ISP_RDMA_RING_BUFFER_SIZE14_C RING_BUFFER_SIZE14;
	volatile ISP_RDMA_RING_BUFFER_SIZE15_C RING_BUFFER_SIZE15;
	volatile ISP_RDMA_RING_BUFFER_SIZE16_C RING_BUFFER_SIZE16;
	volatile ISP_RDMA_RING_BUFFER_SIZE17_C RING_BUFFER_SIZE17;
	volatile ISP_RDMA_RING_BUFFER_SIZE18_C RING_BUFFER_SIZE18;
	volatile ISP_RDMA_RING_BUFFER_SIZE19_C RING_BUFFER_SIZE19;
	volatile ISP_RDMA_RING_BUFFER_SIZE20_C RING_BUFFER_SIZE20;
	volatile ISP_RDMA_RING_BUFFER_SIZE21_C RING_BUFFER_SIZE21;
	volatile ISP_RDMA_RING_BUFFER_SIZE22_C RING_BUFFER_SIZE22;
	volatile ISP_RDMA_RING_BUFFER_SIZE23_C RING_BUFFER_SIZE23;
	volatile ISP_RDMA_RING_BUFFER_SIZE24_C RING_BUFFER_SIZE24;
	volatile ISP_RDMA_RING_BUFFER_SIZE25_C RING_BUFFER_SIZE25;
	volatile ISP_RDMA_RING_BUFFER_SIZE26_C RING_BUFFER_SIZE26;
	volatile ISP_RDMA_RING_BUFFER_SIZE27_C RING_BUFFER_SIZE27;
	volatile ISP_RDMA_RING_BUFFER_SIZE28_C RING_BUFFER_SIZE28;
	volatile ISP_RDMA_RING_BUFFER_SIZE29_C RING_BUFFER_SIZE29;
	volatile ISP_RDMA_RING_BUFFER_SIZE30_C RING_BUFFER_SIZE30;
	volatile ISP_RDMA_RING_BUFFER_SIZE31_C RING_BUFFER_SIZE31;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS0_C NEXT_DMA_ADDR_STS0;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS1_C NEXT_DMA_ADDR_STS1;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS2_C NEXT_DMA_ADDR_STS2;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS3_C NEXT_DMA_ADDR_STS3;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS4_C NEXT_DMA_ADDR_STS4;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS5_C NEXT_DMA_ADDR_STS5;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS6_C NEXT_DMA_ADDR_STS6;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS7_C NEXT_DMA_ADDR_STS7;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS8_C NEXT_DMA_ADDR_STS8;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS9_C NEXT_DMA_ADDR_STS9;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS10_C NEXT_DMA_ADDR_STS10;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS11_C NEXT_DMA_ADDR_STS11;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS12_C NEXT_DMA_ADDR_STS12;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS13_C NEXT_DMA_ADDR_STS13;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS14_C NEXT_DMA_ADDR_STS14;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS15_C NEXT_DMA_ADDR_STS15;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS16_C NEXT_DMA_ADDR_STS16;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS17_C NEXT_DMA_ADDR_STS17;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS18_C NEXT_DMA_ADDR_STS18;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS19_C NEXT_DMA_ADDR_STS19;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS20_C NEXT_DMA_ADDR_STS20;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS21_C NEXT_DMA_ADDR_STS21;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS22_C NEXT_DMA_ADDR_STS22;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS23_C NEXT_DMA_ADDR_STS23;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS24_C NEXT_DMA_ADDR_STS24;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS25_C NEXT_DMA_ADDR_STS25;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS26_C NEXT_DMA_ADDR_STS26;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS27_C NEXT_DMA_ADDR_STS27;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS28_C NEXT_DMA_ADDR_STS28;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS29_C NEXT_DMA_ADDR_STS29;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS30_C NEXT_DMA_ADDR_STS30;
	volatile ISP_RDMA_NEXT_DMA_ADDR_STS31_C NEXT_DMA_ADDR_STS31;
} ISP_RDMA_C;
#ifdef __cplusplus

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
static void isp_rdma_dump_ini(FILE* fp, ISP_RDMA_C* p) {
	fprintf(fp, "reg_shadow_rd_sel = 0x%x\n",p->SHADOW_RD_SEL.reg_shadow_rd_sel);
	fprintf(fp, "reg_abort_mode = 0x%x\n",p->SHADOW_RD_SEL.reg_abort_mode);
	fprintf(fp, "reg_max_ostd = 0x%x\n",p->SHADOW_RD_SEL.reg_max_ostd);
	fprintf(fp, "reg_ostd_sw_en = 0x%x\n",p->SHADOW_RD_SEL.reg_ostd_sw_en);
	// fprintf(fp, "reg_ip_disable = 0x%x\n",p->IP_DISABLE.reg_ip_disable);
	fprintf(fp, "reg_up_ring_base = 0x%x\n",p->UP_RING_BASE.reg_up_ring_base);
	fprintf(fp, "reg_abort_done = 0x%x\n",p->NORM_STATUS0.reg_abort_done);
	fprintf(fp, "reg_error_axi = 0x%x\n",p->NORM_STATUS0.reg_error_axi);
	fprintf(fp, "reg_error_id = 0x%x\n",p->NORM_STATUS0.reg_error_id);
	fprintf(fp, "reg_dma_version = 0x%x\n",p->NORM_STATUS0.reg_dma_version);
	fprintf(fp, "reg_id_idle = 0x%x\n",p->NORM_STATUS1.reg_id_idle);
	fprintf(fp, "reg_ring_enable = 0x%x\n",p->RING_BUFFER_EN.reg_ring_enable);
	fprintf(fp, "reg_bwlwin = 0x%x\n",p->NORM_PERF.reg_bwlwin);
	fprintf(fp, "reg_bwltxn = 0x%x\n",p->NORM_PERF.reg_bwltxn);
	fprintf(fp, "reg_ar_priority_sel = 0x%x\n",p->AR_PRIORITY_SEL.reg_ar_priority_sel);
	fprintf(fp, "reg_qos_priority_sel = 0x%x\n",p->AR_PRIORITY_SEL.reg_qos_priority_sel);
	fprintf(fp, "reg_arb_hist_disable = 0x%x\n",p->AR_PRIORITY_SEL.reg_arb_hist_disable);
	fprintf(fp, "reg_arb_usage_th = 0x%x\n",p->AR_PRIORITY_SEL.reg_arb_usage_th);
	fprintf(fp, "reg_ring_patch_enable = 0x%x\n",p->RING_PATCH_ENABLE.reg_ring_patch_enable);
	fprintf(fp, "reg_set_ring_base = 0x%x\n",p->SET_RING_BASE.reg_set_ring_base);
	fprintf(fp, "reg_ring_base_l = 0x%x\n",p->RING_BASE_ADDR_L.reg_ring_base_l);
	fprintf(fp, "reg_ring_base_h = 0x%x\n",p->RING_BASE_ADDR_H.reg_ring_base_h);
	fprintf(fp, "reg_rbuf_size0 = 0x%x\n",p->RING_BUFFER_SIZE0.reg_rbuf_size0);
	fprintf(fp, "reg_rbuf_size1 = 0x%x\n",p->RING_BUFFER_SIZE1.reg_rbuf_size1);
	fprintf(fp, "reg_rbuf_size2 = 0x%x\n",p->RING_BUFFER_SIZE2.reg_rbuf_size2);
	fprintf(fp, "reg_rbuf_size3 = 0x%x\n",p->RING_BUFFER_SIZE3.reg_rbuf_size3);
	fprintf(fp, "reg_rbuf_size4 = 0x%x\n",p->RING_BUFFER_SIZE4.reg_rbuf_size4);
	fprintf(fp, "reg_rbuf_size5 = 0x%x\n",p->RING_BUFFER_SIZE5.reg_rbuf_size5);
	fprintf(fp, "reg_rbuf_size6 = 0x%x\n",p->RING_BUFFER_SIZE6.reg_rbuf_size6);
	fprintf(fp, "reg_rbuf_size7 = 0x%x\n",p->RING_BUFFER_SIZE7.reg_rbuf_size7);
	fprintf(fp, "reg_rbuf_size8 = 0x%x\n",p->RING_BUFFER_SIZE8.reg_rbuf_size8);
	fprintf(fp, "reg_rbuf_size9 = 0x%x\n",p->RING_BUFFER_SIZE9.reg_rbuf_size9);
	fprintf(fp, "reg_rbuf_size10 = 0x%x\n",p->RING_BUFFER_SIZE10.reg_rbuf_size10);
	fprintf(fp, "reg_rbuf_size11 = 0x%x\n",p->RING_BUFFER_SIZE11.reg_rbuf_size11);
	fprintf(fp, "reg_rbuf_size12 = 0x%x\n",p->RING_BUFFER_SIZE12.reg_rbuf_size12);
	fprintf(fp, "reg_rbuf_size13 = 0x%x\n",p->RING_BUFFER_SIZE13.reg_rbuf_size13);
	fprintf(fp, "reg_rbuf_size14 = 0x%x\n",p->RING_BUFFER_SIZE14.reg_rbuf_size14);
	fprintf(fp, "reg_rbuf_size15 = 0x%x\n",p->RING_BUFFER_SIZE15.reg_rbuf_size15);
	fprintf(fp, "reg_rbuf_size16 = 0x%x\n",p->RING_BUFFER_SIZE16.reg_rbuf_size16);
	fprintf(fp, "reg_rbuf_size17 = 0x%x\n",p->RING_BUFFER_SIZE17.reg_rbuf_size17);
	fprintf(fp, "reg_rbuf_size18 = 0x%x\n",p->RING_BUFFER_SIZE18.reg_rbuf_size18);
	fprintf(fp, "reg_rbuf_size19 = 0x%x\n",p->RING_BUFFER_SIZE19.reg_rbuf_size19);
	fprintf(fp, "reg_rbuf_size20 = 0x%x\n",p->RING_BUFFER_SIZE20.reg_rbuf_size20);
	fprintf(fp, "reg_rbuf_size21 = 0x%x\n",p->RING_BUFFER_SIZE21.reg_rbuf_size21);
	fprintf(fp, "reg_rbuf_size22 = 0x%x\n",p->RING_BUFFER_SIZE22.reg_rbuf_size22);
	fprintf(fp, "reg_rbuf_size23 = 0x%x\n",p->RING_BUFFER_SIZE23.reg_rbuf_size23);
	fprintf(fp, "reg_rbuf_size24 = 0x%x\n",p->RING_BUFFER_SIZE24.reg_rbuf_size24);
	fprintf(fp, "reg_rbuf_size25 = 0x%x\n",p->RING_BUFFER_SIZE25.reg_rbuf_size25);
	fprintf(fp, "reg_rbuf_size26 = 0x%x\n",p->RING_BUFFER_SIZE26.reg_rbuf_size26);
	fprintf(fp, "reg_rbuf_size27 = 0x%x\n",p->RING_BUFFER_SIZE27.reg_rbuf_size27);
	fprintf(fp, "reg_rbuf_size28 = 0x%x\n",p->RING_BUFFER_SIZE28.reg_rbuf_size28);
	fprintf(fp, "reg_rbuf_size29 = 0x%x\n",p->RING_BUFFER_SIZE29.reg_rbuf_size29);
	fprintf(fp, "reg_rbuf_size30 = 0x%x\n",p->RING_BUFFER_SIZE30.reg_rbuf_size30);
	fprintf(fp, "reg_rbuf_size31 = 0x%x\n",p->RING_BUFFER_SIZE31.reg_rbuf_size31);
	fprintf(fp, "reg_next_dma_addr0 = 0x%x\n",p->NEXT_DMA_ADDR_STS0.reg_next_dma_addr0);
	fprintf(fp, "reg_next_dma_addr1 = 0x%x\n",p->NEXT_DMA_ADDR_STS1.reg_next_dma_addr1);
	fprintf(fp, "reg_next_dma_addr2 = 0x%x\n",p->NEXT_DMA_ADDR_STS2.reg_next_dma_addr2);
	fprintf(fp, "reg_next_dma_addr3 = 0x%x\n",p->NEXT_DMA_ADDR_STS3.reg_next_dma_addr3);
	fprintf(fp, "reg_next_dma_addr4 = 0x%x\n",p->NEXT_DMA_ADDR_STS4.reg_next_dma_addr4);
	fprintf(fp, "reg_next_dma_addr5 = 0x%x\n",p->NEXT_DMA_ADDR_STS5.reg_next_dma_addr5);
	fprintf(fp, "reg_next_dma_addr6 = 0x%x\n",p->NEXT_DMA_ADDR_STS6.reg_next_dma_addr6);
	fprintf(fp, "reg_next_dma_addr7 = 0x%x\n",p->NEXT_DMA_ADDR_STS7.reg_next_dma_addr7);
	fprintf(fp, "reg_next_dma_addr8 = 0x%x\n",p->NEXT_DMA_ADDR_STS8.reg_next_dma_addr8);
	fprintf(fp, "reg_next_dma_addr9 = 0x%x\n",p->NEXT_DMA_ADDR_STS9.reg_next_dma_addr9);
	fprintf(fp, "reg_next_dma_addr10 = 0x%x\n",p->NEXT_DMA_ADDR_STS10.reg_next_dma_addr10);
	fprintf(fp, "reg_next_dma_addr11 = 0x%x\n",p->NEXT_DMA_ADDR_STS11.reg_next_dma_addr11);
	fprintf(fp, "reg_next_dma_addr12 = 0x%x\n",p->NEXT_DMA_ADDR_STS12.reg_next_dma_addr12);
	fprintf(fp, "reg_next_dma_addr13 = 0x%x\n",p->NEXT_DMA_ADDR_STS13.reg_next_dma_addr13);
	fprintf(fp, "reg_next_dma_addr14 = 0x%x\n",p->NEXT_DMA_ADDR_STS14.reg_next_dma_addr14);
	fprintf(fp, "reg_next_dma_addr15 = 0x%x\n",p->NEXT_DMA_ADDR_STS15.reg_next_dma_addr15);
	fprintf(fp, "reg_next_dma_addr16 = 0x%x\n",p->NEXT_DMA_ADDR_STS16.reg_next_dma_addr16);
	fprintf(fp, "reg_next_dma_addr17 = 0x%x\n",p->NEXT_DMA_ADDR_STS17.reg_next_dma_addr17);
	fprintf(fp, "reg_next_dma_addr18 = 0x%x\n",p->NEXT_DMA_ADDR_STS18.reg_next_dma_addr18);
	fprintf(fp, "reg_next_dma_addr19 = 0x%x\n",p->NEXT_DMA_ADDR_STS19.reg_next_dma_addr19);
	fprintf(fp, "reg_next_dma_addr20 = 0x%x\n",p->NEXT_DMA_ADDR_STS20.reg_next_dma_addr20);
	fprintf(fp, "reg_next_dma_addr21 = 0x%x\n",p->NEXT_DMA_ADDR_STS21.reg_next_dma_addr21);
	fprintf(fp, "reg_next_dma_addr22 = 0x%x\n",p->NEXT_DMA_ADDR_STS22.reg_next_dma_addr22);
	fprintf(fp, "reg_next_dma_addr23 = 0x%x\n",p->NEXT_DMA_ADDR_STS23.reg_next_dma_addr23);
	fprintf(fp, "reg_next_dma_addr24 = 0x%x\n",p->NEXT_DMA_ADDR_STS24.reg_next_dma_addr24);
	fprintf(fp, "reg_next_dma_addr25 = 0x%x\n",p->NEXT_DMA_ADDR_STS25.reg_next_dma_addr25);
	fprintf(fp, "reg_next_dma_addr26 = 0x%x\n",p->NEXT_DMA_ADDR_STS26.reg_next_dma_addr26);
	fprintf(fp, "reg_next_dma_addr27 = 0x%x\n",p->NEXT_DMA_ADDR_STS27.reg_next_dma_addr27);
	fprintf(fp, "reg_next_dma_addr28 = 0x%x\n",p->NEXT_DMA_ADDR_STS28.reg_next_dma_addr28);
	fprintf(fp, "reg_next_dma_addr29 = 0x%x\n",p->NEXT_DMA_ADDR_STS29.reg_next_dma_addr29);
	fprintf(fp, "reg_next_dma_addr30 = 0x%x\n",p->NEXT_DMA_ADDR_STS30.reg_next_dma_addr30);
	fprintf(fp, "reg_next_dma_addr31 = 0x%x\n",p->NEXT_DMA_ADDR_STS31.reg_next_dma_addr31);

}
static void isp_rdma_print(ISP_RDMA_C* p) {
    fprintf(stderr, "isp_rdma\n");
	fprintf(stderr, "\tSHADOW_RD_SEL.reg_shadow_rd_sel = 0x%x\n", p->SHADOW_RD_SEL.reg_shadow_rd_sel);
	fprintf(stderr, "\tSHADOW_RD_SEL.reg_abort_mode = 0x%x\n", p->SHADOW_RD_SEL.reg_abort_mode);
	fprintf(stderr, "\tSHADOW_RD_SEL.reg_max_ostd = 0x%x\n", p->SHADOW_RD_SEL.reg_max_ostd);
	fprintf(stderr, "\tSHADOW_RD_SEL.reg_ostd_sw_en = 0x%x\n", p->SHADOW_RD_SEL.reg_ostd_sw_en);
	// fprintf(stderr, "\tIP_DISABLE.reg_ip_disable = 0x%x\n", p->IP_DISABLE.reg_ip_disable);
	fprintf(stderr, "\tUP_RING_BASE.reg_up_ring_base = 0x%x\n", p->UP_RING_BASE.reg_up_ring_base);
	fprintf(stderr, "\tNORM_STATUS0.reg_abort_done = 0x%x\n", p->NORM_STATUS0.reg_abort_done);
	fprintf(stderr, "\tNORM_STATUS0.reg_error_axi = 0x%x\n", p->NORM_STATUS0.reg_error_axi);
	fprintf(stderr, "\tNORM_STATUS0.reg_error_id = 0x%x\n", p->NORM_STATUS0.reg_error_id);
	fprintf(stderr, "\tNORM_STATUS0.reg_dma_version = 0x%x\n", p->NORM_STATUS0.reg_dma_version);
	fprintf(stderr, "\tNORM_STATUS1.reg_id_idle = 0x%x\n", p->NORM_STATUS1.reg_id_idle);
	fprintf(stderr, "\tRING_BUFFER_EN.reg_ring_enable = 0x%x\n", p->RING_BUFFER_EN.reg_ring_enable);
	fprintf(stderr, "\tNORM_PERF.reg_bwlwin = 0x%x\n", p->NORM_PERF.reg_bwlwin);
	fprintf(stderr, "\tNORM_PERF.reg_bwltxn = 0x%x\n", p->NORM_PERF.reg_bwltxn);
	fprintf(stderr, "\tAR_PRIORITY_SEL.reg_ar_priority_sel = 0x%x\n", p->AR_PRIORITY_SEL.reg_ar_priority_sel);
	fprintf(stderr, "\tAR_PRIORITY_SEL.reg_qos_priority_sel = 0x%x\n", p->AR_PRIORITY_SEL.reg_qos_priority_sel);
	fprintf(stderr, "\tAR_PRIORITY_SEL.reg_arb_hist_disable = 0x%x\n", p->AR_PRIORITY_SEL.reg_arb_hist_disable);
	fprintf(stderr, "\tAR_PRIORITY_SEL.reg_arb_usage_th = 0x%x\n", p->AR_PRIORITY_SEL.reg_arb_usage_th);
	fprintf(stderr, "\tRING_PATCH_ENABLE.reg_ring_patch_enable = 0x%x\n", p->RING_PATCH_ENABLE.reg_ring_patch_enable);
	fprintf(stderr, "\tSET_RING_BASE.reg_set_ring_base = 0x%x\n", p->SET_RING_BASE.reg_set_ring_base);
	fprintf(stderr, "\tRING_BASE_ADDR_L.reg_ring_base_l = 0x%x\n", p->RING_BASE_ADDR_L.reg_ring_base_l);
	fprintf(stderr, "\tRING_BASE_ADDR_H.reg_ring_base_h = 0x%x\n", p->RING_BASE_ADDR_H.reg_ring_base_h);
	fprintf(stderr, "\tRING_BUFFER_SIZE0.reg_rbuf_size0 = 0x%x\n", p->RING_BUFFER_SIZE0.reg_rbuf_size0);
	fprintf(stderr, "\tRING_BUFFER_SIZE1.reg_rbuf_size1 = 0x%x\n", p->RING_BUFFER_SIZE1.reg_rbuf_size1);
	fprintf(stderr, "\tRING_BUFFER_SIZE2.reg_rbuf_size2 = 0x%x\n", p->RING_BUFFER_SIZE2.reg_rbuf_size2);
	fprintf(stderr, "\tRING_BUFFER_SIZE3.reg_rbuf_size3 = 0x%x\n", p->RING_BUFFER_SIZE3.reg_rbuf_size3);
	fprintf(stderr, "\tRING_BUFFER_SIZE4.reg_rbuf_size4 = 0x%x\n", p->RING_BUFFER_SIZE4.reg_rbuf_size4);
	fprintf(stderr, "\tRING_BUFFER_SIZE5.reg_rbuf_size5 = 0x%x\n", p->RING_BUFFER_SIZE5.reg_rbuf_size5);
	fprintf(stderr, "\tRING_BUFFER_SIZE6.reg_rbuf_size6 = 0x%x\n", p->RING_BUFFER_SIZE6.reg_rbuf_size6);
	fprintf(stderr, "\tRING_BUFFER_SIZE7.reg_rbuf_size7 = 0x%x\n", p->RING_BUFFER_SIZE7.reg_rbuf_size7);
	fprintf(stderr, "\tRING_BUFFER_SIZE8.reg_rbuf_size8 = 0x%x\n", p->RING_BUFFER_SIZE8.reg_rbuf_size8);
	fprintf(stderr, "\tRING_BUFFER_SIZE9.reg_rbuf_size9 = 0x%x\n", p->RING_BUFFER_SIZE9.reg_rbuf_size9);
	fprintf(stderr, "\tRING_BUFFER_SIZE10.reg_rbuf_size10 = 0x%x\n", p->RING_BUFFER_SIZE10.reg_rbuf_size10);
	fprintf(stderr, "\tRING_BUFFER_SIZE11.reg_rbuf_size11 = 0x%x\n", p->RING_BUFFER_SIZE11.reg_rbuf_size11);
	fprintf(stderr, "\tRING_BUFFER_SIZE12.reg_rbuf_size12 = 0x%x\n", p->RING_BUFFER_SIZE12.reg_rbuf_size12);
	fprintf(stderr, "\tRING_BUFFER_SIZE13.reg_rbuf_size13 = 0x%x\n", p->RING_BUFFER_SIZE13.reg_rbuf_size13);
	fprintf(stderr, "\tRING_BUFFER_SIZE14.reg_rbuf_size14 = 0x%x\n", p->RING_BUFFER_SIZE14.reg_rbuf_size14);
	fprintf(stderr, "\tRING_BUFFER_SIZE15.reg_rbuf_size15 = 0x%x\n", p->RING_BUFFER_SIZE15.reg_rbuf_size15);
	fprintf(stderr, "\tRING_BUFFER_SIZE16.reg_rbuf_size16 = 0x%x\n", p->RING_BUFFER_SIZE16.reg_rbuf_size16);
	fprintf(stderr, "\tRING_BUFFER_SIZE17.reg_rbuf_size17 = 0x%x\n", p->RING_BUFFER_SIZE17.reg_rbuf_size17);
	fprintf(stderr, "\tRING_BUFFER_SIZE18.reg_rbuf_size18 = 0x%x\n", p->RING_BUFFER_SIZE18.reg_rbuf_size18);
	fprintf(stderr, "\tRING_BUFFER_SIZE19.reg_rbuf_size19 = 0x%x\n", p->RING_BUFFER_SIZE19.reg_rbuf_size19);
	fprintf(stderr, "\tRING_BUFFER_SIZE20.reg_rbuf_size20 = 0x%x\n", p->RING_BUFFER_SIZE20.reg_rbuf_size20);
	fprintf(stderr, "\tRING_BUFFER_SIZE21.reg_rbuf_size21 = 0x%x\n", p->RING_BUFFER_SIZE21.reg_rbuf_size21);
	fprintf(stderr, "\tRING_BUFFER_SIZE22.reg_rbuf_size22 = 0x%x\n", p->RING_BUFFER_SIZE22.reg_rbuf_size22);
	fprintf(stderr, "\tRING_BUFFER_SIZE23.reg_rbuf_size23 = 0x%x\n", p->RING_BUFFER_SIZE23.reg_rbuf_size23);
	fprintf(stderr, "\tRING_BUFFER_SIZE24.reg_rbuf_size24 = 0x%x\n", p->RING_BUFFER_SIZE24.reg_rbuf_size24);
	fprintf(stderr, "\tRING_BUFFER_SIZE25.reg_rbuf_size25 = 0x%x\n", p->RING_BUFFER_SIZE25.reg_rbuf_size25);
	fprintf(stderr, "\tRING_BUFFER_SIZE26.reg_rbuf_size26 = 0x%x\n", p->RING_BUFFER_SIZE26.reg_rbuf_size26);
	fprintf(stderr, "\tRING_BUFFER_SIZE27.reg_rbuf_size27 = 0x%x\n", p->RING_BUFFER_SIZE27.reg_rbuf_size27);
	fprintf(stderr, "\tRING_BUFFER_SIZE28.reg_rbuf_size28 = 0x%x\n", p->RING_BUFFER_SIZE28.reg_rbuf_size28);
	fprintf(stderr, "\tRING_BUFFER_SIZE29.reg_rbuf_size29 = 0x%x\n", p->RING_BUFFER_SIZE29.reg_rbuf_size29);
	fprintf(stderr, "\tRING_BUFFER_SIZE30.reg_rbuf_size30 = 0x%x\n", p->RING_BUFFER_SIZE30.reg_rbuf_size30);
	fprintf(stderr, "\tRING_BUFFER_SIZE31.reg_rbuf_size31 = 0x%x\n", p->RING_BUFFER_SIZE31.reg_rbuf_size31);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS0.reg_next_dma_addr0 = 0x%x\n", p->NEXT_DMA_ADDR_STS0.reg_next_dma_addr0);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS1.reg_next_dma_addr1 = 0x%x\n", p->NEXT_DMA_ADDR_STS1.reg_next_dma_addr1);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS2.reg_next_dma_addr2 = 0x%x\n", p->NEXT_DMA_ADDR_STS2.reg_next_dma_addr2);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS3.reg_next_dma_addr3 = 0x%x\n", p->NEXT_DMA_ADDR_STS3.reg_next_dma_addr3);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS4.reg_next_dma_addr4 = 0x%x\n", p->NEXT_DMA_ADDR_STS4.reg_next_dma_addr4);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS5.reg_next_dma_addr5 = 0x%x\n", p->NEXT_DMA_ADDR_STS5.reg_next_dma_addr5);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS6.reg_next_dma_addr6 = 0x%x\n", p->NEXT_DMA_ADDR_STS6.reg_next_dma_addr6);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS7.reg_next_dma_addr7 = 0x%x\n", p->NEXT_DMA_ADDR_STS7.reg_next_dma_addr7);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS8.reg_next_dma_addr8 = 0x%x\n", p->NEXT_DMA_ADDR_STS8.reg_next_dma_addr8);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS9.reg_next_dma_addr9 = 0x%x\n", p->NEXT_DMA_ADDR_STS9.reg_next_dma_addr9);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS10.reg_next_dma_addr10 = 0x%x\n", p->NEXT_DMA_ADDR_STS10.reg_next_dma_addr10);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS11.reg_next_dma_addr11 = 0x%x\n", p->NEXT_DMA_ADDR_STS11.reg_next_dma_addr11);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS12.reg_next_dma_addr12 = 0x%x\n", p->NEXT_DMA_ADDR_STS12.reg_next_dma_addr12);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS13.reg_next_dma_addr13 = 0x%x\n", p->NEXT_DMA_ADDR_STS13.reg_next_dma_addr13);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS14.reg_next_dma_addr14 = 0x%x\n", p->NEXT_DMA_ADDR_STS14.reg_next_dma_addr14);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS15.reg_next_dma_addr15 = 0x%x\n", p->NEXT_DMA_ADDR_STS15.reg_next_dma_addr15);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS16.reg_next_dma_addr16 = 0x%x\n", p->NEXT_DMA_ADDR_STS16.reg_next_dma_addr16);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS17.reg_next_dma_addr17 = 0x%x\n", p->NEXT_DMA_ADDR_STS17.reg_next_dma_addr17);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS18.reg_next_dma_addr18 = 0x%x\n", p->NEXT_DMA_ADDR_STS18.reg_next_dma_addr18);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS19.reg_next_dma_addr19 = 0x%x\n", p->NEXT_DMA_ADDR_STS19.reg_next_dma_addr19);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS20.reg_next_dma_addr20 = 0x%x\n", p->NEXT_DMA_ADDR_STS20.reg_next_dma_addr20);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS21.reg_next_dma_addr21 = 0x%x\n", p->NEXT_DMA_ADDR_STS21.reg_next_dma_addr21);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS22.reg_next_dma_addr22 = 0x%x\n", p->NEXT_DMA_ADDR_STS22.reg_next_dma_addr22);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS23.reg_next_dma_addr23 = 0x%x\n", p->NEXT_DMA_ADDR_STS23.reg_next_dma_addr23);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS24.reg_next_dma_addr24 = 0x%x\n", p->NEXT_DMA_ADDR_STS24.reg_next_dma_addr24);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS25.reg_next_dma_addr25 = 0x%x\n", p->NEXT_DMA_ADDR_STS25.reg_next_dma_addr25);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS26.reg_next_dma_addr26 = 0x%x\n", p->NEXT_DMA_ADDR_STS26.reg_next_dma_addr26);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS27.reg_next_dma_addr27 = 0x%x\n", p->NEXT_DMA_ADDR_STS27.reg_next_dma_addr27);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS28.reg_next_dma_addr28 = 0x%x\n", p->NEXT_DMA_ADDR_STS28.reg_next_dma_addr28);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS29.reg_next_dma_addr29 = 0x%x\n", p->NEXT_DMA_ADDR_STS29.reg_next_dma_addr29);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS30.reg_next_dma_addr30 = 0x%x\n", p->NEXT_DMA_ADDR_STS30.reg_next_dma_addr30);
	fprintf(stderr, "\tNEXT_DMA_ADDR_STS31.reg_next_dma_addr31 = 0x%x\n", p->NEXT_DMA_ADDR_STS31.reg_next_dma_addr31);

}
#pragma GCC diagnostic pop
#define DEFINE_ISP_RDMA_C(X) \
	 ISP_RDMA_C X = \
{\
	{	/*.SHADOW_RD_SEL.reg_shadow_rd_sel = */0x1,\
	/*.SHADOW_RD_SEL.reg_abort_mode = */0x0,\
	/*.SHADOW_RD_SEL.reg_max_ostd = */0x2,\
	/*.SHADOW_RD_SEL.reg_ostd_sw_en = */0x0,\
	},\
	{	/*.UP_RING_BASE.reg_up_ring_base = */0x0,\
	},\
	{	/*.NORM_STATUS0.reg_abort_done = */0x0,\
	/*uint32_t rsv_1_3:3;=*/0,\
	/*.NORM_STATUS0.reg_error_axi = */0x0,\
	/*uint32_t rsv_5_7:3;=*/0,\
	/*.NORM_STATUS0.reg_error_id = */0x1f,\
	/*uint32_t rsv_13_15:3;=*/0,\
	/*.NORM_STATUS0.reg_dma_version = */0x0423,\
	},\
	{	/*.NORM_STATUS1.reg_id_idle = */0x0,\
	},\
	{	/*.RING_BUFFER_EN.reg_ring_enable = */0x0,\
	},\
	{	/*.NORM_PERF.reg_bwlwin = */0x0,\
	/*.NORM_PERF.reg_bwltxn = */0x0,\
	},\
	{	/*.AR_PRIORITY_SEL.reg_ar_priority_sel = */0x0,\
	/*.AR_PRIORITY_SEL.reg_qos_priority_sel = */0x0,\
	/*.AR_PRIORITY_SEL.reg_arb_hist_disabel = */0x0,\
	/*.AR_PRIORITY_SEL.reg_arb_usage_th = */0x5,\
	},\
	{	/*.RING_PATCH_ENABLE.reg_ring_patch_enable = */0x0,\
	},\
	{	/*.SET_RING_BASE.reg_set_ring_base = */0x0,\
	},\
	{	/*.RING_BASE_ADDR_L.reg_ring_base_l = */0x0,\
	},\
	{	/*.RING_BASE_ADDR_H.reg_ring_base_h = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE0.reg_rbuf_size0 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE1.reg_rbuf_size1 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE2.reg_rbuf_size2 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE3.reg_rbuf_size3 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE4.reg_rbuf_size4 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE5.reg_rbuf_size5 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE6.reg_rbuf_size6 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE7.reg_rbuf_size7 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE8.reg_rbuf_size8 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE9.reg_rbuf_size9 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE10.reg_rbuf_size10 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE11.reg_rbuf_size11 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE12.reg_rbuf_size12 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE13.reg_rbuf_size13 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE14.reg_rbuf_size14 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE15.reg_rbuf_size15 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE16.reg_rbuf_size16 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE17.reg_rbuf_size17 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE18.reg_rbuf_size18 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE19.reg_rbuf_size19 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE20.reg_rbuf_size20 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE21.reg_rbuf_size21 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE22.reg_rbuf_size22 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE23.reg_rbuf_size23 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE24.reg_rbuf_size24 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE25.reg_rbuf_size25 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE26.reg_rbuf_size26 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE27.reg_rbuf_size27 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE28.reg_rbuf_size28 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE29.reg_rbuf_size29 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE30.reg_rbuf_size30 = */0x0,\
	},\
	{	/*.RING_BUFFER_SIZE31.reg_rbuf_size31 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS0.reg_next_dma_addr0 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS1.reg_next_dma_addr1 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS2.reg_next_dma_addr2 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS3.reg_next_dma_addr3 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS4.reg_next_dma_addr4 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS5.reg_next_dma_addr5 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS6.reg_next_dma_addr6 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS7.reg_next_dma_addr7 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS8.reg_next_dma_addr8 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS9.reg_next_dma_addr9 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS10.reg_next_dma_addr10 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS11.reg_next_dma_addr11 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS12.reg_next_dma_addr12 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS13.reg_next_dma_addr13 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS14.reg_next_dma_addr14 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS15.reg_next_dma_addr15 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS16.reg_next_dma_addr16 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS17.reg_next_dma_addr17 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS18.reg_next_dma_addr18 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS19.reg_next_dma_addr19 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS20.reg_next_dma_addr20 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS21.reg_next_dma_addr21 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS22.reg_next_dma_addr22 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS23.reg_next_dma_addr23 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS24.reg_next_dma_addr24 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS25.reg_next_dma_addr25 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS26.reg_next_dma_addr26 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS27.reg_next_dma_addr27 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS28.reg_next_dma_addr28 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS29.reg_next_dma_addr29 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS30.reg_next_dma_addr30 = */0x0,\
	},\
	{	/*.NEXT_DMA_ADDR_STS31.reg_next_dma_addr31 = */0x0,\
	}\
};
#else /* !ifdef __cplusplus */
#define _DEFINE_ISP_RDMA_C \
{\
	.SHADOW_RD_SEL.reg_shadow_rd_sel = 0x1,\
	.SHADOW_RD_SEL.reg_abort_mode = 0x0,\
	.SHADOW_RD_SEL.reg_max_ostd = 0x2,\
	.SHADOW_RD_SEL.reg_ostd_sw_en = 0x0,\
	.UP_RING_BASE.reg_up_ring_base = 0x0,\
	.NORM_STATUS0.reg_abort_done = 0x0,\
	.NORM_STATUS0.reg_error_axi = 0x0,\
	.NORM_STATUS0.reg_error_id = 0x1f,\
	.NORM_STATUS0.reg_dma_version = 0x0423,\
	.NORM_STATUS1.reg_id_idle = 0x0,\
	.RING_BUFFER_EN.reg_ring_enable = 0x0,\
	.NORM_PERF.reg_bwlwin = 0x0,\
	.NORM_PERF.reg_bwltxn = 0x0,\
	.AR_PRIORITY_SEL.reg_ar_priority_sel = 0x0,\
	.AR_PRIORITY_SEL.reg_qos_priority_sel = 0x0,\
	.AR_PRIORITY_SEL.reg_arb_hist_disable = 0x0,\
	.AR_PRIORITY_SEL.reg_arb_usage_th = 0x5,\
	.RING_PATCH_ENABLE.reg_ring_patch_enable = 0x0,\
	.SET_RING_BASE.reg_set_ring_base = 0x0,\
	.RING_BASE_ADDR_L.reg_ring_base_l = 0x0,\
	.RING_BASE_ADDR_H.reg_ring_base_h = 0x0,\
	.RING_BUFFER_SIZE0.reg_rbuf_size0 = 0x0,\
	.RING_BUFFER_SIZE1.reg_rbuf_size1 = 0x0,\
	.RING_BUFFER_SIZE2.reg_rbuf_size2 = 0x0,\
	.RING_BUFFER_SIZE3.reg_rbuf_size3 = 0x0,\
	.RING_BUFFER_SIZE4.reg_rbuf_size4 = 0x0,\
	.RING_BUFFER_SIZE5.reg_rbuf_size5 = 0x0,\
	.RING_BUFFER_SIZE6.reg_rbuf_size6 = 0x0,\
	.RING_BUFFER_SIZE7.reg_rbuf_size7 = 0x0,\
	.RING_BUFFER_SIZE8.reg_rbuf_size8 = 0x0,\
	.RING_BUFFER_SIZE9.reg_rbuf_size9 = 0x0,\
	.RING_BUFFER_SIZE10.reg_rbuf_size10 = 0x0,\
	.RING_BUFFER_SIZE11.reg_rbuf_size11 = 0x0,\
	.RING_BUFFER_SIZE12.reg_rbuf_size12 = 0x0,\
	.RING_BUFFER_SIZE13.reg_rbuf_size13 = 0x0,\
	.RING_BUFFER_SIZE14.reg_rbuf_size14 = 0x0,\
	.RING_BUFFER_SIZE15.reg_rbuf_size15 = 0x0,\
	.RING_BUFFER_SIZE16.reg_rbuf_size16 = 0x0,\
	.RING_BUFFER_SIZE17.reg_rbuf_size17 = 0x0,\
	.RING_BUFFER_SIZE18.reg_rbuf_size18 = 0x0,\
	.RING_BUFFER_SIZE19.reg_rbuf_size19 = 0x0,\
	.RING_BUFFER_SIZE20.reg_rbuf_size20 = 0x0,\
	.RING_BUFFER_SIZE21.reg_rbuf_size21 = 0x0,\
	.RING_BUFFER_SIZE22.reg_rbuf_size22 = 0x0,\
	.RING_BUFFER_SIZE23.reg_rbuf_size23 = 0x0,\
	.RING_BUFFER_SIZE24.reg_rbuf_size24 = 0x0,\
	.RING_BUFFER_SIZE25.reg_rbuf_size25 = 0x0,\
	.RING_BUFFER_SIZE26.reg_rbuf_size26 = 0x0,\
	.RING_BUFFER_SIZE27.reg_rbuf_size27 = 0x0,\
	.RING_BUFFER_SIZE28.reg_rbuf_size28 = 0x0,\
	.RING_BUFFER_SIZE29.reg_rbuf_size29 = 0x0,\
	.RING_BUFFER_SIZE30.reg_rbuf_size30 = 0x0,\
	.RING_BUFFER_SIZE31.reg_rbuf_size31 = 0x0,\
	.NEXT_DMA_ADDR_STS0.reg_next_dma_addr0 = 0x0,\
	.NEXT_DMA_ADDR_STS1.reg_next_dma_addr1 = 0x0,\
	.NEXT_DMA_ADDR_STS2.reg_next_dma_addr2 = 0x0,\
	.NEXT_DMA_ADDR_STS3.reg_next_dma_addr3 = 0x0,\
	.NEXT_DMA_ADDR_STS4.reg_next_dma_addr4 = 0x0,\
	.NEXT_DMA_ADDR_STS5.reg_next_dma_addr5 = 0x0,\
	.NEXT_DMA_ADDR_STS6.reg_next_dma_addr6 = 0x0,\
	.NEXT_DMA_ADDR_STS7.reg_next_dma_addr7 = 0x0,\
	.NEXT_DMA_ADDR_STS8.reg_next_dma_addr8 = 0x0,\
	.NEXT_DMA_ADDR_STS9.reg_next_dma_addr9 = 0x0,\
	.NEXT_DMA_ADDR_STS10.reg_next_dma_addr10 = 0x0,\
	.NEXT_DMA_ADDR_STS11.reg_next_dma_addr11 = 0x0,\
	.NEXT_DMA_ADDR_STS12.reg_next_dma_addr12 = 0x0,\
	.NEXT_DMA_ADDR_STS13.reg_next_dma_addr13 = 0x0,\
	.NEXT_DMA_ADDR_STS14.reg_next_dma_addr14 = 0x0,\
	.NEXT_DMA_ADDR_STS15.reg_next_dma_addr15 = 0x0,\
	.NEXT_DMA_ADDR_STS16.reg_next_dma_addr16 = 0x0,\
	.NEXT_DMA_ADDR_STS17.reg_next_dma_addr17 = 0x0,\
	.NEXT_DMA_ADDR_STS18.reg_next_dma_addr18 = 0x0,\
	.NEXT_DMA_ADDR_STS19.reg_next_dma_addr19 = 0x0,\
	.NEXT_DMA_ADDR_STS20.reg_next_dma_addr20 = 0x0,\
	.NEXT_DMA_ADDR_STS21.reg_next_dma_addr21 = 0x0,\
	.NEXT_DMA_ADDR_STS22.reg_next_dma_addr22 = 0x0,\
	.NEXT_DMA_ADDR_STS23.reg_next_dma_addr23 = 0x0,\
	.NEXT_DMA_ADDR_STS24.reg_next_dma_addr24 = 0x0,\
	.NEXT_DMA_ADDR_STS25.reg_next_dma_addr25 = 0x0,\
	.NEXT_DMA_ADDR_STS26.reg_next_dma_addr26 = 0x0,\
	.NEXT_DMA_ADDR_STS27.reg_next_dma_addr27 = 0x0,\
	.NEXT_DMA_ADDR_STS28.reg_next_dma_addr28 = 0x0,\
	.NEXT_DMA_ADDR_STS29.reg_next_dma_addr29 = 0x0,\
	.NEXT_DMA_ADDR_STS30.reg_next_dma_addr30 = 0x0,\
	.NEXT_DMA_ADDR_STS31.reg_next_dma_addr31 = 0x0,\
}
#endif /* ifdef __cplusplus */
#endif //__REG_ISP_RDMA_STRUCT_H__
