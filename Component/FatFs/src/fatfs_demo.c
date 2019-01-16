
#include "fatfs_demo.h"
#include "stm32f10x.h"
#include "GlobalDef.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "ff.h"
#include "spi_flash_interface.h"
#include "os_global.h"

#if GIZWITS_TYPE
#include "gizwits_port.h"
extern char * PRODUCT_KEY_STRING(void);
extern char *PRODUCT_SECRET_STRING(void);
#endif

#if FAT_DEBUG_EN
#define fat_printf(fmt, ...)  printf(fmt, ##__VA_ARGS__)
#else
#define fat_printf(...)
#endif


/* Private variables ---------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
FATFS FlashDiskFatFs;         /* Work area (file system object) for logical drive */
char FlashDiskPath[3] = "0:";
char UpdateDirPath[16] = "0:/update";   // 升级文件路径
static char FileOutPath[64];

#if _USE_LFN
char Lfname[_MAX_LFN + 1];
#endif

FIL   fileFIL;


#include <string.h>


//得到磁盘剩余容量
//drv:磁盘编号("0:"/"1:")
//total:总容量	 （单位KB）
//free:剩余容量	 （单位KB）
//返回值:0,正常.其他,错误代码
FRESULT exf_getfree(uint8_t * drv, uint32_t * total, uint32_t *free)
{
	FATFS * fs1;
	FRESULT res;
    uint32_t fre_clust = 0, fre_sect = 0, tot_sect = 0;
	
    //得到磁盘信息及空闲簇数量
    res = f_getfree((const TCHAR*)drv, (DWORD*)&fre_clust, &fs1);
    if(res == FR_OK)
	{											   
	    tot_sect = (fs1->n_fatent - 2) * fs1->csize;	//得到总扇区数
	    fre_sect = fre_clust * fs1->csize;			//得到空闲扇区数	   
//#if _MAX_SS != 512				  			  
//		tot_sect* = fs1->ssize / 512;
//		fre_sect* = fs1->ssize / 512;
//#endif	  
        #if (SPI_FLASH_SECTOR_SIZE == 4096L)
		*total = tot_sect  << 2;	// x4, 单位为KB, 扇区大小为 4096 B
		*free  = fre_sect  << 2;	// x4, 单位为KB 
		#elif (SPI_FLASH_SECTOR_SIZE == 512L)
        *total = tot_sect  / 2;	//单位为KB, 这里的FatFs扇区大小为 512 B
		*free  = fre_sect  / 2;	//单位为KB 
        #else
		#error "ext_getfree error"
		#endif
 	}
	else
	{
	    fat_printf("FatFs error = %d\n", res);
	}
	return res;
}	

// 文件的第0行显示传感器名称字符串
const char SensorNameItem[] = "Number	  Tag   Timestamp	     HCHO	  PM2.5  Temp  Humidity\r\n";

// 文件的第1行显示传感器数据单位
const char SensorUnitItem[] = "0-65535  tag   Y-Mon-Day  HH:MM:SS  mg/m3  ug/m3  'C\r\n";

// 打开指定文件
E_RESULT FILE_Open(FIL * pFileFIL, char * file_name)
{
   FRESULT res;
   char sensor_dir[64] = "0:/sensor";        // 传感器的目录
   int len = 0;
   DIR dirs;
   uint8_t is_first_create = 0;  // 是否第一次创建
   //DWORD fixed_len = os_strlen(SensorUnitItem)  + os_strlen(SensorNameItem);
   
   // 判断目录是否存在, 不存在则创建
   res = f_opendir(&dirs, (const TCHAR *)sensor_dir);  // 打开所在目录
   fat_printf("open dir %s %s\n", sensor_dir, (res == FR_OK ? "ok" : "failed"));
   if(res == FR_NO_PATH)  
   {
       // 创建该目录
       res = f_mkdir((const TCHAR *)sensor_dir);
	   if(res != FR_OK){ fat_printf("create dir %s failed, res = %d\n", sensor_dir, res); return APP_FAILED; }
	   else { fat_printf("create dir %s success\n", sensor_dir); }
	   is_first_create = E_TRUE;
   }
   if(res){ INSERT_ERROR_INFO(res); return APP_FAILED; }
   f_closedir(&dirs);
   
   // 打开文件获取文件指针
   len = os_strlen(sensor_dir);
   sensor_dir[len] = '/';
   os_strncpy(&sensor_dir[len + 1], file_name, sizeof(sensor_dir));

   res = f_open(pFileFIL, (const TCHAR * )sensor_dir, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);  // 打开文件, 可写
   //if(is_first_create || (pFileFIL->fsize < fixed_len))
    if(is_first_create)
   {
      UINT bw = 0;

      fat_printf("write sensor header: fsize = %d, is_first_create = %d \r\n",   pFileFIL->fsize,  is_first_create);
      //pFileFIL->fsize = 0;
      // 头两行提示单位
      res = f_write(pFileFIL, SensorNameItem, os_strlen(SensorNameItem), &bw);
      res = f_write(pFileFIL, SensorUnitItem, os_strlen(SensorUnitItem), &bw);
   }
   f_sync(pFileFIL);  // 最小化文件作用域, 同步文件数据, 防止被破坏
   fat_printf("f_open %s, fsize = %d\n", (res == FR_OK ? "ok" : "failed"), pFileFIL->fsize);
   if(res){ INSERT_ERROR_INFO(res); return APP_FAILED;  }

   return APP_SUCCESS;
}

/*
功能: 在字符流中查找匹配条件的字符并将其转换为数字
参数: char * buf: 待查找的字符串
             char * str: 匹配的子字符串
             uint16_t strLen: 子字符串的长度
             uint16_t min: 转换为数字, 数字的最小值
             uint16_t max: 转换为数字后的最大值
             char **p: 符合匹配条件的字符串的起始指针的指针
             uint16_t * num: 成功转换为数字的指针
返回值: 转换成功返回 APP_SUCCESS; 失败返回 APP_FAILED
*/
E_RESULT StringToInt(char * buf, char * str, uint16_t strLen, 
                                                         uint16_t min, uint16_t max, char ** p, uint16_t * num)
{
     char * p1 = NULL;
	 char * pEnd = NULL;
	 uint16_t len = 0;
	 uint16_t n = 0;

	 *num = 0;
     p1 = os_strstr((const char *)buf, str);  // 查找匹配的子字符串
     if(p1)
     {
        p1 += strLen;
        len = os_strlen(p1);
		if(len)
		{
		    n = strtol(p1, &pEnd, 10);
			if(0 == n && pEnd == (char *)p1)
			{
			   fat_printf("err:%s, %d, n=%d, 0x%x, 0x%x\n", __FILE__, __LINE__, n, (uint32_t)pEnd, (uint32_t)p1);
			   goto Exit;
			}
			if(min <= n && n <= max)
			{
			   *p   = p1;
			   *num = n;
			   fat_printf("n = %d\n", n);
			   return APP_SUCCESS;
			}
			else { goto Exit; }
		}
     }
	 else { fat_printf("p1 null: %s, %d\n", __FILE__, __LINE__); }
	 
Exit:
	*p = NULL;
	return APP_FAILED;
}

/*
功能: 在字符流中查找匹配条件的字符串
参数: char * buf: 待查找的字符串
             char * str: 匹配的子字符串
             char **p: 符合匹配条件的字符串的起始指针的指针
             uint16_t * matchStrLen: 目的字符串长度
返回值: 转换成功返回 APP_SUCCESS; 失败返回 APP_FAILED
*/
E_RESULT StringToString(char * buf, char * str, char ** p, uint16_t * matchStrLen)
{
        char * p1 = NULL;
	 char * pEnd = NULL;
	 uint16_t len = 0;

	 *matchStrLen = 0;
        p1 = os_strstr((const char *)buf, str);  // 查找匹配的子字符串
        if(p1)
       {
              p1 += os_strlen(str);
              len = os_strlen(p1);
	       if(len)
		{
		       pEnd = os_strchr(p1, '\"');
		       *matchStrLen = pEnd - p1;
		       *p = p1;
		       return APP_SUCCESS;
		}
        }
	 else 
	 { fat_printf("p1 null: %s, %d\n", __FILE__, __LINE__); }
	 
//Exit:
	*p = NULL;
	return APP_FAILED;
}

extern uint16_t record_gap;


#include "RTCDrv.h"
// 读取时间配置
void FILE_ReadConfig(void)
{
   char sensor_dir[32] = "0:/config.txt";        // 根目录
   FRESULT res;
   
   // 判断目录是否存在, 不存在则创建
   res = f_open(&fileFIL, (const TCHAR * )sensor_dir, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);  // 打开文件, 可写
   f_sync(&fileFIL);  // 最小化文件作用域, 同步文件数据, 防止被破坏
   fat_printf("open file %s %s\n", sensor_dir, (res == FR_OK ? "ok" : "failed"));
   if(res == FR_OK)
   {
        char buf[512];
	  uint32_t bytes_to_read = 0;
	  char * p = NULL;
	  uint16_t n;
	  char * p_action = NULL;
	  uint8_t action = 0;
	  
      os_memset(buf, 0, sizeof(buf));
	  res = f_read(&fileFIL, buf, sizeof(buf), (UINT *)&bytes_to_read);  
	  if(res == FR_OK)
	  {
	     fat_printf("buf = %s\n", buf);

         // 读取记录间隔
         if(! StringToInt(buf, "gap=", 4, 0, 600, &p, &n))
         {
            if(n)
            {
               record_gap = n;   // 改变记录间隔
            }
         }

#if GIZWITS_TYPE
	   if(! StringToString(buf,  "product_key=\"", &p, &n))
	   {
	             char str_buf[64];
	             
	            if(n < sizeof(str_buf))
	            {
	                  os_memset(str_buf, 0, sizeof(str_buf));
   	                  os_memcpy(str_buf,  p,  n);
   	                  os_memcpy(PRODUCT_KEY_STRING(),   p,  n);
                         fat_printf("product_key = %s \r\n",  str_buf);
                   }
                   else
                   {
                         fat_printf("product key n = %d err \r\n",   n);
                   }
	   }
	   else
	   {
                   fat_printf("read key failed %s %d \r\n",  __FILE__, __LINE__);
	   }
	   if(! StringToString(buf,  "product_secret=\"", &p, &n))
	   {
                   char str_buf[64];

                   if(n < sizeof(str_buf))
                   {
	                 os_memset(str_buf, 0, sizeof(str_buf));
	                 os_memcpy(str_buf,  p,  n);
	                 os_memcpy(PRODUCT_SECRET_STRING(),  p,  n);
                        os_printf("product_secret = %s \r\n",  str_buf);
                   }
                   else
                   {
                         fat_printf("product secret n = %d err \r\n",   n);
                   }
	   }
	    else
	   {
                   fat_printf("read secret failed %s %d \r\n",  __FILE__, __LINE__);
	   }
#endif
	   
         if(! StringToInt(buf, "action=", 7, 0, 10, &p, &n))
         {
		     T_Calendar_Obj cal;

		     // action: 1 或 2 时， 需要往 RTC 写入时间
			 if(n == 0){ goto SecureExit; }
			 
			 p_action = p;
			 action    = n;
			 os_memset(&cal, 0, sizeof(cal));

			 // 读取年月日
			 if(! StringToInt(buf, "date=", 5, 2000, 2099, &p, &n))
			 {
			    cal.year = n;
				if(! StringToInt(p, "-", 1, 1, 12, &p, &n))
				{
				   cal.month = n;
				   if(! StringToInt(p, "-", 1, 1, 31, &p, &n))cal.day = n;
				}
			 }
	         else{ goto SecureExit; }

             // 读取时分秒
			 if(! StringToInt(buf, "time=", 5, 0, 23, &p, &n))
			 {
			    cal.hour = n;
				if(! StringToInt(p, ":", 1, 0, 59, &p, &n))
				{
				   cal.min = n;
				   if(! StringToInt(p, ":", 1, 0, 59, &p, &n))cal.sec = n;
				}
			 }
	         else{ goto SecureExit; }

             sys_printf("set rtc: %04d-%02d-%02d %02d:%02d:%02d\n", cal.year, cal.month, cal.day, cal.hour, cal.min, cal.sec);
           
			 // 改写action 值
			 if(action == 1) // action = 1: 只执行一次设置时间
			 {
				 //p = os_strchr(p_action, action + 0x30);
				 //if(p)
				 //{
				    char write_buf[] = "0";

					// 将 "action=1" 改写为"action=0"
				    if(f_lseek(&fileFIL, p_action - buf) == FR_OK) // 文件指针移到"action=" 的末尾
	                        {
	                                 f_write(&fileFIL, write_buf, sizeof(write_buf), (UINT *)&n);
				   }
				 //}
         	 }
			 RTCDrv_SetTime(cal.year, cal.month, cal.day, cal.hour, cal.min, cal.sec);
         }
		 else{ goto SecureExit; }
	  }
	  else
	  {
         fat_printf("failed: %d, %s, %d\n", res, __FILE__, __LINE__);
	  }
   }
   
SecureExit:
   f_close(&fileFIL);
}

// 创建文件夹并写入传感器数据
E_RESULT FILE_Write(char * file_name, char * write_buf)
{
	FRESULT res;
	int  len = 0;
	UINT bw;  // 输出参数, 写入后长度 

   if(FILE_Open(&fileFIL, file_name) == APP_FAILED) return APP_FAILED;
   
   res = f_lseek(&fileFIL, fileFIL.fsize); // 文件指针移到字符串结束位置
   fat_printf("f_lseek res = %d, fptr = %ld, fsize = %ld\n", res, fileFIL.fptr, fileFIL.fsize);
   
   len = os_strlen(write_buf);
   res = f_write(&fileFIL, write_buf, len, &bw);
   f_close(&fileFIL);
   fat_printf("f_write res = %d, len = %d, bw = %d\n", res, len, bw);
   
   return (E_RESULT)res;
}



#define RECURSIVE_EN   0  // 递归搜索使能: 1; 禁止: 0

FRESULT FILE_Scan(
	char * path,		   /* Pointer to the working buffer with start path */
	int   pathMaxLen,    /* the max length of the working buffer  */
	char * fileInName,   /*   待查找的文件名 */
	char * filePath,     // 如果查找成功, 则拷贝文件路径到此
	int filePathMaxLen // filePath buf 的最大长度
)
{
	FILINFO  Finfo;
	DIR dirs;
	FRESULT res;
    char *fn;
    WORD AccFiles = 0;  /* AccFiles 个file 及AccDirs 个folders */
	DWORD AccSize = 0;				/* 文件中的字节数 */
	char * p = NULL;
      char * postfix = NULL;
      char * pfix = NULL;
	
#if FAT_DEBUG_EN
	FRESULT result;
#endif
	
#if RECURSIVE_EN
    WORD AccDirs = 0;		
    int len;                
#endif
	
#if _USE_LFN
	Finfo.lfname = Lfname;
	Finfo.lfsize = sizeof(Lfname);
#endif
	res = f_opendir(&dirs, path);
	if (res == FR_OK) 
	{
	    #if RECURSIVE_EN
		len = os_strlen(path);  // 不需要递归搜索
		#endif
		
		while ((f_readdir(&dirs, &Finfo) == FR_OK) && Finfo.fname[0]) 
		{
			if (_FS_RPATH && Finfo.fname[0] == '.') 
				continue;
#if _USE_LFN
			fn = *Finfo.lfname ? Finfo.lfname : Finfo.fname;
#else
			fn = Finfo.fname;
#endif
			if (Finfo.fattrib & AM_DIR)   // 是目录
			{
			    #if RECURSIVE_EN // 如需递归搜索打开这部分代码 , 并且FileOutPath 为外部静态数组
				AccDirs++;
				path[len] = '/'; 
				os_strncpy(path + len + 1, fn, pathMaxLen);
				//记录文件目录

				res = FILE_Scan(path, fileInName, FileOutPath);
				path[len] = '\0';
				if (res != FR_OK)
				{
				   fat_printf("scan file() error  = %d\n", res);
				   break;
				}	
				#else
				
				fat_printf("scan file failed, dirs\n");
				res = FR_NO_FILE;
				break;
				
				#endif
			} 
			else   // 是文件
			{
				AccFiles++;
				AccSize += Finfo.fsize;

				//记录文件
				os_snprintf(filePath, filePathMaxLen, "%s/%s\r\n", path, fn);  // 文件路径最长为256 B
				p = strstr(filePath, fileInName);
				postfix = strstr(filePath, "bin");
				pfix = strstr(filePath, "2to1");   // 不含boot文件
				if (p && postfix && (!pfix))
				{
				    
					sys_printf("search file ok: %s\n", filePath);
					res = FR_OK;
				}
				else
				{
				    fat_printf("not find file\n");
				    res = FR_NO_FILE;
				}
			}
		}
	}
#if FAT_DEBUG_EN
	result = 
#endif
    f_closedir(&dirs);
	fat_printf("closedir %s\n", (result == FR_OK ? "OK" : "Failed"));
	
	return res;
}


#include "sfud.h"
#include "F10X_Flash_If.h"
#include "delay.h"

void FILE_SearchUpdateBinFile(void)
{
    FRESULT res;
	
	res = FILE_Scan(UpdateDirPath, sizeof(UpdateDirPath), "update", FileOutPath, sizeof(FileOutPath));  // 查找升级文件
	if(res == FR_OK)
	{
	    fat_printf("finded file path = %s\n", FileOutPath);
	    res = f_open(&fileFIL, FileOutPath, FA_OPEN_EXISTING | FA_READ);	  //打开文件
        if(res == FR_OK)
        {
            uint32_t bytes_to_read = 0;
            uint16_t sector_index = 0;   // 扇区序号
			const sfud_flash *flash = sfud_get_device_table() + 0;
			
			while(1)
			{
			    res = f_read(&fileFIL, spi_flash_buf, sizeof(spi_flash_buf), (UINT *)&bytes_to_read);  // 每次读取 4KB
		        if(res || bytes_to_read == 0)  /* 文件结束错误 */
		        {
		            f_close(&fileFIL);
					
		            sys_printf("read bin file: res = %d, br = %d\r\n", res, bytes_to_read);
		            // 文件全部读取完成, 则 res = 0, 同时 bytes_to_read 为 0
                    if(FR_OK == res && 0 == bytes_to_read)
					{
					    fat_printf("read file all done\n"); 
                        do
						{
						   T_APP_FLASH_ATTR appAttr;

						   // 删除升级文件
						   res = f_unlink(FileOutPath);
                           fat_printf("delete file in %s %s\n", FileOutPath, (res == FR_OK ? "ok" : "failed"));
						   
						   // 文件结束, 置升级标志位
						   os_memset(&appAttr, 0, sizeof(T_APP_FLASH_ATTR));
                           appAttr.fileLen = fileFIL.fsize;
						   appAttr.upgrade = 1;
						   appAttr.upgrade_inverse = ~(appAttr.upgrade);
						   Sys_WriteAppAttr(&appAttr);
						   fat_printf("read file success, len = %d, file_size = %ld Bytes\r\n", bytes_to_read, fileFIL.fsize);
						   
						   // 调试
						   //fat_printf("before AppAttr: fsize = %ld, upgrade = %d, inverse = 0x%lx, checkSum = 0x%lx, checkInverse = 0x%lx\r\n", 
						   //              appAttr.fileLen, appAttr.upgrade, appAttr.upgrade_inverse, appAttr.checkSum, appAttr.checkSumInverse);
                           os_memset(&appAttr, 0, sizeof(T_APP_FLASH_ATTR));
                           Sys_ReadAppAttr(&appAttr);
                           //fat_printf("reread AppAttr: fsize = %ld, upgrade = %d, inverse = 0x%lx, checkSum = 0x%lx, checkInverse = 0x%lx\r\n", 
						   //	            appAttr.fileLen, appAttr.upgrade, appAttr.upgrade_inverse, appAttr.checkSum, appAttr.checkSumInverse);
						   
						   // 启动复位
						   fat_printf("write spi flash done, ready to jump to the bootloader...\n");
						   delay_ms(3000);
						   JumpToBootloader();
						}while(0);
					}
					break;
		        }
				else
				{
					// 往flash 写入bin文件, 该位置并不在文件系统上, 对外隐藏
				    sfud_erase(flash, (FLASH_APP1_START_SECTOR + sector_index) << 12, 4096);     //擦除这个扇区

					if(bytes_to_read < sizeof(spi_flash_buf))  // 文件结束
					{
					   // 填充边界
					   os_memset(&spi_flash_buf[bytes_to_read], 0xCC, sizeof(spi_flash_buf) - bytes_to_read);
					}
                    sfud_write(flash, (FLASH_APP1_START_SECTOR + sector_index) << 12, 4096, spi_flash_buf);
                    sector_index++;
					
				    fat_printf("read file success, len = %d, file_size = %ld Bytes, index = %d\r\n", 
						        bytes_to_read, fileFIL.fsize, sector_index);
				}
			}  
        }
		else
		{
		    fat_printf("can't open file, error = %d, %s %d\n", res, __FILE__, __LINE__);
		}
	}
}
void FILE_FormatDisk(void)
{
	FRESULT res;
#if FAT_DEBUG_EN
    uint32_t total = 0, free = 0;
#endif

	// 挂载磁盘
	res = f_mount(&FlashDiskFatFs, (TCHAR const*)FlashDiskPath, 1);
	fat_printf("f_mount = %d\n", res);
	
	if (res == FR_NO_FILESYSTEM)  // FAT文件系统错误,重新格式化FLASH
	{
	    #if 1  // 降低ROM占用
	    fat_printf("FatFs Error, do Format...\r\n");
		res = f_mkfs((TCHAR const*)FlashDiskPath, 1, 4096);  //格式化FLASH,0,盘符; 1,不需要引导区,8个扇区为1个簇
		if(res == FR_OK)
		{
		    fat_printf("Flash Disk Format Finish !\n");  
		}
		else
		{
		    fat_printf("Flash Disk Format Failed = %d\n", res);
		}
		#endif
	}
	else
	{
	    if(res != FR_OK)
	    {
	        fat_printf("FatFs mount error = %d\r\n", res);
	    }
		else
		{
		    fat_printf("FatFs mount success\r\n");

			FILE_SearchUpdateBinFile(); // 搜索bin文件, 符合条件则升级
          
            #if FAT_DEBUG_EN  // 降低 ROM占用
			if(exf_getfree("0", &total, &free) == FR_OK)
		    {
		       fat_printf("read fatfs file size success\r\n");
			   fat_printf("total = %ld KB, free = %ld KB\n", total, free);
		    }
			#endif
		}
	}	
}



// 得到传感器文件中包含的数据总条目数
uint32_t FILE_GetSensorTotalItems(void)
{
   uint8_t res;
   uint32_t index = 0;
   
   if(FILE_Open(&fileFIL, SENSOR_TXT_FILE_NAME) == APP_FAILED) return APP_FAILED;
   if(fileFIL.fsize > 256)
   {
       res = f_lseek(&fileFIL, fileFIL.fsize - 150); // 文件指针移到最后一条数据之前
       fat_printf("1, res = %d\n", res);
   }
   else
   {
       res = f_lseek(&fileFIL, fileFIL.fsize); // 文件指针移到最后一条数据之前
       fat_printf("1, res = %d\n", res);
   }
   fat_printf("f_lseek res = %d, fptr = %ld, fsize = %ld\n", res, fileFIL.fptr, fileFIL.fsize);
   if(res == FR_OK)
   {
      char buf[256];
	  uint32_t bytes_to_read = 0;
	  char * p1 = NULL;
	  char * p2 = NULL;
	  char * pEnd = NULL;
	  
	  	
      os_memset(buf, 0, sizeof(buf));
	  res = f_read(&fileFIL, buf, sizeof(buf), (UINT *)&bytes_to_read);  
	  if(res == FR_OK)
	  {
	     fat_printf("buf = %s\n", buf);
	     p1 = os_strrchr((const char *)buf, '[');  // 查找最后一次出现 '[' 字符的位置
	     if(p1)
	     {
	        p2 = os_strrchr(p1, ']');
			if(p2)
			{
			    // 将序号的字符串转化为数字
			    index = strtol(&p1[1], &pEnd, 10);
				if(0 == index && pEnd == (char *)&p1[1])
				{
				   fat_printf("convert failed, index = %d, pEnd = 0x%x, &p1[1] = 0x%x\n", index, (uint32_t)pEnd, (uint32_t)&p1[1]);
				   f_close(&fileFIL);
				   return 0;
				}
				index += 1;
				fat_printf("had %ld items of sensor data\n", index);
			}
			else { fat_printf("p2 null\n"); }
	     }
		 else { fat_printf("p1 null\n"); }
	  }
	  else
	  {
         fat_printf("line = %d read failed = %d\n", __LINE__, res);
	  }
   }
   f_close(&fileFIL);
   return index;
}






void FatFs_Demo(void)
{

   
   FILE_FormatDisk();

   
}


