/*
 * W6100Adapter.h
 *
 *  Created on: Jun 8, 2022
 *      Author: GeneKong
 *
 */

#ifndef IO6LIBRARY_APPLICATION_W6100ADAPTER_H_
#define IO6LIBRARY_APPLICATION_W6100ADAPTER_H_

#include <functional>
#include "wizchip_conf.h"
#include <FEmbed.h>

#ifdef LOG_TAG
#define STASH_TAG						LOG_TAG
#undef LOG_TAG
#endif
#define LOG_TAG "W6100Adapter"

namespace WizNet {

class W6100AdapterOp {
 public:
    virtual ~W6100AdapterOp() {};

    virtual void reset() {};

    virtual void csEnable() {};
    virtual void csDisable() {};

    virtual uint8_t readByte() { return 0; }
    virtual void    writeByte(uint8_t byte) { }
    virtual void    writeBytes(uint8_t* pBuf, datasize_t len) { }
    virtual void    readBytes(uint8_t* pBuf, datasize_t len) { }
    virtual void    vdmXfer(uint8_t* addr, datasize_t alen, uint8_t* data, datasize_t dlen) { }
};

/**
 * @class W6100Adapter
 * @brief W6100Adapter use singleton mode, so one system only support on W6100 SPI Eth phy.
 * 			User must implement W6100AdapterOp for spi byte and bytes operations.
 */
class W6100Adapter {
public:
	W6100Adapter(W6100AdapterOp* op) {
		m_op = op;
        
        static auto readByte = []() -> uint8_t {
            if(W6100AdapterOp* op = W6100Adapter::getOp()) {
                return op->readByte();
            }        
			return 0;
        };
        this->W6100SpiReadByte = readByte;

        static auto writeByte = [](uint8_t byte) {
            if(W6100AdapterOp* op = W6100Adapter::getOp()) {
                op->writeByte(byte);
            }
        };
        this->W6100SpiWriteByte = writeByte;

        static auto writeBytes = [](uint8_t* pBuf, datasize_t len) {
            if(W6100AdapterOp* op = W6100Adapter::getOp()) {
                op->writeBytes(pBuf, len);
            }
        };
        this->W6100SpiWriteBurst = writeBytes;

        static auto readBytes = [](uint8_t* pBuf, datasize_t len) {
            if(W6100AdapterOp* op = W6100Adapter::getOp()) {
                op->readBytes(pBuf, len);
            }
        };
        this->W6100SpiReadBurst = readBytes;

        static auto vdmWriteRead = [](uint8_t* addr, datasize_t alen, uint8_t* data, datasize_t dlen) {
            if(W6100AdapterOp* op = W6100Adapter::getOp()) {
                op->vdmXfer(addr, alen, data, dlen);
            }
        };
        this->W6100SpiVDMXfer = vdmWriteRead;

        static auto csEnable = []() {
            if(W6100AdapterOp* op = W6100Adapter::getOp()) {
                op->csEnable();
            }
        };
        this->W6100CsEnable = csEnable;

        static auto csDisable = []() {
            if(W6100AdapterOp* op = W6100Adapter::getOp()) {
                op->csDisable();
            }
        };
        this->W6100CsDisable = csDisable;

		gWIZNETINFO = {
				{0x00, 0x08, 0xdc, 0xff, 0xff, 0xff},				// Source Hardware Address
				{192, 168, 111, 25},								// Source IPv4 Address
				{255, 255, 255, 0},                                 // Subnet Mask value
				{192, 168, 111, 1},                                 //
				{0xfe,0x80,0x00,0x00,
				0x00,0x00, 0x00,0x00,
				0x02,0x08, 0xdc,0xff,
				0xfe,0xff, 0xff,0xff},                              // Source Link Local Address
				{0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00},                            // Source Global Unicast Address
				{0xff,0xff,0xff,0xff,
				0xff,0xff,0xff,0xff,
				0x00,0x00,0x00, 0x00,
				0x00,0x00,0x00,0x00 },                              // IPv6 Prefix
				{0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00},                             // Gateway IPv6 Address
                {223, 5, 5, 5}
		};   ///< Gateway IPv6 Address
	}

	virtual ~W6100Adapter()
	{
        // Nothing to do
	}

	/**
     * @fn void init()
	 * @brief Initial w6100 adapter implement operators.
	 *
	 */
	void init()
	{
		reg_wizchip_spi_cbfunc(
				W6100SpiReadByte, W6100SpiWriteByte,
				W6100SpiReadBurst, W6100SpiWriteBurst,
				W6100SpiVDMXfer);
		reg_wizchip_cs_cbfunc(W6100CsEnable, W6100CsDisable);

		if(m_op)
        {
			m_op->reset();
        }
	}

	/**
	 * @fn bool isPHYLinkOn()
	 * @brief Check Register PHYSR get LNK[bit 0]
	 *
	 * @return true if phy link is on.
	 */
	bool isPHYLinkOn()
	{
		uint8_t ret = PHY_LINK_OFF;
		if (ctlwizchip(CW_GET_PHYLINK, (void *)&ret) == -1)
		{
			log_w("Unknown PHY link status.");
		}
		return ret == PHY_LINK_ON;
	}

	/**
	 * @fn void updateBufferMap(uint8_t*, uint8_t*)
	 * @brief set socket tx buffer size and rx buffer size.
	 *
	 * @param t_r_map[2][8] socket tx buffer size(0/1/2/4/8/16)
	 */
	bool updateBufferMap(uint8_t t_r_map[2][8])
	{
		if (ctlwizchip(CW_INIT_WIZCHIP, (void *)t_r_map) == -1)
		{
			log_w("W6100 initialized fail.");
			return false;
		}
		return true;
	}

	/**
     * @fn bool setInterruptMask(intr_kind)
	 * @brief flag mapped to IMR, SIMR and SLIMR.
	 *
	 * @param flag flag can OR to get more functions.
	 *
	 * @return execute ok or not?
	 */
	bool setInterruptMask(intr_kind flag)
	{
	    if (ctlwizchip(CW_SET_INTRMASK, &flag) == -1)
	    {
	    	log_w("W6100 set interrupt mask fail.");
	    	return false;
	    }
	    return true;
	}

	bool getInterruptMask(intr_kind *flag)
	{
		if (ctlwizchip(CW_GET_INTRMASK, flag) == -1)
		{
			log_w("W6100 get interrupt mask fail.");
			return false;
		}
		return true;
	}

	/**
	 * @fn bool chipUnlock(uint8_t)
	 * @brief Unlock chip register for write
	 *
	 * @param type must be SYS_CHIP_LOCK,SYS_NET_LOCK,SYS_PHY_LOCK
	 *
	 * @return execute ok or not?
	 */
	bool chipUnlock(uint8_t type)
	{
		if (ctlwizchip(CW_SYS_UNLOCK, &type) == -1)
		{
			log_w("W6100 unlock %d fail.", type);
			return false;
		}
		return true;
	}

	bool chipLock(uint8_t type)
	{
		if (ctlwizchip(CW_SYS_LOCK, &type) == -1)
		{
			log_w("W6100 lock %d fail.", type);
			return false;
		}
		return true;
	}

	bool registerNetInfo()
	{
		if(ctlnetwork(CN_SET_NETINFO, &gWIZNETINFO) == -1)
		{
			log_w("W6100 register network information fail.");
			return false;
		}
		return true;
	}

	wiz_NetInfo gWIZNETINFO;			/// Current w6100 information.
private:
	static W6100AdapterOp* m_op;
    static W6100AdapterOp* getOp() { return m_op; }

	/// Callback for C run library.
    uint8_t (*W6100SpiReadByte)();
	void (*W6100SpiWriteByte)(uint8_t);
	void (*W6100SpiReadBurst)(uint8_t* , datasize_t);
	void (*W6100SpiWriteBurst)(uint8_t* , datasize_t);
	void (*W6100SpiVDMXfer)(uint8_t* addr, datasize_t alen, uint8_t* data, datasize_t dlen);

	void (*W6100CsEnable)();
	void (*W6100CsDisable)();
};

 inline W6100AdapterOp* W6100Adapter::m_op = nullptr;
} /* namespace WizNet */

#ifdef STASH_TAG
#undef LOG_TAG
#define  LOG_TAG	STASH_TAG
#undef STASH_TAG
#endif
#endif /* IO6LIBRARY_APPLICATION_W6100ADAPTER_HPP_ */
