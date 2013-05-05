typedef signed char		S8;
typedef signed short	S16;
typedef signed int		S32;

typedef unsigned char	U8;
typedef unsigned short	U16;
typedef unsigned int	U32;
typedef unsigned long long	U64;

typedef unsigned char	uchar;
typedef unsigned short	ushort;
typedef unsigned long	ulong;

typedef int BOOL;

#define   TRUE    1
#define   FALSE   0

//sector means 512 byte memory
BOOL nand_phy_readpage(U32 pagenum, U8* buf);
BOOL nand_phy_writepage(U32 pagenum, U8* buf);
BOOL nand_phy_eraseblock(U32 blocknum);
BOOL nand_phy_readspare(U32 pagenum,U8 *buffer,U32 len);
BOOL nand_phy_writespare(U32 pagenum,U8* buffer,U32 len);
BOOL nand_phy_readblock(U32 blocknum,U8* buffer);
BOOL nand_phy_writeblock(U32 blocknum,U8* buffer);
BOOL nand_phy_isbadblock(U32 blocknum);

BOOL nand_set_sparedata(U32 lognum);
void nand_set_cfg(U32 cfg);
BOOL nand_init();
U32 nand_get_pagesize();
U32 nand_get_blocksize();
U32 nand_get_pageperblock();
U32 nand_get_blockcnt();
U32 nand_get_memsize_kbyte();
U32 nand_get_phy_block(U32 lognum);

BOOL nand_eraseblock(U32 logblocknum);
BOOL nand_readpage(U32 pagenum,U8* buf);
BOOL nand_writepage(U32 pagenum,U8* buf);
void nand_flushdata();

// for file-system
BOOL nand_phy_readsect(U32 sector,U8* buf);//read only 512 byte
U32 nand_get_sectorcount();//sector is 512
U32 nand_get_sectorsize();
BOOL nand_readsects(U32 sector,U8* buf,U32 cnt);
BOOL nand_updatesects(U32 sector,U8* buf,U32 cnt);


