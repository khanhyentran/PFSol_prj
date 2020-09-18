#include "soc_common.h"
#include "pinctrl.h"

/*************************************
 * UART-CLK
 * HiHope Board:
 *  -
 *  -
 *************************************/
static RcarPin_t uart_clk_pins[] = {
	{
		// Table 8.5
		//   Register = IP17[7:4]
        //   IPSR = H'1
		.name = "SCIF_CLK_A",        
		.ipsr = 17,
		.ipsrVal = 1,
		.ipsrPos = 4,
		
		// Table 8.4
		//   GP6_23 = AUDIO_CLKB_B
		.gpsr = 6,
		.gpsrPos = 23,
	},

};
static RcarPinGroup_t uart_clk_group = {
	.numPins = ARRAY_SIZE(uart_clk_pins),
	.pins = uart_clk_pins,
	// Table 8.18
	//   MOD_SEL1, bit 10 = register "sel_scif"
	.modsel = 1,
	.modselMask = ~BIT(10),
	
	// Table 8.19:
	//   Register = "sel_scif"
	//   Set Value = H'0
	.modselVal = 0,
	
};

/*************************************
 * UART-1
 * HiHope Board:
 *  -
 *  -
 *************************************/
static RcarPin_t uart1_pins[] = {
	{
		// Table 8.5
		//   Register = IP12[15:12]
        //   IPSR = H'0		
		.name = "RX1_A",
		.ipsr = 12,
		.ipsrVal = 0,
		.ipsrPos = 12,

		// Table 8.4
		//   GP5_5 = RX1_A
		.gpsr = 5,
		.gpsrPos = 5,
	},
	{
		// Table 8.5
		//   Register = IP12[19:16]
        //   IPSR = H'0		
		.name = "TX1_A",
		.ipsr = 12,
		.ipsrVal = 0,
		.ipsrPos = 16,

		// Table 8.4
		//   GP5_6 = TX1_A		
		.gpsr = 5,
		.gpsrPos = 6,
	},
};
static RcarPinGroup_t uart1_group = {
	.numPins = ARRAY_SIZE(uart1_pins),
	.pins = uart1_pins,
	// Table 8.18
	//   MOD_SEL1, bit 11 = register "sel_scif1"	
	.modsel = 1,
	.modselMask = ~BIT(11),

	// Table 8.19:
	//   Register = "sel_scif1"
	//   Set Value = H'0		
	.modselVal = 0,	
};

/*************************************
 * UART-2
 * HiHope Board:
 *  -
 *  -
 *************************************/
static RcarPin_t uart2_pins[] = {
	{
		// Table 8.5
		//   Register = IP13[7:4]
        //   IPSR = H'0			
		.name = "RX2_A",
		.ipsr = 13,
		.ipsrVal = 0,
		.ipsrPos = 4,
		
		// Table 8.4
		//   GP5_11 = RX2_A		
		.gpsr = 5,
		.gpsrPos = 11,
	},
	{
		// Table 8.5
		//   Register = IP13[3:0]
        //   IPSR = H'0			
		.name = "TX2_A",
		.ipsr = 13,
		.ipsrVal = 0,
		.ipsrPos = 0,

		// Table 8.4
		//   GP5_10 = TX2_A		
		.gpsr = 5,
		.gpsrPos = 10,
	},
};
static RcarPinGroup_t uart2_group = {
	.numPins = ARRAY_SIZE(uart2_pins),
	.pins = uart2_pins,
	// Table 8.18
	//   MOD_SEL1, bit 12 = register "sel_scif2"	
	.modsel = 1,
	.modselMask = ~BIT(12),

	// Table 8.19:
	//   Register = "sel_scif2"
	//   Set Value = H'0	
	.modselVal = 0,
	
};

/*************************************
 * UART-3
 * HiHope Board:
 *  -
 *  -
 *************************************/
static RcarPin_t uart3_pins[] = {
	{
		// Table 8.5
		//   Register = IP4[27:24]
        //   IPSR = H'3			
		.name = "RX3_A",
		.ipsr = 4,
		.ipsrVal = 3,
		.ipsrPos = 24,
		
		// Table 8.3
		//   GP1_23 = RD#		
		.gpsr = 1,
		.gpsrPos = 23,
	},
	{
		// Table 8.5
		//   Register = IP4[32:28]
        //   IPSR = H'3		
		.name = "TX3_A",
		.ipsr = 4,
		.ipsrVal = 3,
		.ipsrPos = 28,
		
		// Table 8.3
		//   GP1_24 = RD/WR#		
		.gpsr = 1,
		.gpsrPos = 24,
	},
};
static RcarPinGroup_t uart3_group = {
	.numPins = ARRAY_SIZE(uart3_pins),
	.pins = uart3_pins,
	// Table 8.18
	//   MOD_SEL1, bit 13 = register "sel_scif3"	
	.modsel = 1,
	.modselMask = ~BIT(13),

	// Table 8.19:
	//   Register = "sel_scif3"
	//   Set Value = H'0		
	.modselVal = 0,
};

/*************************************
 * UART-4
 * HiHope Board:
 *  -
 *  -
 *************************************/
static RcarPin_t uart4_pins[] = {
	{
		// Table 8.5
		//   Register = IP0[11:8]
        //   IPSR = H'3
		.name = "RX4_A",
		.ipsr = 0,
		.ipsrVal = 3,
		.ipsrPos = 8,

		// Table 8.3
		//   GP2_11 = AVB_PHY_INT	
		.gpsr = 2,
		.gpsrPos = 11,
	},
	{
		// Table 8.5
		//   Register = IP0[15:12]
        //   IPSR = H'3		
		.name = "TX4_A",
		.ipsr = 0,
		.ipsrVal = 3,
		.ipsrPos = 12,

		// Table 8.3
		//   GP2_12 = AVB_LINK		
		.gpsr = 2,
		.gpsrPos = 12,
	},
	{
		// Table 8.5
		//   Register = IP0[7:4]
        //   IPSR = H'3			
		.name = "SCK4_A",
		.ipsr = 0,
		.ipsrVal = 3,
		.ipsrPos = 4,
		
		// Table 8.3
		//   GP2_10 = AVB_MAGIC	
		.gpsr = 2,
		.gpsrPos = 10,
	},	
	{
		// Table 8.5
		//   Register = IP0[19:16]
        //   IPSR = H'3		
		.name = "CTS4#_A",
		.ipsr = 0,
		.ipsrVal = 3,
		.ipsrPos = 16,
		
		// Table 8.3
		//   GP2_13 = AVB_AVTP_MATCH_A	
		.gpsr = 2,
		.gpsrPos = 13,
	},	
	{
		// Table 8.5
		//   Register = IP0[23:20]
        //   IPSR = H'3		
		.name = "RTS4#_A",
		.ipsr = 0,
		.ipsrVal = 3,
		.ipsrPos = 20,
		
		// Table 8.3
		//   GP2_14 = AVB_AVTP_CAPTURE_A 		
		.gpsr = 2,
		.gpsrPos = 14,
	},			
};
static RcarPinGroup_t uart4_group = {
	.numPins = ARRAY_SIZE(uart4_pins),
	.pins = uart4_pins,
	// Table 8.18
	//   MOD_SEL1, bit 14 = register "sel_scif4_0"	
	.modsel = 1,
	.modselMask = ~BIT(14),

	// Table 8.19:
	//   Register = "sel_scif4_0"
	//   Set Value = H'0	
	.modselVal = 0,
};

/*************************************
 * UART-5
 * HiHope Board:
 *  -
 *  -
 *************************************/
static RcarPin_t uart5_pins[] = {
	{
		// Table 8.5
		//   Register = IP14[3:0]
        //   IPSR = H'1			
		.name = "RX5_A",
		.ipsr = 14,
		.ipsrVal = 1,
		.ipsrPos = 0,
		
		// Table 8.4
		//   GP5_19 = MSIOF0_SS1	
		.gpsr = 5,
		.gpsrPos = 19,
	},
	{
		// Table 8.5
		//   Register = IP14[7:4]
        //   IPSR = H'1			
		.name = "TX5_A",
		.ipsr = 14,
		.ipsrVal = 1,
		.ipsrPos = 4,

		// Table 8.4
		//   GP5_21 = MSIOF0_SS2		
		.gpsr = 5,
		.gpsrPos = 21,
	},
	{
		// Table 8.5
		//   Register = IP16[31:28]
        //   IPSR = H'7			
		.name = "SCK5_A",
		.ipsr = 16,
		.ipsrVal = 7,
		.ipsrPos = 28,
		
		// Table 8.4
		//   GP5_6 = SSI_SDATA9_A		
		.gpsr = 6,
		.gpsrPos = 21,
	},
		
};
static RcarPinGroup_t uart5_group = {
	.numPins = ARRAY_SIZE(uart5_pins),
	.pins = uart5_pins,
	// Table 8.18
	//   MOD_SEL2, bit 26 = register "sel_scif5"	
	.modsel = 2,
	.modselMask = ~BIT(26),

	// Table 8.19:
	//   Register = "sel_scif5"
	//   Set Value = H'0	
	.modselVal = 0,
	
};


RcarPinGroup_t *pin_groups[] = {
	&uart_clk_group,
	&uart1_group,
	&uart2_group,
	&uart3_group,
	&uart4_group,
	&uart5_group,
	NULL	// End of list marker
};
