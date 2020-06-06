/* QSPI MODE */
#define READ_MODE		(0)
#define SPI_MODE		(1)

/* QSPI registers */
#define	QSPI_CMNCR		(0x0000)
#define	QSPI_SSLDR		(0x0004)
#define	QSPI_SPBCR		(0x0008)
#define	QSPI_DRCR		(0x000c)
#define	QSPI_DRCMR		(0x0010)
#define	QSPI_DREAR		(0x0014)
#define	QSPI_DROPR		(0x0018)
#define	QSPI_DRENR		(0x001c)
#define	QSPI_SMCR		(0x0020)
#define	QSPI_SMCMR		(0x0024)
#define	QSPI_SMADR		(0x0028)
#define	QSPI_SMOPR		(0x002c)
#define	QSPI_SMENR		(0x0030)

#define	QSPI_SMRDR0		(0x0038)
#define	QSPI_SMRDR1		(0x003c)
#define	QSPI_SMWDR0		(0x0040)
#define	QSPI_SMWDR1		(0x0044)
#define	QSPI_CMNSR		(0x0048)

#define	QSPI_DRDMCR		(0x0058)
#define	QSPI_DRDRENR		(0x005c)
#define	QSPI_SMDMCR		(0x0060)
#define	QSPI_SMDRENR		(0x0064)

/* CMNCR */
#define	CMNCR_MD		(1u << 31)
#define	CMNCR_SFDE		(1u << 24)

#define	CMNCR_MOIIO3(x)		(((u32)(x) & 0x3) << 22)
#define	CMNCR_MOIIO2(x)		(((u32)(x) & 0x3) << 20)
#define	CMNCR_MOIIO1(x)		(((u32)(x) & 0x3) << 18)
#define	CMNCR_MOIIO0(x)		(((u32)(x) & 0x3) << 16)
#define	CMNCR_IO3FV(x)		(((u32)(x) & 0x3) << 14)
#define	CMNCR_IO2FV(x)		(((u32)(x) & 0x3) << 12)
#define	CMNCR_IO0FV(x)		(((u32)(x) & 0x3) << 8)

#define	CMNCR_CPHAT		(1u << 6)
#define	CMNCR_CPHAR		(1u << 5)
#define	CMNCR_SSLP		(1u << 4)
#define	CMNCR_CPOL		(1u << 3)
#define	CMNCR_BSZ(n)		(((u32)(n) & 0x3) << 0)

#define	OUT_0			(0u)
#define	OUT_1			(1u)
#define	OUT_REV			(2u)
#define	OUT_HIZ			(3u)

#define	BSZ_SINGLE		(0)
#define	BSZ_DUAL		(1)

/* SSLDR */
#define	SSLDR_SPNDL(x)		(((u32)(x) & 0x7) << 16)
#define	SSLDR_SLNDL(x)		(((u32)(x) & 0x7) << 8)
#define	SSLDR_SCKDL(x)		(((u32)(x) & 0x7) << 0)

#define	SPBCLK_1_0		(0)
#define	SPBCLK_1_5		(0)
#define	SPBCLK_2_0		(1)
#define	SPBCLK_2_5		(1)
#define	SPBCLK_3_0		(2)
#define	SPBCLK_3_5		(2)
#define	SPBCLK_4_0		(3)
#define	SPBCLK_4_5		(3)
#define	SPBCLK_5_0		(4)
#define	SPBCLK_5_5		(4)
#define	SPBCLK_6_0		(5)
#define	SPBCLK_6_5		(5)
#define	SPBCLK_7_0		(6)
#define	SPBCLK_7_5		(6)
#define	SPBCLK_8_0		(7)
#define	SPBCLK_8_5		(7)

/* SPBCR */
#define	SPBCR_SPBR(x)		(((u32)(x) & 0xff) << 8)
#define	SPBCR_BRDV(x)		(((u32)(x) & 0x3) << 0)

/* DRCR (read mode) */
#define	DRCR_SSLN		(1u << 24)
#define	DRCR_RBURST(x)		(((u32)(x) & 0xf) << 16)
#define	DRCR_RCF		(1u << 9)
#define	DRCR_RBE		(1u << 8)
#define	DRCR_SSLE		(1u << 0)

/* DRCMR (read mode) */
#define	DRCMR_CMD(c)		(((u32)(c) & 0xff) << 16)
#define	DRCMR_OCMD(c)		(((u32)(c) & 0xff) << 0)

/* DREAR (read mode) */
#define	DREAR_EAV(v)		(((u32)(v) & 0xff) << 16)
#define	DREAR_EAC(v)		(((u32)(v) & 0x7) << 0)

/* DROPR (read mode) */
#define	DROPR_OPD3(o)		(((u32)(o) & 0xff) << 24)
#define	DROPR_OPD2(o)		(((u32)(o) & 0xff) << 16)
#define	DROPR_OPD1(o)		(((u32)(o) & 0xff) << 8)
#define	DROPR_OPD0(o)		(((u32)(o) & 0xff) << 0)

/* DRENR (read mode) */
#define	DRENR_CDB(b)		(((u32)(b) & 0x3) << 30)
#define	DRENR_OCDB(b)		(((u32)(b) & 0x3) << 28)
#define	DRENR_ADB(b)		(((u32)(b) & 0x3) << 24)
#define	DRENR_OPDB(b)		(((u32)(b) & 0x3) << 20)
#define	DRENR_DRDB(b)		(((u32)(b) & 0x3) << 16)
#define	DRENR_DME		(1u << 15)
#define	DRENR_CDE		(1u << 14)
#define	DRENR_OCDE		(1u << 12)
#define	DRENR_ADE(a)		(((u32)(a) & 0xf) << 8)
#define	DRENR_OPDE(o)		(((u32)(o) & 0xf) << 4)

/* SMCR (spi mode) */
#define	SMCR_SSLKP		(1u << 8)
#define	SMCR_SPIRE		(1u << 2)
#define	SMCR_SPIWE		(1u << 1)
#define	SMCR_SPIE		(1u << 0)

/* SMCMR (spi mode) */
#define	SMCMR_CMD(c)		(((u32)(c) & 0xff) << 16)
#define	SMCMR_OCMD(o)		(((u32)(o) & 0xff) << 0)

/* SMADR (spi mode) */

/* SMOPR (spi mode) */
#define	SMOPR_OPD3(o)		(((u32)(o) & 0xff) << 24)
#define	SMOPR_OPD2(o)		(((u32)(o) & 0xff) << 16)
#define	SMOPR_OPD1(o)		(((u32)(o) & 0xff) << 8)
#define	SMOPR_OPD0(o)		(((u32)(o) & 0xff) << 0)

/* SMENR (spi mode) */
#define	SMENR_CDB(b)		(((u32)(b) & 0x3) << 30)
#define	SMENR_OCDB(b)		(((u32)(b) & 0x3) << 28)
#define	SMENR_ADB(b)		(((u32)(b) & 0x3) << 24)
#define	SMENR_OPDB(b)		(((u32)(b) & 0x3) << 20)
#define	SMENR_SPIDB(b)		(((u32)(b) & 0x3) << 16)
#define	SMENR_DME		(1u << 15)
#define	SMENR_CDE		(1u << 14)
#define	SMENR_OCDE		(1u << 12)
#define	SMENR_ADE(b)		(((u32)(b) & 0xf) << 8)
#define	SMENR_OPDE(b)		(((u32)(b) & 0xf) << 4)
#define	SMENR_SPIDE(b)		(((u32)(b) & 0xf) << 0)

#define	ADE_23_16		(0x4)
#define	ADE_23_8		(0x6)
#define	ADE_23_0		(0x7)
#define	ADE_31_0		(0xf)

#define	BITW_1BIT		(0)
#define	BITW_2BIT		(1)
#define	BITW_4BIT		(2)

#define	SPIDE_16BITS_DUAL	(0x8)
#define	SPIDE_32BITS_DUAL	(0xc)
#define	SPIDE_64BITS_DUAL	(0xf)
#define	SPIDE_8BITS	(0x8)
#define	SPIDE_16BITS	(0xc)
#define	SPIDE_32BITS	(0xf)

#define	OPDE_3			(0x8)
#define	OPDE_3_2		(0xc)
#define	OPDE_3_2_1		(0xe)
#define	OPDE_3_2_1_0		(0xf)

/* SMRDR0 (spi mode) */
/* SMRDR1 (spi mode) */
/* SMWDR0 (spi mode) */
/* SMWDR1 (spi mode) */

/* CMNSR (spi mode) */
#define	CMNSR_SSLF		(1u << 1)
#define	CMNSR_TEND		(1u << 0)

/* DRDMCR (read mode) */
#define	DRDMCR_DMDB(b)		(((u32)(b) & 0x3) << 16)
#define	DRDMCR_DMCYC(b)		(((u32)(b) & 0x7) << 0)

/* DRDRENR (read mode) */
#define	DRDRENR_ADDRE		(1u << 8)
#define	DRDRENR_OPDRE		(1u << 4)
#define	DRDRENR_DRDRE		(1u << 0)

/* SMDMCR (spi mode) */
#define	SMDMCR_DMDB(b)		(((u32)(b) & 0x3) << 16)
#define	SMDMCR_DMCYC(b)		(((u32)(b) & 0x7) << 0)

/* SMDRENR (spi mode) */
#define	SMDRENR_ADDRE		(1u << 8)
#define	SMDRENR_OPDRE		(1u << 4)
#define	SMDRENR_SPIDRE		(1u << 0)

#define	QSPI_BASE_CLK		(133333333)

/*
 *  FlashROM Chip Commands
 */

#define	CMD_READ_ID		(0x9f)	/* (REMS) Read Electronic Manufacturer Signature */
#define	CMD_PP			(0x02)	/* Page Program (3-byte address) */
#define	CMD_QPP			(0x32)	/* Quad Page Program (3-byte address) */
#define	CMD_READ		(0x03)	/* Read (3-byte address) */
#define	CMD_FAST_READ		(0x0b)	/* Fast Read (3-byte address) */
#define	CMD_DOR			(0x3b)	/* Read Dual Out (3-byte address) */
#define	CMD_QOR			(0x6b)	/* Read Quad Out (3-byte address) */
#define	CMD_SE			(0xd8)	/* Sector Erase */

#define	CMD_4READ		(0x13)	/* Read (4-byte address) */
#define	CMD_4FAST_READ		(0x0c)	/* Fast Read (4-byte address) */
#define	CMD_4PP			(0x12)	/* Page Program (4-byte address) */
#define	CMD_4SE			(0xdc)	/* Sector Erase */


#define SPI_FLASH_3B_ADDR_LEN           3
#define SPI_FLASH_CMD_LEN		        (1 + SPI_FLASH_3B_ADDR_LEN)
#define CONFIG_SPI_FLASH_BAR            1
#define CMD_ERASE_64K                   0xD8
#define FLASH_SHIFT                     0
#define CMD_BANKADDR_BRWR               0x17
#define SPI_FLASH_16MB_BOUN		        0x1000000
#define CONFIG_SYS_HZ                   1000
#define SPI_FLASH_PAGE_ERASE_TIMEOUT    (5 * CONFIG_SYS_HZ)
#define SPI_FLASH_PROG_TIMEOUT          (2 * CONFIG_SYS_HZ)
#define CMD_WRITE_ENABLE                0x06
#define STATUS_WIP			            (1 << 0)
#define CMD_READ_STATUS                 0x05
#define CMD_FLAG_STATUS		            0x70
#define STATUS_PEC                      (1 << 7)
#define CMD_PAGE_PROGRAM				0x02

#define QSPI0_BASE	(0x3FEFA000)
#define XIP_BASE	(0x18000000)

#define MEM_REGION_SIZE		(256)
#define XIP_REGION_SIZE		(128)

#define CS_KEEP_LOW 1
#define CS_BACK_HIGH 0

enum operations { OP_IDLE= 0, OP_READ, OP_ERASE, OP_PROG };

/* Our Private Data Structure */
struct qspi_flash {
	//spinlock_t lock;
	void * __iomem *reg;

	/* dual - Single to dual flash */
	int dual;

	u16 op;			/* operation OP_xxx */
	u32 op_addr;	/* operation address (next address) */
	int op_err;		/* operation error */

	/*
	 * disable_auto_combine
	 *
	 * This feature is useful only when you are in dual SPI flash mode
	 * and you want to send a command but not have the results from
	 * the 2 devices OR-ed together (becase you need to check each SPI
	 * Flash individually).
	 *
	 * Just remember that you need to send a buffer size big enough to handle
	 * the results from both SPI flashes.
	 *
	 * This only effects the very next command sent, after that it will
	 * automatically reset.
	 *
	 * NOTE: You will need to add a prototype of this function in your
	 * code to use it (it's not in any header file).
	 */
	int disable_combine;	// Don't combine responses from dual SPI flash

	int combine_status_mode; // Dual mode only, 0='OR results'(Spansion), 1='AND results (Micron)'

	unsigned long flags;	/* for savign IRW flags */
	u32 cmncr_save;
	u32 drcr_save;
	u8 this_cmd;
};


static inline u32 qspi_read32(struct qspi_flash *qf, u16 offset)
{
	return ioread32((u8*)qf->reg + offset);
}
static inline void qspi_write32(struct qspi_flash *qf, u32 val,
				  u16 offset)
{
	iowrite32(val, (u8*)qf->reg + offset);
}
static inline u32 qspi_read16(struct qspi_flash *qf, u16 offset)
{
	return ioread16((u8*)qf->reg + offset);
}
static inline void qspi_write16(struct qspi_flash *qf, u16 val,
				  u16 offset)
{
	iowrite16(val, (u8*)qf->reg + offset);
}
static inline u32 qspi_read8(struct qspi_flash *qf, u16 offset)
{
	return ioread8((u8*)qf->reg + offset);
}
static inline void qspi_write8(struct qspi_flash *qf, u8 val,
				  u16 offset)
{
	iowrite8(val, (u8*)qf->reg + offset);
}
