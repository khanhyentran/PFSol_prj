struct cam_reg imx219_q_test[] = {
	
    {0x0114,0x01},  // 2-Lane
	{0x0128,0x00},  // timing setting:0-auto 1-manual
	
	{0x012A,0x0C},	// INCK
	{0x012B,0x00}, 	// INCK = 12MHz
	
	
	// 800 X 480
	/* frame length(height) = 1080(=438H) */
	/* line  length(width)  = 3448(=D78H)(default) */
	{0x0160, 0x04},
	{0x0161, 0x38},
	{0x0162, 0x0D},
	{0x0163, 0x78},
	/* X range */
	{0x0164, 0x02},
	{0x0165, 0xA8},
	{0x0166, 0x0A},
	{0x0167, 0x27},
	/* Y range */
	{0x0168, 0x02},
	{0x0169, 0xB4},
	{0x016A, 0x06},
	{0x016B, 0xEB},
	/* signal range */
	{0x016C, 0x05},
	{0x016D, 0x00},
	{0x016E, 0x04},
	{0x016F, 0x38},
	

	
	/* output format RAW8 */
	{0x018C, 0x08},
	{0x018D, 0x08},
	
	//~{0x0170,0x01},	// X Increment for odd pixels 1, 3
	//~{0x0171,0x01},	// Y Increment for odd pixels 1, 3 
	{0x0172,0x03},	// Image oriantation

					// binning(digital) mode
	{0x0174,0x01}, 	// H_direction
	{0x0175,0x01},	// V_direction
	{0x0176,0x01},	// binning cal - H
	{0x0177,0x01},  // binning cal - V
	
	{0x0157,0xAF},  // gain(LONG)
	//~{0x015A,0x07},	// coarse intergration time
	//~{0x015B,0x70},
	//~{0x0189,0x32},
	//~{0x018A,0x03},
	//~{0x018B,0xE8},	
					// clock setting
	{0x0301,0x05}, 	// Video timing pixel clock divider value
	//~{0x0303,0x01}, 	// Video timing system clock divider value 
	 
	{0x0304,0x02},	// Pre PLL clock video timing system divider value
	{0x0305,0x02},	// Pre PLL clock output system divder value
	{0x0306,0x00},	// PLL video timing system multiplier value
	{0x0307,0x3A},
	
	{0x0309,0x08},	// Output pixel clock divider value
	//~{0x030B,0x01}, 	// Output system clock divider value
	{0x030C,0x00},	// PLL output system multiplier value
	{0x030D,0x2A},             
					
	//~SYSCLK = EXCK/0x304 * 0x0306&0x0307 / 0x0303
	//~PIXCLK = EXCK/0x304 * 0x0306&0x0307 / 0x0301
	
	{CAM_TABLE_END, 0x00 }
};

struct cam_reg imx129_miscellaneous[] = {
	{ 0x30EB, 0x05 }, /* Access Code for address over 0x3000 */
	{ 0x30EB, 0x0C }, /* Access Code for address over 0x3000 */
	{ 0x300A, 0xFF }, /* Access Code for address over 0x3000 */
	{ 0x300B, 0xFF }, /* Access Code for address over 0x3000 */
	{ 0x30EB, 0x05 }, /* Access Code for address over 0x3000 */
	{ 0x30EB, 0x09 }, /* Access Code for address over 0x3000 */
	
	{ 0x0114, 0x01 }, /* CSI_LANE_MODE[1:0} */
	{ 0x0128, 0x00 }, /* DPHY_CNTRL */
	{ 0x012A, 0x0C }, /* EXCK_FREQ[15:8] */
	{ 0x012B, 0x00 }, /* EXCK_FREQ[7:0] */
	
	{ 0x0160, 0x0A }, /* FRM_LENGTH_A[15:8] */
	{ 0x0161, 0x83 }, /* FRM_LENGTH_A[7:0] */
	{ 0x0162, 0x0D }, /* LINE_LENGTH_A[15:8] */
	{ 0x0163, 0x78 }, /* LINE_LENGTH_A[7:0] */
	
	{ 0x0170, 0x01 }, /* X_ODD_INC_A[2:0] */
	{ 0x0171, 0x01 }, /* Y_ODD_INC_A[2:0] */
	{ 0x0174, 0x00 }, /* BINNING_MODE_H_A */
	{ 0x0175, 0x00 }, /* BINNING_MODE_V_A */
	
	{ 0x018C, 0x08 }, /* CSI_DATA_FORMAT_A[15:8] */ 
	{ 0x018D, 0x08 }, /* CSI_DATA_FORMAT_A[7:0] */
	
	{ 0x0301, 0x05 }, /* VTPXCK_DIV */
	{ 0x0303, 0x01 }, /* VTSYCK_DIV */
	{ 0x0304, 0x02 }, /* PREPLLCK_VT_DIV[3:0] */
	{ 0x0305, 0x02 }, /* PREPLLCK_OP_DIV[3:0] */
	{ 0x0306, 0x00 }, /* PLL_VT_MPY[10:8] */
	{ 0x0307, 0x3A }, /* PLL_VT_MPY[7:0] */
	{ 0x0309, 0x08 }, /* OPPXCK_DIV[4:0] */
	{ 0x030C, 0x00 }, /* PLL_OP_MPY[10:8] */
	{ 0x030D, 0x2A }, /* PLL_OP_MPY[7:0] */
	
	{ 0x455E, 0x00 }, /* CIS Tuning */
	{ 0x471E, 0x4B }, /* CIS Tuning */
	{ 0x4767, 0x0F }, /* CIS Tuning */
	{ 0x4750, 0x14 }, /* CIS Tuning */
	{ 0x4540, 0x00 }, /* CIS Tuning */
	{ 0x47B4, 0x14 }, /* CIS Tuning */
	{ 0x4713, 0x30 }, /* CIS Tuning */
	{ 0x478B, 0x10 }, /* CIS Tuning */
	{ 0x478F, 0x10 }, /* CIS Tuning */
	{ 0x4793, 0x10 }, /* CIS Tuning */
	{ 0x4797, 0x0E }, /* CIS Tuning */
	{ 0x479B, 0x0E }, /* CIS Tuning */
	{ CAM_TABLE_END, 0x00 }
};