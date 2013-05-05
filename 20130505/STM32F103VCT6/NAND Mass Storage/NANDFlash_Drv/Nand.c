#include "nandctrl.h"
#include "nand_if.h"
#include "fsmc_nand.h"


#define NAND_FLASH_START_ADDR     ((uint32_t)0x70000000) 
#define ROW_ADDRESS (Address.Page + (Address.Block + (Address.Zone * NAND_ZONE_SIZE)) * NAND_BLOCK_SIZE)


#define PARITY_OFFSET_4 36
#define PARITY_OFFSET_24 100 

// Device Code Define
#define   K9F1208U  	0x76EC
#define   K9F1G08U  	0xF1EC
#define   K9F2G08U  	0xDAEC
#define   K9F4G08U  	0xDCEC
#define   K9F8G08U  	0xD3EC
#define   K9GAG08U  	0xD5EC

#define BLOCK2PAGE(BlockNum) (BlockNum * NandInfo.pageperblock)

typedef struct _tagNand
{
	U16 id;
	U32 pagesize;
	U32 blocksize;
	U32 pageperblock;
	U32 blockcnt;
	U32 sparesize;
	U32 maxbadblockcnt;
	U32 rowaddrlen;
	U32 datastartblock; //file system area
	U32 datablockcnt;
	U8 eccbit;
}NANDTYPE;

NANDTYPE nand_db[11]= {
	{
		K9F1208U,
		512,
		512*32,
		32,
		4096,
		16,
		33,
		9,
		64,//1Mbyte
		4096-4-33,
		0,
	},
	{
		K9F1G08U,
		2048,
		(2048*64),
		64,
		1024,
		64,
		20,
		16,
		8,//1Mbyte
		1024-8-20,
		0,
	},
	{
		K9F2G08U,
		2048,
		(2048*64),
		64,
		2048,
		64,
		40,
		17,
		8,//1Mbyte
		2048-8-40,
		0,
	},
	{
		K9F4G08U,
		2048,
		(2048*64),
		64,
		4096,
		64,
		70,
		18,
		8,//1Mbyte
		4096-8-70,
		0,
	},
	{
		K9F8G08U,
		2048,
		(2048*64),
		64,
		8192,
		64,
		160,
		19,
		8,
		8192-8-160,
		0,
		},
	{
		K9GAG08U,
		(8*1024),
		(8*1024*128),
		128,
		2048,
		436,
		58,
		19,
		1,//1Mbyte
	    2048-1-58,
		24,
	},
	{0,},
};

static NANDTYPE NandInfo=
{
	K9F1208U,
	512,
	512*32,
	32,
	4096-4,//-4(for copyback)
	16,
	33,
	9,
	64//-4(for copyback)
};


static void WaitBusy()
{
	// Check Busy End
	//while ( !(*(volatile U8*)R_NFMSTAT & 0x01) );
	//while ( !(*(volatile U8*)R_NFMSTAT & 0x02) ); // for "trr" timing,see Nand-Flash data sheet
	//int i;
	//for( i=0;i<1;i++);
	//FSMC_NAND_GetStatus();
	while( GPIO_ReadInputDataBit(GPIOG, GPIO_Pin_6) == 0 );
}

void nand_reset()
{
	//*(volatile char *)R_NFMCMD  = NAND_COMM_RESET ;  // Read 
    *(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_RESET;

	WaitBusy();
}

U16 nand_id_read(void)
{
  U32 id;
  nand_reset();

  //U32 id = *R_NFMID;

  /* Send Command to the command area */ 	
  *(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = 0x90;
  *(vu8 *)(NAND_FLASH_START_ADDR | ADDR_AREA) = 0x00;
  
  /* Sequence to read ID from NAND flash */	
  id = *(vu32 *)(NAND_FLASH_START_ADDR | DATA_AREA);
  
  return (U16)id;
}


static void setpageaddress(U32 pagenum,U32 offset) {
  int i;
  int k;
  U8 mask;
  U8 validbit;
  
  //*(volatile U8 *)R_NFMADDR  = offset&0xff ;  // A0-A7
  *(vu8 *)(NAND_FLASH_START_ADDR | ADDR_AREA) = offset&0xff ;  // A0-A7
  
  if(NandInfo.pagesize!=512)
          //*(volatile U8 *)R_NFMADDR  = offset>>8 ;  
  *(vu8 *)(NAND_FLASH_START_ADDR | ADDR_AREA) = offset>>8 ;
  
  for(i=0;i<NandInfo.rowaddrlen;i+=8) {
    mask=0;
    validbit = (NandInfo.rowaddrlen-i)>8?8:(NandInfo.rowaddrlen-i);
    for(k=0;k<validbit;k++)
            mask |= (1<<k);
  
    //*(volatile U8 *)R_NFMADDR  = (U8)(pagenum  & mask) ;  // Row Address
    *(vu8 *)(NAND_FLASH_START_ADDR | ADDR_AREA) = (U8)(pagenum  & mask) ;  // Row Address
            pagenum = pagenum>>8;
  }
}

static void seteraseblockaddr(U32 blockaddr)
{
	int i;
	int k;
	U8 mask;
	U8 validbit;
	for(i=0;i<NandInfo.rowaddrlen;i+=8)
	{
		mask=0;
		validbit = (NandInfo.rowaddrlen-i)>8?8:(NandInfo.rowaddrlen-i);
		for(k=0;k<validbit;k++)
			mask |= (1<<k);
		//*(volatile U8 *)R_NFMADDR  = (U8)(blockaddr  & mask) ;  // Row Address
		*(vu8 *)(NAND_FLASH_START_ADDR | ADDR_AREA) = (U8)(blockaddr  & mask) ;  // Row Address
		blockaddr = blockaddr>>8;
	}
}

static void setspareaddr(U32 pagenum)
{
	int i;
	int k;

	if(NandInfo.pagesize==512)
		setpageaddress(pagenum,0);
	else
	{
		U8 mask;
		U8 validbit;
		//*(volatile U8 *)R_NFMADDR  = 0 ; 
		*(vu8 *)(NAND_FLASH_START_ADDR | ADDR_AREA) = 0; 

		if(NandInfo.pagesize!=512)
			//*(volatile U8 *)R_NFMADDR  =  NandInfo.pagesize>>8; 
			*(vu8 *)(NAND_FLASH_START_ADDR | ADDR_AREA) = NandInfo.pagesize>>8;

		for(i=0;i<NandInfo.rowaddrlen;i+=8)
		{
			mask=0;
			validbit = (NandInfo.rowaddrlen-i)>8?8:(NandInfo.rowaddrlen-i);
			for(k=0;k<validbit;k++)	mask |= (1<<k);

			//*(volatile U8 *)R_NFMADDR  = (U8)(pagenum  & mask) ;  // Row Address
            *(vu8 *)(NAND_FLASH_START_ADDR | ADDR_AREA) = (U8)(pagenum  & mask) ;  // Row Address
			pagenum = pagenum>>8;
		}
	}
}


BOOL nand_phy_readpage(U32 pageaddr, U8* buffer)
{
	U32 i;

	U8* buf = (U8*)buffer;
  
		//*(volatile char *)R_NFMCMD  = NAND_COMM_READ1 ;  // Read 
        *(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_READ_1; 

		setpageaddress(pageaddr,0);

		if(NandInfo.pagesize != 512)
			//*(volatile char *)R_NFMCMD  = NAND_COMM_READ2 ;  // Read 
			*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_READ_TRUE;

		
		WaitBusy();

		for(i=0;i<NandInfo.pagesize;)
		{
			buf[i]   = *(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA);
			buf[i+1] = *(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA);
			buf[i+2] = *(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA);
			buf[i+3] = *(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA);
			i+=4;
		}

		//while ( !(*(volatile U8*)R_NFMSTAT & 0x02) ); // for "trr" timing,see Nand-Flash data sheet
		FSMC_NAND_GetStatus();
		return TRUE;

}
//read only 512byte for FAT
BOOL nand_phy_readsect(U32 sector, U8* buffer) {
	int i;
	U8* buf = (U8*)buffer;
	U32 sectsize;
	U32 pagenum;
	U32 offset;
	U32 secperpage = NandInfo.pagesize/sectsize;
	sectsize = nand_get_sectorsize();

	if(NandInfo.pagesize==sectsize)
		return nand_phy_readpage(sector,buf);
	else
	{
		pagenum = sector/secperpage;
		offset = (sector%secperpage)*sectsize;

		//g_dummy = *R_NFMSTAT; //clear busy
		//*(volatile char *)R_NFMCMD  = NAND_COMM_READ1 ;  // Read 
		*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_READ_1;
		setpageaddress(pagenum,offset);

		//*(volatile char *)R_NFMCMD  = NAND_COMM_READ2 ;  // Read 
		*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_READ_TRUE;
		// Check Busy End
		WaitBusy();


		for(i=0;i<sectsize;)
		{
			buf[i]   = *(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA);
			buf[i+1] = *(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA);
			buf[i+2] = *(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA);
			buf[i+3] = *(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA);
			i+=4;
		}

		//while ( !(*(volatile U8*)R_NFMSTAT & 0x02) ); // for "trr" timing,see Nand-Flash data sheet
		FSMC_NAND_GetStatus();
		return TRUE;
	}

}

BOOL nand_phy_readspare(U32 pageaddr,U8 *buffer,U32 len)
{
	int i;
	char* buf;
	//g_dummy = *R_NFMSTAT; //clear busy
	buf = (char*)buffer;
	if(NandInfo.pagesize == 512)
	{
		//*(volatile U8 *)R_NFMCMD  = NAND_COMM_SPAREREAD ;  // Read 
		*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_COMM_SPAREREAD;
		setspareaddr(pageaddr);
	}
	else
	{
		//*(volatile U8 *)R_NFMCMD  = NAND_COMM_READ1 ; 
		*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_READ_1;
		setspareaddr(pageaddr);
		//*(volatile U8 *)R_NFMCMD  = NAND_COMM_READ2 ; 
		*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_READ_TRUE;
	}

	WaitBusy();

	if(len > NandInfo.sparesize) len = NandInfo.sparesize;

	for(i=0;i<len;i++)
		//buf[i] = *(volatile U8*)R_NFMDATA;
	   buf[i] = *(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA);

	
	//while ( !(*(volatile U8*)R_NFMSTAT & 0x02) ); // for "trr" timing,see Nand-Flash data sheet
	FSMC_NAND_GetStatus();
	return TRUE;
}

BOOL nand_phy_readblock(U32 blocknum,U8* buffer)
{
	int i;
	U32 startpage = BLOCK2PAGE(blocknum);
	for(i=0;i<NandInfo.pageperblock;i++)
	{
		nand_phy_readpage(startpage,buffer);
		startpage++;
		buffer += NandInfo.pagesize;
	}	
	return TRUE;
}

BOOL nand_phy_writepage(U32 pageaddr, U8* buffer)
{
	int i;
	U8* buf = (U8*)buffer;

    //g_dummy = *R_NFMSTAT; //clear busy
	if(NandInfo.pagesize == 512)
		//*(volatile U8*)R_NFMCMD  = 0x0 ; 
		*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = 0;

	//*(volatile U8*)R_NFMCMD  = NAND_COMM_PPRO1 ; 
	*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_PAGEPROGRAM;

	setpageaddress(pageaddr,0);


	for(i=0;i<NandInfo.pagesize;)
	{
		*(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA) = buf[i];
		*(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA) = buf[i+1];
		*(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA) = buf[i+2];
		*(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA) = buf[i+3];
		i+=4;
	}
	
	//*(volatile U8*)R_NFMCMD = NAND_COMM_PPRO2 ; // Confirm
	*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_PAGEPROGRAM_TRUE;
	WaitBusy();

	//*(volatile U8*)R_NFMCMD  = NAND_STATUS ; // Command
	//if(*(volatile U8*)R_NFMDATA & 1)
	//	return FALSE;
	FSMC_NAND_GetStatus();
	return TRUE;	
}

BOOL nand_phy_writespare(U32 pageaddr,U8* buffer,U32 len)
{
	int i;
	U8* buf = (U8*)buffer;
    //g_dummy = *R_NFMSTAT; //clear busy

	if(NandInfo.pagesize == 512)
	{
		//*(volatile U8 *)R_NFMCMD  = NAND_COMM_SPAREREAD ;  
		//*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_COMM_SPAREREAD;
		*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = 0x00;
		//*(volatile U8*)R_NFMCMD  = NAND_COMM_PPRO1 ; 
		*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_PAGEPROGRAM;
		setspareaddr(pageaddr);
	}
	else
	{
		//*(volatile U8 *)R_NFMCMD  = NAND_COMM_PPRO1 ;  
		*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_PAGEPROGRAM;
		setspareaddr(pageaddr);
	}

	if(len > NandInfo.sparesize) len = NandInfo.sparesize;
	for(i=0;i<len;i++)
	{
		*(vu8 *)(NAND_FLASH_START_ADDR | DATA_AREA) = buf[i];
	}
	//*(volatile U8*)R_NFMCMD = NAND_COMM_PPRO2 ; // Confirm
	*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_PAGEPROGRAM_TRUE;

	// Check Busy End
	WaitBusy();

	//*(volatile U8*)R_NFMCMD  = NAND_STATUS ; // Command
	//if(*(volatile U8*)R_NFMDATA & 1)
	//	return FALSE;
	FSMC_NAND_GetStatus();
	return TRUE;	
}

BOOL nand_phy_writeblock(U32 blocknum,U8* buffer)
{
	int i;
	U32 startpage = BLOCK2PAGE(blocknum);
	U8* buf = (U8*)buffer;
	for(i=0;i<NandInfo.pageperblock;i++)
	{
		if(nand_phy_writepage(startpage+i,buf)==FALSE)
			return FALSE;
		buf += NandInfo.pagesize;
	}
	return TRUE;
}
BOOL nand_phy_eraseblock(U32 blocknum)
{
	U32 pagenum = BLOCK2PAGE(blocknum);
	//g_dummy = *R_NFMSTAT; //clear busy
	//*(volatile char *)R_NFMCMD  = NAND_COMM_BERS1 ; 
	*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_ERASE0;
	seteraseblockaddr(pagenum);
	//*(volatile U8*)R_NFMCMD  = NAND_COMM_BERS2 ; // Command
	*(vu8 *)(NAND_FLASH_START_ADDR | CMD_AREA) = NAND_CMD_ERASE1;	
	// Check Busy End
	WaitBusy();

	//*(volatile U8*)R_NFMCMD  = NAND_STATUS ; // Command
	//if(*(volatile U8*)R_NFMDATA & 1)
	//	return FALSE;//bad?
	FSMC_NAND_GetStatus();
	return TRUE;
}



#define BI_GOOD	0xff //not used block
#define BI_USED	0x7f //allocated block
#define BI_NOTUSED	0x0f //need for erase to check bad?
#define BI_BAD 0x00 //can't use

#define MAX_BLOCK_CNT (8192)
typedef struct _tagSpare
{
	U8 LSN[3];
	U8 paritybit;
	U8 reserve; // reserve[0] is used parity
	U8 BadInfo;
	U8 Ecc[3]; //not used
	U8 SEcc[2]; //not used
	U8 reserve2[5];

}SPAREDATA; //small page


typedef struct _tagSpare2
{
	U8 BadInfo;
	U8 paritybit;// reserve is used parity
	U8 LSN[3]; // Logical Number
	U8 reserve2[3];
	U8 Ecc[3];//not used
	U8 SEcc[2];//not used
	U8 reserve3[3];
}SPAREDATA2;//large page

static U8 BlockInfoTable[MAX_BLOCK_CNT];
static U16 PhyBlockTable[MAX_BLOCK_CNT];

static BOOL nand_make_bbt()
{
	static int badclockcnt=0;
	int i;
	int pageperblock = NandInfo.pageperblock;
	U32 logicalBlocknum;
	U8 sparedatabuf[16];
	U8 sparedatabuf2[16];
	U8 paritybit = 0;
	int k;

	memset(PhyBlockTable,0xff,MAX_BLOCK_CNT*2);

	for(i=NandInfo.datastartblock;i<NandInfo.blockcnt;i++)
	{
	    //	PRINTVAR(i);
		nand_phy_readspare(i*pageperblock,sparedatabuf,16);
		if(NandInfo.eccbit==24)
			nand_phy_readspare(i*pageperblock+pageperblock-1,sparedatabuf2,16);
		else
			nand_phy_readspare(i*pageperblock+1,sparedatabuf2,16);

		if(NandInfo.pagesize==512)
		{
			SPAREDATA* sparedata = (SPAREDATA*)sparedatabuf;
			SPAREDATA* sparedata2 = (SPAREDATA*)sparedatabuf2;
			if( (sparedata->BadInfo != 0xff) || (sparedata2->BadInfo != 0xff) )
			{
				BlockInfoTable[i]=BI_BAD;	
				//debugprintf("%dblock is bad block\r\n",i);
				badclockcnt++;
				continue;
			}
			//make logical block number
			logicalBlocknum = (U32)sparedata->LSN[0] + ((U32)sparedata->LSN[1]<<8) + ((U32)sparedata->LSN[2]<<16);
			if(logicalBlocknum==0xffffff)
			{
				BlockInfoTable[i]=BI_GOOD;
			}
			else
			{
				U8 paritybit = 0;
				int k;
				//check parity bit
				for(k=0;k<24;k++)
				{
					if(logicalBlocknum & (1<<k))
					{
						if(paritybit)
							paritybit = 0;
						else
							paritybit = 1;
					}
				}
				if(sparedata->paritybit != paritybit)
				{
					BlockInfoTable[i]=BI_NOTUSED;					
				}
				else
				{
					PhyBlockTable[logicalBlocknum]=i;			
					BlockInfoTable[i]=BI_USED;
					//NAND_DEBUGPRINTF("P-%d : L-%d\r\n",i,logicalBlocknum);
				}
			}
		}
		else //large block
		{
			SPAREDATA2* sparedata = (SPAREDATA2*)sparedatabuf;
			SPAREDATA2* sparedata2 = (SPAREDATA2*)sparedatabuf2;
			if( (sparedata->BadInfo != 0xff) || (sparedata2->BadInfo != 0xff) )
			{
				BlockInfoTable[i]=BI_BAD;
				//debugprintf("%d is bad block\r\n",i);//
				//PRINTVAR(sparedata->BadInfo);
				//PRINTVAR(sparedata2->BadInfo);
				badclockcnt++;
				continue;
			}
			//make logical block number
			logicalBlocknum = (U32)sparedata->LSN[0] + ((U32)sparedata->LSN[1]<<8) + ((U32)sparedata->LSN[2]<<16);
			if(logicalBlocknum==0xffffff)
			{
				BlockInfoTable[i]=BI_GOOD;
			}
			else
			{
				//check parity bit
				for(k=0;k<24;k++)
				{
					if(logicalBlocknum & (1<<k))
					{
						if(paritybit)
							paritybit = 0;
						else
							paritybit = 1;
					}
				}
				if(sparedata->paritybit != paritybit)
				{
					BlockInfoTable[i]=BI_NOTUSED;					
					//debugprintf("%d block paritybit error\r\n",i);
				}
				else
				{
					PhyBlockTable[logicalBlocknum]=i;			
					BlockInfoTable[i]=BI_USED;
					//NAND_DEBUGPRINTF("P-%d : L-%d\r\n",i,logicalBlocknum);
				}
			}
		}
	}
	//debugprintf("make bad block inforamation done(bad-block(%d))\r\n",badclockcnt);
	return TRUE;
}
/*
 * find empty block and erase it
 **/
static int nand_get_phy_freeblock(U32 logicalblocknum)
{
	int i;
	int k=logicalblocknum;
	for(i=NandInfo.datastartblock;i<NandInfo.blockcnt;i++)
	{
		if(BlockInfoTable[k]==BI_GOOD)
		{
			//if(nand_phy_eraseblock(k)==TRUE)
				return k;
		}
		else if(BlockInfoTable[i]==BI_NOTUSED)
		{
			if(nand_phy_eraseblock(k)==TRUE)
			{
				BlockInfoTable[i] = BI_GOOD;
				return k;
			}
			BlockInfoTable[k]=BI_BAD;
		}
		k++;
		if(k>=NandInfo.blockcnt) k=NandInfo.datastartblock;
	}
	return -1;
}

static BOOL nand_allocate_block(U32 phynum,U32 lognum)
{
	int k;
	U32 pagenum;
	U16 oldphynum;
	U8 paritybit;
	U8 sparebuf[16]={0,};

	if(phynum > NandInfo.blockcnt || lognum > NandInfo.blockcnt)
		return FALSE;

	if(NandInfo.pagesize == 512)
	{
		SPAREDATA* sparedata = (SPAREDATA*)sparebuf;
		sparedata->BadInfo=0xff;
		sparedata->LSN[0] = lognum;
		sparedata->LSN[1] = lognum>>8;
		sparedata->LSN[2] = lognum>>16;
		paritybit = 0;
		//check parity bit
		for(k=0;k<24;k++)
		{
			if(lognum & (1<<k))
			{
				if(paritybit)
					paritybit = 0;
				else
					paritybit = 1;
			}
		}
		sparedata->paritybit = paritybit;
	}
	else//2048
	{
		SPAREDATA2* sparedata = (SPAREDATA2*)sparebuf;
		sparedata->BadInfo=0xff;
		sparedata->LSN[0] = lognum;
		sparedata->LSN[1] = lognum>>8;
		sparedata->LSN[2] = lognum>>16;
		paritybit = 0;
		//int k;
		//check parity bit
		for(k=0;k<24;k++)
		{
			if(lognum & (1<<k))
			{
				if(paritybit)
					paritybit = 0;
				else
					paritybit = 1;
			}
		}
		sparedata->paritybit = paritybit;
	}

	pagenum = phynum*NandInfo.pageperblock;
	if(nand_phy_writespare(pagenum,sparebuf,16) == FALSE)
	{
		//debugprintf("pagenum:%d writespare error\r\n",pagenum);
		return FALSE;
	}

	oldphynum = PhyBlockTable[lognum];
	if(oldphynum !=phynum && oldphynum <NandInfo.blockcnt)
	{
		BlockInfoTable[oldphynum]=BI_NOTUSED;
		//nand_phy_eraseblock(oldphynum);
	}
	BlockInfoTable[phynum]=BI_USED;
	PhyBlockTable[lognum]=phynum;

	return TRUE;
}

U32 nand_get_phy_block(U32 lognum)
{
	U32 phyblocknum;
	phyblocknum = PhyBlockTable[lognum];

	if(phyblocknum == 0xffff) //not allocate
	{
		phyblocknum = nand_get_phy_freeblock(lognum);
		if(phyblocknum ==-1)
		{
			//NAND_DEBUGPRINTF("failed to get free block\r\n");
			return -1;
		}
		//NAND_DEBUGPRINTF("new block allocated P-%d:L-%d\r\n",phyblocknum,lognum);
		nand_allocate_block(phyblocknum,lognum);
	}
	return phyblocknum;
}

BOOL nand_set_sparedata(U32 lognum)
{
	U32 phynum;
	phynum = nand_get_phy_block(lognum);
	return nand_allocate_block(phynum,lognum);
}

U32 nand_get_pagesize()
{
	return NandInfo.pagesize;
}

U32 nand_get_blocksize()
{
	return NandInfo.blocksize;
}
U32 nand_get_pageperblock()
{
	return NandInfo.blocksize/NandInfo.pagesize;
}
U32 nand_get_blockcnt()
{
	return NandInfo.blockcnt;
}
U32 nand_get_memsize_kbyte()
{
	return NandInfo.blocksize/1024 * NandInfo.datablockcnt;
}

U32 nand_get_sectorcount()
{
	return (NandInfo.blocksize * NandInfo.datablockcnt)/nand_get_sectorsize();
}


// Flash Translation Layer(FTL)

U32 nand_get_phy_page(U32 pagenum)
{
	U32 blocknum = pagenum / NandInfo.pageperblock;
	//blocknum = nand_get_phy_block(blocknum);
	return (blocknum*NandInfo.pageperblock + (pagenum % NandInfo.pageperblock));
}

BOOL nand_eraseblock(U32 blocknum)
{
	U32 phyblocknum;
	phyblocknum = nand_get_phy_block(blocknum);

	if(nand_phy_eraseblock(phyblocknum)==FALSE)
		return FALSE;
	nand_set_sparedata(blocknum);
	return TRUE;
}

BOOL nand_writepage(U32 pagenum,U8* buf)
{
	pagenum = nand_get_phy_page(pagenum);
	return nand_phy_writepage(pagenum,buf);
}

BOOL nand_readpage(U32 pagenum,U8* buf)
{
	pagenum = nand_get_phy_page(pagenum);
	return nand_phy_readpage(pagenum,buf);
}

U32 nand_get_sectorsize()
{
	//return 512;
	return 2048;
}

typedef struct
{
	int blocknum;
	BOOL dirty; // if true, data is updated so it should be written at storage
	//U8* buf;
	U8 buf[2048];
}BLOCKBUF;
BLOCKBUF blockbuf;
/*
{
.blocknum=-1,
.dirty=0,
.buf={0,}
};
*/
static void init_blockbuf()
{
	blockbuf.blocknum=-1;
	blockbuf.dirty=FALSE;
	//blockbuf.buf = malloc(nand_get_blocksize());
}

static void flushblockbuf()
{
	int i;
	U32 pagesize;
	U32 pageperblock;
	U32 startpage;

	if(blockbuf.dirty==0)
		return;

	pagesize = nand_get_pagesize();
	pageperblock = nand_get_pageperblock();
	startpage = blockbuf.blocknum*pageperblock;

	nand_eraseblock(blockbuf.blocknum);
	for(i=0;i<pageperblock;i++)
	{
		nand_writepage(startpage+i,blockbuf.buf+(i*pagesize));
	}
	blockbuf.dirty=0;	
}

static void fillblockbuf(U32 blocknum)
{
	nand_phy_readblock(nand_get_phy_block(blocknum),blockbuf.buf);
	blockbuf.blocknum=blocknum;
}

void nand_flushdata()
{
	flushblockbuf();
}

static BOOL nand_updatesect(U32 sector,U8* buffer)
{
	U8* buf = (U8*)buffer;
	U32 pagesize;
	int sectorsize;
	U32 pagenum;
	U32 nandblocknum;
	U8* pbuf;

	pagesize = nand_get_pagesize();
	sectorsize=nand_get_sectorsize();
	pagenum = sector*sectorsize/pagesize;
	nandblocknum = pagenum/nand_get_pageperblock();


	if(blockbuf.blocknum!=nandblocknum)
	{
		flushblockbuf();
		fillblockbuf(nandblocknum);
	}
	pbuf = blockbuf.buf+((sector*sectorsize)%nand_get_blocksize());
	memcpy(pbuf,buf,sectorsize);	
	blockbuf.dirty=1;
	return TRUE;
}


BOOL nand_updatesects(U32 startsector,U8* buf,U32 cnt)
{
	int i;
	int sectorsize;
	U32 fatsector;

	sectorsize=nand_get_sectorsize();
	fatsector = NandInfo.datastartblock*nand_get_pageperblock();

	fatsector *=nand_get_pagesize()/sectorsize;
	startsector+=fatsector;
	for(i=0;i<cnt;i++)
	{
		if(nand_updatesect(startsector+i,(U8*)(((U32)buf)+(i*sectorsize)))==FALSE)
			return FALSE;
	}
	return TRUE;
}
static BOOL nand_readsect(U32 sector,U8* buffer) {
  U8* buf = (U8*)buffer;
  U32 sectsize;
  U32 pagesize;
  U32 nandblocknum;
  U32 pagenum;
  BOOL re;
  U32 offset;
  
  pagenum = sector*sectsize/pagesize;
  
  sectsize = nand_get_sectorsize();
  pagesize = nand_get_pagesize();
  nandblocknum = pagenum/nand_get_pageperblock();
  
  
  //if(blockbuf.blocknum == nandblocknum)
  //{
  //	offset = (sector*sectsize)%nand_get_blocksize();
  //	memcpy(buf,blockbuf.buf+offset,sectsize);
  //	return TRUE;
  //}
  //else
  //{
  
  if(NandInfo.pagesize==sectsize) {
    sector = nand_get_phy_page(sector);
  } else {
    pagenum = nand_get_phy_page(pagenum);
    sector = (pagenum*(pagesize/sectsize))+(sector%(pagesize/sectsize));
  }
  re = nand_phy_readsect(sector,buf);
  //printmem(buf,sectsize);
  return re;
  //}
}


BOOL nand_readsects(U32 startsector,U8* buf,U32 cnt)
{
	int i;
	U32 fatsector;
	U32 sectsize;

	fatsector = NandInfo.datastartblock*nand_get_pageperblock();
	sectsize = nand_get_sectorsize();

	fatsector *=nand_get_pagesize()/sectsize;
	startsector+=fatsector;
	for(i=0;i<cnt;i++)
	{
		if(nand_readsect(startsector+i,buf+(i*sectsize))==FALSE)
			return FALSE;
	}
	return TRUE;
}


static U32 nandcfg=0x2222;
void nand_set_cfg(U32 cfg)
{
	nandcfg = cfg;
}

void nand_eraseall()
{
	int i;
	for(i=0;i<NandInfo.blockcnt;i++)
	{
		if(nand_phy_eraseblock(i)==FALSE)
		{
			//debugprintf("%d block erase error\r\n",i);
		}
	}
}

BOOL nand_init() {
  int i=0;
  U16 id;
  BOOL re;

  //if(get_ahb_clock()>40000000)
  //	*R_NFMCFG = nandcfg|(1<<20);
  //else
  //	*R_NFMCFG = nandcfg;

  NAND_Init();

  id = nand_id_read();

  while(1)
  {
    if(id == nand_db[i].id)
    {
            memcpy(&NandInfo,&nand_db[i],sizeof(NandInfo));
            //NandInfo = nand_db[i];
            break;
    }
    i++;
    
    if(nand_db[i].id==0)
    {
            //debugprintf("nand id(%x) is not supported\r\n",id);
            return FALSE;
    }
  }

  printf("nand id(%x) is \r\n",id);

  //if(NandInfo.eccbit==24)
  //{
  //	*R_NFMCON = 0;//24bit ecc mode
  //}
  //else
  //	*R_NFMCON =(1<<15);

  NandInfo.datablockcnt = NandInfo.blockcnt-NandInfo.datastartblock-NandInfo.maxbadblockcnt;
  
  printf("Nand Flash Memory Info:%dMbyte\r\n",(NandInfo.blocksize/1024)*NandInfo.blockcnt/1024);

  init_blockbuf();
  //nand_eraseall();
  re = nand_make_bbt();
  return re;
}

