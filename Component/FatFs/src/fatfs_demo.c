
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
char UpdateDirPath[16] = "0:/update";   // �����ļ�·��
static char FileOutPath[64];

#if _USE_LFN
char Lfname[_MAX_LFN + 1];
#endif

FIL   fileFIL;


#include <string.h>


//�õ�����ʣ������
//drv:���̱��("0:"/"1:")
//total:������	 ����λKB��
//free:ʣ������	 ����λKB��
//����ֵ:0,����.����,�������
FRESULT exf_getfree(uint8_t * drv, uint32_t * total, uint32_t *free)
{
	FATFS * fs1;
	FRESULT res;
    uint32_t fre_clust = 0, fre_sect = 0, tot_sect = 0;
	
    //�õ�������Ϣ�����д�����
    res = f_getfree((const TCHAR*)drv, (DWORD*)&fre_clust, &fs1);
    if(res == FR_OK)
	{											   
	    tot_sect = (fs1->n_fatent - 2) * fs1->csize;	//�õ���������
	    fre_sect = fre_clust * fs1->csize;			//�õ�����������	   
//#if _MAX_SS != 512				  			  
//		tot_sect* = fs1->ssize / 512;
//		fre_sect* = fs1->ssize / 512;
//#endif	  
        #if (SPI_FLASH_SECTOR_SIZE == 4096L)
		*total = tot_sect  << 2;	// x4, ��λΪKB, ������СΪ 4096 B
		*free  = fre_sect  << 2;	// x4, ��λΪKB 
		#elif (SPI_FLASH_SECTOR_SIZE == 512L)
        *total = tot_sect  / 2;	//��λΪKB, �����FatFs������СΪ 512 B
		*free  = fre_sect  / 2;	//��λΪKB 
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

// �ļ��ĵ�0����ʾ�����������ַ���
const char SensorNameItem[] = "Number	  Tag   Timestamp	     HCHO	  PM2.5  Temp  Humidity\r\n";

// �ļ��ĵ�1����ʾ���������ݵ�λ
const char SensorUnitItem[] = "0-65535  tag   Y-Mon-Day  HH:MM:SS  mg/m3  ug/m3  'C\r\n";

// ��ָ���ļ�
E_RESULT FILE_Open(FIL * pFileFIL, char * file_name)
{
   FRESULT res;
   char sensor_dir[64] = "0:/sensor";        // ��������Ŀ¼
   int len = 0;
   DIR dirs;
   uint8_t is_first_create = 0;  // �Ƿ��һ�δ���
   //DWORD fixed_len = os_strlen(SensorUnitItem)  + os_strlen(SensorNameItem);
   
   // �ж�Ŀ¼�Ƿ����, �������򴴽�
   res = f_opendir(&dirs, (const TCHAR *)sensor_dir);  // ������Ŀ¼
   fat_printf("open dir %s %s\n", sensor_dir, (res == FR_OK ? "ok" : "failed"));
   if(res == FR_NO_PATH)  
   {
       // ������Ŀ¼
       res = f_mkdir((const TCHAR *)sensor_dir);
	   if(res != FR_OK){ fat_printf("create dir %s failed, res = %d\n", sensor_dir, res); return APP_FAILED; }
	   else { fat_printf("create dir %s success\n", sensor_dir); }
	   is_first_create = E_TRUE;
   }
   if(res){ INSERT_ERROR_INFO(res); return APP_FAILED; }
   f_closedir(&dirs);
   
   // ���ļ���ȡ�ļ�ָ��
   len = os_strlen(sensor_dir);
   sensor_dir[len] = '/';
   os_strncpy(&sensor_dir[len + 1], file_name, sizeof(sensor_dir));

   res = f_open(pFileFIL, (const TCHAR * )sensor_dir, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);  // ���ļ�, ��д
   //if(is_first_create || (pFileFIL->fsize < fixed_len))
    if(is_first_create)
   {
      UINT bw = 0;

      fat_printf("write sensor header: fsize = %d, is_first_create = %d \r\n",   pFileFIL->fsize,  is_first_create);
      //pFileFIL->fsize = 0;
      // ͷ������ʾ��λ
      res = f_write(pFileFIL, SensorNameItem, os_strlen(SensorNameItem), &bw);
      res = f_write(pFileFIL, SensorUnitItem, os_strlen(SensorUnitItem), &bw);
   }
   f_sync(pFileFIL);  // ��С���ļ�������, ͬ���ļ�����, ��ֹ���ƻ�
   fat_printf("f_open %s, fsize = %d\n", (res == FR_OK ? "ok" : "failed"), pFileFIL->fsize);
   if(res){ INSERT_ERROR_INFO(res); return APP_FAILED;  }

   return APP_SUCCESS;
}

/*
����: ���ַ����в���ƥ���������ַ�������ת��Ϊ����
����: char * buf: �����ҵ��ַ���
             char * str: ƥ������ַ���
             uint16_t strLen: ���ַ����ĳ���
             uint16_t min: ת��Ϊ����, ���ֵ���Сֵ
             uint16_t max: ת��Ϊ���ֺ�����ֵ
             char **p: ����ƥ���������ַ�������ʼָ���ָ��
             uint16_t * num: �ɹ�ת��Ϊ���ֵ�ָ��
����ֵ: ת���ɹ����� APP_SUCCESS; ʧ�ܷ��� APP_FAILED
*/
E_RESULT StringToInt(char * buf, char * str, uint16_t strLen, 
                                                         uint16_t min, uint16_t max, char ** p, uint16_t * num)
{
     char * p1 = NULL;
	 char * pEnd = NULL;
	 uint16_t len = 0;
	 uint16_t n = 0;

	 *num = 0;
     p1 = os_strstr((const char *)buf, str);  // ����ƥ������ַ���
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
����: ���ַ����в���ƥ���������ַ���
����: char * buf: �����ҵ��ַ���
             char * str: ƥ������ַ���
             char **p: ����ƥ���������ַ�������ʼָ���ָ��
             uint16_t * matchStrLen: Ŀ���ַ�������
����ֵ: ת���ɹ����� APP_SUCCESS; ʧ�ܷ��� APP_FAILED
*/
E_RESULT StringToString(char * buf, char * str, char ** p, uint16_t * matchStrLen)
{
        char * p1 = NULL;
	 char * pEnd = NULL;
	 uint16_t len = 0;

	 *matchStrLen = 0;
        p1 = os_strstr((const char *)buf, str);  // ����ƥ������ַ���
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
// ��ȡʱ������
void FILE_ReadConfig(void)
{
   char sensor_dir[32] = "0:/config.txt";        // ��Ŀ¼
   FRESULT res;
   
   // �ж�Ŀ¼�Ƿ����, �������򴴽�
   res = f_open(&fileFIL, (const TCHAR * )sensor_dir, FA_OPEN_ALWAYS | FA_WRITE | FA_READ);  // ���ļ�, ��д
   f_sync(&fileFIL);  // ��С���ļ�������, ͬ���ļ�����, ��ֹ���ƻ�
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

         // ��ȡ��¼���
         if(! StringToInt(buf, "gap=", 4, 0, 600, &p, &n))
         {
            if(n)
            {
               record_gap = n;   // �ı��¼���
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

		     // action: 1 �� 2 ʱ�� ��Ҫ�� RTC д��ʱ��
			 if(n == 0){ goto SecureExit; }
			 
			 p_action = p;
			 action    = n;
			 os_memset(&cal, 0, sizeof(cal));

			 // ��ȡ������
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

             // ��ȡʱ����
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
           
			 // ��дaction ֵ
			 if(action == 1) // action = 1: ִֻ��һ������ʱ��
			 {
				 //p = os_strchr(p_action, action + 0x30);
				 //if(p)
				 //{
				    char write_buf[] = "0";

					// �� "action=1" ��дΪ"action=0"
				    if(f_lseek(&fileFIL, p_action - buf) == FR_OK) // �ļ�ָ���Ƶ�"action=" ��ĩβ
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

// �����ļ��в�д�봫��������
E_RESULT FILE_Write(char * file_name, char * write_buf)
{
	FRESULT res;
	int  len = 0;
	UINT bw;  // �������, д��󳤶� 

   if(FILE_Open(&fileFIL, file_name) == APP_FAILED) return APP_FAILED;
   
   res = f_lseek(&fileFIL, fileFIL.fsize); // �ļ�ָ���Ƶ��ַ�������λ��
   fat_printf("f_lseek res = %d, fptr = %ld, fsize = %ld\n", res, fileFIL.fptr, fileFIL.fsize);
   
   len = os_strlen(write_buf);
   res = f_write(&fileFIL, write_buf, len, &bw);
   f_close(&fileFIL);
   fat_printf("f_write res = %d, len = %d, bw = %d\n", res, len, bw);
   
   return (E_RESULT)res;
}



#define RECURSIVE_EN   0  // �ݹ�����ʹ��: 1; ��ֹ: 0

FRESULT FILE_Scan(
	char * path,		   /* Pointer to the working buffer with start path */
	int   pathMaxLen,    /* the max length of the working buffer  */
	char * fileInName,   /*   �����ҵ��ļ��� */
	char * filePath,     // ������ҳɹ�, �򿽱��ļ�·������
	int filePathMaxLen // filePath buf ����󳤶�
)
{
	FILINFO  Finfo;
	DIR dirs;
	FRESULT res;
    char *fn;
    WORD AccFiles = 0;  /* AccFiles ��file ��AccDirs ��folders */
	DWORD AccSize = 0;				/* �ļ��е��ֽ��� */
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
		len = os_strlen(path);  // ����Ҫ�ݹ�����
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
			if (Finfo.fattrib & AM_DIR)   // ��Ŀ¼
			{
			    #if RECURSIVE_EN // ����ݹ��������ⲿ�ִ��� , ����FileOutPath Ϊ�ⲿ��̬����
				AccDirs++;
				path[len] = '/'; 
				os_strncpy(path + len + 1, fn, pathMaxLen);
				//��¼�ļ�Ŀ¼

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
			else   // ���ļ�
			{
				AccFiles++;
				AccSize += Finfo.fsize;

				//��¼�ļ�
				os_snprintf(filePath, filePathMaxLen, "%s/%s\r\n", path, fn);  // �ļ�·���Ϊ256 B
				p = strstr(filePath, fileInName);
				postfix = strstr(filePath, "bin");
				pfix = strstr(filePath, "2to1");   // ����boot�ļ�
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
	
	res = FILE_Scan(UpdateDirPath, sizeof(UpdateDirPath), "update", FileOutPath, sizeof(FileOutPath));  // ���������ļ�
	if(res == FR_OK)
	{
	    fat_printf("finded file path = %s\n", FileOutPath);
	    res = f_open(&fileFIL, FileOutPath, FA_OPEN_EXISTING | FA_READ);	  //���ļ�
        if(res == FR_OK)
        {
            uint32_t bytes_to_read = 0;
            uint16_t sector_index = 0;   // �������
			const sfud_flash *flash = sfud_get_device_table() + 0;
			
			while(1)
			{
			    res = f_read(&fileFIL, spi_flash_buf, sizeof(spi_flash_buf), (UINT *)&bytes_to_read);  // ÿ�ζ�ȡ 4KB
		        if(res || bytes_to_read == 0)  /* �ļ��������� */
		        {
		            f_close(&fileFIL);
					
		            sys_printf("read bin file: res = %d, br = %d\r\n", res, bytes_to_read);
		            // �ļ�ȫ����ȡ���, �� res = 0, ͬʱ bytes_to_read Ϊ 0
                    if(FR_OK == res && 0 == bytes_to_read)
					{
					    fat_printf("read file all done\n"); 
                        do
						{
						   T_APP_FLASH_ATTR appAttr;

						   // ɾ�������ļ�
						   res = f_unlink(FileOutPath);
                           fat_printf("delete file in %s %s\n", FileOutPath, (res == FR_OK ? "ok" : "failed"));
						   
						   // �ļ�����, ��������־λ
						   os_memset(&appAttr, 0, sizeof(T_APP_FLASH_ATTR));
                           appAttr.fileLen = fileFIL.fsize;
						   appAttr.upgrade = 1;
						   appAttr.upgrade_inverse = ~(appAttr.upgrade);
						   Sys_WriteAppAttr(&appAttr);
						   fat_printf("read file success, len = %d, file_size = %ld Bytes\r\n", bytes_to_read, fileFIL.fsize);
						   
						   // ����
						   //fat_printf("before AppAttr: fsize = %ld, upgrade = %d, inverse = 0x%lx, checkSum = 0x%lx, checkInverse = 0x%lx\r\n", 
						   //              appAttr.fileLen, appAttr.upgrade, appAttr.upgrade_inverse, appAttr.checkSum, appAttr.checkSumInverse);
                           os_memset(&appAttr, 0, sizeof(T_APP_FLASH_ATTR));
                           Sys_ReadAppAttr(&appAttr);
                           //fat_printf("reread AppAttr: fsize = %ld, upgrade = %d, inverse = 0x%lx, checkSum = 0x%lx, checkInverse = 0x%lx\r\n", 
						   //	            appAttr.fileLen, appAttr.upgrade, appAttr.upgrade_inverse, appAttr.checkSum, appAttr.checkSumInverse);
						   
						   // ������λ
						   fat_printf("write spi flash done, ready to jump to the bootloader...\n");
						   delay_ms(3000);
						   JumpToBootloader();
						}while(0);
					}
					break;
		        }
				else
				{
					// ��flash д��bin�ļ�, ��λ�ò������ļ�ϵͳ��, ��������
				    sfud_erase(flash, (FLASH_APP1_START_SECTOR + sector_index) << 12, 4096);     //�����������

					if(bytes_to_read < sizeof(spi_flash_buf))  // �ļ�����
					{
					   // ���߽�
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

	// ���ش���
	res = f_mount(&FlashDiskFatFs, (TCHAR const*)FlashDiskPath, 1);
	fat_printf("f_mount = %d\n", res);
	
	if (res == FR_NO_FILESYSTEM)  // FAT�ļ�ϵͳ����,���¸�ʽ��FLASH
	{
	    #if 1  // ����ROMռ��
	    fat_printf("FatFs Error, do Format...\r\n");
		res = f_mkfs((TCHAR const*)FlashDiskPath, 1, 4096);  //��ʽ��FLASH,0,�̷�; 1,����Ҫ������,8������Ϊ1����
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

			FILE_SearchUpdateBinFile(); // ����bin�ļ�, ��������������
          
            #if FAT_DEBUG_EN  // ���� ROMռ��
			if(exf_getfree("0", &total, &free) == FR_OK)
		    {
		       fat_printf("read fatfs file size success\r\n");
			   fat_printf("total = %ld KB, free = %ld KB\n", total, free);
		    }
			#endif
		}
	}	
}



// �õ��������ļ��а�������������Ŀ��
uint32_t FILE_GetSensorTotalItems(void)
{
   uint8_t res;
   uint32_t index = 0;
   
   if(FILE_Open(&fileFIL, SENSOR_TXT_FILE_NAME) == APP_FAILED) return APP_FAILED;
   if(fileFIL.fsize > 256)
   {
       res = f_lseek(&fileFIL, fileFIL.fsize - 150); // �ļ�ָ���Ƶ����һ������֮ǰ
       fat_printf("1, res = %d\n", res);
   }
   else
   {
       res = f_lseek(&fileFIL, fileFIL.fsize); // �ļ�ָ���Ƶ����һ������֮ǰ
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
	     p1 = os_strrchr((const char *)buf, '[');  // �������һ�γ��� '[' �ַ���λ��
	     if(p1)
	     {
	        p2 = os_strrchr(p1, ']');
			if(p2)
			{
			    // ����ŵ��ַ���ת��Ϊ����
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


