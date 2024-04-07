#include "hdmi_reg.h"
#include "irq.h"
#include "bsp/access.h"
#include "phy/phy.h"
#include "phy/phy_i2c.h"
#include "bsp/i2cm.h"


typedef struct irq_vector {
	irq_sources_t source;
	unsigned int stat_reg;
	unsigned int mute_reg;
}irq_vector_t;

static irq_vector_t irq_vec[] = {
		{AUDIO_PACKETS, 	IH_FC_STAT0,		IH_MUTE_FC_STAT0},
		{OTHER_PACKETS, 	IH_FC_STAT1,		IH_MUTE_FC_STAT1},
		{PACKETS_OVERFLOW, 	IH_FC_STAT2,		IH_MUTE_FC_STAT2},
		{AUDIO_SAMPLER, 	IH_AS_STAT0,		IH_MUTE_AS_STAT0},
		{PHY, 				IH_PHY_STAT0,		IH_MUTE_PHY_STAT0},
		{I2C_DDC, 			IH_I2CM_STAT0,		IH_MUTE_I2CM_STAT0},
		{CEC, 				IH_CEC_STAT0,		IH_MUTE_CEC_STAT0},
		{VIDEO_PACKETIZER, 	IH_VP_STAT0,		IH_MUTE_VP_STAT0},
		{I2C_PHY, 			IH_I2CMPHY_STAT0,   IH_MUTE_I2CMPHY_STAT0},
		{AUDIO_DMA, 		IH_AHBDMAAUD_STAT0, IH_MUTE_AHBDMAAUD_STAT0},
		{0, 0, 0},
};

int irq_read_stat(hdmi_tx_dev_t *dev, irq_sources_t irq_source, u8 *stat)
{
	int i = 0;
	for(i = 0; irq_vec[i].source != 0; i++){
		if(irq_vec[i].source == irq_source){
			*stat = dev_read(irq_vec[i].stat_reg);
			pr_debug("IRQ read state: irq[%d] stat[%d]", irq_source, *stat);
			return TRUE;
		}
	}
	pr_err("IRQ source [%d] is not supported", irq_source);
	*stat = 0;
	return FALSE;
}

/*******************************************************************
 * Clear IRQ miscellaneous
 */
int irq_clear_source(hdmi_tx_dev_t *dev, irq_sources_t irq_source)
{
	int i = 0;

	for(i = 0; irq_vec[i].source != 0; i++){
		if(irq_vec[i].source == irq_source){
			pr_debug("IRQ write clear: irq[%d] mask[%d]", irq_source, 0xff);
			dev_write(irq_vec[i].stat_reg,  0xff);
			return TRUE;
		}
	}
	pr_err("IRQ source [%d] is not supported", irq_source);
	return FALSE;
}

int irq_clear_bit(hdmi_tx_dev_t *dev, irq_sources_t irq_source, u8 bit_mask)
{
	int i = 0;

	for(i = 0; irq_vec[i].source != 0; i++){
		if(irq_vec[i].source == irq_source){
			pr_debug("IRQ write clear bit: irq[%d] bitmask[%d]", irq_source, bit_mask);
			dev_write_mask(irq_vec[i].stat_reg, bit_mask, 1);
			return TRUE;
		}
	}
	pr_err("IRQ source [%d] is not supported", irq_source);
	return FALSE;
}

/*******************************************************************
 * Mute IRQ miscellaneous
 */
int irq_mute_source(hdmi_tx_dev_t *dev, irq_sources_t irq_source)
{
	int i = 0;

	for(i = 0; irq_vec[i].source != 0; i++){
		if(irq_vec[i].source == irq_source){
			pr_debug("IRQ write mute: irq[%d] mask[%d]", irq_source, 0xff);
			dev_write(irq_vec[i].mute_reg,  0xff);
			return TRUE;
		}
	}
	pr_err("IRQ source [%d] is not supported", irq_source);
	return FALSE;
}

int irq_unmute_source(hdmi_tx_dev_t *dev, irq_sources_t irq_source)
{
	int i = 0;

	for(i = 0; irq_vec[i].source != 0; i++){
		if(irq_vec[i].source == irq_source){
			pr_debug("IRQ write unmute: irq[%d] mask[%d]", irq_source, 0x00);
			dev_write(irq_vec[i].mute_reg,  0x00);
			return TRUE;
		}
	}
	pr_err("IRQ source [%d] is not supported", irq_source);
	return FALSE;
}

int irq_mask_bit(hdmi_tx_dev_t *dev, irq_sources_t irq_source, u8 bit_mask)
{
	int i = 0;

	for(i = 0; irq_vec[i].source != 0; i++){
		if(irq_vec[i].source == irq_source){
			pr_debug("IRQ mask bit: irq[%d] bit_mask[%d]", irq_source, bit_mask);
			dev_write_mask(irq_vec[i].mute_reg, bit_mask, 1);
			return TRUE;
		}
	}
	pr_err("IRQ source [%d] is not supported", irq_source);
	return FALSE;
}

int irq_unmask_bit(hdmi_tx_dev_t *dev, irq_sources_t irq_source, u8 bit_mask)
{
	int i = 0;

	for(i = 0; irq_vec[i].source != 0; i++){
		if(irq_vec[i].source == irq_source){
			pr_debug("IRQ unmask bit: irq[%d] bit_mask[%d]", irq_source, bit_mask);
			dev_write_mask(irq_vec[i].mute_reg, bit_mask, 0);
			return TRUE;
		}
	}
	pr_err("IRQ source [%d] is not supported", irq_source);
	return FALSE;
}

void irq_mute(hdmi_tx_dev_t *dev)
{
	dev_write(IH_MUTE,  0x3);
}

void irq_unmute(hdmi_tx_dev_t *dev)
{
	dev_write(IH_MUTE,  0x0);
}

void irq_clear_all(hdmi_tx_dev_t *dev)
{
	irq_clear_source(dev, AUDIO_PACKETS);
	irq_clear_source(dev, OTHER_PACKETS);
	irq_clear_source(dev, PACKETS_OVERFLOW);
	irq_clear_source(dev, AUDIO_SAMPLER);
	irq_clear_source(dev, PHY);
	irq_clear_source(dev, I2C_DDC);
	irq_clear_source(dev, CEC);
	irq_clear_source(dev, VIDEO_PACKETIZER);
	irq_clear_source(dev, I2C_PHY);
	irq_clear_source(dev, AUDIO_DMA);
}

void irq_mask_all(hdmi_tx_dev_t *dev)
{
	irq_mute(dev);
	irq_mute_source(dev, AUDIO_PACKETS);
	irq_mute_source(dev, OTHER_PACKETS);
	irq_mute_source(dev, PACKETS_OVERFLOW);
	irq_mute_source(dev, AUDIO_SAMPLER);
	irq_mute_source(dev, PHY);
	irq_mute_source(dev, I2C_DDC);
	irq_mute_source(dev, CEC);
	irq_mute_source(dev, VIDEO_PACKETIZER);
	irq_mute_source(dev, I2C_PHY);
	irq_mute_source(dev, AUDIO_DMA);
}

void irq_scdc_read_request(hdmi_tx_dev_t *dev, int enable)
{
	if(enable)
		irq_unmask_bit(dev, I2C_DDC, IH_MUTE_I2CM_STAT0_SCDC_READREQ_MASK);
	else
		irq_mask_bit(dev, I2C_DDC, IH_MUTE_I2CM_STAT0_SCDC_READREQ_MASK);
}

void un_mask_i2c_interrupt(hdmi_tx_dev_t *dev)
{
	irq_clear_source(dev, I2C_DDC);

	// scdc_readreq
	irq_unmask_bit(dev, I2C_DDC, IH_MUTE_I2CM_STAT0_I2CMASTERERROR_MASK);
	// I2Cmasterdone
	irq_unmask_bit(dev, I2C_DDC, IH_MUTE_I2CM_STAT0_I2CMASTERDONE_MASK);
	// I2Cmastererror
	irq_unmask_bit(dev, I2C_DDC, IH_MUTE_I2CM_STAT0_SCDC_READREQ_MASK);
}

void irq_hpd_sense_enable(hdmi_tx_dev_t *dev)
{
	i2cddc_fast_mode(dev, 0);

	// Enable HDMI TX PHY HPD Detector
	phy_enable_hpd_sense(dev, TRUE);

	dev_write(IH_MUTE_PHY_STAT0, ~(IH_MUTE_PHY_STAT0_HPD_MASK |
						IH_MUTE_PHY_STAT0_RX_SENSE_0_MASK |
						IH_MUTE_PHY_STAT0_RX_SENSE_1_MASK |
						IH_MUTE_PHY_STAT0_RX_SENSE_2_MASK |
						IH_MUTE_PHY_STAT0_RX_SENSE_3_MASK ));
	dev_write(PHY_MASK0, ~(PHY_MASK0_HPD_MASK |
				  PHY_MASK0_RX_SENSE_0_MASK |
				  PHY_MASK0_RX_SENSE_1_MASK |
				  PHY_MASK0_RX_SENSE_2_MASK |
				  PHY_MASK0_RX_SENSE_3_MASK));
	// Un-mask main interrupt
	irq_unmute(dev);

}

u32 read_interrupt_decode(hdmi_tx_dev_t *dev)
{
	return (dev_read(IH_DECODE) & 0xFF);
}

int decode_is_fc_stat0(u32 decode)
{
	return (decode & IH_DECODE_IH_FC_STAT0_MASK) ? 1 : 0;
}

int decode_is_fc_stat1(u32 decode)
{
	return (decode & IH_DECODE_IH_FC_STAT1_MASK) ? 1 : 0;
}

int decode_is_fc_stat2_vp(u32 decode)
{
	return (decode & IH_DECODE_IH_FC_STAT2_VP_MASK) ? 1 : 0;
}

int decode_is_as_stat0(u32 decode)
{
	return (decode & IH_DECODE_IH_AS_STAT0_MASK) ? 1 : 0;
}

int decode_is_phy(u32 decode)
{
	return (decode & IH_DECODE_IH_PHY_MASK) ? 1 : 0;
}

int decode_is_phy_lock(u32 decode)
{
	return (decode & IH_PHY_STAT0_TX_PHY_LOCK_MASK) ? 1 : 0;
}

int decode_is_phy_hpd(u32 decode)
{
	return (decode & IH_PHY_STAT0_HPD_MASK) ? 1 : 0;
}

int decode_is_phy_rx_s0(u32 decode)
{
	return (decode & IH_PHY_STAT0_RX_SENSE_0_MASK) ? 1 : 0;
}

int decode_is_phy_rx_s1(u32 decode)
{
	return (decode & IH_PHY_STAT0_RX_SENSE_1_MASK) ? 1 : 0;
}

int decode_is_phy_rx_s2(u32 decode)
{
	return (decode & IH_PHY_STAT0_RX_SENSE_2_MASK) ? 1 : 0;
}

int decode_is_phy_rx_s3(u32 decode)
{
	return (decode & IH_PHY_STAT0_RX_SENSE_3_MASK) ? 1 : 0;
}

int decode_is_i2c_stat0(u32 decode)
{
	return (decode & IH_DECODE_IH_I2CM_STAT0_MASK) ? 1 : 0;
}

int decode_is_cec_stat0(u32 decode)
{
	return (decode & IH_DECODE_IH_CEC_STAT0_MASK) ? 1 : 0;
}
