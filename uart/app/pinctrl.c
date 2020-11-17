/*
 * Copyright (c) 2017-2020, Renesas Electronics Corporation
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */
#include "soc_common.h"
#include "pinctrl.h"
#include "task.h"

typedef struct RcarPin {
	uint8_t	ipsr;
	uint8_t	ipsrVal;
	uint8_t	ipsrPos;
	uint8_t	gpsr;
	uint8_t	gpsrPos;
	const char *name;
} RcarPin_t;

typedef struct RcarPinGroup {
	uint8_t		numPins;
	uint16_t	*pinSet;
	uint8_t		modsel;
	uint32_t	modselVal;
	uint32_t	modselMask;
} RcarPinGroup_t;

enum RcarPinName {
	SCL2_A = 0,
	SDA2_A,

	CAN0_TX_A,
	CAN0_RX_A,

	CAN_CLK,
	
	SCIF_CLK_A,
	
	RX1_A,
	TX1_A,
	
	RX2_A,
	TX2_A,

	RX3_A,
	TX3_A,

	RX4_A,
	TX4_A,

	RX5_A,
	TX5_A,
				
	CAN1_TX,
	CAN1_RX,

	SSI_SDATA0,
	SSI_WS01239,
	SSI_SCK01239,

	AUDIO_CLKA_A,
	AUDIO_CLKOUT3_A,
}RcarPinName_t;

enum RcarDev {
	RCAR_DEV_I2C2 = 0,

	RCAR_DEV_CAN0,

	RCAR_DEV_CAN1,

	RCAR_DEV_SSI0,

	RCAR_DEV_ADG,
	
	RCAR_DEV_SCIF0,
	
	RCAR_DEV_SCIF1,
	
	RCAR_DEV_SCIF2,
	
	RCAR_DEV_SCIF3,
	
	RCAR_DEV_SCIF4,
	
	RCAR_DEV_SCIF5,
	
	/* Add more controllers as needed */
} RcarDev_t;

static RcarPin_t rcarPins[] = {
	[SCL2_A] = {
		.name = "SCL2_A",
		.ipsr = 12,
		.ipsrVal = 4,
		.ipsrPos = 8,
		.gpsr = 5,
		.gpsrPos = 4,
	},
	[SDA2_A] = {
		.name = "SDA2_A",
		.ipsr = 11,
		.ipsrVal = 4,
		.ipsrPos = 24,
		.gpsr = 5,
		.gpsrPos = 0,
	},

#if (R_BOARD == RCAR_SALVATOR_XS) || (R_BOARD == RZG2_HIHOPE)
	/* Salvator-X/XS CAN Adaptor Board */
	[CAN0_TX_A] = {		/* Pin 11, MDT1/RDn on EXIO D (CN28) */
		.name = "CAN0_TX_A",
		.ipsr = 4,
		.ipsrVal = 8,
		.ipsrPos = 24,
		.gpsr = 1,
		.gpsrPos = 23,
	},
	[CAN0_RX_A] = {		/* Pin 48, GP1_24 on EXIO D (CN28) */
		.name = "CAN0_RX_A",
		.ipsr = 4,
		.ipsrVal = 8,
		.ipsrPos = 28,
		.gpsr = 1,
		.gpsrPos = 24,
	},
	[CAN1_TX] = {		/* Pin 14, MDT0/BSn on EXIO D (CN28) */
		.name = "CAN1_TX",
		.ipsr = 4,
		.ipsrVal = 8,
		.ipsrPos = 20,
		.gpsr = 1,
		.gpsrPos = 22,
	},
	[CAN1_RX] = {		/* Pin 13, WE1n on EXIO D (CN28) */
		.name = "CAN1_RX",
		.ipsr = 5,
		.ipsrVal = 8,
		.ipsrPos = 4,
		.gpsr = 1,
		.gpsrPos = 26,
	},
	[CAN_CLK] = {
		.name = "CAN_CLK",
		.ipsr = 5,
		.ipsrVal = 8,
		.ipsrPos = 0,
		.gpsr = 1,
		.gpsrPos = 25,
	},
	
// SCIF(UART) Modules	
	[SCIF_CLK_A] = {
		.name = "SCIF_CLK_A",
		.ipsr = 17,
		.ipsrVal = 1,
		.ipsrPos = 4,
		.gpsr = 6,
		.gpsrPos = 23,
	},	
	[RX1_A] = {
		.name = "RX1_A",
		.ipsr = 12,
		.ipsrVal = 0,
		.ipsrPos = 12,
		.gpsr = 5,
		.gpsrPos = 5,
	},
	[TX1_A] = {
		.name = "TX1_A",
		.ipsr = 12,
		.ipsrVal = 0,
		.ipsrPos = 16,
		.gpsr = 5,
		.gpsrPos = 6,
	},
	[RX2_A] = {
		.name = "RX2_A",
		.ipsr = 13,
		.ipsrVal = 0,
		.ipsrPos = 4,
		.gpsr = 5,
		.gpsrPos = 11,
	},
	[TX2_A] = {
		.name = "TX2_A",
		.ipsr = 13,
		.ipsrVal = 0,
		.ipsrPos = 0,
		.gpsr = 5,
		.gpsrPos = 10,
	},
	[RX3_A] = {
		.name = "RX3_A",
		.ipsr = 4,
		.ipsrVal = 3,
		.ipsrPos = 24,
		.gpsr = 1,
		.gpsrPos = 23,
	},
	[TX3_A] = {
		.name = "TX3_A",
		.ipsr = 4,
		.ipsrVal = 3,
		.ipsrPos = 28,
		.gpsr = 1,
		.gpsrPos = 24,
	},	
	[RX4_A] = {
		.name = "RX4_A",
		.ipsr = 0,
		.ipsrVal = 3,
		.ipsrPos = 8,
		.gpsr = 2,
		.gpsrPos = 11,
	},
	[TX4_A] = {
		.name = "TX4_A",
		.ipsr = 0,
		.ipsrVal = 3,
		.ipsrPos = 12,
		.gpsr = 2,
		.gpsrPos = 12,
	},
	[RX5_A] = {
		.name = "RX5_A",
		.ipsr = 14,
		.ipsrVal = 1,
		.ipsrPos = 0,
		.gpsr = 5,
		.gpsrPos = 19,
	},
	[TX5_A] = {
		.name = "TX5_A",
		.ipsr = 14,
		.ipsrVal = 1,
		.ipsrPos = 4,
		.gpsr = 5,
		.gpsrPos = 21,
	},				
			
#elif (R_BOARD == RCAR_EBISU)
	/* E3 Ebisu board CN10 */
	[CAN0_TX_A] = {		/* CAN0_TX: GP0_12 / D12 / VI5_DATA3_B */
		.name = "CAN0_TX_A",
		.ipsr = 7,
		.ipsrVal = 3,
		.ipsrPos = 4,
		.gpsr = 0,
		.gpsrPos = 12,
	},
	[CAN0_RX_A] = {		/* CAN0_RX: GP0_13 / D13 / VI5_DATA4_B */
		.name = "CAN0_RX_A",
		.ipsr = 7,
		.ipsrVal = 3,
		.ipsrPos = 8,
		.gpsr = 0,
		.gpsrPos = 13,
	},
#endif

	[SSI_SDATA0] = {
		.name = "SSI_SDATA0",
		.ipsr = 14,
		.ipsrVal = 0,
		.ipsrPos = 28,
		.gpsr = 6,
		.gpsrPos = 2,
	},

	[SSI_WS01239] = {
		.name = "SSI_WS01239",
		.ipsr = 14,
		.ipsrVal = 0,
		.ipsrPos = 24,
		.gpsr = 6,
		.gpsrPos = 1,
	},
	[SSI_SCK01239] = {
		.name = "SSI_SCK01239",
		.ipsr = 14,
		.ipsrVal = 0,
		.ipsrPos = 20,
		.gpsr = 6,
		.gpsrPos = 0,
	},
	[AUDIO_CLKA_A] = {
		.name = "AUDIO_CLKA_A",
		.ipsr = 17,
		.ipsrVal = 0,
		.ipsrPos = 0,
		.gpsr = 6,
		.gpsrPos = 22,
	},
	[AUDIO_CLKOUT3_A] = {
		.name = "AUDIO_CLKOUT3_A",
		.ipsr = 14,
		.ipsrVal = 8,
		.ipsrPos = 0,
		.gpsr = 5,
		.gpsrPos = 19,
	},
};

static uint16_t rcarPinSet_I2C2_A[] = {
	SDA2_A,
	SCL2_A,
};

static uint16_t rcarPinSet_CAN0_A[] = {
	CAN0_TX_A,
	CAN0_RX_A,
#if (R_BOARD != RZG2_HIHOPE)
	CAN_CLK,
#endif
};

static uint16_t rcarPinSet_CAN1[] = {
	CAN1_TX,
	CAN1_RX,
};

static uint16_t rcarPinSet_SCIF0[] = {
	SCIF_CLK_A,
};

static uint16_t rcarPinSet_SCIF1[] = {
	RX1_A,
	TX1_A,
};

static uint16_t rcarPinSet_SCIF2[] = {
	RX2_A,
	TX2_A,
};

static uint16_t rcarPinSet_SCIF3[] = {
	RX3_A,
	TX3_A,
};

static uint16_t rcarPinSet_SCIF4[] = {
	RX4_A,
	TX4_A,
};

static uint16_t rcarPinSet_SCIF5[] = {
	RX5_A,
	TX5_A,
};

static uint16_t rcarPinSet_SSI0[] = {
	SSI_SDATA0,
	SSI_WS01239,
	SSI_SCK01239,
};

static uint16_t rcarPinSet_ADG[] = {
	AUDIO_CLKA_A,
	AUDIO_CLKOUT3_A,
};

static RcarPinGroup_t rcarPinGroup[] = {
	[RCAR_DEV_I2C2] = {
		.numPins = ARRAY_SIZE(rcarPinSet_I2C2_A),
		.pinSet = rcarPinSet_I2C2_A,
		.modsel = 0,
		.modselVal = 0,
		.modselMask = ~BIT(21),
	},

	[RCAR_DEV_CAN0] = {
		.numPins = ARRAY_SIZE(rcarPinSet_CAN0_A),
		.pinSet = rcarPinSet_CAN0_A,
		.modsel = 1,
		.modselVal = 0,
		.modselMask = ~BIT(6),
	},

	[RCAR_DEV_CAN1] = {
		.numPins = ARRAY_SIZE(rcarPinSet_CAN1),
		.pinSet = rcarPinSet_CAN1,
		.modsel = 0xFF,	/* nothing to do */
	},

	[RCAR_DEV_SSI0] = {
		.numPins = ARRAY_SIZE(rcarPinSet_SSI0),
		.pinSet = rcarPinSet_SSI0,
		.modsel = 0xFF,	/* nothing to do */
	},

	[RCAR_DEV_ADG] = {
		.numPins = ARRAY_SIZE(rcarPinSet_ADG),
		.pinSet = rcarPinSet_ADG,
		.modsel = 0,
		.modselVal = 0,
		.modselMask = ~(BIT(4) | BIT(3)),
	},
	
	[RCAR_DEV_SCIF0] = {
		.numPins = ARRAY_SIZE(rcarPinSet_SCIF0),
		.pinSet = rcarPinSet_SCIF0,
		.modsel = 1,
		.modselVal = 0,
		.modselMask = ~BIT(10),
	},
		
	[RCAR_DEV_SCIF1] = {
		.numPins = ARRAY_SIZE(rcarPinSet_SCIF1),
		.pinSet = rcarPinSet_SCIF1,
		.modsel = 1,
		.modselVal = 0,
		.modselMask = ~BIT(11),
	},
	
	[RCAR_DEV_SCIF2] = {
		.numPins = ARRAY_SIZE(rcarPinSet_SCIF2),
		.pinSet = rcarPinSet_SCIF2,
		.modsel = 1,
		.modselVal = 0,
		.modselMask = ~BIT(12),
	},

	[RCAR_DEV_SCIF3] = {
		.numPins = ARRAY_SIZE(rcarPinSet_SCIF3),
		.pinSet = rcarPinSet_SCIF3,
		.modsel = 1,
		.modselVal = 0,
		.modselMask = ~BIT(13),
	},

	[RCAR_DEV_SCIF4] = {
		.numPins = ARRAY_SIZE(rcarPinSet_SCIF4),
		.pinSet = rcarPinSet_SCIF4,
		.modsel = 1,
		.modselVal = 0,
		.modselMask = ~BIT(14),
	},

	[RCAR_DEV_SCIF5] = {
		.numPins = ARRAY_SIZE(rcarPinSet_SCIF5),
		.pinSet = rcarPinSet_SCIF5,
		.modsel = 2,
		.modselVal = 0,
		.modselMask = ~BIT(26),
	},			
};

#if (R_BOARD == RCAR_SALVATOR_XS)
static int rcarPinGroupEnabled[] = {
	RCAR_DEV_I2C2,
	RCAR_DEV_CAN1,
	RCAR_DEV_SSI0,
	RCAR_DEV_ADG,
};
#elif (R_BOARD == RCAR_EBISU)
static int rcarPinGroupEnabled[] = {
	RCAR_DEV_CAN0,
};
#elif (R_BOARD == RZG2_HIHOPE)
static int rcarPinGroupEnabled[] = {
	RCAR_DEV_CAN0,
	RCAR_DEV_SCIF0,
	RCAR_DEV_SCIF1,
	RCAR_DEV_SCIF2,
	RCAR_DEV_SCIF3,
	RCAR_DEV_SCIF4,
	RCAR_DEV_SCIF5,
};
#endif

static const uint32_t GPIO_IOINTSEL[8]= {
	GPIO_IOINTSEL0,
	GPIO_IOINTSEL1,
	GPIO_IOINTSEL2,
	GPIO_IOINTSEL3,
	GPIO_IOINTSEL4,
	GPIO_IOINTSEL5,
	GPIO_IOINTSEL6,
	GPIO_IOINTSEL7,
};

static const uint32_t GPIO_INOUTSEL[8] = {
	GPIO_INOUTSEL0,
	GPIO_INOUTSEL1,
	GPIO_INOUTSEL2,
	GPIO_INOUTSEL3,
	GPIO_INOUTSEL4,
	GPIO_INOUTSEL5,
	GPIO_INOUTSEL6,
	GPIO_INOUTSEL7,
};

static const uint32_t GPIO_INDT[8] = {
	GPIO_INDT0,
	GPIO_INDT1,
	GPIO_INDT2,
	GPIO_INDT3,
	GPIO_INDT4,
	GPIO_INDT5,
	GPIO_INDT6,
	GPIO_INDT7,
};

static const uint32_t GPIO_OUTDT[8] = {
	GPIO_OUTDT0,
	GPIO_OUTDT1,
	GPIO_OUTDT2,
	GPIO_OUTDT3,
	GPIO_OUTDT4,
	GPIO_OUTDT5,
	GPIO_OUTDT6,
	GPIO_OUTDT7,
};

static const uint32_t GPIO_POSNEG[8]= {
	GPIO_POSNEG0,
	GPIO_POSNEG1,
	GPIO_POSNEG2,
	GPIO_POSNEG3,
	GPIO_POSNEG4,
	GPIO_POSNEG5,
	GPIO_POSNEG6,
	GPIO_POSNEG7,
};

static const uint32_t GPIO_OUTDTSEL[8]= {
	GPIO_OUTDTSEL0,
	GPIO_OUTDTSEL1,
	GPIO_OUTDTSEL2,
	GPIO_OUTDTSEL3,
	GPIO_OUTDTSEL4,
	GPIO_OUTDTSEL5,
	GPIO_OUTDTSEL6,
	GPIO_OUTDTSEL7,
};

static void setbit_l(uint32_t addr, uint32_t pos)
{
	uint32_t val = readl(addr);

	writel(val | BIT(pos), addr);
}

static void clearbit_l(uint32_t addr, uint32_t pos)
{
	uint32_t val = readl(addr);

	writel(val & ~BIT(pos), addr);
}

static void modifybit_l(uint32_t addr, int bit, uint32_t value)
{
	u32 tmp = readl(addr);

	if (value)
		tmp |= BIT(bit);
	else
		tmp &= ~BIT(bit);

	writel(tmp, addr);
}

static void pfcWrite(uint32_t addr, uint32_t val)
{
	writel(~val, PFC_PMMR);
	writel(val, addr);
}

static void pfcSetGPSR(uint8_t gpio, uint8_t block, uint8_t pos)
{
	uint32_t val = readl(PFC_GPSR(block));

	val = gpio ? val & ~BIT(pos) : val | BIT(pos);
	pfcWrite(PFC_GPSR(block), val);
	dbg_printf("%s: gpio%d,%d (PFC_GPSR reg 0x%x = 0x%x)\n", __func__, block, pos, PFC_GPSR(block), val);
}

static void pfcSetPeripheral(uint8_t block, uint8_t pos)
{
	pfcSetGPSR(0, block, pos);
}

static void pfcSetIPSR(uint8_t ipsr, uint8_t val, uint8_t pos)
{
	uint32_t reg = readl(PFC_IPSR(ipsr));

	reg &= ~(0xF << pos);
	reg |= (val << pos);
	pfcWrite(PFC_IPSR(ipsr), reg);
	dbg_printf("%s: ipsr%d,%d set to 0x%x (PFC_GPSR reg 0x%x = 0x%x)\n", __func__, ipsr, pos, val, PFC_IPSR(ipsr), reg);
}

static void pfcSetModsel(uint8_t modsel, uint32_t val, uint32_t mask)
{
	uint32_t reg = readl(PFC_MODSEL(modsel));

	reg &= ~mask;
	reg |= val;
	pfcWrite(PFC_MODSEL(modsel), reg);
}

static void gpioSetPin(uint32_t block, uint32_t gpio, uint32_t output)
{
	/* Configure positive logic in POSNEG */
	modifybit_l(GPIO_POSNEG[block], gpio, 0);
	/* Select "General Input/Output Mode" in IOINTSEL */
	modifybit_l(GPIO_IOINTSEL[block], gpio, 0);
	/* Select Input Mode or Output Mode in INOUTSEL */
	modifybit_l(GPIO_INOUTSEL[block], gpio, output);
	/* Select General Output Register to output data in OUTDTSEL */
	modifybit_l(GPIO_OUTDTSEL[block], gpio, 0);
}

void gpioSetOutput(uint8_t block, uint8_t bit, uint8_t lvl)
{
	taskENTER_CRITICAL();
	if (lvl)
		setbit_l(GPIO_OUTDT[block], bit);
	else
		clearbit_l(GPIO_OUTDT[block], bit);
	taskEXIT_CRITICAL();
}

uint8_t gpioGetInput(uint8_t block, uint8_t bit)
{
	return (readl(GPIO_INDT[block]) >> bit) & 1;
}

void pfcSetGPIO(uint8_t block, uint8_t pos, uint8_t output)
{
	/* Enable module clock */
	clkEnable(9, 12 - block);

	taskENTER_CRITICAL();
	if (output) {
		setbit_l(GPIO_INOUTSEL[block], pos);
		dbg_printf("%s: gpio%d,%d output (GPIO_INOUTSEL reg 0x%x = 0x%x)\n", __func__, block, pos, GPIO_INOUTSEL[block], readl(GPIO_INOUTSEL[block]));
	} else {
		clearbit_l(GPIO_INOUTSEL[block], pos);
		dbg_printf("%s: gpio%d,%d input  (GPIO_INOUTSEL reg 0x%x = 0x%x)\n", __func__, block, pos, GPIO_INOUTSEL[block], readl(GPIO_INOUTSEL[block]));
	}

	pfcSetGPSR(1, block, pos);
	gpioSetPin(block, pos, output);
	taskEXIT_CRITICAL();
}

void rcarPFCInit(void)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(rcarPinGroupEnabled); i++) {
		RcarPinGroup_t *g = &rcarPinGroup[rcarPinGroupEnabled[i]];
		int j;

		for (j = 0; j < g->numPins; j++) {
			RcarPin_t p = rcarPins[g->pinSet[j]];

			if (p.name)
				dbg_printf("%s: %s\n", __func__, p.name);
			pfcSetPeripheral(p.gpsr, p.gpsrPos);
			pfcSetIPSR(p.ipsr, p.ipsrVal, p.ipsrPos);
		}
		if (g->modsel == 0xFF)
			continue;

		pfcSetModsel(g->modsel, g->modselVal, g->modselMask);
	}
}
